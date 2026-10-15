#include "test.hpp"
#include "core/geometry.hpp"
using namespace HyprCore;
TEST(rectangles_compare_all_coordinates) {
    CHECK((Rect{1, 2, 3, 4} == Rect{1, 2, 3, 4}));
    CHECK(!(Rect{1, 2, 3, 4} == Rect{0, 2, 3, 4}));
}
TEST(rectangles_use_half_open_hit_testing) {
    CHECK(contains(Rect{-10, 3, 20, 8}, -10, 3));
    CHECK(!contains(Rect{-10, 3, 20, 8}, 10, 3));
    CHECK(!contains(Rect{0, 0, 0, 8}, 0, 0));
}
TEST_MAIN()
