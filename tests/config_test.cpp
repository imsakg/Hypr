#include "test.hpp"
#include "core/config.hpp"
using namespace HyprCore;

TEST(trim_handles_empty_and_ascii_whitespace) {
    CHECK(trim("").empty());
    CHECK(trim(" \t\r\n\v\f").empty());
    CHECK(trim(" \talpha beta\r\n") == "alpha beta");
    CHECK(trim(std::string("\xC3\xA9 ")) == "\xC3\xA9");
}

TEST(comments_respect_quotes_and_escapes) {
    CHECK(stripComment("key=value # note") == "key=value ");
    CHECK(stripComment("# note").empty());
    CHECK(stripComment("key='a#b' # note") == "key='a#b' ");
    CHECK(stripComment("key=\"a#b\" # note") == "key=\"a#b\" ");
    CHECK(stripComment(R"(key=a\#b # note)") == R"(key=a\#b )");
    CHECK(stripComment(R"(key=a\\# note)") == R"(key=a\\)");
}

TEST(signed_decimal_is_strict_and_bounded) {
    CHECK(parseInteger("  +42 ") == 42);
    CHECK(parseInteger("-17") == -17);
    CHECK(parseInteger("0012") == 12);
    CHECK(parseInteger("2147483647") == 2147483647);
    CHECK(parseInteger("-2147483648") == (-2147483647 - 1));
    for (const auto* text : {"", "+", "--1", "1a", "1 2", "2147483648", "-2147483649", "9999999999999999999999999"})
        CHECK(!parseInteger(text));
}

TEST(hexadecimal_is_signed_not_unsigned_reinterpretation) {
    CHECK(parseInteger("0x2a") == 42);
    CHECK(parseInteger("+0X7f") == 127);
    CHECK(parseInteger("-0x80000000") == (-2147483647 - 1));
    CHECK(parseInteger("0x7fffffff") == 2147483647);
    for (const auto* text : {"0x", "-0X", "0xg", "0x1tail", "0x80000000", "0xffffffff", "-0x80000001", "0b11"})
        CHECK(!parseInteger(text));
}

TEST(real_numbers_are_decimal_finite_and_fully_consumed) {
    CHECK(parseReal("  -1.25e2 ") == -125.0);
    CHECK(parseReal("+.5") == 0.5);
    CHECK(parseReal("1.") == 1.0);
    CHECK(parseReal("0") == 0.0);
    for (const auto* text : {"", ".", "1e", "nan", "NaN", "inf", "-Infinity", "1e9999", "2.0px", "0x1p2", "1,5", "1 2"})
        CHECK(!parseReal(text));
}

TEST(booleans_use_explicit_case_insensitive_tokens) {
    for (const auto* text : {"1", "true", "TRUE", "Yes", " on "}) CHECK(parseBoolean(text) == true);
    for (const auto* text : {"0", "false", "FALSE", "No", " off "}) CHECK(parseBoolean(text) == false);
    for (const auto* text : {"", "2", "-1", "truthy", "true tail", "0x1"}) CHECK(!parseBoolean(text));
}

TEST(comma_fields_keep_empty_positions) {
    CHECK(splitFields("") == std::vector<std::string>{""});
    CHECK(splitFields("a,, b ,") == std::vector<std::string>({"a", "", "b", ""}));
    CHECK(splitFields(",") == std::vector<std::string>({"", ""}));
    CHECK(splitFields("single") == std::vector<std::string>{"single"});
}

TEST(fields_respect_quotes_and_delimiter_escapes) {
    CHECK(splitFields(R"(a,"b,c",'d,e',f\,g)") == std::vector<std::string>({"a", "b,c", "d,e", "f,g"}));
    CHECK(splitFields(R"("say \"hi\"",'it\'s',\d+)") == std::vector<std::string>({"say \"hi\"", "it's", R"(\d+)"}));
    CHECK(splitFields(R"(a\\,b)") == std::vector<std::string>({R"(a\)", "b"}));
    bool rejected = false;
    try { (void)splitFields("a,\"unfinished"); } catch (const std::invalid_argument&) { rejected = true; }
    CHECK(rejected);
}

