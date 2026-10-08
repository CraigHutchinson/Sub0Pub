# Cross-module example: retained, not currently enabled

This example represents publishing in a shared library to subscribers in an executable. It is retained as
an unresolved v2 use case, not treated as obsolete code. `examples/CMakeLists.txt` currently disables it,
with a note about exporting thread-local broker state on MSVC.

The example explicitly instantiates and exports `detail::BrokerImpl`, an implementation type. A release
recipe should instead establish a supported ownership boundary using public APIs. Compiling or linking
alone is insufficient: independent module-local tables can silently prevent delivery.

Before enabling it as a supported v2 example:

- Decide where shared registry/domain state lives and how publishers and subscribers reach that same state.
  Evaluate an explicitly passed public endpoint or application-owned domain; do not assume inline globals
  or thread-local contexts are shared identically on ELF, Mach-O and Windows DLLs.
- Test both directions of delivery, more than one subscribing module, consistent per-type configuration,
  and the supported filter/cancellation/nested-dispatch contract across the boundary.
- Define compatible compiler/runtime/ABI requirements and object ownership. Unsubscribe subscribers and
  drain callbacks before unloading a module containing their code; unload must leave no callable pointers.
- Add Windows, Linux and macOS shared-library correctness tests, and measure the chosen boundary against
  direct cross-module calls and the equivalent in-process path. Keep static, dynamic and mixed variants distinct.

`Sink<T>` is worth evaluating as an explicit boundary for static/mixed composition, but its non-owning
object and function pointers do not by themselves provide module lifetime safety or a stable cross-compiler ABI.
The existing separate-translation-unit benchmarks do not establish shared-library correctness.
