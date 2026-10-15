#include "test.hpp"
#include "core/rules.hpp"
using namespace HyprCore;

TEST(class_rules_precompile_regex_and_use_search_semantics) {
    const auto parsed = parseRule("float,class:term");
    CHECK(parsed.ok());
    CHECK(parsed.rule->action().type == RuleActionType::Float);
    CHECK(parsed.rule->matches({"xterm", "", ""}));
    CHECK(!parsed.rule->matches({"browser", "", ""}));
    CHECK(parseRule("tile,class:^term$").rule->matches({"term", "", ""}));
    CHECK(!parseRule("tile,class:^term$").rule->matches({"xterm", "", ""}));
}

TEST(role_and_title_conditions_use_their_own_properties) {
    const WindowProperties dialog{"browser", "pop-up", "Save document"};
    const auto role = parseRule("float,role:^pop-up$");
    const auto title = parseRule("tile,title:Save");
    CHECK(role.ok());
    CHECK(title.ok());
    CHECK(role.rule->matches(dialog));
    CHECK(title.rule->matches(dialog));
    CHECK(!role.rule->matches({"pop-up", "normal", ""}));
    CHECK(!title.rule->matches({"Save", "Save", "Other"}));
    CHECK(parseRule("float,title:^$").rule->matches({"", "", ""}));
}

TEST(invalid_or_missing_regexes_never_create_matching_rules) {
    for (const auto* text : {"float,class:", "float,class:  ", "float,class:[", "float,role:(", "float,title:*",
                             "float,unknown:.*", "float,class", "float", "float,"}) {
        const auto result = parseRule(text);
        CHECK(!result.ok());
        CHECK(!result.rule);
        CHECK(!result.error.empty());
    }
    CHECK(!Rule{}.matches({"anything", "anything", "anything"}));
    const auto valid = parseRule("float,class:.*", 17);
    CHECK(valid.ok());
    CHECK(valid.rule->line() == 17);
    CHECK(valid.rule->matches({"", "", ""}));
}

TEST(flag_actions_are_exact_and_validate_optional_booleans) {
    CHECK(parseRuleAction("pseudo").ok());
    CHECK(parseRuleAction("pseudo").action->type == RuleActionType::Pseudo);
    CHECK(parseRuleAction("nointerventions").action->enabled);
    CHECK(!parseRuleAction("pseudo off").action->enabled);
    CHECK(parseRuleAction(" nointerventions TRUE ").action->enabled);
    CHECK(parseRule("pseudo,class:discord").ok());
    for (const auto* action : {"", "floating", "float 1", "tile extra", "pseudojunk", "pseudo maybe", "pseudo on tail", "nointerventionsXYZ"})
        CHECK(!parseRuleAction(action).ok());
}

TEST(workspace_and_monitor_actions_validate_exact_bounded_ids) {
    for (const auto* text : {"workspace 1", "workspace +2", "workspace 0x10", "workspace 2147483647", "monitor 0", "monitor +2", " monitor\t3 "})
        CHECK(parseRuleAction(text).ok());
    CHECK(parseRuleAction("workspace +2").action->type == RuleActionType::Workspace);
    CHECK(parseRuleAction("workspace 0x10").action->first == 16);
    CHECK(parseRuleAction("monitor 0").action->type == RuleActionType::Monitor);
    CHECK(parseRuleAction("monitor +2").action->first == 2);
    for (const auto* text : {"workspace", "workspace 0", "workspace -1", "workspace 1x", "workspace 1 2", "workspace 2147483648", "monitor", "monitor -1", "monitor 2x", "monitor 0 1", "monitor 999999999999999999", "monitorXYZ 1"})
        CHECK(!parseRuleAction(text).ok());
    CHECK(parseRule("workspace 2,class:term").ok());
}

TEST(move_accepts_signed_coordinates_and_size_requires_positive_dimensions) {
    const auto move = parseRuleAction(" move\t-2147483648 +0x7fffffff ");
    CHECK(move.ok());
    CHECK(move.action->type == RuleActionType::Move);
    CHECK(move.action->first == (-2147483647 - 1));
    CHECK(move.action->second == 2147483647);
    CHECK(parseRuleAction("move 0 -9").ok());
    const auto size = parseRuleAction("size +500 0x32");
    CHECK(size.ok());
    CHECK(size.action->type == RuleActionType::Size);
    CHECK(size.action->first == 500);
    CHECK(size.action->second == 50);
    for (const auto* text : {"move", "move 1", "move 1 2 3", "move 1x 2", "move 2147483648 0", "size 0 1", "size 1 -1", "size 1", "size 1 2tail", "size 1 2 3", "size -2147483649 2"})
        CHECK(!parseRuleAction(text).ok());
}

