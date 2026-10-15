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
TEST(folded_layout_alternates_width_and_height) {
 LayoutOptions o; o.kind=LayoutKind::Folded;
 const auto cells=tile(Rect{0,0,100,80},3,o);
 CHECK(cells[0]==Rect{0,0,50,80}); CHECK(cells[1]==Rect{50,0,50,40}); CHECK(cells[2]==Rect{50,40,50,40});
}
TEST(folded_ratios_are_finite_and_bounded) {
 LayoutOptions o; o.kind=LayoutKind::Folded; o.ratio=0.8;
 CHECK(tile(Rect{0,0,100,80},2,o)[0].width==80);
 o.ratio=std::numeric_limits<double>::quiet_NaN(); CHECK(tile(Rect{0,0,100,80},2,o)[0].width==50);
 o.ratio=2; CHECK(tile(Rect{0,0,100,80},2,o)[0].width==90);
}
TEST(folded_layout_can_start_on_the_vertical_axis) {
 LayoutOptions o; o.kind=LayoutKind::Folded; o.foldHorizontal=false;
 CHECK(tile(Rect{-30,4,80,100},2,o)[0]==Rect{-30,4,80,50});
}
TEST(arrangements_skip_floating_and_hidden_clients) {
 const auto cells=arrange(Rect{0,0,100,80},{{10,false,false,false},{20,true,false,false},{30,false,true,false},{40,false,false,false}});
 CHECK(cells.size()==2); CHECK(cells[1].id==40); CHECK(cells[1].bounds.x==50);
}
TEST(fullscreen_ignores_tile_gaps_and_floating_state) {
 LayoutOptions o; o.outerGap=10;
 const auto cells=arrange(Rect{-10,2,100,80},{{1,false,false,false},{2,true,false,true}},o);
 CHECK(cells.size()==1); CHECK(cells[0].id==2); CHECK(cells[0].bounds==Rect{-10,2,100,80});
}
TEST(first_snapshot_owns_layout_eligibility) {
 const auto cells=arrange(Rect{0,0,100,80},{{1,false,true,false},{1,false,false,true},{2,false,false,false},{2,false,false,false}});
 CHECK(cells.size()==1); CHECK(cells[0].id==2);
}
TEST(reverse_layout_changes_placement_not_client_data) {
 LayoutOptions o; o.reverse=true;
 const auto cells=arrange(Rect{0,0,100,80},{{1,false,false,false},{2,false,false,false}},o);
 CHECK(cells[0].id==2); CHECK(cells[1].id==1);
}
TEST(smart_gaps_expand_only_single_client_workspaces) {
 LayoutOptions o; o.smartGaps=true; o.outerGap=10;
 CHECK(tile(Rect{0,0,100,80},1,o)[0]==Rect{0,0,100,80});
 CHECK(tile(Rect{0,0,100,80},2,o)[0].x==10);
}
TEST_MAIN()
