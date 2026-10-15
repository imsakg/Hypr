#pragma once

#include <cmath>
#include <limits>
#include <locale>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace HyprCore {

inline bool configSpace(char c) {
    return c == ' ' || c == '\t' || c == '\r' || c == '\n' || c == '\v' || c == '\f';
}

inline std::string trim(const std::string& text) {
    std::size_t first = 0;
    while (first < text.size() && configSpace(text[first])) ++first;
    std::size_t last = text.size();
    while (last > first && configSpace(text[last - 1])) --last;
    return text.substr(first, last - first);
}

inline std::string stripComment(const std::string& text) {
    char quote = 0;
    bool escaped = false;
    for (std::size_t i = 0; i < text.size(); ++i) {
        const char c = text[i];
        if (escaped) { escaped = false; continue; }
        if (c == '\\') { escaped = true; continue; }
        if (quote) {
            if (c == quote) quote = 0;
        } else if (c == '\'' || c == '"') {
            quote = c;
        } else if (c == '#') {
            return text.substr(0, i);
        }
    }
    return text;
}

inline std::optional<int> parseInteger(const std::string& input) {
    const auto text = trim(input);
    std::size_t i = 0;
    bool negative = false;
    if (!text.empty() && (text[0] == '+' || text[0] == '-')) {
        negative = text[0] == '-';
        ++i;
    }
    unsigned int base = 10;
    if (text.size() - i >= 2 && text[i] == '0' && (text[i + 1] == 'x' || text[i + 1] == 'X')) {
        base = 16;
        i += 2;
    }
    if (i == text.size()) return std::nullopt;
    const auto positiveLimit = static_cast<unsigned int>(std::numeric_limits<int>::max());
    const auto limit = negative ? positiveLimit + 1U : positiveLimit;
    unsigned int value = 0;
    for (; i < text.size(); ++i) {
        unsigned int digit = base;
        if (text[i] >= '0' && text[i] <= '9') digit = static_cast<unsigned int>(text[i] - '0');
        else if (text[i] >= 'a' && text[i] <= 'f') digit = static_cast<unsigned int>(text[i] - 'a') + 10U;
        else if (text[i] >= 'A' && text[i] <= 'F') digit = static_cast<unsigned int>(text[i] - 'A') + 10U;
        if (digit >= base || value > (limit - digit) / base) return std::nullopt;
        value = value * base + digit;
    }
    if (negative && value == positiveLimit + 1U) return std::numeric_limits<int>::min();
    const auto signedValue = static_cast<int>(value);
    return negative ? -signedValue : signedValue;
}

inline std::optional<double> parseReal(const std::string& input) {
    const auto text = trim(input);
    if (text.empty()) return std::nullopt;
    // Reject alternate numeric syntaxes regardless of the active locale.
    for (char c : text) {
        if ((c < '0' || c > '9') && c != '+' && c != '-' && c != '.' && c != 'e' && c != 'E')
            return std::nullopt;
    }
    std::istringstream stream(text);
    stream.imbue(std::locale::classic());
    double value = 0;
    stream >> std::noskipws >> value;
    if (stream.fail() || !stream.eof() || !std::isfinite(value)) return std::nullopt;
    return value;
}

inline std::string configLower(std::string text) {
    for (auto& c : text) if (c >= 'A' && c <= 'Z') c = static_cast<char>(c - 'A' + 'a');
    return text;
}

inline std::optional<bool> parseBoolean(const std::string& input) {
    const auto text = configLower(trim(input));
    if (text == "true" || text == "yes" || text == "on" || text == "1") return true;
    if (text == "false" || text == "no" || text == "off" || text == "0") return false;
    return std::nullopt;
}

