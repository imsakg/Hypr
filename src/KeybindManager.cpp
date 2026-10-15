#include "KeybindManager.hpp"
#include "core/runtime.hpp"
#include "utilities/Util.hpp"
#include "events/events.hpp"
#include "windowManager.hpp"

#include <algorithm>
#include <string.h>

Keybind* KeybindManager::findKeybindByKey(int mod, xcb_keysym_t keysym) {
    for(auto& key : KeybindManager::keybinds) {
        if (keysym == key.getKeysym() && HyprCore::modifiersMatch(key.getMod(),mod)) {
            return &key;
        }
    }
    return nullptr;
}

uint32_t KeybindManager::getKeyCodeFromName(std::string name) {
    return HyprCore::runtimeKeysym(name).value_or(0);
}

unsigned int KeybindManager::modToMask(std::string mod) {
    return HyprCore::runtimeModifiers(mod).value_or(0);
}

xcb_keysym_t KeybindManager::getKeysymFromKeycode(xcb_keycode_t keycode) {
    const auto KEYSYMS = xcb_key_symbols_alloc(g_pWindowManager->DisplayConnection);
    const auto KEYSYM = (!(KEYSYMS) ? 0 : xcb_key_symbols_get_keysym(KEYSYMS, keycode, 0));
    xcb_key_symbols_free(KEYSYMS);
    return KEYSYM;
}

xcb_keycode_t KeybindManager::getKeycodeFromKeysym(xcb_keysym_t keysym) {
    auto* symbols=xcb_key_symbols_alloc(g_pWindowManager->DisplayConnection);
    if (!symbols) return 0;
    auto* codes=xcb_key_symbols_get_keycode(symbols,keysym);
    const auto result=codes?*codes:0;
    free(codes);
    xcb_key_symbols_free(symbols);
    return result;
}

// Dispatchers

void KeybindManager::killactive(std::string args) {
    // args unused
    const auto PLASTWINDOW = g_pWindowManager->getWindowFromDrawable(g_pWindowManager->LastWindow);

    if (!PLASTWINDOW)
        return;
    
    if (PLASTWINDOW->getCanKill()) {
        // Send a kill message
        xcb_client_message_event_t event;
        bzero(&event, sizeof(event));
        event.response_type = XCB_CLIENT_MESSAGE;
        event.window = PLASTWINDOW->getDrawable();
        event.type = HYPRATOMS["WM_PROTOCOLS"];
        event.format = 32;
        event.data.data32[0] = HYPRATOMS["WM_DELETE_WINDOW"];
        event.data.data32[1] = 0;

        xcb_send_event(g_pWindowManager->DisplayConnection, 0, PLASTWINDOW->getDrawable(), XCB_EVENT_MASK_NO_EVENT, (const char*)&event);

        return; // Do not remove it yet. The user might've cancelled the operation or something. We will get
                // an unmap event.
    } else 
        xcb_kill_client(g_pWindowManager->DisplayConnection, g_pWindowManager->LastWindow);

    g_pWindowManager->closeWindowAllChecks(g_pWindowManager->LastWindow);
}

void KeybindManager::call(std::string args) {

    if (fork() == 0) {
        //setsid();

        execl("/bin/sh", "/bin/sh", "-c", args.c_str(), nullptr);

        _exit(0);
    }
    //wait(NULL);
}

void KeybindManager::movewindow(std::string arg) {
    if (arg.size()==1 && std::string("lrud").find(arg[0])!=std::string::npos) g_pWindowManager->moveActiveWindowTo(arg[0]);
}

void KeybindManager::movefocus(std::string arg) {
    if (arg.size()==1 && std::string("lrud").find(arg[0])!=std::string::npos) g_pWindowManager->moveActiveFocusTo(arg[0]);
}

