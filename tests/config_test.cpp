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

TEST_MAIN()
