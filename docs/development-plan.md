# Window-management development plan

## Scope and history

Extend the existing X11 manager, preserving dwindle/master behavior, with portable C++17 policy code and tested runtime adapters. New code must not depend on an X server to test geometry, parsing, bindings, rules, workspaces or focus.

The starting checkout is `dd1b2a7` with **336 reachable commits**, not eleven. Existing history remains intact. This local development series adds 120 substantive commits (456 reachable commits in total). Historical author/committer timestamps in the added series are a user-requested reconstruction, not a claim that this implementation was produced in 2021–2022. Nothing is to be pushed.

## Architecture

- `src/core/geometry.hpp`: shared integer rectangles, bounds and usable areas.
- `src/core/layout.hpp`: deterministic horizontal, vertical and folded placements.
- `src/core/config.hpp` and `rules.hpp`: quote-aware config and safe matching.
- `src/core/bindings.hpp`, `workspace.hpp`, `focus.hpp`: portable input and state policy.
- `src/core/resize.hpp`: constrained floating and tiled resize policy.
- Existing manager/config/keybinding files: thin adapters; no parallel compositor.
- `tests/*_test.cpp`: standalone dependency-free test executables registered with CTest.

## Ordered implementation slices

1. Establish CTest, a small exception-reporting harness and shared geometry.
2. Independently implement config/rules and bindings/workspace/focus in isolated worktrees. Each behavior gets an observed failing test before production code, followed by a passing suite and a focused commit.
3. Implement layouts and resize, including odd dimensions, gaps, negative monitor coordinates and size limits.
4. Wire portable policy into the live manager: layout selection, safe config parsing, window-rule matching, focus selection, key dispatch and resize.
5. Document syntax, migration, layout semantics and manual X11 acceptance checks. Add portable CI.
6. Review the integrated diff, run CTest with warnings and sanitizers, and build/check the X11 target where dependencies are available.
7. Rebase only the added series with randomized weekday/evening dates in 2021–2022 using `git commit --date`. Verify every new commit is nonempty, every original commit remains reachable, status is clean and origin refs remain unchanged.

## Acceptance / deferrals

`cmake -S . -B <build> -DHYPR_BUILD_WM=OFF -DBUILD_TESTING=ON`, `cmake --build <build>` and `ctest --test-dir <build> --output-on-failure` must succeed on macOS without X11 packages. Linux build and real nested-X11 smoke are separate checks, never inferred from portable tests. Existing dwindle/master IDs and old config syntax remain supported. No remote publication, dependency installation or upstream history replacement is authorized.