namespace {
std::optional<int> resolveWorkspaceArgument(const std::string& argument) {
    const auto monitor=g_pWindowManager->getMonitorFromCursor();
    if (!monitor || monitor->ID<0 || static_cast<std::size_t>(monitor->ID)>=g_pWindowManager->activeWorkspaces.size()) return std::nullopt;
    std::vector<int> ids;
    for (auto& workspace:g_pWindowManager->workspaces) ids.push_back(workspace.getID());
    return HyprCore::workspaceTarget(ids,g_pWindowManager->activeWorkspaces[monitor->ID],g_pWindowManager->lastActiveWorkspaceID,argument);
}
}
void KeybindManager::movetoworkspace(std::string arg) {
    if (const auto target=resolveWorkspaceArgument(arg)) g_pWindowManager->moveActiveWindowToWorkspace(*target);
}
void KeybindManager::changeworkspace(std::string arg) {
    if (const auto target=resolveWorkspaceArgument(arg)) g_pWindowManager->changeWorkspaceByID(*target);
}

void KeybindManager::changetolastworkspace(std::string arg) {
    Debug::log(LOG, "Changing the current workspace to the last workspace");
    g_pWindowManager->changeToLastWorkspace();
}

void KeybindManager::toggleActiveWindowFullscreen(std::string unusedArg) {
    g_pWindowManager->toggleWindowFullscrenn(g_pWindowManager->LastWindow);
}

void KeybindManager::toggleActiveWindowFloating(std::string unusedArg) {
    if (const auto PWINDOW = g_pWindowManager->getWindowFromDrawable(g_pWindowManager->LastWindow); PWINDOW) {
        PWINDOW->setIsFloating(!PWINDOW->getIsFloating());
        PWINDOW->setDirty(true);

        // Fix window as if it's closed if we just made it floating
        if (PWINDOW->getIsFloating()) {
            g_pWindowManager->fixWindowOnClose(PWINDOW);
            g_pWindowManager->calculateNewWindowParams(PWINDOW);
        }
        else {
            // It's remapped again

            // SAVE ALL INFO NOW, THE POINTER WILL BE DEAD
            const auto RESTOREACSIZE = PWINDOW->getDefaultSize();
            const auto RESTOREACPOS = PWINDOW->getDefaultPosition();
            const auto RESTOREWINID = PWINDOW->getDrawable();
            const auto RESTORECANKILL = PWINDOW->getCanKill();
            const auto RESTOREPSEUDO = PWINDOW->getPseudoSize();
            const auto RESTOREISPSEUDO = PWINDOW->getIsPseudotiled();
            const auto RESTOREREALS = PWINDOW->getRealSize();
            const auto RESTOREREALP = PWINDOW->getRealPosition();

            g_pWindowManager->removeWindowFromVectorSafe(PWINDOW->getDrawable());

            CWindow newWindow;
            newWindow.setDrawable(RESTOREWINID);
            newWindow.setFirstOpen(false);
            newWindow.setConstructed(false);
            g_pWindowManager->addWindowToVectorSafe(newWindow);

            const auto PNEWWINDOW = Events::remapWindow(RESTOREWINID, true);

            PNEWWINDOW->setDefaultPosition(RESTOREACPOS);
            PNEWWINDOW->setDefaultSize(RESTOREACSIZE);
            PNEWWINDOW->setCanKill(RESTORECANKILL);
            PNEWWINDOW->setPseudoSize(RESTOREPSEUDO);
            PNEWWINDOW->setIsPseudotiled(RESTOREISPSEUDO);
            PNEWWINDOW->setRealPosition(RESTOREREALP);
            PNEWWINDOW->setRealSize(RESTOREREALS);
        }

        // EWMH to let everyone know
        EWMH::updateClientList();

        EWMH::updateWindow(PWINDOW->getDrawable());

        g_pWindowManager->setAllFloatingWindowsTop();
    }
}

void KeybindManager::changeSplitRatio(std::string args) {
    if (args=="+" || args=="-") g_pWindowManager->changeSplitRatioCurrent(args[0]);
}

void KeybindManager::togglePseudoActive(std::string args) {
    const auto PWINDOW = g_pWindowManager->getWindowFromDrawable(g_pWindowManager->LastWindow);

    if (!PWINDOW)
        return;

    PWINDOW->setIsPseudotiled(!PWINDOW->getIsPseudotiled());

    PWINDOW->setDirty(true);
}