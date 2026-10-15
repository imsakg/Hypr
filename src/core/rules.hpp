#pragma once

#include "config.hpp"
#include <algorithm>
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
    if (input.find('\0') != std::string::npos) return {std::nullopt, "embedded NUL byte in rule action"};
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
                const bool matched = std::regex_search(*property, condition.expression);
                if (matched == condition.negated) return false;
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

inline std::size_t ruleConditionSeparator(const std::string& text, std::size_t first) {
    bool escaped = false;
    for (std::size_t i = first; i < text.size(); ++i) {
        if (escaped) { escaped = false; continue; }
        if (text[i] == '\\') { escaped = true; continue; }
        if (text[i] == '&' && i + 1 < text.size() && text[i + 1] == '&' &&
            (i == first || configSpace(text[i - 1])) &&
            (i + 2 == text.size() || configSpace(text[i + 2]))) return i;
    }
    return std::string::npos;
}

inline RuleParseResult parseRule(const std::string& input, std::size_t line = 0) {
    if (input.find('\0') != std::string::npos) return {std::nullopt, "embedded NUL byte in rule"};
    const auto comma = input.find(',');
    if (comma == std::string::npos) return {std::nullopt, "expected action,condition"};
    const auto action = parseRuleAction(input.substr(0, comma));
    if (!action.ok()) return {std::nullopt, action.error};
    const auto selectors = trim(input.substr(comma + 1));
    Rule rule;
    rule.action_ = *action.action;
    rule.line_ = line;
    std::size_t first = 0;
    while (true) {
        const auto separator = ruleConditionSeparator(selectors, first);
        const auto condition = trim(selectors.substr(first, separator == std::string::npos ? separator : separator - first));
        const auto colon = condition.find(':');
        if (colon == std::string::npos) return {std::nullopt, "expected target:regex condition"};
        auto targetName = condition.substr(0, colon);
        const bool negated = !targetName.empty() && targetName[0] == '!';
        if (negated) targetName.erase(0, 1);
        RuleTarget target = RuleTarget::Class;
        if (targetName == "role") target = RuleTarget::Role;
        else if (targetName == "title") target = RuleTarget::Title;
        else if (targetName != "class") return {std::nullopt, "unknown rule target: " + targetName};
        const auto pattern = trim(condition.substr(colon + 1));
        if (pattern.empty()) return {std::nullopt, "rule regex must not be empty"};
        try {
            rule.conditions_.push_back({target, pattern, std::regex(pattern, std::regex::ECMAScript | std::regex::optimize), negated});
        } catch (const std::regex_error& error) {
            return {std::nullopt, std::string("invalid regex: ") + error.what()};
        }
        if (separator == std::string::npos) break;
        first = separator + 2;
    }
    return {std::move(rule), ""};
}

struct ResolvedRules {
    // Disengaged optionals leave the corresponding window property unchanged.
    std::optional<bool> floating;
    std::optional<bool> pseudo;
    std::optional<bool> noInterventions;
    std::optional<int> workspace;
    std::optional<int> monitor;
    std::optional<std::pair<int, int>> position;
    std::optional<std::pair<int, int>> size;
};

struct RuleSet {
    std::vector<Rule> rules;
    std::vector<ConfigError> errors;
    bool ok() const { return errors.empty(); }
    // Returned pointers remain valid only while this unchanged set is alive.
    std::vector<const Rule*> matching(const WindowProperties& window) const {
        std::vector<const Rule*> result;
        if (!ok()) return result;
        for (const auto& rule : rules) if (rule.matches(window)) result.push_back(&rule);
        return result;
    }
    ResolvedRules resolve(const WindowProperties& window) const {
        ResolvedRules result;
        if (!ok()) return result;
        for (const auto& rule : rules) {
            if (!rule.matches(window)) continue;
            const auto& action = rule.action();
            switch (action.type) {
                case RuleActionType::Float: result.floating = true; break;
                case RuleActionType::Tile: result.floating = false; break;
                case RuleActionType::Pseudo: result.pseudo = action.enabled; break;
                case RuleActionType::NoInterventions: result.noInterventions = action.enabled; break;
                case RuleActionType::Workspace: result.workspace = action.first; break;
                case RuleActionType::Monitor: result.monitor = action.first; break;
                case RuleActionType::Move: result.position = std::make_pair(action.first, action.second); break;
                case RuleActionType::Size: result.size = std::make_pair(action.first, action.second); break;
            }
        }
        return result;
    }
};

inline RuleSet parseRules(const ConfigDocument& document) {
    RuleSet result;
    result.errors = document.errors;
    for (const auto& entry : document.all("windowrule")) {
        auto parsed = parseRule(entry.value, entry.line);
        if (!parsed.ok()) result.errors.push_back({entry.line, parsed.error});
        else result.rules.push_back(std::move(*parsed.rule));
    }
    if (!result.ok()) result.rules.clear();
    return result;
}

struct RuleConfigSnapshot {
    ConfigDocument document;
    ConfigBindingList bindings;
    RuleSet rules;
};

// Validation transaction only; callers provide synchronization and apply X11 state.
class RuleConfigStore {
public:
    bool load(const std::string& text) {
        RuleConfigSnapshot candidate;
        candidate.document = parseConfig(text);
        errors_ = candidate.document.errors;
        if (!candidate.document.ok()) return false;
        candidate.bindings = parseConfigBindings(candidate.document);
        candidate.rules = parseRules(candidate.document);
        errors_ = candidate.bindings.errors;
        errors_.insert(errors_.end(), candidate.rules.errors.begin(), candidate.rules.errors.end());
        std::stable_sort(errors_.begin(), errors_.end(), [](const ConfigError& left, const ConfigError& right) {
            return left.line < right.line;
        });
        if (!errors_.empty()) return false;
        snapshot_ = std::move(candidate);
        return true;
    }
    const RuleConfigSnapshot& snapshot() const { return snapshot_; }
    const std::vector<ConfigError>& errors() const { return errors_; }
private:
    RuleConfigSnapshot snapshot_;
    std::vector<ConfigError> errors_;
};

} // namespace HyprCore
