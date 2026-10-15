#pragma once
#include <algorithm>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

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
inline constexpr ModifierMask KeyboardModifiers = 0xffu;
inline constexpr ModifierMask DefaultIgnoredModifiers = ModLock | Mod2;

inline ModifierMask normalizeModifiers(ModifierMask state, ModifierMask ignored = DefaultIgnoredModifiers) {
    return state & KeyboardModifiers & ~ignored;
}
inline bool modifiersMatch(ModifierMask binding, ModifierMask state,
                           ModifierMask ignored = DefaultIgnoredModifiers) {
    return normalizeModifiers(binding, ignored) == normalizeModifiers(state, ignored);
}

inline std::vector<ModifierMask> grabVariants(ModifierMask binding,
                                             ModifierMask ignored = DefaultIgnoredModifiers) {
    std::vector<ModifierMask> masks{normalizeModifiers(binding, ignored)};
    ignored &= KeyboardModifiers;
    for (ModifierMask bit = 1; bit <= Mod5; bit <<= 1) {
        if (!(ignored & bit)) continue;
        const auto count = masks.size();
        for (std::size_t i = 0; i < count; ++i) masks.push_back(masks[i] | bit);
    }
    std::sort(masks.begin(), masks.end());
    return masks;
}

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
    if (BindingDetail::upper(text) == "NONE") return result;
    while (!text.empty()) {
        std::size_t end = 0;
        while (end < text.size() && !BindingDetail::space(text[end]) && text[end] != '+') ++end;
        const auto bit = BindingDetail::modifier(text.substr(0, end));
        if (!bit || (result & *bit)) return std::nullopt;
        result |= *bit;
        text = BindingDetail::trim(text.substr(end));
        if (!text.empty() && text.front() == '+') {
            text = BindingDetail::trim(text.substr(1));
            if (text.empty()) return std::nullopt;
        }
    }
    return result;
}

using KeySym = std::uint32_t;
inline std::optional<KeySym> parseKeysym(std::string_view text) {
    if (text.size() != 1) text = BindingDetail::trim(text);
    if (text.size() == 1) {
        const auto value = static_cast<unsigned char>(text.front());
        if (value >= 32 && value <= 126) return value;
    }
    const auto name = BindingDetail::upper(text);
    struct NamedKey { std::string_view name; KeySym value; };
    static constexpr NamedKey navigation[] = {
        {"LEFT", 0xff51}, {"UP", 0xff52}, {"RIGHT", 0xff53}, {"DOWN", 0xff54},
        {"RETURN", 0xff0d}, {"ENTER", 0xff0d}, {"ESCAPE", 0xff1b}, {"ESC", 0xff1b},
        {"TAB", 0xff09}, {"SPACE", 0x20}, {"BACKSPACE", 0xff08}, {"HOME", 0xff50},
        {"END", 0xff57}, {"INSERT", 0xff63}, {"DELETE", 0xffff},
        {"PAGE_UP", 0xff55}, {"PRIOR", 0xff55}, {"PAGE_DOWN", 0xff56}, {"NEXT", 0xff56}
    };
    for (const auto& key : navigation) if (key.name == name) return key.value;
    return std::nullopt;
}
}
