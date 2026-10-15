#pragma once

#include <cmath>
#include <limits>
#include <locale>
#include <optional>
#include <sstream>
#include <string>

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

} // namespace HyprCore
