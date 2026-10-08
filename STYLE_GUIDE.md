# Sub0Pub Code Style Guide

Style conventions derived from the existing codebase. Follow these for consistency.

---

## Naming

| Element | Convention | Example |
|---------|-----------|---------|
| Namespaces | lowercase | `sub0`, `detail`, `utility` |
| Classes/Structs | PascalCase | `Broker`, `Subscribe`, `BinaryReader` |
| Template parameters | PascalCase with `_t` suffix for type params | `Data`, `Prefix_t`, `Header_t` |
| Member variables | camelCase with `_` suffix | `state_`, `publishCanceled_`, `ostream_` |
| Local variables | camelCase | `iSubscription`, `readCount` |
| Constants | `c` prefix + PascalCase | `cMaxSubscriptions`, `cMaxDataBufferCount` |
| Macros/Defines | UPPER_SNAKE_CASE with `SUB0PUB_` prefix | `SUB0PUB_AUDIT`, `SUB0PUB_MAX_SUBSCRIPTIONS` |
| Free functions | camelCase | `publish()`, `cancel()` |
| Type aliases | PascalCase | `OStream`, `IStream`, `StreamSize` |

## Formatting

- **Indentation:** 4 spaces (no tabs)
- **Braces:** Opening brace on same line for control flow, next line for class/function definitions
- **Line width:** ~120 characters soft limit
- **Pointer/reference alignment:** `Type* name` (pointer with type), `const Type& name` (reference with type)

```cpp
// Class definition
class Broker
{
public:
    void publish(const Data& data) const noexcept
    {
        for (uint32_t i = 0U; i < count; ++i)
        {
            if (subscriptions[i]->filter(data))
                subscriptions[i]->receive(data);
        }
    }
};
```

## Language baseline

- C++23 is required. Prefer standard facilities and requires-expressions over new detection/SFINAE boilerplate.
- Adopt features for a concrete simplification, with compiler coverage and unchanged documented semantics.
- Keep runtime costs opt-in. A language upgrade must not introduce allocation, scheduling or locking by default.
- Preserve dated evidence as historical measurements; rerun the relevant gates when changing generated code.

## Templates

- Use angle brackets with space inside for readability: `template< typename Data >`
- CRTP pattern classes should document the Target type expectation
- Use `using` aliases over `typedef` for new code

## Documentation

- Doxygen-style comments with `/** */` for public API
- `@tparam`, `@param[in]`, `@return`, `@remark`, `@note`, `@warning` tags
- Inline `///<` for member variable documentation
- No documentation needed for obvious getters/setters

## Preprocessor

- Feature flags use `#ifndef` / `#define` / `#endif` pattern with default values
- Guard conditions: `#if SUB0PUB_FLAG` (not `#ifdef`)
- Include guard: `#ifndef CROG_SUB0PUB_<PATH>_HPP`, the header's path under `include/` in upper case, e.g.
  `CROG_SUB0PUB_BROKER_SUBSCRIBE_HPP` for `sub0pub/broker/subscribe.hpp` (the umbrella keeps `CROG_SUB0PUB_HPP`)

## Headers

- One responsibility per header, in the area directory it belongs to (`utility/`, `broker/`, `wiring/`, `ipc/`);
  `types.hpp` collects general-purpose helper types until a group of them earns its own header
- Every header includes what it uses (`tests/headers` compiles each one on its own) and includes
  `sub0pub/config_macros.hpp` before reading a `SUB0PUB_*` macro; only `config_macros.hpp` defines their defaults
- An area's entry header (`broker.hpp`, `wiring.hpp`, `ipc.hpp`) must not reach another area; bridges between
  areas get their own header (`tests/headers` checks the isolation)

## Error Handling

- Use `assert()` guarded by `#if SUB0PUB_ASSERT` for debug checks
- Use `std::runtime_error` guarded by `#if __cpp_exceptions` for recoverable errors
- `noexcept` on all publish/receive hot-path functions

## Integer Types

- Use `<cstdint>` fixed-width types: `uint32_t`, `uint8_t`, `uint_fast16_t`
- Unsigned literals with `U` suffix: `0U`, `8U`
- Cast explicitly when narrowing: `static_cast<uint_fast16_t>(value)`

