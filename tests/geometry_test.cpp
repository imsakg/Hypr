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
TEST(insets_handle_oversized_and_negative_gaps) {
 CHECK(inset(Rect{10,20,100,60},5) == Rect{15,25,90,50});
 CHECK(inset(Rect{0,0,2,2},9) == Rect{1,1,0,0});
 CHECK(inset(Rect{0,0,2,2},-1) == Rect{0,0,2,2});
}
TEST_MAIN()
