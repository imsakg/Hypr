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

TEST_MAIN()
