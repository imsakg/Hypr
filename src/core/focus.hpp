#pragma once
#include "geometry.hpp"
#include <algorithm>
#include <cstdint>
#include <optional>
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

// Pure selection: callers supply only the relevant workspace/monitor snapshot.
inline std::optional<FocusWindowId> directionalFocus(const std::vector<FocusTarget>& targets,
                                                    FocusWindowId current, FocusDirection direction) {
    const auto axis = FocusDetail::axis(direction);
    if (!axis) return std::nullopt;
    const auto source = std::find_if(targets.begin(), targets.end(),
        [current](const FocusTarget& target) { return target.id == current; });
    if (source == targets.end() || !focusEligible(*source)) return std::nullopt;
    using Score = std::tuple<int, long long, long long, FocusWindowId>;
    std::optional<Score> best;
    std::optional<FocusWindowId> selected;
    for (const auto& target : targets) {
        if (target.id == current || !focusEligible(target)) continue;
        const auto forward = axis->sign * (FocusDetail::center(target.bounds, axis->horizontal) -
                                          FocusDetail::center(source->bounds, axis->horizontal));
        if (forward <= 0) continue;
        const auto cross = FocusDetail::magnitude(FocusDetail::center(target.bounds, !axis->horizontal) -
                                                 FocusDetail::center(source->bounds, !axis->horizontal));
        const auto overlap = FocusDetail::overlaps(source->bounds, target.bounds, !axis->horizontal);
        const Score score{overlap ? 0 : 1, forward, cross, target.id};
        if (!best || score < *best) { best = score; selected = target.id; }
    }
    return selected;
}
}
