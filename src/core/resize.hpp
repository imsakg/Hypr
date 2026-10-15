#pragma once
#include "geometry.hpp"
#include <cmath>
namespace HyprCore {
struct SizeHints { int minWidth=1; int minHeight=1; int maxWidth=std::numeric_limits<int>::max(); int maxHeight=std::numeric_limits<int>::max(); int widthIncrement=1; int heightIncrement=1; int baseWidth=0; int baseHeight=0; };
inline Rect constrainSize(Rect r, SizeHints hints = {}) {
    const int minWidth=std::max(1,hints.minWidth), minHeight=std::max(1,hints.minHeight);
    r.width=std::clamp(r.width,minWidth,std::max(minWidth,hints.maxWidth));
    r.height=std::clamp(r.height,minHeight,std::max(minHeight,hints.maxHeight));
    const auto snap=[](int value,int base,int increment,int minimum,int maximum) {
        increment=std::max(1,increment); base=std::max(0,base);
        const long long delta=static_cast<long long>(value)-base;
        if (delta<0) return value;
        long long result=static_cast<long long>(base)+(delta/increment)*increment;
        if (result<minimum) result+=((minimum-result+increment-1)/increment)*increment;
        return result<=maximum ? static_cast<int>(result) : value;
    };
    r.width=snap(r.width,hints.baseWidth,hints.widthIncrement,minWidth,std::max(minWidth,hints.maxWidth));
    r.height=snap(r.height,hints.baseHeight,hints.heightIncrement,minHeight,std::max(minHeight,hints.maxHeight));
    return r;
}
inline Rect resizeBy(Rect r,int deltaWidth,int deltaHeight,SizeHints hints = {}) {
    r.width=saturatedInt(static_cast<long long>(r.width)+deltaWidth);
    r.height=saturatedInt(static_cast<long long>(r.height)+deltaHeight);
    return constrainSize(r,hints);
}
} // namespace HyprCore
