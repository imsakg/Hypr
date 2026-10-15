#pragma once
#include <map>
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
