# Portable window rules

[`src/core/rules.hpp`](../src/core/rules.hpp) provides C++17 parsing, precompiled
ECMAScript regex matching, ordered resolution, and validated reload snapshots in
`HyprCore`. It includes the [portable configuration API](config-reference.md).
These are policy primitives: this header does not read X properties, move or
resize real windows, change workspaces, or replace the existing X11 rule path.

## Rule syntax

One top-level `windowrule` assignment contains one action and one or more
selectors:

```ini
windowrule = float,class:^browser$ && role:^pop-up$
windowrule = tile,class:^term$ && !title:^Scratch
windowrule = pseudo,class:^term$
windowrule = pseudo off,title:^Read-only$
windowrule = nointerventions on,role:^notification$
windowrule = workspace 2,class:^term$
windowrule = monitor 0,class:^browser$
windowrule = move -100 +20,role:^pop-up$
windowrule = size 500 50,role:^pop-up$
```

`parseRule(value, line)` parses the value without the `windowrule=` prefix. The
line argument defaults to zero for direct calls. `parseRules(document)` compiles
only top-level `windowrule` entries and uses their one-based source lines.

The **first comma** separates the action from the selectors. Later commas are
regex data, so quantifiers such as `class:^x{1,3}$` need no escaping. Normal
configuration comment stripping and same-line quote balancing apply when a rule
comes from `parseConfig`. Direct `parseRule` does not strip comments or decode
quotes. In particular, quotes around a regex are literal regex characters, not a
rule-string quoting mechanism. Embedded NUL bytes are rejected before regex
compilation.

## Actions and bounds

Action names are exact and case-sensitive. Leading/trailing ASCII whitespace is
ignored, and action arguments are separated by classic-locale whitespace.
Every action rejects missing or extra arguments.

- `float`, `tile`: no arguments; opposite values of the same floating property.
- `pseudo [boolean]`, `nointerventions [boolean]`: optional boolean, enabled when
  omitted. Boolean spellings are case-insensitive `true/false`, `yes/no`,
  `on/off`, or `1/0`.
- `workspace id`: positive integer (`1` through `INT_MAX`).
- `monitor id`: zero-based nonnegative integer (`0` through `INT_MAX`).
- `move x y`: two signed integers, each `INT_MIN` through `INT_MAX`; zero and
  negative positions are allowed.
- `size width height`: two positive integers, each `1` through `INT_MAX`.

All integer arguments use strict `parseInteger`: decimal and signed hexadecimal
are supported, overflow and trailing text are rejected. Percentages, fractions,
relative workspace expressions, monitor names, and runtime monitor-existence
checks are not implemented. `move` stores coordinates; it does not interpret a
coordinate system or silently clamp them. A valid numeric monitor ID need not
refer to a connected monitor; that is an application-layer check.

Examples rejected by the parser include `floating`, `float 1`, `pseudo maybe`,
`workspace 0`, `monitor -1`, `move 1 2 trailing`, and `size 0 10`.

## Selectors, conjunction, and negation

`WindowProperties` supplies three independent strings in aggregate order:
`{ className, role, title }`. Selectors target one string:

- `class:regex`
- `role:regex`
- `title:regex`
- `!class:regex`, `!role:regex`, `!title:regex` negate the corresponding match.

Matching is case-sensitive `std::regex_search`, not a full-string match. Use
`^...$` to anchor a complete value. Empty properties can match `^$` or `.*`;
negation of `^$` requires a nonempty property. Patterns must be nonempty, so an
intended empty-string match is written as `^$`, not an empty selector.

Join selectors with an **unescaped, whitespace-delimited `&&` token outside
regex groups and character classes**. Every selector must match. Spaces or tabs
may surround the token. There is no cross-property OR grammar; regex `|` remains
available within an individual property, and multiple rules can express
alternative predicates.

The following are single regex patterns followed by a real selector separator:

```text
title:^(A && B|C)$ && class:^term$
title:^[ && ]+$ && !role:^normal$
title:^A \&\& B$ && class:^term$
class:^x{1,3},term$ && title:^A&&B$
```

Regex groups, character classes, escapes, and commas are preserved rather than
processed as generic CSV. `A&&B` without surrounding whitespace is regex text.
For a literal whitespace-surrounded `&&` in a regex, put it in a group or escape
the ampersands. Empty conjunction operands, unknown targets, repeated `!`
prefixes, empty patterns, and malformed regexes fail parsing. `title:^!text$`
means a literal exclamation mark; negation belongs before the target name.

