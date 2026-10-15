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

TEST_MAIN()
