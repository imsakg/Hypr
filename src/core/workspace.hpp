#pragma once
#include <algorithm>
#include <charconv>
#include <cstdint>
#include <iterator>
#include <limits>
#include <map>
#include <optional>
#include <set>
#include <string_view>
#include <system_error>
#include <vector>

namespace HyprCore {
using WorkspaceId = int;
using WorkspaceWindowId = std::uint64_t;
struct Workspace {
    explicit Workspace(WorkspaceId value = 0) : id(value) {}
    WorkspaceId id = 0;
    std::vector<WorkspaceWindowId> windows;
    std::optional<WorkspaceWindowId> focused;
    bool pinned = false;
};

// Values are read-only outside the state machine; rejected operations do not mutate it.
class WorkspaceState {
public:
    bool addWorkspace(WorkspaceId id) {
        if (id <= 0) return false;
        if (!workspaces_.emplace(id, Workspace{id}).second) return false;
        if (!active_) active_ = id;
        return true;
    }
    bool setPinned(WorkspaceId id, bool pinned) {
        const auto it = workspaces_.find(id);
        if (it == workspaces_.end()) return false;
        it->second.pinned = pinned;
        return true;
    }
    bool removeWorkspace(WorkspaceId id) {
        const auto it = workspaces_.find(id);
        if (it == workspaces_.end() || active_ == id || it->second.pinned || !it->second.windows.empty())
            return false;
        workspaces_.erase(it);
        if (last_ == id) last_.reset();
        return true;
    }
    std::vector<WorkspaceId> cleanupEmpty() {
        std::vector<WorkspaceId> removed;
        for (const auto id : ids()) if (removeWorkspace(id)) removed.push_back(id);
        return removed;
    }
    const Workspace* workspace(WorkspaceId id) const {
        const auto it = workspaces_.find(id);
        return it == workspaces_.end() ? nullptr : &it->second;
    }
    std::optional<WorkspaceId> workspaceOf(WorkspaceWindowId window) const {
        for (const auto& item : workspaces_) {
            const auto& windows = item.second.windows;
            if (std::find(windows.begin(), windows.end(), window) != windows.end()) return item.first;
        }
        return std::nullopt;
    }
    bool addWindow(WorkspaceId id, WorkspaceWindowId window) {
        const auto it = workspaces_.find(id);
        if (!window || it == workspaces_.end() || workspaceOf(window)) return false;
        it->second.windows.push_back(window);
        if (!it->second.focused) it->second.focused = window;
        return true;
    }
    bool setUrgent(WorkspaceWindowId window, bool urgent) {
        if (!workspaceOf(window)) return false;
        if (urgent) urgent_.insert(window);
        else urgent_.erase(window);
        return true;
    }
    bool isUrgent(WorkspaceWindowId window) const { return urgent_.count(window) != 0; }
    std::vector<WorkspaceWindowId> urgentWindows(WorkspaceId id) const {
        std::vector<WorkspaceWindowId> result;
        const auto* local = workspace(id);
        if (local) for (const auto window : local->windows)
            if (isUrgent(window)) result.push_back(window);
        return result;
    }
    bool setSticky(WorkspaceWindowId window, bool sticky) {
        if (!workspaceOf(window)) return false;
        if (sticky) sticky_.insert(window);
        else sticky_.erase(window);
        return true;
    }
    bool isSticky(WorkspaceWindowId window) const { return sticky_.count(window) != 0; }
    std::vector<WorkspaceWindowId> visibleWindows(WorkspaceId id) const {
        const auto* local = workspace(id);
        if (!local) return {};
        auto result = local->windows;
        for (const auto& item : workspaces_)
            if (item.first != id)
                for (const auto window : item.second.windows)
                    if (isSticky(window)) result.push_back(window);
        return result;
    }
    bool moveWindow(WorkspaceWindowId window, WorkspaceId destination) {
        const auto from = workspaceOf(window);
        const auto target = workspaces_.find(destination);
        if (!from || target == workspaces_.end()) return false;
        if (*from == destination) return true;
        auto& source = workspaces_.at(*from);
        target->second.windows.push_back(window);
        detach(source, window);
        if (!target->second.focused) target->second.focused = window;
        return true;
    }
    bool removeWindow(WorkspaceWindowId window) {
        const auto home = workspaceOf(window);
        if (!home) return false;
        detach(workspaces_.at(*home), window);
        sticky_.erase(window);
        urgent_.erase(window);
        return true;
    }
    std::optional<WorkspaceWindowId> focused() const {
        return active_ ? workspaces_.at(*active_).focused : std::nullopt;
    }
    bool focusWindow(WorkspaceWindowId window) {
        const auto home = workspaceOf(window);
        if (!home || home != active_) return false;
        workspaces_.at(*home).focused = window;
        urgent_.erase(window);
        return true;
    }
    std::optional<WorkspaceId> active() const { return active_; }
    std::optional<WorkspaceId> last() const { return last_; }
    bool activate(WorkspaceId id) {
        if (!workspace(id)) return false;
        if (active_ != id) {
            last_ = active_;
            active_ = id;
        }
        const auto selected = workspaces_.at(id).focused;
        if (selected) urgent_.erase(*selected);
        return true;
    }
    bool toggleLast() { return last_ && activate(*last_); }
    std::optional<WorkspaceId> nextWorkspace(WorkspaceId from) const {
        auto it = workspaces_.find(from);
        if (it == workspaces_.end()) return std::nullopt;
        if (++it == workspaces_.end()) it = workspaces_.begin();
        return it->first;
    }
    std::optional<WorkspaceId> previousWorkspace(WorkspaceId from) const {
        auto it = workspaces_.find(from);
        if (it == workspaces_.end()) return std::nullopt;
        if (it == workspaces_.begin()) it = workspaces_.end();
        return std::prev(it)->first;
    }
    std::optional<WorkspaceId> resolveTarget(std::string_view text) const {
        const auto space = [](char c) { return c == ' ' || c == '\t' || c == '\r' || c == '\n'; };
        while (!text.empty() && space(text.front())) text.remove_prefix(1);
        while (!text.empty() && space(text.back())) text.remove_suffix(1);
        if (text == "next") return active_ ? nextWorkspace(*active_) : std::nullopt;
        if (text == "prev") return active_ ? previousWorkspace(*active_) : std::nullopt;
        if (text == "lastworkspace" || text == "last") return last_;
        if (text.empty()) return std::nullopt;
        const char sign = text.front();
        const bool relative = sign == '+' || sign == '-';
        if (relative) text.remove_prefix(1);
        if (text.empty() || text.front() < '0' || text.front() > '9') return std::nullopt;
        WorkspaceId number = 0;
        const auto parsed = std::from_chars(text.data(), text.data() + text.size(), number);
        if (parsed.ec != std::errc{} || parsed.ptr != text.data() + text.size() || number <= 0)
            return std::nullopt;
        if (!relative) return number;
        if (!active_) return std::nullopt;
        const auto result = static_cast<long long>(*active_) + (sign == '-' ? -static_cast<long long>(number) : number);
        if (result <= 0 || result > std::numeric_limits<WorkspaceId>::max()) return std::nullopt;
        return static_cast<WorkspaceId>(result);
    }
    std::vector<WorkspaceId> ids() const {
        std::vector<WorkspaceId> result;
        result.reserve(workspaces_.size());
        for (const auto& item : workspaces_) result.push_back(item.first);
        return result;
    }
private:
    static void detach(Workspace& source, WorkspaceWindowId window) {
        source.windows.erase(std::find(source.windows.begin(), source.windows.end(), window));
        if (source.focused == window) source.focused = source.windows.empty() ?
            std::nullopt : std::optional<WorkspaceWindowId>{source.windows.front()};
    }
    std::set<WorkspaceWindowId> sticky_;
    std::set<WorkspaceWindowId> urgent_;
    std::map<WorkspaceId, Workspace> workspaces_;
    std::optional<WorkspaceId> active_;
    std::optional<WorkspaceId> last_;
};
}