TEST(compound_selectors_require_every_condition_and_keep_regex_commas) {
    const auto both = parseRule("float,class:^browser$ && role:^pop-up$ && title:Save");
    CHECK(both.ok());
    CHECK(both.rule->matches({"browser", "pop-up", "Save document"}));
    CHECK(!both.rule->matches({"browser", "normal", "Save document"}));
    CHECK(!both.rule->matches({"other", "pop-up", "Save document"}));
    CHECK(!both.rule->matches({"browser", "pop-up", "Open document"}));
    const auto commas = parseRule(R"(float,class:^x{1,3},term$ && title:^A&&B$)");
    CHECK(commas.ok());
    CHECK(commas.rule->matches({"xx,term", "", "A&&B"}));
    CHECK(parseRule("float,class:term\t&&\trole:dialog").rule->matches({"term", "dialog", ""}));
    for (const auto* text : {"float,class:.* &&", "float,&& class:.*", "float,class:.* && && title:.*", "float,class:.* && title:", "float,class:.* && unknown:.*", "float,class:.* && role:["})
        CHECK(!parseRule(text).ok());
}

TEST(negated_selectors_invert_only_successfully_compiled_regex_matches) {
    const auto rule = parseRule("float,class:^browser$ && !role:^normal$ && !title:^Secret");
    CHECK(rule.ok());
    CHECK(rule.rule->matches({"browser", "dialog", "Save"}));
    CHECK(!rule.rule->matches({"browser", "normal", "Save"}));
    CHECK(!rule.rule->matches({"browser", "dialog", "Secret note"}));
    CHECK(!rule.rule->matches({"other", "dialog", "Save"}));
    CHECK(parseRule("tile,!class:^browser$").rule->matches({"term", "", ""}));
    CHECK(!parseRule("tile,!title:^$").rule->matches({"", "", ""}));
    CHECK(parseRule("tile,title:^!literal$").rule->matches({"", "", "!literal"}));
    for (const auto* text : {"float,!class:", "float,!title:[", "float,!!class:.*", "float,!:.*", "float,!other:.*"})
        CHECK(!parseRule(text).ok());
}

TEST(rule_sets_compile_in_source_order_with_fail_closed_line_diagnostics) {
    const auto valid = parseRules(parseConfig("# heading\nwindowrule=float,class:term\nignored=1\nwindowrule=pseudo,title:Edit\n"));
    CHECK(valid.ok());
    CHECK(valid.rules.size() == 2);
    CHECK(valid.rules[0].line() == 2);
    CHECK(valid.rules[1].line() == 4);
    const auto matches = valid.matching({"term", "", "Editor"});
    CHECK(matches.size() == 2);
    CHECK(matches[0]->action().type == RuleActionType::Float);
    CHECK(matches[1]->action().type == RuleActionType::Pseudo);
    CHECK(valid.matching({"other", "", "Other"}).empty());
    CHECK(parseRules(parseConfig("")).ok());
    const auto invalid = parseRules(parseConfig("windowrule=float,class:term\nwindowrule=size 0 5,class:.*\nwindowrule=tile,title:[\n"));
    CHECK(!invalid.ok());
    CHECK(invalid.rules.empty());
    CHECK(invalid.errors.size() == 2);
    CHECK(invalid.errors[0].line == 2);
    CHECK(invalid.errors[1].line == 3);
    CHECK(!invalid.errors[0].message.empty());
    const auto syntax = parseRules(parseConfig("missing equals\nwindowrule=float,class:term\n"));
    CHECK(!syntax.ok());
    CHECK(syntax.rules.empty());
    CHECK(syntax.errors[0].line == 1);
    CHECK(parseRules(parseConfig("bar {\nwindowrule=not a top-level rule\n}\n")).rules.empty());
}

