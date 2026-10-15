#pragma once
#include "geometry.hpp"
#include <cmath>
namespace HyprCore {
struct SizeHints { int minWidth=1; int minHeight=1; int maxWidth=std::numeric_limits<int>::max(); int maxHeight=std::numeric_limits<int>::max(); };
inline Rect constrainSize(Rect r, SizeHints hints = {}) {
    const int minWidth=std::max(1,hints.minWidth), minHeight=std::max(1,hints.minHeight);
    r.width=std::clamp(r.width,minWidth,std::max(minWidth,hints.maxWidth));
    r.height=std::clamp(r.height,minHeight,std::max(minHeight,hints.maxHeight));
    return r;
}
} // namespace HyprCore
