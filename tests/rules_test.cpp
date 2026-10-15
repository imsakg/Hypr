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

TEST_MAIN()
