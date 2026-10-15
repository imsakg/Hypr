# Portable configuration reference

This page describes the header-only C++17 API in
[`src/core/config.hpp`](../src/core/config.hpp), not a replacement for the existing
X11 `ConfigManager`. Including the header does not read files, execute commands,
expand variables, register key grabs, or change the desktop. All APIs below live
in `HyprCore`.

## Document grammar

```ini
# One assignment or category delimiter per line.
max_fps = 60
speed = 1.25e1
label = "a#b" # This comment is removed; the value still contains quotes.
Bar {
    enabled = yes
    Colors {
        background = 0x112233
    }
}
exec = first command
exec = second command
bind = SUPER,R,exec,printf '%s,%s' "a,b" c\,d
bind = ,F1,killactive,
```

- Blank lines and comments are ignored. `#` begins a comment only outside single
  or double quotes and when not backslash-escaped. Comment removal preserves the
  original quoting and backslashes in the retained text.
- Assignments split at the first unquoted, unescaped `=`. Keys and values are
  trimmed of ASCII whitespace; empty values are valid. Further `=` characters,
  commas, braces, and spaces belong to the value.
- Nonempty identifiers use ASCII letters, digits, `_`, `-`, `.`, and `:`. A key
  need not begin with a letter. Category names use the same characters except
  `:`. Category names are lowercased; keys remain case-sensitive.
- Categories open with `name {` (or `name{`) and close with a line containing only
  `}`. Nesting produces colon-qualified keys: the example contains
  `bar:enabled` and `bar:colors:background`. There are no inline blocks or
  multiple statements per line.
- A fully qualified key such as `bar:enabled` may also appear directly in an
  assignment. Lookups use its exact spelling.
- Quotes must close on the same line, including quotes in command values. There
  is no multiline continuation. A trailing backslash is retained as data.
- Embedded NUL bytes are errors, even in comments. Line numbers are one-based.

`parseConfig(text)` returns a `ConfigDocument` with ordered `entries` and `errors`.
Each entry contains `category`, `key`, raw `value`, `line`, and `qualifiedKey()`.
Unknown keys are retained: parsing is not scalar-schema validation. A malformed
line can coexist with usable entries in the returned document, so **check
`ok()` before treating a document as a valid configuration**.

Unexpected closing braces, invalid identifiers, missing assignments, unclosed
quotes, and unclosed categories produce `ConfigError { line, message }` records.
An unclosed category reports its opening line. Error messages are diagnostic
text, not stable machine-readable error codes.

## Repeated values and typed access

- `all(key)` copies every matching entry in source order. Repeated `exec`, `bind`,
  or other directives are not deduplicated.
- `last(key)` returns a pointer to the last matching entry, or `nullptr`.
- `value(key)` returns the last raw value as `optional<string>`.
- `integer(key)`, `real(key)`, and `boolean(key)` convert the last value, returning
  an empty optional when it is missing or invalid. They do not fall back to an
  earlier valid value and do not append document errors.
- Entry pointers remain valid only while the owning document remains alive and
  its entries are unchanged.

Conversions consume the entire trimmed token:

- `parseInteger`: signed decimal or signed `0x`/`0X` hexadecimal, bounded by
  `numeric_limits<int>`. A leading zero is decimal, not octal. A sign without
  digits, suffixes, overflow, and unsigned reinterpretation are rejected.
  For example, `-0x2` is `-2`; on the tested 32-bit-int targets, `0xffffffff`
  cannot be represented. ARGB color strings can still be retained by `value()`;
  this API does not provide an unsigned color accessor.
- `parseReal`: finite decimal floating point with optional exponent, using the
  classic locale. Hex floats, `nan`, infinity, comma decimal separators, overflow,
  and suffixes are rejected. Normal floating-point rounding still applies.
- `parseBoolean`: case-insensitive `1`, `true`, `yes`, `on`, or `0`, `false`, `no`,
  `off`. Other numbers and prefix matches are rejected.

These conversions do not unquote values: `count="3"` is a valid raw assignment
but has no integer value.

## Config binding records

Use the config-specific names:

- `ConfigBinding { modifiers, key, dispatcher, argument, line }`
- `ConfigBindingParseResult` and `parseConfigBinding(value)`
- `ConfigBindingList` and `parseConfigBindings(document)`