// The final bounded field is opaque text, suitable for an exec command tail.
inline std::vector<std::string> splitFields(const std::string& text,
                                          std::size_t maxFields = std::numeric_limits<std::size_t>::max()) {
    std::vector<std::string> fields;
    if (maxFields == 0) return fields;
    std::string field;
    char quote = 0;
    for (std::size_t i = 0; i < text.size(); ++i) {
        if (fields.size() == maxFields - 1) {
            fields.push_back(trim(text.substr(i)));
            return fields;
        }
        const char c = text[i];
        if (c == '\\' && i + 1 < text.size()) {
            const char next = text[i + 1];
            if (next == ',' || next == '\\' || next == '\'' || next == '"' || next == '#') {
                field += next;
                ++i;
                continue;
            }
        }
        if (quote) {
            if (c == quote) quote = 0;
            else field += c;
        } else if (c == '\'' || c == '"') quote = c;
        else if (c == ',') { fields.push_back(trim(field)); field.clear(); }
        else field += c;
    }
    if (quote) throw std::invalid_argument("unclosed quote in fields");
    fields.push_back(trim(field));
    return fields;
}

struct ConfigError {
    std::size_t line = 0;
    std::string message;
};

struct ConfigEntry {
    std::string category;
    std::string key;
    std::string value; // Raw value: no command execution or variable expansion.
    std::size_t line = 0;
    std::string qualifiedKey() const { return category.empty() ? key : category + ':' + key; }
};

struct ConfigDocument {
    std::vector<ConfigEntry> entries;
    std::vector<ConfigError> errors;
    bool ok() const { return errors.empty(); }
    const ConfigEntry* last(const std::string& qualifiedKey) const {
        for (auto it = entries.rbegin(); it != entries.rend(); ++it)
            if (it->qualifiedKey() == qualifiedKey) return &*it;
        return nullptr;
    }
    std::vector<ConfigEntry> all(const std::string& qualifiedKey) const {
        std::vector<ConfigEntry> result;
        for (const auto& entry : entries)
            if (entry.qualifiedKey() == qualifiedKey) result.push_back(entry);
        return result;
    }
    std::optional<std::string> value(const std::string& key) const {
        const auto* entry = last(key);
        return entry ? std::optional<std::string>(entry->value) : std::nullopt;
    }
    std::optional<int> integer(const std::string& key) const {
        const auto* entry = last(key);
        return entry ? parseInteger(entry->value) : std::nullopt;
    }
    std::optional<double> real(const std::string& key) const {
        const auto* entry = last(key);
        return entry ? parseReal(entry->value) : std::nullopt;
    }
    std::optional<bool> boolean(const std::string& key) const {
        const auto* entry = last(key);
        return entry ? parseBoolean(entry->value) : std::nullopt;
    }
};

inline bool configIdentifier(const std::string& text) {
    if (text.empty()) return false;
    for (char c : text) {
        if ((c < 'a' || c > 'z') && (c < 'A' || c > 'Z') && (c < '0' || c > '9') &&
            c != '_' && c != '-' && c != '.' && c != ':') return false;
    }
    return true;
}

inline std::size_t configFindUnquoted(const std::string& text, char delimiter) {
    char quote = 0;
    bool escaped = false;
    for (std::size_t i = 0; i < text.size(); ++i) {
        const char c = text[i];
        if (escaped) { escaped = false; continue; }
        if (c == '\\') { escaped = true; continue; }
        if (quote) { if (c == quote) quote = 0; }
        else if (c == '\'' || c == '"') quote = c;
        else if (c == delimiter) return i;
    }
    return std::string::npos;
}

inline bool configQuotesClosed(const std::string& text) {
    char quote = 0;
    bool escaped = false;
    for (char c : text) {
        if (escaped) { escaped = false; continue; }
        if (c == '\\') { escaped = true; continue; }
        if (quote) { if (c == quote) quote = 0; }
        else if (c == '\'' || c == '"') quote = c;
    }
    return quote == 0;
}

