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
TEST_MAIN()
