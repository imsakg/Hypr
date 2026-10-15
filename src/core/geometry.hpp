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
inline bool contains(const Rect& r, int x, int y) {
    return r.width > 0 && r.height > 0 && x >= r.x && y >= r.y &&
           static_cast<long long>(x) < static_cast<long long>(r.x) + r.width &&
           static_cast<long long>(y) < static_cast<long long>(r.y) + r.height;
}
}
