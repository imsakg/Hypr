#include "test.hpp"
#include "core/resize.hpp"
using namespace HyprCore;
TEST(size_hints_enforce_positive_minima) {
 SizeHints h; h.minWidth=40; h.minHeight=20;
 CHECK(constrainSize(Rect{-5,7,2,0},h)==Rect{-5,7,40,20});
}
TEST_MAIN()