inline ConfigDocument parseConfig(const std::string& text) {
    ConfigDocument document;
    std::istringstream stream(text);
    std::string line;
    std::size_t lineNumber = 0;
    std::vector<std::pair<std::string, std::size_t>> scopes;
    while (std::getline(stream, line)) {
        ++lineNumber;
        line = trim(stripComment(line));
        if (line.empty()) continue;
        if (!configQuotesClosed(line)) {
            document.errors.push_back({lineNumber, "unclosed quote"});
            continue;
        }
        const auto equal = configFindUnquoted(line, '=');
        if (line == "}") {
            if (scopes.empty()) document.errors.push_back({lineNumber, "unexpected closing category"});
            else scopes.pop_back();
            continue;
        }
        if (equal == std::string::npos && line.back() == '{') {
            const auto name = configLower(trim(line.substr(0, line.size() - 1)));
            if (!configIdentifier(name) || name.find(':') != std::string::npos)
                document.errors.push_back({lineNumber, "invalid category"});
            else scopes.emplace_back(name, lineNumber);
            continue;
        }
        if (equal == std::string::npos) {
            document.errors.push_back({lineNumber, "expected key=value"});
            continue;
        }
        const auto key = trim(line.substr(0, equal));
        if (!configIdentifier(key)) {
            document.errors.push_back({lineNumber, "invalid or empty key"});
            continue;
        }
        std::string category;
        for (const auto& scope : scopes) {
            if (!category.empty()) category += ':';
            category += scope.first;
        }
        document.entries.push_back({category, key, trim(line.substr(equal + 1)), lineNumber});
    }
    for (const auto& scope : scopes)
        document.errors.push_back({scope.second, "unclosed category: " + scope.first});
    return document;
}

class ConfigStore {
public:
    bool load(const std::string& text) {
        auto candidate = parseConfig(text);
        errors_ = candidate.errors;
        if (!candidate.ok()) return false;
        document_ = std::move(candidate);
        return true;
    }
    const ConfigDocument& document() const { return document_; }
    const std::vector<ConfigError>& errors() const { return errors_; }
private:
    ConfigDocument document_;
    std::vector<ConfigError> errors_;
};

struct Binding {
    std::string modifiers;
    std::string key;
    std::string dispatcher;
    std::string argument;
    std::size_t line = 0;
};

struct BindingParseResult {
    std::optional<Binding> binding;
    std::string error;
    bool ok() const { return binding.has_value(); }
};

inline BindingParseResult parseBinding(const std::string& text) {
    if (!configQuotesClosed(text)) return {std::nullopt, "unclosed quote in binding"};
    std::vector<std::string> fields;
    try { fields = splitFields(text, 4); }
    catch (const std::invalid_argument& error) { return {std::nullopt, error.what()}; }
    if (fields.size() != 4) return {std::nullopt, "binding needs modifiers,key,dispatcher,argument"};
    if (fields[1].empty() || fields[2].empty()) return {std::nullopt, "binding key and dispatcher must not be empty"};
    bool known = false;
    for (const auto* name : {"exec", "killactive", "fullscreen", "movewindow", "movefocus", "workspace",
                             "movetoworkspace", "lastworkspace", "togglefloating", "splitratio", "pseudo"})
        if (fields[2] == name) known = true;
    if (!known) return {std::nullopt, "unknown binding dispatcher: " + fields[2]};
    if (fields[2] == "exec" && fields[3].empty()) return {std::nullopt, "exec requires a command"};
    for (const auto* name : {"killactive", "fullscreen", "lastworkspace", "togglefloating", "pseudo"})
        if (fields[2] == name && !fields[3].empty()) return {std::nullopt, "dispatcher does not take an argument"};
    return {Binding{fields[0], fields[1], fields[2], fields[3], 0}, ""};
}

struct BindingList {
    std::vector<Binding> bindings;
    std::vector<ConfigError> errors;
    bool ok() const { return errors.empty(); }
};

inline BindingList parseBindings(const ConfigDocument& document) {
    BindingList result;
    result.errors = document.errors;
    for (const auto& entry : document.all("bind")) {
        auto parsed = parseBinding(entry.value);
        if (!parsed.ok()) result.errors.push_back({entry.line, parsed.error});
        else {
            parsed.binding->line = entry.line;
            result.bindings.push_back(std::move(*parsed.binding));
        }
    }
    if (!result.ok()) result.bindings.clear();
    return result;
}

} // namespace HyprCore
