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
TEST(workspace_navigation_wraps_existing_sorted_ids_not_numeric_gaps) {
    WorkspaceState state;
    CHECK(!state.nextWorkspace(1));
    CHECK(!state.previousWorkspace(1));
    state.addWorkspace(8);
    CHECK(state.nextWorkspace(8) == 8);
    CHECK(state.previousWorkspace(8) == 8);
    state.addWorkspace(2);
    state.addWorkspace(20);
    CHECK(state.nextWorkspace(2) == 8);
    CHECK(state.nextWorkspace(20) == 2);
    CHECK(state.previousWorkspace(2) == 20);
    CHECK(state.previousWorkspace(20) == 8);
    CHECK(!state.nextWorkspace(3));
    CHECK(!state.previousWorkspace(-1));
    CHECK(state.ids() == std::vector<WorkspaceId>({2, 8, 20}));
}
TEST(lastworkspace_toggles_only_successful_distinct_activations) {
    WorkspaceState state;
    CHECK(!state.active());
    CHECK(!state.last());
    CHECK(!state.toggleLast());
    CHECK(!state.activate(99));
    state.addWorkspace(4);
    state.addWorkspace(1);
    state.addWorkspace(9);
    CHECK(state.active() == 4);
    CHECK(!state.toggleLast());
    CHECK(state.activate(1));
    CHECK(state.active() == 1 && state.last() == 4);
    CHECK(state.activate(1));
    CHECK(state.last() == 4);
    CHECK(!state.activate(-1));
    CHECK(!state.activate(99));
    CHECK(state.active() == 1 && state.last() == 4);
    CHECK(state.toggleLast());
    CHECK(state.active() == 4 && state.last() == 1);
    CHECK(state.toggleLast());
    CHECK(state.active() == 1 && state.last() == 4);
}
TEST(workspace_membership_is_ordered_and_unique_across_workspaces) {
    WorkspaceState state;
    state.addWorkspace(1);
    state.addWorkspace(2);
    CHECK(state.addWindow(1, 30));
    CHECK(state.addWindow(1, 10));
    CHECK(state.addWindow(1, 20));
    CHECK(!state.addWindow(1, 10));
    CHECK(!state.addWindow(2, 10));
    CHECK(!state.addWindow(3, 40));
    CHECK(!state.addWindow(1, 0));
    CHECK(state.workspace(1)->windows == std::vector<WorkspaceWindowId>({30, 10, 20}));
    CHECK(state.workspace(2)->windows.empty());
    CHECK(state.workspaceOf(10) == 1);
    CHECK(!state.workspaceOf(40));
    CHECK(!state.workspaceOf(0));
}
TEST_MAIN()
