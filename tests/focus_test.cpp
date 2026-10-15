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
TEST_MAIN()
