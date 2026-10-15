#include "test.hpp"
#include "core/geometry.hpp"
using namespace HyprCore;
TEST(rectangles_compare_all_coordinates) {
    CHECK((Rect{1, 2, 3, 4} == Rect{1, 2, 3, 4}));
    CHECK(!(Rect{1, 2, 3, 4} == Rect{0, 2, 3, 4}));
}
TEST_MAIN()
