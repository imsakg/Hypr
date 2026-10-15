#pragma once
#include <functional>
#include <iostream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>
namespace Test {
using Case = std::pair<std::string, std::function<void()>>;
inline std::vector<Case>& cases() { static std::vector<Case> list; return list; }
struct Register { Register(const char* name, std::function<void()> fn) { cases().emplace_back(name, std::move(fn)); } };
inline int run() {
    int failures = 0;
    for (const auto& item : cases()) {
        try { item.second(); }
        catch (const std::exception& e) { ++failures; std::cerr << item.first << ": " << e.what() << '\n'; }
        catch (...) { ++failures; std::cerr << item.first << ": unknown exception\n"; }
    }
    std::cout << cases().size() << " cases, " << failures << " failures\n";
    return failures ? 1 : 0;
}
}
#define TEST(name) static void name(); static Test::Register reg_##name(#name, name); static void name()
#define CHECK(...) do { if (!(__VA_ARGS__)) throw std::runtime_error(std::string(__FILE__) + ":" + std::to_string(__LINE__) + ": " + #__VA_ARGS__); } while (false)
#define TEST_MAIN() int main() { return Test::run(); }
