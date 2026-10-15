#include "test.hpp"
#include "core/focus.hpp"
using namespace HyprCore;
TEST(directional_focus_selects_forward_centers_on_each_axis) {
    const std::vector<FocusTarget> targets{
        {1, {100, 100, 80, 80}}, {2, {0, 100, 80, 80}},
        {3, {200, 100, 80, 80}}, {4, {100, 0, 80, 80}},
        {5, {100, 200, 80, 80}}, {6, {400, 100, 80, 80}}};
    CHECK(directionalFocus(targets, 1, FocusDirection::Left) == 2);
    CHECK(directionalFocus(targets, 1, FocusDirection::Right) == 3);
    CHECK(directionalFocus(targets, 1, FocusDirection::Up) == 4);
    CHECK(directionalFocus(targets, 1, FocusDirection::Down) == 5);
    CHECK(!directionalFocus(targets, 99, FocusDirection::Right));
    CHECK(!directionalFocus({}, 1, FocusDirection::Right));
    CHECK(!directionalFocus({{1, {0, 0, 10, 10}}}, 1, FocusDirection::Left));
}
TEST(directional_focus_prefers_overlapping_lanes_before_nearer_diagonals) {
    const std::vector<FocusTarget> right{
        {1, {0, 0, 100, 100}}, {2, {110, 100, 100, 100}}, {3, {400, 99, 100, 100}}};
    CHECK(directionalFocus(right, 1, FocusDirection::Right) == 3);
    const std::vector<FocusTarget> down{
        {1, {0, 0, 100, 100}}, {2, {100, 110, 100, 100}}, {3, {99, 400, 100, 100}}};
    CHECK(directionalFocus(down, 1, FocusDirection::Down) == 3);
    CHECK(directionalFocus({{1, {0, 0, 100, 100}}, {2, {150, 200, 100, 100}},
                            {3, {300, 150, 100, 100}}}, 1, FocusDirection::Right) == 2);
}
TEST(directional_focus_ties_are_stable_under_input_permutations) {
    std::vector<FocusTarget> targets{
        {1, {0, 0, 100, 100}}, {7, {100, -10, 100, 100}}, {3, {100, 10, 100, 100}}};
    CHECK(directionalFocus(targets, 1, FocusDirection::Right) == 3);
    std::reverse(targets.begin(), targets.end());
    CHECK(directionalFocus(targets, 1, FocusDirection::Right) == 3);
    targets.push_back({9, {100, 0, 100, 100}});
    CHECK(directionalFocus(targets, 1, FocusDirection::Right) == 9);
}
TEST(focus_eligibility_excludes_hidden_unmapped_unfocusable_and_degenerate_targets) {
    FocusTarget target{2, {100, 0, 50, 50}};
    CHECK(focusEligible(target));
    target.focusable = false; CHECK(!focusEligible(target)); target.focusable = true;
    target.mapped = false; CHECK(!focusEligible(target)); target.mapped = true;
    target.hidden = true; CHECK(!focusEligible(target)); target.hidden = false;
    target.id = 0; CHECK(!focusEligible(target)); target.id = 2;
    target.bounds.width = 0; CHECK(!focusEligible(target));
    target.bounds.width = -1; CHECK(!focusEligible(target)); target.bounds.width = 50;
    target.bounds.height = 0; CHECK(!focusEligible(target));
    target.bounds.height = -1; CHECK(!focusEligible(target));
    const std::vector<FocusTarget> targets{
        {1, {0, 0, 50, 50}}, {2, {60, 0, 50, 50}, false},
        {3, {70, 0, 50, 50}, true, false}, {4, {80, 0, 50, 50}, true, true, true},
        {0, {90, 0, 50, 50}}, {5, {100, 0, 0, 50}}, {6, {110, 0, 50, 50}}};
    CHECK(directionalFocus(targets, 1, FocusDirection::Right) == 6);
    CHECK(!directionalFocus(targets, 2, FocusDirection::Right));
    CHECK(!directionalFocus(targets, 3, FocusDirection::Right));
    CHECK(!directionalFocus(targets, 4, FocusDirection::Right));
    CHECK(!directionalFocus(targets, 5, FocusDirection::Right));
}
TEST(directional_wrap_is_opt_in_and_chooses_the_opposite_edge_only_when_needed) {
    const std::vector<FocusTarget> targets{
        {1, {100, 100, 50, 50}}, {2, {0, 100, 50, 50}}, {3, {-100, 100, 50, 50}},
        {4, {100, 0, 50, 50}}, {5, {100, -100, 50, 50}}};
    CHECK(!directionalFocus(targets, 1, FocusDirection::Right));
    CHECK(directionalFocus(targets, 1, FocusDirection::Right, true) == 3);
    CHECK(directionalFocus(targets, 1, FocusDirection::Down, true) == 5);
    CHECK(directionalFocus(targets, 3, FocusDirection::Left, true) == 1);
    CHECK(directionalFocus(targets, 5, FocusDirection::Up, true) == 1);
    CHECK(directionalFocus(targets, 1, FocusDirection::Left, true) == 2);
    CHECK(directionalFocus(targets, 1, FocusDirection::Up, true) == 4);
    CHECK(!directionalFocus({{1, {0, 0, 10, 10}}}, 1, FocusDirection::Right, true));
    CHECK(!directionalFocus({{1, {0, 0, 10, 10}}, {2, {0, 100, 10, 10}}},
                             1, FocusDirection::Right, true));
    CHECK(directionalFocus({{1, {100, 100, 50, 50}}, {2, {-500, 0, 50, 50}},
                            {3, {0, 100, 50, 50}}}, 1, FocusDirection::Right, true) == 3);
}
TEST(focus_cycles_preserve_input_order_filter_eligibility_and_support_reverse) {
    const std::vector<FocusTarget> targets{
        {30, {0, 0, 10, 10}}, {10, {10, 0, 10, 10}, false},
        {20, {20, 0, 10, 10}}, {40, {30, 0, 10, 10}, true, false},
        {50, {40, 0, 10, 10}, true, true, true}, {60, {50, 0, 10, 10}}};
    CHECK(cycleFocus(targets) == 30);
    CHECK(cycleFocus(targets, std::nullopt, true) == 60);
    CHECK(cycleFocus(targets, 30) == 20);
    CHECK(cycleFocus(targets, 20) == 60);
    CHECK(cycleFocus(targets, 60) == 30);
    CHECK(cycleFocus(targets, 30, true) == 60);
    CHECK(cycleFocus(targets, 60, true) == 20);
    CHECK(!cycleFocus(targets, 60, false, false));
    CHECK(!cycleFocus(targets, 30, true, false));
    CHECK(cycleFocus(targets, 10) == 30);
    CHECK(cycleFocus(targets, 999, true, false) == 60);
    CHECK(!cycleFocus({}));
    CHECK(!cycleFocus({{1, {0, 0, 0, 10}}}));
    CHECK(cycleFocus({{1, {0, 0, 10, 10}}}, 1) == 1);
    CHECK(!cycleFocus({{1, {0, 0, 10, 10}}}, 1, false, false));
}
TEST(focus_history_deduplicates_recency_and_falls_back_only_to_eligible_survivors) {
    FocusHistory history;
    CHECK(history.entries().empty());
    CHECK(!history.remember(0));
    CHECK(history.remember(10)); CHECK(history.remember(20)); CHECK(history.remember(30));
    CHECK(history.remember(10));
    CHECK(history.entries() == std::vector<FocusWindowId>({10, 30, 20}));
    const std::vector<FocusTarget> targets{
        {40, {0, 0, 10, 10}}, {10, {10, 0, 10, 10}, false},
        {20, {20, 0, 10, 10}}, {30, {30, 0, 10, 10}, true, true, true}};
    CHECK(history.fallback(targets) == 20);
    CHECK(history.fallback(targets, 20) == 40);
    CHECK(history.entries() == std::vector<FocusWindowId>({10, 30, 20}));
    CHECK(history.forget(20)); CHECK(!history.forget(20)); CHECK(!history.forget(0));
    CHECK(history.fallback(targets) == 40);
    CHECK(history.fallback(targets, 40) == 20);
    CHECK(!history.fallback({}));
    CHECK(!history.fallback({{10, {0, 0, 10, 10}, false}}));
    CHECK(!history.fallback({{40, {0, 0, 10, 10}}}, 40));
    CHECK(history.remember(20));
    CHECK(history.entries() == std::vector<FocusWindowId>({20, 10, 30}));
}
TEST_MAIN()
