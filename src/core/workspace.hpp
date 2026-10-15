#pragma once
#include <algorithm>
#include <cstdint>
#include <iterator>
#include <map>
#include <optional>
#include <vector>

namespace HyprCore {
using WorkspaceId = int;
using WorkspaceWindowId = std::uint64_t;
struct Workspace {
    explicit Workspace(WorkspaceId value = 0) : id(value) {}
    WorkspaceId id = 0;
    std::vector<WorkspaceWindowId> windows;
    std::optional<WorkspaceWindowId> focused;
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
    bool moveWindow(WorkspaceWindowId window, WorkspaceId destination) {
        const auto from = workspaceOf(window);
        const auto target = workspaces_.find(destination);
        if (!from || target == workspaces_.end()) return false;
        if (*from == destination) return true;
        auto& source = workspaces_.at(*from);
        target->second.windows.push_back(window);
        source.windows.erase(std::find(source.windows.begin(), source.windows.end(), window));
        if (source.focused == window) source.focused = source.windows.empty() ?
            std::nullopt : std::optional<WorkspaceWindowId>{source.windows.front()};
        if (!target->second.focused) target->second.focused = window;
        return true;
    }
    std::optional<WorkspaceWindowId> focused() const {
        return active_ ? workspaces_.at(*active_).focused : std::nullopt;
    }
    bool focusWindow(WorkspaceWindowId window) {
        const auto home = workspaceOf(window);
        if (!home || home != active_) return false;
        workspaces_.at(*home).focused = window;
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
    std::vector<WorkspaceId> ids() const {
        std::vector<WorkspaceId> result;
        result.reserve(workspaces_.size());
        for (const auto& item : workspaces_) result.push_back(item.first);
        return result;
    }
private:
    std::map<WorkspaceId, Workspace> workspaces_;
    std::optional<WorkspaceId> active_;
    std::optional<WorkspaceId> last_;
};
}
