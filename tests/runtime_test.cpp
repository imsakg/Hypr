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
TEST(runtime_config_handles_blank_lines_quotes_and_partial_numbers) {
 const auto doc=parseConfig("  \n# comment\nexec = printf '#ok' # tail\n");
 CHECK(doc.ok()); CHECK(doc.entries.size()==1); CHECK(doc.entries[0].value=="printf '#ok'");
 CHECK(!parseReal("nan")); CHECK(!parseInteger("20px"));
}
TEST(runtime_key_adapters_preserve_old_configs_without_accepting_garbage) {
 CHECK(runtimeModifiers("SUPERSHIFT")==runtimeModifiers("SUPER SHIFT"));
 CHECK(!runtimeModifiers("NOTSUPER")); CHECK(!runtimeModifiers("SUPERCTRLJUNK"));
 CHECK(runtimeKeysym("Q")==static_cast<KeySym>('q')); CHECK(runtimeKeysym("equals")==static_cast<KeySym>('='));
 CHECK(!runtimeKeysym("$(touch /tmp/bad)"));
}
TEST(live_chords_ignore_lock_bits_but_not_extra_shortcut_modifiers) {
 const auto super=*runtimeModifiers("SUPER");
 CHECK(modifiersMatch(super,super|ModLock|Mod2)); CHECK(!modifiersMatch(super,super|ModShift));
 CHECK(grabVariants(super).size()==4);
}
TEST(runtime_rule_compilation_never_applies_broken_regexes) {
 CHECK(!parseRule("float,class:[").ok());
 const auto rule=parseRule("float,class:^Term$ && !title:^Secret$"); CHECK(rule.ok());
 CHECK(rule.rule->matches({"Term","","Public"})); CHECK(!rule.rule->matches({"Term","","Secret"}));
}
TEST(workspace_dispatch_resolves_targets_without_unsigned_wrap) {
 CHECK(workspaceTarget({1,3,8},3,1,"next")==8); CHECK(workspaceTarget({1,3,8},1,3,"prev")==8);
 CHECK(workspaceTarget({1,3,8},3,1,"+2")==5); CHECK(!workspaceTarget({1,3},1,3,"-2"));
 CHECK(parseConfigBinding("SUPER,1,workspace,-1").ok()); CHECK(parseConfigBinding("SUPER,1,workspace,next").ok());
}
TEST(focus_history_cleanup_is_idempotent_for_closed_clients) {
 FocusHistory history; history.remember(9); history.remember(3); CHECK(history.forget(9)); CHECK(!history.forget(9));
 CHECK(history.fallback({{9,{0,0,20,20}},{3,{30,0,20,20}}})==3);
}
TEST(resize_bindings_accept_exact_signed_pixel_pairs) {
 CHECK(resizeArgument("20 -10")==std::make_pair(20,-10)); CHECK(!resizeArgument("20x -10"));
 CHECK(!resizeArgument("20 -10 extra")); CHECK(parseConfigBinding("SUPER,H,resizeactive,20 -10").ok());
 CHECK(!parseConfigBinding("SUPER,H,resizeactive,20").ok());
}
TEST(runtime_preflight_catches_schema_errors_before_any_effects) {
 CHECK(validateRuntimeConfig("layout = folded\nbar {\ncol.bg=0xffffffff\n}\nbind=SUPERSHIFT,Q,resizeactive,20 -10\n").empty());
 CHECK(!validateRuntimeConfig("exec=touch never-run\ngaps_in=10bad\n").empty());
 CHECK(!validateRuntimeConfig("animations {\nspeed=nan\n}\n").empty());
 CHECK(!validateRuntimeConfig("unknown=1\n").empty()); CHECK(runtimeInteger("0xffffffff")==4294967295LL);
}
TEST_MAIN()
