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

TEST_MAIN()
