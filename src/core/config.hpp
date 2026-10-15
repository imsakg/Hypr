#pragma once

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

} // namespace HyprCore
