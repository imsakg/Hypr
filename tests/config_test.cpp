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

TEST_MAIN()
