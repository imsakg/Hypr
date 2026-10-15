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
TEST(moving_windows_appends_without_reordering_survivors_or_switching_workspace) {
    WorkspaceState state;
    state.addWorkspace(1);
    state.addWorkspace(2);
    for (const auto id : {30u, 10u, 20u}) state.addWindow(1, id);
    state.addWindow(2, 99);
    CHECK(!state.moveWindow(10, 3));
    CHECK(!state.moveWindow(777, 2));
    CHECK(!state.moveWindow(0, 2));
    CHECK(!state.moveWindow(10, -1));
    CHECK(state.moveWindow(10, 2));
    CHECK(state.workspaceOf(10) == 2);
    CHECK(state.workspace(1)->windows == std::vector<WorkspaceWindowId>({30, 20}));
    CHECK(state.workspace(2)->windows == std::vector<WorkspaceWindowId>({99, 10}));
    CHECK(state.moveWindow(99, 2));
    CHECK(state.workspace(2)->windows == std::vector<WorkspaceWindowId>({99, 10}));
    CHECK(state.active() == 1 && !state.last());
}
TEST(focus_always_belongs_to_the_active_workspace_and_survives_moves) {
    WorkspaceState state;
    state.addWorkspace(1); state.addWorkspace(2); state.addWorkspace(3);
    state.addWindow(1, 10); state.addWindow(1, 20); state.addWindow(2, 30);
    CHECK(state.focused() == 10);
    CHECK(state.focusWindow(20));
    CHECK(!state.focusWindow(30));
    CHECK(!state.focusWindow(999));
    CHECK(state.focused() == 20);
    CHECK(state.moveWindow(20, 2));
    CHECK(state.workspace(1)->focused == 10);
    CHECK(state.workspace(2)->focused == 30);
    CHECK(state.focused() == 10);
    CHECK(state.activate(2));
    CHECK(state.focused() == 30);
    CHECK(state.focusWindow(20));
    CHECK(state.moveWindow(20, 3));
    CHECK(state.focused() == 30);
    CHECK(state.workspace(3)->focused == 20);
    CHECK(state.activate(3) && state.focused() == 20);
}
TEST(closing_windows_preserves_focus_until_the_focused_member_is_removed) {
    WorkspaceState state;
    state.addWorkspace(1);
    for (const auto id : {30u, 10u, 20u}) state.addWindow(1, id);
    state.focusWindow(10);
    CHECK(!state.removeWindow(0));
    CHECK(!state.removeWindow(99));
    CHECK(state.removeWindow(30));
    CHECK(state.focused() == 10);
    CHECK(state.removeWindow(10));
    CHECK(state.focused() == 20);
    CHECK(!state.workspaceOf(10));
    CHECK(state.removeWindow(20));
    CHECK(!state.focused());
    CHECK(state.workspace(1)->windows.empty());
    CHECK(state.active() == 1);
    CHECK(!state.removeWindow(20));
}
TEST(empty_cleanup_protects_active_pinned_and_populated_workspaces) {
    WorkspaceState state;
    for (const auto id : {1, 2, 3, 4, 5}) state.addWorkspace(id);
    CHECK(state.setPinned(2, true));
    CHECK(!state.setPinned(99, true));
    state.addWindow(3, 30);
    state.activate(4);
    state.activate(1);
    CHECK(state.last() == 4);
    CHECK(!state.removeWorkspace(1));
    CHECK(!state.removeWorkspace(2));
    CHECK(!state.removeWorkspace(3));
    CHECK(!state.removeWorkspace(-1));
    CHECK(state.cleanupEmpty() == std::vector<WorkspaceId>({4, 5}));
    CHECK(state.ids() == std::vector<WorkspaceId>({1, 2, 3}));
    CHECK(!state.last() && !state.toggleLast());
    CHECK(state.workspace(2)->pinned);
    CHECK(state.setPinned(2, false));
    CHECK(state.removeWorkspace(2));
    CHECK(!state.removeWorkspace(2));
    state.removeWindow(30);
    CHECK(state.cleanupEmpty() == std::vector<WorkspaceId>({3}));
    CHECK(state.active() == 1);
}
TEST(workspace_targets_validate_full_signed_offsets_before_arithmetic) {
    WorkspaceState state;
    CHECK(!state.resolveTarget("-1"));
    CHECK(state.resolveTarget("12") == 12);
    state.addWorkspace(1); state.addWorkspace(3); state.addWorkspace(9);
    CHECK(!state.resolveTarget("-1"));
    CHECK(state.resolveTarget("+1") == 2);
    CHECK(state.resolveTarget("next") == 3);
    CHECK(state.resolveTarget("prev") == 9);
    CHECK(!state.resolveTarget("lastworkspace"));
    state.activate(3);
    CHECK(state.resolveTarget("-1") == 2);
    CHECK(state.resolveTarget(" +6 ") == 9);
    CHECK(state.resolveTarget("lastworkspace") == 1);
    for (const auto* text : {"", "0", "+0", "-0", "+", "-", "--1", "+-1", "1x", "-1x", "3 4", "2147483648", "99999999999999999999999"})
        CHECK(!state.resolveTarget(text));
    state.addWorkspace(2147483647);
    state.activate(2147483647);
    CHECK(!state.resolveTarget("+1"));
    CHECK(state.resolveTarget("-2147483646") == 1);
    CHECK(!state.resolveTarget("-2147483647"));
    CHECK(state.active() == 2147483647);
}
TEST(sticky_visibility_does_not_duplicate_or_transfer_ownership) {
    WorkspaceState state;
    state.addWorkspace(1); state.addWorkspace(2);
    state.addWindow(1, 10); state.addWindow(1, 11); state.addWindow(2, 20);
    CHECK(!state.setSticky(99, true));
    CHECK(state.setSticky(10, true));
    CHECK(state.isSticky(10));
    CHECK(state.visibleWindows(1) == std::vector<WorkspaceWindowId>({10, 11}));
    CHECK(state.visibleWindows(2) == std::vector<WorkspaceWindowId>({20, 10}));
    CHECK(state.visibleWindows(-1).empty());
    CHECK(state.workspaceOf(10) == 1);
    state.activate(2);
    CHECK(!state.focusWindow(10));
    CHECK(state.moveWindow(10, 2));
    CHECK(state.visibleWindows(2) == std::vector<WorkspaceWindowId>({20, 10}));
    CHECK(state.visibleWindows(1) == std::vector<WorkspaceWindowId>({11, 10}));
    CHECK(state.setSticky(10, false));
    CHECK(state.visibleWindows(1) == std::vector<WorkspaceWindowId>({11}));
    state.setSticky(10, true);
    state.removeWindow(10);
    CHECK(!state.isSticky(10));
    state.addWindow(1, 10);
    CHECK(!state.isSticky(10));
}
TEST(urgency_tracks_live_members_and_clears_when_the_window_receives_focus) {
    WorkspaceState state;
    state.addWorkspace(1); state.addWorkspace(2);
    state.addWindow(1, 10); state.addWindow(1, 11); state.addWindow(2, 20);
    CHECK(!state.setUrgent(0, true));
    CHECK(!state.setUrgent(99, true));
    CHECK(state.setUrgent(11, true));
    CHECK(state.setUrgent(20, true));
    CHECK(state.urgentWindows(1) == std::vector<WorkspaceWindowId>{11});
    CHECK(state.urgentWindows(99).empty());
    CHECK(!state.focusWindow(20) && state.isUrgent(20));
    CHECK(state.focusWindow(11) && !state.isUrgent(11));
    CHECK(state.activate(2) && !state.isUrgent(20));
    CHECK(state.setUrgent(11, true));
    CHECK(state.moveWindow(11, 2));
    CHECK(state.isUrgent(11));
    CHECK(state.urgentWindows(2) == std::vector<WorkspaceWindowId>{11});
    CHECK(state.setUrgent(11, false) && !state.isUrgent(11));
    state.setUrgent(11, true);
    CHECK(state.removeWindow(11) && !state.isUrgent(11));
    CHECK(state.addWindow(1, 11) && !state.isUrgent(11));
}
TEST(workspace_focus_fallback_prefers_recent_survivors_and_prunes_departures) {
    WorkspaceState state;
    state.addWorkspace(1); state.addWorkspace(2);
    for (const auto id : {10u, 20u, 30u, 40u}) state.addWindow(1, id);
    CHECK(state.focusWindow(20));
    CHECK(state.focusWindow(30));
    CHECK(state.removeWindow(30));
    CHECK(state.focused() == 20);
    CHECK(state.focusWindow(40));
    CHECK(state.focusWindow(20));
    CHECK(state.setUrgent(40, true));
    CHECK(state.moveWindow(20, 2));
    CHECK(state.focused() == 40 && !state.isUrgent(40));
    CHECK(state.workspace(1)->focusHistory == std::vector<WorkspaceWindowId>({40, 10}));
    CHECK(state.workspace(2)->focusHistory == std::vector<WorkspaceWindowId>{20});
    CHECK(state.removeWindow(10));
    CHECK(state.workspace(1)->focusHistory == std::vector<WorkspaceWindowId>{40});
    CHECK(state.removeWindow(40));
    CHECK(!state.focused() && state.workspace(1)->focusHistory.empty());
    CHECK(state.addWindow(1, 30));
    CHECK(state.focused() == 30);
    CHECK(state.workspace(1)->focusHistory == std::vector<WorkspaceWindowId>{30});
}
TEST_MAIN()
