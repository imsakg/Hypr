# Keybindings and focus policy

`src/core/bindings.hpp` and `src/core/focus.hpp` are header-only C++17 policy
components in `HyprCore`. They do not require X11, grab keys, execute commands,
or send focus events. A window-manager adapter must translate backend events,
validate supported actions, apply the selected action, and acknowledge success.
The legacy manager is not automatically wired to these policies by including them.
See [workspace state](workspaces.md) for membership and active-focus ownership.

## Binding grammar

Pass the value **after** `bind =` to `parseBinding`:

```text
SUPER SHIFT, Return, exec, terminal --title "work, notes"
NONE, Space, close
SUPER, comma, exec, printf '%s,%s' a b # literal argument
```

The first three commas separate modifiers, key, action, and optional argument.
Everything after the third comma belongs to the argument. Leading/trailing ASCII
space, tab, CR, and LF are trimmed from fields; internal commas, quotes,
backslashes, and `#` are preserved. There is no shell evaluation, quote unescaping,
comment removal, or executable/action whitelist in this parser. An absent or empty
argument is valid. Action names start with an ASCII letter and then contain only
ASCII letters, digits, `_`, or `-`; backend-specific argument validation is separate.
Malformed input returns `std::nullopt`, not a partial binding.

### Modifiers and key symbols

- Modifier tokens are case-insensitive and separated by whitespace or `+`.
  `SUPER`/`MOD4`, `CTRL`/`CONTROL`, `ALT`/`MOD1`, `SHIFT`, `LOCK`, `MOD2`, `MOD3`,
  and `MOD5` map to the corresponding protocol bits. Empty text or `NONE` means
  no modifiers. Repeated bits (including aliases), unknown tokens, and incomplete
  `+` expressions are rejected; `NONE` cannot be mixed with other tokens.
- Printable single-byte ASCII keys are literal and case-sensitive: `q` and `Q`
  are different key symbols. Named keys are case-insensitive. Supported names
  include navigation/editing keys, `Return`/`Enter`, `Escape`/`Esc`, `Tab`, `Space`,
  `Page_Up`/`Prior`, `Page_Down`/`Next`, and `F1` through `F35`. Function-key
  numbers must be complete and canonical (`F01` and `F36` are rejected).
- Named ASCII punctuation includes delimiter-safe `comma` and `plus`, and names
  such as `semicolon`, `backslash`, `bracketleft`, `grave`/`quoteleft`, and
  `apostrophe`/`quoteright`. Use `Space` for a space key inside a binding because
  binding fields are trimmed. Unicode and arbitrary XKB key names are not resolved.

`normalizeModifiers` keeps only the eight keyboard modifier bits and ignores
`ModLock | Mod2` by default. `Mod2` is a common Num Lock assignment, not detected
keymap state: supply the backend's actual ignored mask when needed. Pointer-button
and higher state bits never participate in matching. Other non-ignored keyboard
modifiers must match exactly. `grabVariants(binding, ignored)` returns the sorted,
unique masks for every ignored-bit combination; the adapter performs actual grabs.

## Registry updates and reloads

`BindingRegistry(ignoredMask)` uses the same normalization for `set`, `find`, and
`remove`. `set(Binding)` rejects a zero key, non-keyboard modifier bits, and invalid
action names before modifying the registry. `set(text)` parses first. A repeated
normalized modifier/key chord replaces its existing slot (last definition wins)
without changing the position of other entries. `find(state, key)` returns a
borrowed pointer or `nullptr`; do not retain it across registry mutation.

For reloads, parse **all** lines into a temporary `std::vector<Binding>` and then
call `replaceAll`. Invalid replacement entries leave the old registry untouched;
normalization, duplicate handling, and the registry's custom ignored mask are
preserved. Passing an empty vector intentionally clears every binding. `clear`
also clears explicitly. This is a single-threaded logical transaction, not a
thread-synchronization or backend-grab transaction. Update backend grabs only once
the replacement is accepted, handling backend failures separately.

## Portable focus API

`FocusTarget` contains `id`, `Rect bounds`, and flags `focusable = true`,
`mapped = true`, `hidden = false`. IDs are `std::uint64_t`; zero is invalid.
`focusEligible(target)` additionally requires positive width and height. This
eligibility rule applies to **every** selector, including cycles and MRU fallback.
Callers supply the relevant monitor/workspace snapshot; these functions do not
infer workspace visibility or backend focus permissions. A duplicate ID uses its
**first** occurrence's geometry, flags, and ordering; later occurrences cannot
revive an ineligible client or alter its geometry.

