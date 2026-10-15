#pragma once
#include <iterator>
#include <map>
#include <optional>
#include <vector>

namespace HyprCore {
using WorkspaceId = int;
struct Workspace {
    WorkspaceId id = 0;
};

// Values are read-only outside the state machine; rejected operations do not mutate it.
class WorkspaceState {
public:
    bool addWorkspace(WorkspaceId id) {
        if (id <= 0) return false;
        return workspaces_.emplace(id, Workspace{id}).second;
    }
    const Workspace* workspace(WorkspaceId id) const {
        const auto it = workspaces_.find(id);
        return it == workspaces_.end() ? nullptr : &it->second;
    }
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
};
}
