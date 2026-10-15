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
TEST(named_navigation_keys_have_stable_protocol_values) {
    CHECK(parseKeysym("Left") == 0xff51u);
    CHECK(parseKeysym("up") == 0xff52u);
    CHECK(parseKeysym("RIGHT") == 0xff53u);
    CHECK(parseKeysym("Down") == 0xff54u);
    CHECK(parseKeysym("Return") == 0xff0du);
    CHECK(parseKeysym("Enter") == parseKeysym("Return"));
    CHECK(parseKeysym("Escape") == 0xff1bu);
    CHECK(parseKeysym("Esc") == parseKeysym("Escape"));
    CHECK(parseKeysym("Tab") == 0xff09u);
    CHECK(parseKeysym("Space") == 32u);
    CHECK(parseKeysym("BackSpace") == 0xff08u);
    CHECK(parseKeysym("Home") == 0xff50u);
    CHECK(parseKeysym("End") == 0xff57u);
    CHECK(parseKeysym("Insert") == 0xff63u);
    CHECK(parseKeysym("Delete") == 0xffffu);
    CHECK(parseKeysym("Page_Up") == 0xff55u);
    CHECK(parseKeysym("Page_Down") == 0xff56u);
    CHECK(!parseKeysym("LeftOver"));
}
TEST(function_keys_are_bounded_and_require_complete_canonical_numbers) {
    for (unsigned i = 1; i <= 35; ++i)
        CHECK(parseKeysym("F" + std::to_string(i)) == 0xffbdu + i);
    CHECK(parseKeysym("f12") == 0xffc9u);
    for (const auto* text : {"F0", "F01", "F36", "F-1", "F1junk", "F99999999999999999999"})
        CHECK(!parseKeysym(text));
    CHECK(parseKeysym("F") == static_cast<KeySym>('F'));
}
TEST(named_punctuation_includes_delimiter_safe_aliases) {
    const std::pair<const char*, char> entries[] = {
        {"exclam", '!'}, {"quotedbl", '"'}, {"numbersign", '#'}, {"dollar", '$'},
        {"percent", '%'}, {"ampersand", '&'}, {"apostrophe", '\''}, {"parenleft", '('},
        {"parenright", ')'}, {"asterisk", '*'}, {"plus", '+'}, {"comma", ','},
        {"minus", '-'}, {"period", '.'}, {"slash", '/'}, {"colon", ':'},
        {"semicolon", ';'}, {"less", '<'}, {"equal", '='}, {"greater", '>'},
        {"question", '?'}, {"at", '@'}, {"bracketleft", '['}, {"backslash", '\\'},
        {"bracketright", ']'}, {"asciicircum", '^'}, {"underscore", '_'}, {"grave", '`'},
        {"braceleft", '{'}, {"bar", '|'}, {"braceright", '}'}, {"asciitilde", '~'}
    };
    for (const auto& entry : entries) CHECK(parseKeysym(entry.first) == static_cast<KeySym>(entry.second));
    CHECK(parseKeysym("quoteleft") == parseKeysym("grave"));
    CHECK(parseKeysym("quoteright") == parseKeysym("apostrophe"));
    CHECK(!parseKeysym("comma;echo nope"));
}
TEST(binding_fields_are_validated_before_a_value_is_returned) {
    const auto binding = parseBinding(" SUPER SHIFT , Return , togglefloating ");
    CHECK(binding);
    CHECK(binding->modifiers == (ModSuper | ModShift));
    CHECK(binding->key == 0xff0du);
    CHECK(binding->action == "togglefloating");
    CHECK(binding->argument.empty());
    CHECK(parseBinding(",q,close"));
    for (const auto* text : {"", "SUPER,q", "SUPER,,exec", "SUPER,q,", "BOGUS,q,exec", "SUPER,unknown,exec", "SUPER,q,exec junk", "SUPER,q,;exec"})
        CHECK(!parseBinding(text));
}
TEST(action_arguments_preserve_commas_quotes_and_comment_characters) {
    const auto binding = parseBinding("SUPER,comma,exec,  printf '%s,%s' a b # literal  ");
    CHECK(binding);
    CHECK(binding->key == static_cast<KeySym>(','));
    CHECK(binding->argument == "printf '%s,%s' a b # literal");
    const auto quoted = parseBinding("NONE,Space,exec,echo \"x,y\" \\path");
    CHECK(quoted);
    CHECK(quoted->argument == "echo \"x,y\" \\path");
    CHECK(parseBinding("SUPER,q,close,")->argument.empty());
    CHECK(parseBinding("SUPER,q,exec,,leading,tail")->argument == ",leading,tail");
    CHECK(!parseBinding("SUPER,q,,argument"));
}
TEST_MAIN()