### Directional neighbors

`directionalFocus(targets, current, FocusDirection::{Left,Right,Up,Down}, wrap=false)`
returns an optional target ID. Missing/ineligible current, an invalid direction,
no eligible neighbor, and a singleton without a neighbor return `std::nullopt`.
A candidate must have its center strictly beyond the current center on the chosen
axis; intersecting rectangles can still be directional neighbors. Equal primary
centers are never candidates, even with wrapping.

Candidates are ranked, in order:

1. Positive overlap on the perpendicular axis (a shared edge/corner is not overlap).
2. Shortest forward center distance on the movement axis.
3. Shortest perpendicular center distance.
4. Lowest numeric window ID, independent of snapshot order.

Centers are doubled integers to preserve half-pixel differences. Endpoints,
centers, and distances are widened before arithmetic, including extreme `int`
coordinates and sizes. No floating-point epsilon or squared-distance overflow is
involved.

With `wrap=true`, a normal forward candidate **always** wins first. Only when
none exists does selection search strictly behind the current center, preferring
perpendicular overlap, then the opposite extreme center on the primary axis,
then perpendicular distance, then ID. Directional wrap is opt-in and is distinct
from cycling.

### Cycles, pointer selection, and MRU fallback

- `cycleFocus(targets, current=std::nullopt, reverse=false, wrap=true)` follows
  eligible snapshot order, not numeric IDs or geometry. A missing/ineligible
  current starts at the first eligible entry (last in reverse). With wrap disabled,
  crossing an end returns `std::nullopt`. A singleton with wrapping returns its
  own ID as a no-op. Reverse traversal uses the same filtering and order.
- `focusAtPoint(targets, x, y)` treats snapshot order as back-to-front stacking and
  returns the last eligible hit. Rectangles are half-open: left/top included,
  right/bottom excluded. This selects a target only; focus-follows-mouse timing,
  pointer warps, click replay, and raising are adapter responsibilities.
- `FocusHistory::remember(id)` promotes a nonzero ID to the front, deduplicating
  prior history. Call it **after** successful backend focus. `forget(id)` removes
  a departing client; unknown/zero IDs return false. `entries()` is read-only MRU
  order. `fallback(targets, excluded=std::nullopt)` picks the most recent eligible
  survivor, otherwise the first eligible snapshot entry; it never selects the
  excluded ID. It does not prune history or mutate recency. Call `forget` on close
  or when transferring ownership to another history domain.

All selectors are pure: they neither change `WorkspaceState` nor update MRU
history. `std::nullopt` means no selected target; the adapter decides whether to
keep existing focus or clear backend focus. See the workspace ownership boundary
before attempting to focus a foreign sticky window.

## Compilable policy example

This example parses data and selects focus without launching a process or requiring
an X server:

```cpp
#include "core/bindings.hpp"
#include "core/focus.hpp"
#include <cassert>
using namespace HyprCore;
int main() {
    const auto binding = parseBinding("SUPER, Right, movefocus, right");
    assert(binding && binding->argument == "right");
    BindingRegistry registry;
    assert(registry.replaceAll({*binding}));
    assert(registry.find(ModSuper | ModLock, 0xff53u));

    const std::vector<FocusTarget> targets{
        {10, {0, 0, 100, 100}}, {20, {150, 0, 100, 100}}};
    const auto selected = directionalFocus(targets, 10, FocusDirection::Right);
    assert(selected && *selected == 20);
    assert(cycleFocus(targets, selected, true) == 10);
    FocusHistory history;
    history.remember(10);
    history.remember(*selected); // Only after backend success in a real adapter.
    assert(history.fallback(targets, selected) == 10);
}
```

## Verification

The portable suite needs only a C++17 compiler and CMake:

```sh
cmake -S . -B build-policy -DHYPR_BUILD_WM=OFF -DBUILD_TESTING=ON
cmake --build build-policy -j 4 && ctest --test-dir build-policy --output-on-failure
```

Always require a successful build **before** running CTest: after a compile error,
CTest can still execute an older test binary. Tests cover parsing/reload rejection,
modifier normalization, all four focus directions, deterministic overlap ties,
wrap, eligibility, reverse cycles, MRU, duplicate snapshots, pointer edges, and
extreme-coordinate/translation/reflection regressions. Live backend behavior
requires separate integration checks.