The names intentionally differ from runtime `Binding` and `parseBinding` APIs;
config records retain textual keys/modifiers instead of resolved numeric ones.
There are no compatibility aliases that would reintroduce that namespace clash.

A top-level binding value has exactly four logical fields:

```text
modifiers,key,dispatcher,argument
```

The first three fields honor quoted commas and `\,`, `\\`, `\'`, `\"`, and `\#`
escapes. Their wrapping quotes are removed. The fourth field is a trimmed opaque
tail: its quotes, escapes, and additional commas are preserved. Balanced quotes
are checked over the complete binding before field parsing. When parsed from a
configuration document, normal document-level comment stripping happens first.
Direct `parseConfigBinding` does not strip comments.

An empty modifier field is allowed; key and dispatcher must be nonempty.
Modifier spelling, key existence, and duplicate chords are **not** validated or
resolved here. Dispatchers are case-sensitive and use the following arguments:

- `exec`: a nonempty command, preserved without execution or shell validation.
- `killactive`, `fullscreen`, `lastworkspace`, `togglefloating`, `pseudo`: empty
  argument, with the final comma still required.
- `movewindow`, `movefocus`: exactly `l`, `r`, `u`, or `d`.
- `workspace`, `movetoworkspace`: a positive `parseInteger` value.
- `splitratio`: exactly `+` or `-`, matching the existing character-based
  dispatcher contract, not an arbitrary floating-point delta.

Unknown dispatchers, bad arity, invalid arguments, unclosed quotes, or NUL bytes
return an error with no binding. Direct parsing sets `line` to zero;
`parseConfigBindings` assigns each binding its source line, copies document
errors, and collects semantic binding errors. Any error clears the entire
binding list. Only top-level `bind` entries are compiled.

The low-level `splitFields(text, maxFields)` helper preserves empty positions;
its default is unbounded, zero returns no fields, and the final bounded field is
opaque. Unclosed quotes in a decoded field throw `invalid_argument`; binding
parsing converts that into a result error.

## Safe reload boundaries

`ConfigStore::load(text)` replaces its document only after a successful syntax
parse. On failure, `document()` remains the previous valid document and
`errors()` describes the failed attempt. A successful load clears old errors;
loading an empty document deliberately clears the configuration.

`ConfigStore` does **not** validate bindings, window rules, or scalar ranges.
For an all-or-nothing document/binding/rule validation transaction, use
`RuleConfigStore` from [`src/core/rules.hpp`](../src/core/rules.hpp). Neither
store applies X11 effects, retries a failed file read, or provides thread-safe
publication. Callers must supply synchronization, defaults, scalar-schema
validation, runtime key resolution, and actual application of validated state.

## Executable API example

```cpp
#include "core/config.hpp"
#include <cassert>

int main() {
    using namespace HyprCore;
    const auto document = parseConfig(
        "count=1\ncount=+0x2\nexec=first\nexec=second\n"
        "Bar {\nenabled=YES\n}\n"
        "bind=,F1,killactive,\n");
    assert(document.ok());
    assert(document.integer("count") == 2);
    assert(document.boolean("bar:enabled") == true);
    assert(document.all("exec").size() == 2);
    assert(document.last("count")->line == 2);

    const ConfigBindingList bindings = parseConfigBindings(document);
    assert(bindings.ok() && bindings.bindings.size() == 1);
    assert(bindings.bindings[0].line == 8);
    const auto command = parseConfigBinding(
        R"(SUPER,R,exec,printf '%s,%s' "a,b" c\,d)");
    assert(command.ok());
    assert(command.binding->argument == R"(printf '%s,%s' "a,b" c\,d)");

    ConfigStore store;
    assert(store.load("count=7\n"));
    assert(!store.load("count=9\nbar {\n"));
    assert(store.document().integer("count") == 7);
    assert(!store.errors().empty());
}
```

Compile an extracted example with C++17 and `-Isrc`. The portable suite needs no
X server or X11 development packages:

```sh
cmake -S . -B build/policy -DHYPR_BUILD_WM=OFF -DBUILD_TESTING=ON
cmake --build build/policy
ctest --test-dir build/policy --output-on-failure
```
