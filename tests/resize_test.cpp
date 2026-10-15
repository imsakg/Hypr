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
TEST(terminal_cell_increments_snap_from_base_size) {
 SizeHints h; h.baseWidth=4; h.widthIncrement=8; h.baseHeight=2; h.heightIncrement=16;
 CHECK(constrainSize(Rect{0,0,83,51},h)==Rect{0,0,76,50});
 h.widthIncrement=0; CHECK(constrainSize(Rect{0,0,83,51},h).width==83);
}
TEST_MAIN()
