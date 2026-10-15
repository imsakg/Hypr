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
}
