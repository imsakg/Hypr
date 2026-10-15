#include "test.hpp"
#include "core/bindings.hpp"
using namespace HyprCore;
TEST(modifier_tokens_map_to_protocol_bits) {
    CHECK(parseModifiers("SUPER") == ModSuper);
    CHECK(parseModifiers("mod4") == ModSuper);
    CHECK(parseModifiers("CTRL") == ModControl);
    CHECK(parseModifiers("CONTROL") == ModControl);
    CHECK(parseModifiers("ALT") == Mod1);
    CHECK(parseModifiers("MOD1") == Mod1);
    CHECK(parseModifiers("SHIFT") == ModShift);
    CHECK(parseModifiers("LOCK") == ModLock);
    CHECK(parseModifiers("MOD2") == Mod2);
    CHECK(parseModifiers("MOD3") == Mod3);
    CHECK(parseModifiers("MOD5") == Mod5);
}
TEST(modifier_combinations_use_whole_whitespace_tokens) {
    CHECK(parseModifiers("  super\tCTRL shift  ") == (ModSuper | ModControl | ModShift));
    CHECK(parseModifiers("") == 0u);
    CHECK(!parseModifiers("NOSUPER"));
    CHECK(!parseModifiers("SHIFTED"));
}
TEST(modifier_separators_are_explicit_and_duplicates_are_errors) {
    CHECK(parseModifiers("SUPER + SHIFT") == (ModSuper | ModShift));
    CHECK(parseModifiers("NONE") == 0u);
    for (const auto* text : {"+SUPER", "SUPER+", "SUPER++SHIFT", "SUPER+ +SHIFT", "CTRL CONTROL", "NONE SHIFT", "SUPER,SHIFT", "SUPER|SHIFT", "SUPER SUPER"})
        CHECK(!parseModifiers(text));
}
TEST(matching_ignores_locks_and_pointer_button_state) {
    CHECK(normalizeModifiers(ModSuper | ModLock | Mod2 | (1u << 8)) == ModSuper);
    CHECK(modifiersMatch(ModSuper, ModSuper | ModLock | Mod2));
    CHECK(!modifiersMatch(ModSuper, ModSuper | ModShift));
    CHECK(modifiersMatch(ModSuper | ModLock, ModSuper));
    CHECK(!modifiersMatch(ModSuper, ModSuper | Mod2, ModLock));
    CHECK(normalizeModifiers(ModSuper | Mod3, ModLock | Mod3) == ModSuper);
}
TEST(passive_grab_variants_cover_each_ignored_modifier_once) {
    const std::vector<ModifierMask> expected{ModSuper, ModSuper | ModLock, ModSuper | Mod2, ModSuper | ModLock | Mod2};
    CHECK(grabVariants(ModSuper | ModLock) == expected);
    CHECK(grabVariants(ModShift, 0) == std::vector<ModifierMask>{ModShift});
    const auto custom = grabVariants(ModSuper, ModLock | Mod3 | Mod5 | (1u << 15));
    CHECK(custom.size() == 8);
    CHECK(custom.front() == ModSuper);
    CHECK(custom.back() == (ModSuper | ModLock | Mod3 | Mod5));
    for (const auto mask : custom) CHECK(modifiersMatch(ModSuper, mask, ModLock | Mod3 | Mod5));
}
TEST(printable_ascii_keysyms_are_literal_and_never_execute_commands) {
    for (unsigned value = 32; value <= 126; ++value) {
        const std::string key(1, static_cast<char>(value));
        CHECK(parseKeysym(key) == value);
    }
    CHECK(parseKeysym("  q  ") == static_cast<KeySym>('q'));
    CHECK(!parseKeysym(""));
    CHECK(!parseKeysym("ab"));
    CHECK(!parseKeysym("$(touch sentinel)"));
    CHECK(!parseKeysym(std::string(1, '\0')));
    CHECK(!parseKeysym("\xC3\xA9"));
}
TEST_MAIN()
