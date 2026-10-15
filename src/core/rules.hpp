#pragma once

#include "config.hpp"
#include <regex>

namespace HyprCore {

struct WindowProperties {
    std::string className;
    std::string role;
    std::string title;
};

enum class RuleActionType { Float, Tile, Pseudo, NoInterventions, Workspace, Monitor, Move, Size };
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
    std::istringstream stream(text);
    stream.imbue(std::locale::classic());
    std::vector<std::string> words;
    std::string word;
    while (stream >> word) words.push_back(word);
    if (!words.empty() && (words[0] == "pseudo" || words[0] == "nointerventions")) {
        const auto enabled = words.size() == 1 ? std::optional<bool>(true) : parseBoolean(words[1]);
        if (words.size() > 2 || !enabled) return {std::nullopt, "flag action expects one optional boolean"};
        const auto type = words[0] == "pseudo" ? RuleActionType::Pseudo : RuleActionType::NoInterventions;
        return {RuleAction{type, 0, 0, *enabled}, ""};
    }
    if (!words.empty() && (words[0] == "workspace" || words[0] == "monitor")) {
        if (words.size() != 2) return {std::nullopt, "ID action expects exactly one integer"};
        const auto id = parseInteger(words[1]);
        const bool workspace = words[0] == "workspace";
        if (!id || *id < (workspace ? 1 : 0)) return {std::nullopt, "action ID is out of range"};
        return {RuleAction{workspace ? RuleActionType::Workspace : RuleActionType::Monitor, *id, 0, true}, ""};
    }
    if (!words.empty() && (words[0] == "move" || words[0] == "size")) {
        if (words.size() != 3) return {std::nullopt, "geometry action expects exactly two integers"};
        const auto first = parseInteger(words[1]);
        const auto second = parseInteger(words[2]);
        const bool size = words[0] == "size";
        if (!first || !second || (size && (*first <= 0 || *second <= 0)))
            return {std::nullopt, "invalid coordinates or nonpositive size"};
        return {RuleAction{size ? RuleActionType::Size : RuleActionType::Move, *first, *second, true}, ""};
    }
    return {std::nullopt, "unknown or malformed rule action: " + text};
}

enum class RuleTarget { Class, Role, Title };
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
            const std::string* property = &window.className;
            if (condition.target == RuleTarget::Role) property = &window.role;
            else if (condition.target == RuleTarget::Title) property = &window.title;
            try {
                if (!std::regex_search(*property, condition.expression)) return false;
            } catch (const std::regex_error&) {
                return false;
            }
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
    const auto colon = condition.find(':');
    const auto targetName = condition.substr(0, colon);
    RuleTarget target = RuleTarget::Class;
    if (targetName == "role") target = RuleTarget::Role;
    else if (targetName == "title") target = RuleTarget::Title;
    else if (targetName != "class") return {std::nullopt, "unknown rule target: " + targetName};
    if (colon == std::string::npos) return {std::nullopt, "expected target:regex condition"};
    const auto pattern = trim(condition.substr(colon + 1));
    if (pattern.empty()) return {std::nullopt, "rule regex must not be empty"};
    Rule rule;
    rule.action_ = *action.action;
    rule.line_ = line;
    try {
        rule.conditions_.push_back({target, pattern, std::regex(pattern, std::regex::ECMAScript | std::regex::optimize), false});
    } catch (const std::regex_error& error) {
        return {std::nullopt, std::string("invalid regex: ") + error.what()};
    }
    return {std::move(rule), ""};
}

} // namespace HyprCore