Regexes are compiled once while parsing. Matching never recompiles them. An
invalid regex does not create a rule, and a regex execution error fails the
whole rule closed, including negated selectors. A default-constructed `Rule`
has no conditions and never matches.

## Ordered sets and last-wins resolution

`parseRules(document)` returns a `RuleSet` with `rules`, `errors`, and `ok()`.
Source order and line numbers are preserved. Document syntax errors and rule
compilation errors make the result invalid and clear **all** compiled rules;
there is no partially usable rule list.

`matching(window)` returns pointers to matching rules in source order. Those
pointers are valid only while the unchanged set remains alive; a successful store
reload invalidates pointers into its previous set. An invalid set returns no
matches.

`resolve(window)` returns `ResolvedRules` with independent optional properties:

- `floating`: `float` sets true and `tile` sets false.
- `pseudo` and `noInterventions`: their respective boolean flags.
- `workspace` and `monitor`: their respective integer IDs.
- `position` and `size`: `pair<int, int>` coordinate or dimension values.

Every matching action is visited in source order. The last matching assignment
wins **for its own property**, not for the entire rule list. Later `tile` does
not erase an earlier size, monitor, workspace, or pseudo setting. A disengaged
optional means “leave this property unchanged”; an engaged `false` or monitor
`0` is an explicit value, not an absence. Invalid sets resolve to all-disengaged
properties. Resolution returns data and does not mutate the supplied window.

## All-or-nothing reloads

`RuleConfigStore::load(text)` constructs a candidate `RuleConfigSnapshot`:

1. Parse the document. Syntax errors stop further validation without duplicating
   those errors across binding/rule diagnostics.
2. Validate config bindings and compile rules. Semantic diagnostics from both
   are collected and sorted by source line.
3. Replace `snapshot()` only if every stage succeeds.

On failure, `errors()` describes the latest attempt, while the prior document,
binding list, and rule set remain intact. On success, errors clear and removed
directives disappear. Loading an empty string is a valid deliberate reset.

This is a single-caller validation transaction, **not** a lock-free/thread-safe
publication mechanism or an X11 application transaction. Callers must synchronize
readers and reloads, validate scalar schemas, resolve binding keys, check runtime
resources, and arrange safe application of the accepted snapshot. Do not retain
entry/rule pointers across successful reloads. Failed reloads do not replace the
accepted snapshot.

## Executable API example

```cpp
#include "core/rules.hpp"
#include <cassert>

int main() {
    using namespace HyprCore;
    RuleConfigStore store;
    assert(store.load(
        "bind=,F1,killactive,\n"
        "windowrule=float,class:^term$ && !role:^normal$\n"
        "windowrule=size 500 50,class:^term$\n"
        "windowrule=pseudo,class:^term$\n"
        "windowrule=pseudo off,title:^Read-only$\n"
        "windowrule=tile,title:^Read-only$\n"));

    const WindowProperties window{"term", "dialog", "Read-only"};
    const auto resolved = store.snapshot().rules.resolve(window);
    assert(resolved.floating == false);
    assert(resolved.pseudo == false);
    assert(resolved.size == std::make_pair(500, 50));
    assert(!resolved.workspace && !resolved.monitor && !resolved.position);
    assert(store.snapshot().rules.matching(window).size() == 5);

    assert(!store.load("windowrule=float,class:[\n"));
    assert(store.errors().size() == 1 && store.errors()[0].line == 1);
    assert(store.snapshot().rules.resolve(window).floating == false);
    assert(store.snapshot().bindings.bindings.size() == 1);

    assert(store.load(""));
    assert(store.errors().empty());
    assert(store.snapshot().rules.rules.empty());
}
```

The [configuration reference](config-reference.md#executable-api-example) gives
the CMake/CTest commands. Coverage lives in
[`tests/rules_test.cpp`](../tests/rules_test.cpp).

## Limits and trust boundary

`std::regex` is not a sandbox, has no enforced time limit here, and can exhibit
expensive matching or resource exhaustion on adversarial expressions/inputs.
Use trusted configuration patterns and bounded application inputs; do not
advertise this parser as ReDoS protection. Allocation failures are not converted
into ordinary configuration errors. Matching uses the standard-library regex
implementation and byte strings, not Unicode-aware title normalization.

The legacy X11 application path has not been switched to these portable APIs by
this change. Supported parser features must not be represented as verified live
window-manager behavior until a separate integration and X11 smoke test exists.
