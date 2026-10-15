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
TEST_MAIN()
