#pragma once
#include <algorithm>
#include <limits>
namespace HyprCore {
struct Rect {
    int x = 0, y = 0, width = 0, height = 0;
    bool operator==(const Rect& other) const {
        return x == other.x && y == other.y && width == other.width && height == other.height;
    }
    bool operator!=(const Rect& other) const { return !(*this == other); }
};
inline int saturatedInt(long long value) {
    return static_cast<int>(std::clamp(value, static_cast<long long>(std::numeric_limits<int>::min()), static_cast<long long>(std::numeric_limits<int>::max())));
}
inline Rect inset(Rect r, int amount) {
    const int x = std::min(std::max(0, amount), std::max(0, r.width) / 2);
    const int y = std::min(std::max(0, amount), std::max(0, r.height) / 2);
    return {saturatedInt(static_cast<long long>(r.x) + x), saturatedInt(static_cast<long long>(r.y) + y), std::max(0, r.width - 2*x), std::max(0, r.height - 2*y)};
}
inline Rect intersection(const Rect& a, const Rect& b) {
    const long long x = std::max(a.x,b.x), y = std::max(a.y,b.y);
    const auto right = std::min(static_cast<long long>(a.x) + std::max(0,a.width), static_cast<long long>(b.x) + std::max(0,b.width));
    const auto bottom = std::min(static_cast<long long>(a.y) + std::max(0,a.height), static_cast<long long>(b.y) + std::max(0,b.height));
    return {saturatedInt(x), saturatedInt(y), saturatedInt(std::max(0LL,right-x)), saturatedInt(std::max(0LL,bottom-y))};
}
inline Rect reserve(Rect area, int left, int top, int right, int bottom) {
    left = std::clamp(left,0,std::max(0,area.width));
    top = std::clamp(top,0,std::max(0,area.height));
    right = std::clamp(right,0,std::max(0,area.width-left));
    bottom = std::clamp(bottom,0,std::max(0,area.height-top));
    return {saturatedInt(static_cast<long long>(area.x)+left), saturatedInt(static_cast<long long>(area.y)+top), std::max(0,area.width-left-right), std::max(0,area.height-top-bottom)};
}
inline bool contains(const Rect& r, int x, int y) {
    return r.width > 0 && r.height > 0 && x >= r.x && y >= r.y &&
           static_cast<long long>(x) < static_cast<long long>(r.x) + r.width &&
           static_cast<long long>(y) < static_cast<long long>(r.y) + r.height;
}
}
