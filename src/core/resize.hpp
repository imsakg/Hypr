#pragma once
#include "geometry.hpp"
#include <cmath>
namespace HyprCore {
struct SizeHints { int minWidth=1; int minHeight=1; };
inline Rect constrainSize(Rect r, SizeHints hints = {}) {
    r.width=std::max(r.width,std::max(1,hints.minWidth));
    r.height=std::max(r.height,std::max(1,hints.minHeight));
    return r;
}
} // namespace HyprCore
