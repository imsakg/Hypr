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
struct LayoutOptions { LayoutKind kind = LayoutKind::Horizontal; int innerGap = 0; };
// Returns one cell per client; callers must keep membership ordering stable.
inline std::vector<Rect> splitAxis(Rect area, std::size_t count, bool horizontal, int gap = 0) {
    if (count > 65536) throw std::length_error("too many layout clients");
    if (!count) return {};
    const int fullExtent = std::max(0,horizontal ? area.width : area.height);
    gap = count < 2 ? 0 : std::clamp(gap,0,fullExtent/static_cast<int>(count-1));
    const int extent = fullExtent-gap*static_cast<int>(count-1);
    const int each = extent / static_cast<int>(count);
    const int remainder = extent % static_cast<int>(count);
    int offset = 0;
    std::vector<Rect> result;
    for (std::size_t i=0;i<count;++i) {
        Rect cell{area.x,area.y,std::max(0,area.width),std::max(0,area.height)};
        const int size = each + (static_cast<int>(i) < remainder ? 1 : 0);
        if (horizontal) { cell.x = saturatedInt(static_cast<long long>(area.x)+offset); cell.width = size; }
        else { cell.y = saturatedInt(static_cast<long long>(area.y)+offset); cell.height = size; }
        offset += size;
        if (i+1<count) offset += gap;
        result.push_back(cell);
    }
    return result;
}
// Layout internals.
inline std::vector<Rect> tile(Rect area, std::size_t count, LayoutOptions options = {}) {
    return splitAxis(area,count,options.kind != LayoutKind::Vertical,options.innerGap);
}
} // namespace HyprCore