## Includes

- Standard library includes sorted alphabetically
- Project includes use quotes: `#include "sub0pub/sub0pub.hpp"`
- System includes use angle brackets: `#include <algorithm>`

## Vocabulary

One word per idea, in documentation, comments, diagnostics and identifiers. A second word for the same thing reads as
a second thing.

| Idea | Say | Do not say |
|---|---|---|
| A subscriber starts or stops receiving a type at run time | **subscribe** / **unsubscribe** (`trySubscribe()`, `unsubscribe()`, `isSubscribed()`) | register / unregister, join / leave, attach / detach, connect / disconnect, activate, come and go |
| What holds the runtime subscribers of a type | the **subscription table**; it is **full** when a subscription is refused | registry, slots, list |
| Anything with a `receive()` for a message type | **receiver**; a **subscriber** is a receiver that derives from `Subscribe<T>` | listener, handler, sink (a `Sink<T>` is a publication port) |
| One `publish()` call, and one `receive()` call it causes | a **publication**, and a **delivery** | event, dispatch (the loop inside the broker), send (a transport sends) |
| A message type's own receivers (`StaticTo`, `StaticFirst`) | the type **names** or **lists** them: a **listed** receiver | bound, wired in, registered |
| Receivers given to `wire()` or `StaticWiring` | the wiring **binds** them: a **bound** receiver | listed, attached |
| How a message type is delivered | its **topology**: **brokered**, **statically wired** (`StaticTo`), or both (`StaticFirst`) | static / dynamic API, direct wiring, mode |
| A `Domain`'s lifetime | **open** / **close**; closing unsubscribes every subscriber | detach, shut down |

Pairs stay pairs: if one half of an operation is named, name the other with its counterpart, in the API first and then
in the text that describes it.

## Examples: a source-first reading guide

Every C++ sample under `examples/`, including retained/disabled samples, MUST start with a `/** ... */`
header before includes or pragmas. The entry source is a standalone crib: a developer should be able to
judge its relevance and follow its story without opening a README or the library headers.

Use these short sections, in this order:

- **Title:** name the activity and the pattern in plain language.
- **Use when:** the developer's problem this sample helps solve.
- **Demonstrates:** the relevant public APIs and policy choices, connected to their purpose.
- **Story:** who publishes, who receives, the important sequence/lifetime changes and the expected result.
- **Keep in mind:** only the prerequisites, ownership rules or limitations needed to adapt this code safely.
- **Run:** the executable target and what success looks like (printed output or a zero exit status). Clearly
  label a disabled/unvalidated sample; never imply that an illustrative sketch is supported or measured.

Aim for 12–25 comment lines per entry source; clarity takes precedence over a hard word count. A companion
source/header uses the same sections more briefly, explains its part of the story, and points to the entry
source/target rather than repeating the complete introduction. Keep the header beside the code it describes.

Use concrete activity names for objects. Give independently selectable patterns separate source files and
executable targets; do not merge alternatives solely because they share a few small receiver types. Keep
related steps of one story together. Prefer a small self-contained example over forcing readers through
shared teaching scaffolding; named functions may organize the steps within that story. Below the header,
prefer self-describing code and a few comments explaining non-obvious contracts. Do not annotate obvious
C++ line by line, bury the story in assertions, or make unsupported performance/portability claims. Examples
may explain more than library implementation code: their purpose is teaching, not just regression testing.

When adding or changing a sample, review the header against the actual code and observable result. Ask a
source-only first-time reader to explain its use case, participants, sequence and limitations; revise any
ambiguity. Check all affected companion files too, and keep the example index accurate. Build/run enabled
samples; distinguish those checks from any disabled or platform-specific cases that remain unvalidated.

Organize examples by use case for the current API; do not create a `v2` tier beside supposedly legacy
samples. Static wiring, runtime subscription, mixed paths and IPC are first-class current APIs. A familiar
v1-era name is not by itself a compatibility adapter. Keep actual old-contract adapters separate under
`examples/compatibility/<version>/`, with a header identifying the compatibility contract, its current
replacement, limitations and removal criteria. Such adapters are explicitly tracked migration debt;
normal examples must not depend on them. Retain historical benchmark fixtures separately as evidence.
