#pragma once
#include "geometry.hpp"
#include <algorithm>
#include <cstdint>
#include <optional>
#include <set>
#include <tuple>
#include <vector>

namespace HyprCore {
using FocusWindowId = std::uint64_t;
enum class FocusDirection { Left, Right, Up, Down };
struct FocusTarget {
    FocusWindowId id = 0;
    Rect bounds;
    bool focusable = true;
    bool mapped = true;
    bool hidden = false;
};
inline bool focusEligible(const FocusTarget& target) {
    return target.id && target.focusable && target.mapped && !target.hidden &&
           target.bounds.width > 0 && target.bounds.height > 0;
}

namespace FocusDetail {
// A repeated ID cannot override the first snapshot's geometry or eligibility.
inline std::vector<const FocusTarget*> eligibleTargets(const std::vector<FocusTarget>& targets) {
    std::set<FocusWindowId> seen;
    std::vector<const FocusTarget*> eligible;
    eligible.reserve(targets.size());
    for (const auto& target : targets)
        if (seen.insert(target.id).second && focusEligible(target)) eligible.push_back(&target);
    return eligible;
}
struct Axis { bool horizontal; int sign; };
inline std::optional<Axis> axis(FocusDirection direction) {
    switch (direction) {
        case FocusDirection::Left: return Axis{true, -1};
        case FocusDirection::Right: return Axis{true, 1};
        case FocusDirection::Up: return Axis{false, -1};
        case FocusDirection::Down: return Axis{false, 1};
    }
    return std::nullopt;
}
// Doubled centers retain half-pixel precision without floating point or int overflow.
inline long long center(const Rect& bounds, bool horizontal) {
    return horizontal ? 2LL * bounds.x + bounds.width : 2LL * bounds.y + bounds.height;
}
inline bool overlaps(const Rect& a, const Rect& b, bool horizontal) {
    const long long startA = horizontal ? a.x : a.y;
    const long long startB = horizontal ? b.x : b.y;
    const auto endA = startA + (horizontal ? a.width : a.height);
    const auto endB = startB + (horizontal ? b.width : b.height);
    return std::max(startA, startB) < std::min(endA, endB);
}
inline long long magnitude(long long value) { return value < 0 ? -value : value; }
}

// Back-to-front stacking order: the last eligible hit is topmost.
inline std::optional<FocusWindowId> focusAtPoint(const std::vector<FocusTarget>& targets, int x, int y) {
    const auto eligible = FocusDetail::eligibleTargets(targets);
    for (auto it = eligible.rbegin(); it != eligible.rend(); ++it)
        if (contains((*it)->bounds, x, y)) return (*it)->id;
    return std::nullopt;
}

class FocusHistory {
public:
    bool remember(FocusWindowId id) {
        if (!id) return false;
        forget(id);
        history_.insert(history_.begin(), id);
        return true;
    }
    bool forget(FocusWindowId id) {
        const auto found = std::find(history_.begin(), history_.end(), id);
        if (found == history_.end()) return false;
        history_.erase(found);
        return true;
    }
    const std::vector<FocusWindowId>& entries() const { return history_; }
    std::optional<FocusWindowId> fallback(const std::vector<FocusTarget>& targets,
                                         std::optional<FocusWindowId> excluded = std::nullopt) const {
        const auto eligible = FocusDetail::eligibleTargets(targets);
        for (const auto id : history_)
            if (id != excluded)
                for (const auto* target : eligible)
                    if (target->id == id) return id;
        for (const auto* target : eligible)
            if (target->id != excluded) return target->id;
        return std::nullopt;
    }
private:
    std::vector<FocusWindowId> history_;
};

// Input order is the cycle order. Missing/ineligible current starts at the chosen end.
inline std::optional<FocusWindowId> cycleFocus(const std::vector<FocusTarget>& targets,
                                              std::optional<FocusWindowId> current = std::nullopt,
                                              bool reverse = false, bool wrap = true) {
    std::vector<FocusWindowId> eligible;
    for (const auto* target : FocusDetail::eligibleTargets(targets)) eligible.push_back(target->id);
    if (eligible.empty()) return std::nullopt;
    const auto found = current ? std::find(eligible.begin(), eligible.end(), *current) : eligible.end();
    if (found == eligible.end()) return reverse ? eligible.back() : eligible.front();
    const auto index = static_cast<std::size_t>(found - eligible.begin());
    if (reverse) {
        if (index) return eligible[index - 1];
        return wrap ? std::optional<FocusWindowId>{eligible.back()} : std::nullopt;
    }
    if (index + 1 < eligible.size()) return eligible[index + 1];
    return wrap ? std::optional<FocusWindowId>{eligible.front()} : std::nullopt;
}

// Pure selection: callers supply only the relevant workspace/monitor snapshot.
inline std::optional<FocusWindowId> directionalFocus(const std::vector<FocusTarget>& targets,
                                                    FocusWindowId current, FocusDirection direction,
                                                    bool wrap = false) {
    const auto axis = FocusDetail::axis(direction);
    if (!axis) return std::nullopt;
    const auto eligible = FocusDetail::eligibleTargets(targets);
    const auto found = std::find_if(eligible.begin(), eligible.end(),
        [current](const FocusTarget* target) { return target->id == current; });
    if (found == eligible.end()) return std::nullopt;
    const auto* source = *found;
    using Score = std::tuple<int, long long, long long, FocusWindowId>;
    const auto choose = [&](bool wrapping) -> std::optional<FocusWindowId> {
        std::optional<Score> best;
        std::optional<FocusWindowId> selected;
        for (const auto* candidate : eligible) {
            const auto& target = *candidate;
            if (target.id == current) continue;
            const auto targetCenter = FocusDetail::center(target.bounds, axis->horizontal);
            const auto forward = axis->sign * (targetCenter -
                                              FocusDetail::center(source->bounds, axis->horizontal));
            if (wrapping ? forward >= 0 : forward <= 0) continue;
            const auto cross = FocusDetail::magnitude(FocusDetail::center(target.bounds, !axis->horizontal) -
                                                     FocusDetail::center(source->bounds, !axis->horizontal));
            const auto overlap = FocusDetail::overlaps(source->bounds, target.bounds, !axis->horizontal);
            const auto primary = wrapping ? axis->sign * targetCenter : forward;
            const Score score{overlap ? 0 : 1, primary, cross, target.id};
            if (!best || score < *best) { best = score; selected = target.id; }
        }
        return selected;
    };
    const auto forward = choose(false);
    return forward || !wrap ? forward : choose(true);
}
}