TEST(bounded_fields_leave_command_tail_verbatim) {
    const auto fields = splitFields(R"(SUPER,R,exec,printf '%s,%s' "a,b" c\,d)", 4);
    CHECK(fields == std::vector<std::string>({"SUPER", "R", "exec", R"(printf '%s,%s' "a,b" c\,d)"}));
    CHECK(splitFields(" a,b ", 1) == std::vector<std::string>{"a,b"});
    CHECK(splitFields("a,b", 0).empty());
    CHECK(splitFields("a,", 2) == std::vector<std::string>({"a", ""}));
    CHECK(splitFields("a,b", 8) == splitFields("a,b"));
}

TEST(flat_documents_parse_blank_comments_and_assignments) {
    const auto doc = parseConfig("\n# note\nmax_fps=60\nname=terminal # comment\n");
    CHECK(doc.ok());
    CHECK(doc.entries.size() == 2);
    CHECK(doc.entries[0].key == "max_fps");
    CHECK(doc.entries[0].value == "60");
    CHECK(doc.entries[1].value == "terminal");
    CHECK(parseConfig("").entries.empty());
    CHECK(parseConfig(" \t\n# blank\n").ok());
}

TEST(assignments_trim_keys_and_keep_value_syntax_and_source_lines) {
    const auto doc = parseConfig("\n  gaps_in \t= 5 \ncommand = printf \"a=b#c\" = tail\nempty =\n");
    CHECK(doc.ok());
    CHECK(doc.entries[0].key == "gaps_in");
    CHECK(doc.entries[0].line == 2);
    CHECK(doc.entries[1].value == "printf \"a=b#c\" = tail");
    CHECK(doc.entries[2].value.empty());
    CHECK(!parseConfig("=missing\n").ok());
    CHECK(!parseConfig("not a key=value\n").ok());
    CHECK(!parseConfig("\"not=a\"=value\n").ok());
}

TEST(categories_are_case_insensitive_and_scopes_restore) {
    const auto doc = parseConfig("Bar {\nheight=20\n Colors{\nbg=1\n}\nenabled=1\n}\nmax_fps=60\n");
    CHECK(doc.ok());
    CHECK(doc.entries.size() == 4);
    CHECK(doc.entries[0].qualifiedKey() == "bar:height");
    CHECK(doc.entries[1].qualifiedKey() == "bar:colors:bg");
    CHECK(doc.entries[2].qualifiedKey() == "bar:enabled");
    CHECK(doc.entries[3].qualifiedKey() == "max_fps");
    CHECK(doc.entries[0].category == "bar");
    CHECK(parseConfig("value={literal}\n").entries[0].value == "{literal}");
}

TEST(malformed_categories_report_source_lines_without_crashing) {
    const auto doc = parseConfig("}\ninvalid\nbar {\nheight=20\n");
    CHECK(!doc.ok());
    CHECK(doc.errors.size() == 3);
    CHECK(doc.errors[0].line == 1);
    CHECK(doc.errors[1].line == 2);
    CHECK(doc.errors[2].line == 3);
    CHECK(doc.errors[2].message.find("unclosed category") != std::string::npos);
    for (const auto* text : {"{", "bar {}", "bar { trailing", "bar { {", "bad:name {\n}"})
        CHECK(!parseConfig(text).ok());
    CHECK(parseConfig("key=brace } in value\n").ok());
}

