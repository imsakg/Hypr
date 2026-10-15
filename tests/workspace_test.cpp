#include "test.hpp"
#include "core/workspace.hpp"
using namespace HyprCore;
TEST(workspace_ids_are_positive_sorted_and_unique) {
    WorkspaceState state;
    CHECK(state.ids().empty());
    CHECK(!state.addWorkspace(0));
    CHECK(!state.addWorkspace(-1));
    CHECK(state.addWorkspace(9));
    CHECK(state.addWorkspace(2));
    CHECK(state.addWorkspace(5));
    CHECK(!state.addWorkspace(2));
    CHECK(state.ids() == std::vector<WorkspaceId>({2, 5, 9}));
    CHECK(state.workspace(5) && state.workspace(5)->id == 5);
    CHECK(!state.workspace(4));
    CHECK(!state.workspace(-1));
}
TEST_MAIN()
