#include "test.hpp"
#include "core/resize.hpp"
using namespace HyprCore;
TEST(size_hints_enforce_positive_minima) {
 SizeHints h; h.minWidth=40; h.minHeight=20;
 CHECK(constrainSize(Rect{-5,7,2,0},h)==Rect{-5,7,40,20});
}
TEST(maximum_hints_never_invert_clamp_bounds) {
 SizeHints h; h.minWidth=40; h.maxWidth=20; h.maxHeight=50;
 CHECK(constrainSize(Rect{0,0,100,100},h)==Rect{0,0,40,50});
}
TEST_MAIN()