TEST(unclosed_quotes_are_line_errors_and_comments_do_not_open_quotes) {
    const auto doc = parseConfig("bad=\"unfinished # still quoted\ngood=1 # 'not a quote\nbad2='open\n");
    CHECK(!doc.ok());
    CHECK(doc.errors.size() == 2);
    CHECK(doc.errors[0].line == 1);
    CHECK(doc.errors[1].line == 3);
    CHECK(doc.entries.size() == 1);
    CHECK(doc.entries[0].key == "good");
    CHECK(parseConfig(R"(value="a\"b#c" # ignored)").ok());
    CHECK(parseConfig("path=trailing\\").ok());
}

TEST(repeated_directives_survive_and_scalar_lookup_is_last_wins) {
    const auto doc = parseConfig("exec=first\ncount=1\nexec=second\ncount=-0x2\nspeed=2.5\nenabled=off\nempty=\n");
    CHECK(doc.all("exec").size() == 2);
    CHECK(doc.all("exec")[0].value == "first");
    CHECK(doc.all("exec")[1].line == 3);
    CHECK(doc.integer("count") == -2);
    CHECK(doc.real("speed") == 2.5);
    CHECK(doc.boolean("enabled") == false);
    CHECK(doc.value("empty") == "");
    CHECK(!doc.value("missing"));
    CHECK(!doc.integer("exec"));
    CHECK(doc.last("count")->line == 4);
    CHECK(doc.last("missing") == nullptr);
    CHECK(parseConfig("n=3\nn=bad\n").integer("n") == std::nullopt);
}

TEST(config_store_replaces_only_after_successful_parse) {
    ConfigStore store;
    CHECK(store.load("count=3\nexec=first\n"));
    CHECK(!store.load("count=9\nbar {\n"));
    CHECK(store.document().integer("count") == 3);
    CHECK(store.document().all("exec").size() == 1);
    CHECK(store.document().ok());
    CHECK(!store.errors().empty());
    CHECK(store.load("count=4\n"));
    CHECK(store.document().integer("count") == 4);
    CHECK(store.document().all("exec").empty());
    CHECK(store.errors().empty());
    CHECK(store.load(""));
    CHECK(store.document().entries.empty());
}

TEST(bindings_keep_shell_arguments_opaque_and_allow_unmodified_keys) {
    const auto result = parseConfigBinding(R"( SUPER , R , exec , printf '%s,%s' "a,b" c\,d )");
    CHECK(result.ok());
    CHECK(result.binding->modifiers == "SUPER");
    CHECK(result.binding->key == "R");
    CHECK(result.binding->dispatcher == "exec");
    CHECK(result.binding->argument == R"(printf '%s,%s' "a,b" c\,d)");
    CHECK(parseConfigBinding(",F1,killactive,").ok());
    CHECK(!parseConfigBinding("SUPER,R,exec").ok());
    CHECK(!parseConfigBinding("SUPER,,exec,xterm").ok());
    CHECK(!parseConfigBinding("SUPER,R,,xterm").ok());
    CHECK(!parseConfigBinding("\"open,R,exec,xterm").ok());
}

TEST(binding_validation_is_fail_closed_and_source_aware) {
    CHECK(!parseConfigBinding("SUPER,R,unknown,x").ok());
    CHECK(!parseConfigBinding("SUPER,R,exec,").ok());
    CHECK(!parseConfigBinding("SUPER,R,killactive,extra").ok());
    CHECK(!parseConfigBinding("SUPER,R,exec,\"unfinished").ok());
    const auto valid = parseConfigBindings(parseConfig("bind=,F1,killactive,\nbind=SUPER,R,exec,xterm\n"));
    CHECK(valid.ok());
    CHECK(valid.bindings.size() == 2);
    CHECK(valid.bindings[1].line == 2);
    const auto invalid = parseConfigBindings(parseConfig("bind=SUPER,R,exec,xterm\nbind=SUPER,Q,bogus,\n"));
    CHECK(!invalid.ok());
    CHECK(invalid.bindings.empty());
    CHECK(invalid.errors[0].line == 2);
    CHECK(!parseConfigBindings(parseConfig("missing equals\n")).ok());
}

TEST(config_binding_records_have_config_specific_public_names) {
    const ConfigBindingParseResult one = parseConfigBinding(",F1,killactive,");
    CHECK(one.ok());
    const ConfigBinding binding = *one.binding;
    CHECK(binding.key == "F1");
    const ConfigBindingList many = parseConfigBindings(parseConfig("bind=,F1,killactive,\n"));
    CHECK(many.ok());
    CHECK(many.bindings.size() == 1);
}

TEST_MAIN()
