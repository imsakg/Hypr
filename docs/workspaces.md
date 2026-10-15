# Workspace state and lifecycle

`src/core/workspace.hpp` is a header-only C++17 state machine in `HyprCore`.
It stores membership, active/previous workspace, remembered local focus, sticky
visibility, urgency, and pinning without X11 calls or global manager state.
Integrating it with mapping, layouts, EWMH properties, and actual input focus is an
adapter responsibility. [Keybindings and focus policy](keybindings.md) describes
portable geometry selectors and their independent eligibility rules.

## Identity, navigation, and targets

- `WorkspaceId` is `int`; only positive IDs are accepted by `addWorkspace`.
  `ids()` returns existing IDs in ascending order. Duplicate/invalid additions
  return false. The **first added** workspace becomes active, not necessarily the
  smallest ID. `workspace(id)` returns a read-only pointer, or `nullptr` if absent.
- `active()` and `last()` are optional IDs. `activate(id)` requires an existing
  workspace. A successful distinct activation records the previous active ID;
  reactivating the current workspace does not change `last()`. Failed activation
  does not change either. `toggleLast()` switches to the previous workspace and
  supports toggling back; it returns false until one exists.
- `nextWorkspace(from)` and `previousWorkspace(from)` wrap over **existing sorted
  IDs**, not numeric gaps. Unknown `from` returns `std::nullopt`; a singleton
  navigates to itself. These are queries and never activate a workspace.

`resolveTarget(text)` is a read-only parser. It trims outer ASCII space, tab, CR,
and LF, and accepts these case-sensitive forms:

- Positive decimal ID: `12` resolves even if workspace 12 does not yet exist.
- Signed positive offset: `+2` or `-2` is numeric arithmetic relative to the active
  ID, **not** movement through the sorted existing IDs.
- `next`, `prev`: existing-ID navigation relative to the active workspace.
- `last` or `lastworkspace`: the previous active ID, if available.

The full token must parse. Empty text, zero (`0`, `+0`, `-0`), repeated signs,
internal spaces, suffix junk, numeric overflow, a nonpositive offset result, or a
result outside `WorkspaceId` range is rejected. Relative forms need an active
workspace. Resolving never creates or activates anything: the adapter must decide
whether to create an absolute/relative target, and `activate` still rejects a
missing workspace.

## Window ownership and movement

`WorkspaceWindowId` is `std::uint64_t`; zero is invalid. Every managed window has
exactly one owning workspace. `workspaceOf(window)` returns that ID or
`std::nullopt`. `addWindow(workspace, window)` requires an existing workspace and
a globally unused nonzero window ID. `Workspace::windows` preserves insertion
order. A first member becomes the workspace's remembered focus; later additions
do not steal it. The state machine exposes only const workspace pointers.

`moveWindow(window, destination)` validates both ends before changing membership:

1. Append the window to the destination, preserving the order of other members.
2. Detach it from the source, repairing source focus if needed.
3. Select it in the destination only if the destination previously had no focus.

A move to the current owner is a successful no-op, including membership/history
order. An unknown window or destination returns false without mutation. Movement
never activates a workspace or changes `last()`. Sticky and urgent flags travel
with the window; active-focus acknowledgement may clear urgency as described below.

`removeWindow(window)` removes ownership, local focus-history entries, and both
sticky/urgent metadata. Unknown IDs return false without mutation. Reusing a closed
window ID starts without old metadata or focus history. Closing the last member
leaves an empty workspace with no focused window; it does not automatically remove
that workspace or activate another one.

## Local focus and MRU repair

`focused()` queries the active workspace's remembered focus. `focusWindow(window)`
only accepts a member **owned by the active workspace**. It rejects unknown and
inactive-workspace members without activating anything. Workspace membership does
not track mapped/hidden/focusable geometry flags: use the focus policies to filter
eligible candidates before asking the adapter to apply focus.

Each `Workspace` has a read-only `focusHistory`, most recently selected first.
Successful local focus promotes and deduplicates the selected member. Moving or
closing any member prunes it from its old owner's history. If the focused member
departs, repair chooses the most recently focused surviving member, otherwise the
first remaining member in insertion order. Automatic selection is recorded in
history. Removing an unfocused member does not otherwise change focus/history
order. Activating a workspace restores its own remembered focus, not another
workspace's MRU entry.

