#pragma once
#include "bindings.hpp"
#include "config.hpp"
#include "focus.hpp"
#include "layout.hpp"
#include "resize.hpp"
#include "rules.hpp"
#include "workspace.hpp"
namespace HyprCore {
inline bool flatLayout(int id) { return id>=2 && id<=4; }
} // namespace HyprCore
