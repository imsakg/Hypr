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
inline std::optional<std::int64_t> runtimeInteger(const std::string& input) {
    const auto text=trim(input);
    if (text.empty()) return std::nullopt;
    try { std::size_t used=0; const auto value=std::stoll(text,&used,text.rfind("0x",0)==0 || text.rfind("0X",0)==0 ? 16 : 10); if (used==text.size()) return value; }
    catch (const std::exception&) {}
    return std::nullopt;
}
inline std::vector<ConfigError> validateRuntimeConfig(const std::string& text) {
    const auto document=parseConfig(text);
    auto errors=document.errors;
    const std::set<std::string> integers={"border_size","gaps_in","gaps_out","rounding","intelligent_transients","no_unmap_saving","focus_when_hover","focus_wrap","max_fps","layout:no_gaps_when_only","layout:fold_horizontal","layout:reverse","bar:monitor","bar:enabled","bar:height","bar:mod_pad_in","bar:no_tray_saving","animations:enabled","animations:cheap","animations:borders","animations:workspaces"};
    const std::set<std::string> colors={"col.active_border","col.inactive_border","bar:col.bg","bar:col.high"};
    for (const auto& entry:document.entries) {
        const auto key=entry.qualifiedKey(); std::string error;
        if (key=="bind") {
            const auto binding=parseConfigBinding(entry.value);
            if (!binding.ok()) error=binding.error;
            else if (!runtimeModifiers(binding.binding->modifiers) || !runtimeKeysym(binding.binding->key)) error="invalid binding key or modifier";
        } else if (key=="windowrule") {
            const auto rule=parseRule(entry.value); if (!rule.ok()) error=rule.error;
        } else if (key=="layout") {
            if (!parseLayoutId(entry.value)) error="unknown layout";
        } else if (integers.count(key) || colors.count(key)) {
            const auto value=runtimeInteger(entry.value);
            const auto maximum=colors.count(key)?4294967295LL:1000000LL;
            if (!value || *value<0 || *value>maximum || (key=="max_fps" && (*value==0 || *value>1000))) error="invalid integer setting";
        } else if (key=="animations:speed" || key=="layout:fold_ratio") {
            const auto value=parseReal(entry.value);
            if (!value || *value<=0 || *value>1000000 || (key=="layout:fold_ratio" && (*value<0.1 || *value>0.9))) error="invalid finite real setting";
        } else if (key=="main_mod") {
            if (!runtimeModifiers(entry.value)) error="invalid mouse modifier";
        } else if (key=="bar:font.main" || key=="bar:font.secondary" || key=="status_command") {
            // Inert strings; status commands are activated only after publication.
        } else if (key=="exec" || key=="exec-once") {
            if (entry.value.empty()) error="exec requires a command";
        } else if (key=="bar:module") {
            try {
                const auto fields=splitFields(entry.value,6);
                if (fields.empty()) error="empty module";
                else if (fields[0]=="pad") {
                    if (fields.size()!=3 || (fields[1]!="left" && fields[1]!="right" && fields[1]!="center") || !runtimeInteger(fields[2]) || *runtimeInteger(fields[2])<0 || *runtimeInteger(fields[2])>1000000) error="invalid pad module";
                } else if (fields.size()!=6 || (fields[0]!="left" && fields[0]!="right" && fields[0]!="center") || !runtimeInteger(fields[2]) || !runtimeInteger(fields[3]) || !runtimeInteger(fields[4]) || *runtimeInteger(fields[2])<0 || *runtimeInteger(fields[2])>4294967295LL || *runtimeInteger(fields[3])<0 || *runtimeInteger(fields[3])>4294967295LL || *runtimeInteger(fields[4])<0 || *runtimeInteger(fields[4])>1000000) error="invalid module fields";
            } catch (const std::exception&) { error="invalid module quoting"; }
        } else error="unknown setting: "+key;
        if (!error.empty()) errors.push_back({entry.line,error});
    }
    return errors;
}
} // namespace HyprCore
