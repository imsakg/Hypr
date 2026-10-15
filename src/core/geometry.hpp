#pragma once
#include <algorithm>
namespace HyprCore {
struct Rect {
    int x = 0, y = 0, width = 0, height = 0;
    bool operator==(const Rect& other) const {
        return x == other.x && y == other.y && width == other.width && height == other.height;
    }
    bool operator!=(const Rect& other) const { return !(*this == other); }
};
inline bool contains(const Rect& r, int x, int y) {
    return r.width > 0 && r.height > 0 && x >= r.x && y >= r.y &&
           static_cast<long long>(x) < static_cast<long long>(r.x) + r.width &&
           static_cast<long long>(y) < static_cast<long long>(r.y) + r.height;
}
}
