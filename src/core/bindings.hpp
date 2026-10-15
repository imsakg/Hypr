#pragma once
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

namespace HyprCore {
using ModifierMask = std::uint32_t;
inline constexpr ModifierMask ModShift = 1u << 0;
inline constexpr ModifierMask ModLock = 1u << 1;
inline constexpr ModifierMask ModControl = 1u << 2;
inline constexpr ModifierMask Mod1 = 1u << 3;
inline constexpr ModifierMask Mod2 = 1u << 4;
inline constexpr ModifierMask Mod3 = 1u << 5;
inline constexpr ModifierMask ModSuper = 1u << 6;
inline constexpr ModifierMask Mod5 = 1u << 7;

namespace BindingDetail {
inline bool space(char c) { return c == ' ' || c == '\t' || c == '\r' || c == '\n'; }
inline std::string_view trim(std::string_view text) {
    while (!text.empty() && space(text.front())) text.remove_prefix(1);
    while (!text.empty() && space(text.back())) text.remove_suffix(1);
    return text;
}
inline std::string upper(std::string_view text) {
    std::string result(text);
    for (char& c : result) if (c >= 'a' && c <= 'z') c = static_cast<char>(c - 'a' + 'A');
    return result;
}
inline std::optional<ModifierMask> modifier(std::string_view token) {
    const auto name = upper(token);
    if (name == "SUPER" || name == "MOD4") return ModSuper;
    if (name == "CTRL" || name == "CONTROL") return ModControl;
    if (name == "ALT" || name == "MOD1") return Mod1;
    if (name == "SHIFT") return ModShift;
    if (name == "LOCK") return ModLock;
    if (name == "MOD2") return Mod2;
    if (name == "MOD3") return Mod3;
    if (name == "MOD5") return Mod5;
    return std::nullopt;
}
}

inline std::optional<ModifierMask> parseModifiers(std::string_view text) {
    ModifierMask result = 0;
    text = BindingDetail::trim(text);
    while (!text.empty()) {
        std::size_t end = 0;
        while (end < text.size() && !BindingDetail::space(text[end])) ++end;
        const auto bit = BindingDetail::modifier(text.substr(0, end));
        if (!bit) return std::nullopt;
        result |= *bit;
        text = BindingDetail::trim(text.substr(end));
    }
    return result;
}
}
