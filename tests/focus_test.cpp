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
TEST_MAIN()
