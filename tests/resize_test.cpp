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
TEST(resize_deltas_saturate_before_enforcing_hints) {
 CHECK(resizeBy(Rect{4,8,80,60},20,-100)==Rect{4,8,100,1});
 CHECK(resizeBy(Rect{0,0,100,60},std::numeric_limits<int>::max(),0).width==std::numeric_limits<int>::max());
}
TEST(edge_resizing_keeps_right_and_bottom_anchored) {
 CHECK(resizeEdges(Rect{10,20,80,60},20,10,true,true)==Rect{30,30,60,50});
 SizeHints h; h.minWidth=40;
 CHECK(resizeEdges(Rect{10,20,80,60},100,0,true,false,h)==Rect{50,20,40,60});
}
TEST_MAIN()
