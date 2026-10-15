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
struct LayoutOptions { LayoutKind kind = LayoutKind::Horizontal; int innerGap = 0; int outerGap = 0; double ratio = 0.5; bool foldHorizontal = true; bool reverse = false; };
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
inline std::vector<Rect> folded(Rect area, std::size_t count, int gap, double ratio = 0.5, bool horizontal = true) {
    if (count>65536) throw std::length_error("too many layout clients");
    ratio = std::isfinite(ratio) ? std::clamp(ratio,0.1,0.9) : 0.5;
    area.width=std::max(0,area.width); area.height=std::max(0,area.height);
    std::vector<Rect> result;
    for (std::size_t i=0;i<count;++i) {
        if (i+1==count) { result.push_back(area); break; }
        const int extent=std::max(0,horizontal?area.width:area.height);
        const int spacing=std::clamp(gap,0,extent);
        const int taken=static_cast<int>((extent-spacing)*ratio);
        Rect first=area;
        if (horizontal) { first.width=taken; area.x=saturatedInt(static_cast<long long>(area.x)+taken+spacing); area.width=extent-taken-spacing; }
        else { first.height=taken; area.y=saturatedInt(static_cast<long long>(area.y)+taken+spacing); area.height=extent-taken-spacing; }
        result.push_back(first); horizontal=!horizontal;
    }
    return result;
}
// Layout internals.
inline std::vector<Rect> tile(Rect area, std::size_t count, LayoutOptions options = {}) {
    area = inset(area,options.outerGap);
    if (options.kind==LayoutKind::Folded) return folded(area,count,options.innerGap,options.ratio,options.foldHorizontal);
    return splitAxis(area,count,options.kind != LayoutKind::Vertical,options.innerGap);
}
struct LayoutClient { std::uint64_t id=0; bool floating=false; bool hidden=false; bool fullscreen=false; };
struct Placement { std::uint64_t id; Rect bounds; };
inline std::vector<Placement> arrange(Rect area, const std::vector<LayoutClient>& clients, LayoutOptions options = {}) {
    std::set<std::uint64_t> seen;
    std::vector<std::uint64_t> ids;
    for (const auto& client:clients) {
        if (!client.id || !seen.insert(client.id).second || client.hidden) continue;
        if (client.fullscreen) return {{client.id,area}};
        if (!client.floating) ids.push_back(client.id);
    }
    if (options.reverse) std::reverse(ids.begin(),ids.end());
    const auto cells=tile(area,ids.size(),options);
    std::vector<Placement> result;
    for (std::size_t i=0;i<ids.size();++i) result.push_back({ids[i],cells[i]});
    return result;
}
} // namespace HyprCore
