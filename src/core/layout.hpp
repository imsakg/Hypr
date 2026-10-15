#pragma once
#include "geometry.hpp"
#include <cmath>
#include <cstdint>
#include <optional>
#include <set>
#include <stdexcept>
#include <string_view>
#include <vector>
namespace HyprCore {
enum class LayoutKind { Horizontal, Vertical, Folded };
struct LayoutOptions { LayoutKind kind = LayoutKind::Horizontal; };
// Returns one cell per client; callers must keep membership ordering stable.
inline std::vector<Rect> splitAxis(Rect area, std::size_t count, bool horizontal, int gap = 0) {
    if (count > 65536) throw std::length_error("too many layout clients");
    if (!count) return {};
    (void)gap;
    const int extent = std::max(0,horizontal ? area.width : area.height);
    const int each = extent / static_cast<int>(count);
    std::vector<Rect> result;
    for (std::size_t i=0;i<count;++i) {
        Rect cell{area.x,area.y,std::max(0,area.width),std::max(0,area.height)};
        if (horizontal) { cell.x = saturatedInt(static_cast<long long>(area.x)+i*each); cell.width = each; }
        else { cell.y = saturatedInt(static_cast<long long>(area.y)+i*each); cell.height = each; }
        result.push_back(cell);
    }
    return result;
}
// Layout internals.
inline std::vector<Rect> tile(Rect area, std::size_t count, LayoutOptions options = {}) {
    (void)options;
    return splitAxis(area,count,true);
}
} // namespace HyprCore
