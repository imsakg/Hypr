#pragma once

#include "config.hpp"
#include <regex>

namespace HyprCore {

struct WindowProperties {
    std::string className;
    std::string role;
    std::string title;
};

enum class RuleActionType { Float, Tile };
struct RuleAction {
    RuleActionType type = RuleActionType::Float;
    int first = 0;
    int second = 0;
    bool enabled = true;
};
struct ActionParseResult {
    std::optional<RuleAction> action;
    std::string error;
    bool ok() const { return action.has_value(); }
};

inline ActionParseResult parseRuleAction(const std::string& input) {
    const auto text = trim(input);
    if (text == "float") return {RuleAction{RuleActionType::Float, 0, 0, true}, ""};
    if (text == "tile") return {RuleAction{RuleActionType::Tile, 0, 0, true}, ""};
    return {std::nullopt, "unknown or malformed rule action: " + text};
}

enum class RuleTarget { Class };
struct RuleCondition {
    RuleTarget target = RuleTarget::Class;
    std::string pattern;
    std::regex expression;
    bool negated = false;
};

struct RuleParseResult;
class Rule {
public:
    Rule() = default;
    const RuleAction& action() const { return action_; }
    std::size_t line() const { return line_; }
    bool matches(const WindowProperties& window) const {
        if (conditions_.empty()) return false;
        for (const auto& condition : conditions_) {
            if (!std::regex_search(window.className, condition.expression)) return false;
        }
        return true;
    }
private:
    RuleAction action_;
    std::vector<RuleCondition> conditions_;
    std::size_t line_ = 0;
    friend RuleParseResult parseRule(const std::string&, std::size_t);
};

struct RuleParseResult {
    std::optional<Rule> rule;
    std::string error;
    bool ok() const { return rule.has_value(); }
};

inline RuleParseResult parseRule(const std::string& input, std::size_t line = 0) {
    const auto comma = input.find(',');
    if (comma == std::string::npos) return {std::nullopt, "expected action,condition"};
    const auto action = parseRuleAction(input.substr(0, comma));
    if (!action.ok()) return {std::nullopt, action.error};
    const auto condition = trim(input.substr(comma + 1));
    if (condition.compare(0, 6, "class:") != 0) return {std::nullopt, "expected class:regex condition"};
    const auto pattern = condition.substr(6);
    Rule rule;
    rule.action_ = *action.action;
    rule.line_ = line;
    try {
        rule.conditions_.push_back({RuleTarget::Class, pattern, std::regex(pattern), false});
    } catch (const std::regex_error& error) {
        return {std::nullopt, std::string("invalid regex: ") + error.what()};
    }
    return {std::move(rule), ""};
}

} // namespace HyprCore