This state machine records logical focus; it is not proof that the X server accepted
focus. A backend integrating these policies must validate/apply focus and reconcile
failures rather than treating state selection as an input-focus acknowledgement.
The independent `FocusHistory` in `core/focus.hpp` supports geometry/eligibility-aware
fallback for an adapter; it is not implicitly synchronized with `WorkspaceState`.

## Sticky visibility is not ownership

`setSticky(window, bool)` requires a managed member and changes only metadata.
`isSticky(window)` returns false for unknown/closed members. `visibleWindows(id)`
returns a fresh ordered view:

1. All locally owned windows, in membership order.
2. Sticky windows from other workspaces, ordered by source workspace ID and then
   source membership order.

No window appears twice. An unknown workspace gives an empty view. A sticky window
still has exactly one owner, and moving it retains stickiness without creating
extra membership. Clearing stickiness removes foreign visibility only. Visibility
is workspace policy data, **not** a claim that a window is mapped, unclipped, or
focusable on a particular monitor.

In particular, a foreign sticky window can be visible while `focusWindow` rejects
it. The adapter must explicitly choose an ownership transition (move it to the
active workspace or activate its owner) before requesting local focus. Do not
silently duplicate membership to make it focusable. Supply only locally focusable
members to portable selectors when feeding their result into `focusWindow`.

## Urgency and acknowledgement

`setUrgent(window, bool)` requires a managed window; `isUrgent` returns false for
unknown/closed IDs. `urgentWindows(id)` lists urgent **owned** members in membership
order, not the foreign sticky visibility list. Unknown workspace queries are empty.
Setting urgency does not activate, reorder, focus, or move anything, even if the
member is currently selected.

Urgency is cleared by:

- Successful `focusWindow` on that member.
- `activate` restoring that member as the active workspace's focus (including
  an explicit reactivation of the already active workspace).
- Automatic focus repair/initial selection choosing that member in the active
  workspace, for example moving an urgent member into an empty active workspace.
- Explicit `setUrgent(window, false)` or `removeWindow`.

A rejected focus/activation does not clear urgency. Remembered focus changes in an
inactive workspace do not acknowledge urgency until the workspace is activated.
Adapters should apply the same acknowledgement rule to actual backend focus.

## Pinning and empty cleanup

`setPinned(id, bool)` requires an existing workspace. `removeWorkspace(id)` only
removes a workspace when it is inactive, unpinned, and owns no windows; active,
pinned, populated, invalid, and unknown workspaces return false unchanged.
`cleanupEmpty()` applies that rule in ascending ID order and returns the removed
IDs. "Empty" is ownership-based: foreign sticky visibility does not retain a
workspace. If a removed workspace was `last()`, that stale previous ID is cleared.
Cleanup never changes the active workspace.

## Compilable lifecycle example

```cpp
#include "core/workspace.hpp"
#include <cassert>
using namespace HyprCore;
int main() {
    WorkspaceState state;
    assert(state.addWorkspace(1) && state.addWorkspace(3));
    assert(state.addWindow(1, 10) && state.addWindow(1, 11));
    assert(state.addWindow(3, 30));
    assert(state.focusWindow(11));
    assert(state.setSticky(10, true) && state.setUrgent(30, true));
    assert(state.activate(3) && state.focused() == 30);
    assert(!state.isUrgent(30));
    assert(state.visibleWindows(3) == std::vector<WorkspaceWindowId>({30, 10}));
    assert(!state.focusWindow(10)); // Visible, but still owned by workspace 1.
    assert(state.moveWindow(10, 3) && state.focusWindow(10));
    assert(state.resolveTarget("prev") == 1);
    assert(state.removeWindow(30) && state.focused() == 10);
    assert(state.removeWindow(10) && !state.focused());
    assert(state.activate(1) && state.focused() == 11);
    assert(state.cleanupEmpty() == std::vector<WorkspaceId>{3});
    assert(!state.last()); // Cleanup removed the previous workspace.
}
```

## Verification and integration boundary

Run the portable CMake/CTest command in [keybindings](keybindings.md#verification).
Workspace tests cover rejected operations, sorted navigation, last-workspace
toggling, signed target boundaries, global membership uniqueness, insertion order,
move/close focus repair, MRU pruning, sticky visibility, urgency, and protected
cleanup. A deterministic 1,000-operation mixed-lifecycle regression checks live
membership/history/visibility invariants after each operation and checks that
every operation family was exercised.

Backend grabs, input focus, map/unmap, pointer behavior, monitor assignment, and
EWMH export are outside this pure state model and require integration tests.
