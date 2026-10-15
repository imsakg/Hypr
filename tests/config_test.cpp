#include "test.hpp"
#include "core/config.hpp"
using namespace HyprCore;

TEST(trim_handles_empty_and_ascii_whitespace) {
    CHECK(trim("").empty());
    CHECK(trim(" \t\r\n\v\f").empty());
    CHECK(trim(" \talpha beta\r\n") == "alpha beta");
    CHECK(trim(std::string("\xC3\xA9 ")) == "\xC3\xA9");
}

TEST_MAIN()
