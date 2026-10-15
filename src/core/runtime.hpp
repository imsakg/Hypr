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
// Keep old concatenated uppercase modifier spellings only when every token is known.
inline std::optional<ModifierMask> runtimeModifiers(std::string_view text) {
    if (auto parsed=parseModifiers(text)) return parsed;
    text=BindingDetail::trim(text);
    ModifierMask mask=0;
    while (!text.empty()) {
        bool found=false;
        for (const auto token:{"CONTROL","SUPER","SHIFT","CTRL","MOD1","MOD2","MOD3","MOD4","MOD5","LOCK","ALT"}) {
            const std::string_view name(token);
            if (text.substr(0,name.size())!=name) continue;
            const auto bit=BindingDetail::modifier(name);
            if (!bit || (mask & *bit)) return std::nullopt;
            mask|=*bit; text.remove_prefix(name.size()); found=true; break;
        }
        if (!found) return std::nullopt;
    }
    return mask;
}
inline std::optional<KeySym> runtimeKeysym(std::string name) {
    name=trim(name);
    if (name.size()==1 && name[0]>='A' && name[0]<='Z') name[0]+=32;
    if (configLower(name)=="equals") name="equal";
    return parseKeysym(name);
}
inline std::optional<WorkspaceId> workspaceTarget(const std::vector<int>& ids,int active,int last,const std::string& argument) {
    WorkspaceState state;
    for (int id:ids) state.addWorkspace(id);
    if (last>0) state.activate(last);
    if (active>0) state.activate(active);
    return state.resolveTarget(argument);
}
inline std::optional<std::pair<int,int>> resizeArgument(const std::string& input) {
    std::istringstream stream(input); std::string x,y,extra;
    if (!(stream>>x>>y) || (stream>>extra)) return std::nullopt;
    const auto dx=parseInteger(x),dy=parseInteger(y);
    if (!dx || !dy) return std::nullopt;
    return std::pair<int,int>{*dx,*dy};
}
} // namespace HyprCore
