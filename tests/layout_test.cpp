#include "test.hpp"
#include "core/layout.hpp"
using namespace HyprCore;
TEST(horizontal_cells_cover_even_monitor_widths) {
 const auto cells=tile(Rect{0,10,100,60},2);
 CHECK(cells.size()==2); CHECK(cells[0]==Rect{0,10,50,60}); CHECK(cells[1]==Rect{50,10,50,60});
 CHECK(tile(Rect{0,0,10,10},0).empty());
}
TEST(vertical_rows_preserve_monitor_offsets) {
 LayoutOptions options; options.kind=LayoutKind::Vertical;
 CHECK(tile(Rect{-10,-20,100,60},2,options)[1]==Rect{-10,10,100,30});
}
TEST(odd_extents_have_no_missing_pixels) {
 const auto cells=tile(Rect{0,0,101,30},3);
 CHECK(cells[0].width==34); CHECK(cells[1].width==34); CHECK(cells[2]==Rect{68,0,33,30});
}
TEST(inner_gaps_consume_only_available_axis_space) {
 LayoutOptions options; options.innerGap=3;
 const auto cells=tile(Rect{0,0,100,30},3,options);
 CHECK(cells[0].width==32); CHECK(cells[1].x==35); CHECK(cells[2].x+cells[2].width==100);
 options.innerGap=1000; const auto tiny=tile(Rect{0,0,2,3},3,options);
 CHECK(tiny[0].width>=0); CHECK(tiny.back().x+tiny.back().width==2);
}
TEST(outer_gap_is_not_reapplied_to_each_cell) {
 LayoutOptions options; options.outerGap=5;
 const auto cells=tile(Rect{-100,10,100,60},2,options);
 CHECK(cells[0]==Rect{-95,15,45,50}); CHECK(cells[1]==Rect{-50,15,45,50});
}
TEST_MAIN()
