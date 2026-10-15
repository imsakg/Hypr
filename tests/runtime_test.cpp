#include "test.hpp"
#include "core/runtime.hpp"
using namespace HyprCore;
TEST(runtime_headers_compose_without_type_collisions) {
 CHECK(flatLayout(2)); CHECK(flatLayout(4)); CHECK(!flatLayout(1)); CHECK(!flatLayout(5));
 Binding input; ConfigBinding record; CHECK(input.key==0); CHECK(record.key.empty());
}
TEST(runtime_layout_budget_includes_docks_and_gaps_once) {
 auto area=reserve(Rect{-100,0,100,80},0,10,0,0);
 LayoutOptions o; o.innerGap=4; o.outerGap=3;
 const auto cells=arrange(area,{{1,false,false,false},{2,false,false,false}},o);
 CHECK(cells[0].bounds==Rect{-97,13,45,64}); CHECK(cells[1].bounds==Rect{-48,13,45,64});
}
TEST(flat_layout_close_and_reopen_reclaims_the_entire_area) {
 const Rect bounds{0,0,101,80}; LayoutOptions o; o.kind=LayoutKind::Folded;
 CHECK(arrange(bounds,{{1,false,false,false}},o)[0].bounds==bounds);
 const auto split=arrange(bounds,{{1,false,false,false},{3,false,false,false}},o);
 CHECK(split.size()==2); CHECK(split[0].bounds.width+split[1].bounds.width==101);
}
TEST_MAIN()