TEST(rule_resolution_is_last_match_wins_per_independent_property) {
    const auto rules = parseRules(parseConfig(
        "windowrule=float,class:term\nwindowrule=pseudo,class:term\nwindowrule=nointerventions,class:term\n"
        "windowrule=move -50 20,class:term\nwindowrule=size 500 100,class:term\nwindowrule=workspace 3,class:term\n"
        "windowrule=monitor 0,class:term\nwindowrule=tile,title:Edit\nwindowrule=pseudo off,title:Edit\n"
        "windowrule=move 10 -20,title:Edit\nwindowrule=workspace 5,title:Edit\nwindowrule=size 9 9,class:other\n"));
    CHECK(rules.ok());
    const auto resolved = rules.resolve({"term", "", "Editor"});
    CHECK(resolved.floating == false);
    CHECK(resolved.pseudo == false);
    CHECK(resolved.noInterventions == true);
    CHECK(resolved.workspace == 5);
    CHECK(resolved.monitor == 0);
    CHECK(resolved.position == std::make_pair(10, -20));
    CHECK(resolved.size == std::make_pair(500, 100));
    const auto unmodified = rules.resolve({"term", "", "Other"});
    CHECK(unmodified.floating == true);
    CHECK(unmodified.pseudo == true);
    CHECK(unmodified.workspace == 3);
    CHECK(unmodified.position == std::make_pair(-50, 20));
    const auto absent = rules.resolve({"nothing", "", ""});
    CHECK(!absent.floating && !absent.pseudo && !absent.noInterventions);
    CHECK(!absent.workspace && !absent.monitor && !absent.position && !absent.size);
    CHECK(!parseRules(parseConfig("windowrule=float,class:[\n")).resolve({"term", "", ""}).floating);
}

TEST(config_rule_store_reloads_document_bindings_and_rules_as_one_snapshot) {
    RuleConfigStore store;
    CHECK(store.snapshot().document.entries.empty());
    CHECK(store.errors().empty());
    CHECK(store.load("count=3\nbind=,F1,exec,old command\nwindowrule=float,class:term\n"));
    CHECK(store.snapshot().document.integer("count") == 3);
    CHECK(store.snapshot().bindings.bindings[0].argument == "old command");
    CHECK(store.snapshot().rules.resolve({"term", "", ""}).floating == true);
    CHECK(!store.load("count=9\nbind=,F1,bogus,\nwindowrule=tile,class:[\n"));
    CHECK(store.errors().size() == 2);
    CHECK(store.errors()[0].line == 2);
    CHECK(store.errors()[1].line == 3);
    CHECK(store.snapshot().document.integer("count") == 3);
    CHECK(store.snapshot().bindings.bindings[0].argument == "old command");
    CHECK(store.snapshot().rules.resolve({"term", "", ""}).floating == true);
    CHECK(!store.load("count=10\nbar {\n"));
    CHECK(store.errors().size() == 1);
    CHECK(store.errors()[0].line == 2);
    CHECK(store.snapshot().document.integer("count") == 3);
    CHECK(!store.load("windowrule=tile,class:[\nbind=,F1,bogus,\n"));
    CHECK(store.errors()[0].line == 1);
    CHECK(store.errors()[1].line == 2);
    CHECK(store.load("count=4\nwindowrule=tile,class:term\n"));
    CHECK(store.errors().empty());
    CHECK(store.snapshot().document.integer("count") == 4);
    CHECK(store.snapshot().bindings.bindings.empty());
    CHECK(store.snapshot().rules.resolve({"term", "", ""}).floating == false);
    CHECK(store.load(""));
    CHECK(store.snapshot().document.entries.empty());
    CHECK(store.snapshot().rules.rules.empty());
    CHECK(store.snapshot().bindings.bindings.empty());
    CHECK(store.errors().empty());
}

TEST(embedded_nul_rule_patterns_fail_before_regex_compilation_and_reload) {
    const std::string nul(1, '\0');
    const auto direct = parseRule("float,class:before" + nul + "after");
    CHECK(!direct.ok());
    CHECK(direct.error.find("NUL") != std::string::npos);
    CHECK(!parseRuleAction("pseudo" + nul).ok());
    RuleConfigStore store;
    CHECK(store.load("windowrule=float,class:term\n"));
    CHECK(!store.load("windowrule=tile,class:before" + nul + "after\n"));
    CHECK(store.errors().size() == 1 && store.errors()[0].line == 1);
    CHECK(store.snapshot().rules.resolve({"term", "", ""}).floating == true);
}

TEST_MAIN()
