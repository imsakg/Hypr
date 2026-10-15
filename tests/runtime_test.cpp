#include "test.hpp"
#include "core/runtime.hpp"
using namespace HyprCore;
TEST(runtime_headers_compose_without_type_collisions) {
 CHECK(flatLayout(2)); CHECK(flatLayout(4)); CHECK(!flatLayout(1)); CHECK(!flatLayout(5));
 Binding input; ConfigBinding record; CHECK(input.key==0); CHECK(record.key.empty());
}
TEST_MAIN()
