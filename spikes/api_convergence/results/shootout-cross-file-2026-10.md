# API convergence shootout

Measured with `spikes/api_convergence/tools/shootout.py`: the collapse-evidence method of docs/EVIDENCE.md applied to the spike cases. On Windows the `instr` columns are exact executed-instruction counts from the spike driver's single-step mode (`--instr`), not callgrind; the counted region is the same (the driver's loop, the call and the publication).

Final-link evidence per case, build and form; every variant is compared with `handwritten` (equal-work reference, same build and form), or with the extra reference it names, shown as `variant (vs handwritten_<kind>)`: e.g. `handwritten_runtime`, hand-written code that reaches its receivers through addresses stored at setup. Deltas in parentheses. instr = callgrind instructions (publish: per publication of 1000). path = static instructions of `collapse_publish` plus directly reachable functions.

- **msvc-O2**: `Microsoft (R) C/C++ Optimizing Compiler Version 19.51.36246 for x64` `/O2`
- **msvc-O2-lto**: `Microsoft (R) C/C++ Optimizing Compiler Version 19.51.36246 for x64` `/O2 /GL`
- **clangcl-O2**: `clang version 22.1.8 (https://github.com/llvm/llvm-project ca7933e47d3a3451d81e72ac174dcb5aa28b59d1)` `/O2`
- **icx-O2**: `Intel(R) oneAPI DPC++/C++ Compiler 2026.1.1 (2026.1.1.20260724)` `/O2`

## Case: cross_file

### msvc-O2, observable work

| variant | checksum | publish instr | setup instr | teardown instr | path instr | calls (direct/indirect) | text | data+bss | retained sub0 (B) | added deps | verdict |
|---|---|---|---|---|---|---|---|---|---|---|---|
| handwritten | ok | 39.0 (+0.0) | 5 (+0) | 2 (+0) | 27 (+0) | 3/0 | 139070 (+0) | 5704 (+0) | 0 | - | reference |
| route_dynamic (vs today_dynamic) | ok | 63.0 (+0.0) | 42 (+0) | 96 (+0) | 20 (+0) | 0/1 | 140162 (+0) | 6072 (+0) | 584 | - | PASS |
| route_static | ok | 39.0 (+0.0) | 5 (+0) | 2 (+0) | 27 (+0) | 3/0 | 139070 (+0) | 5704 (+0) | 0 | - | PASS |
| today_dynamic | ok | 63.0 (+24.0) | 42 (+37) | 96 (+94) | 20 (-7) | 0/1 | 140162 (+1092) | 6072 (+368) | 584 | pure virtual | FAIL: publish instr, setup instr, teardown instr, no extra indirect calls, no extra RAM, no Sub0Pub retained, no extra dependencies |
| today_static | ok | 39.0 (+0.0) | 5 (+0) | 2 (+0) | 27 (+0) | 3/0 | 139070 (+0) | 5704 (+0) | 0 | - | PASS |

### msvc-O2, removable work

| variant | checksum | publish instr | setup instr | teardown instr | path instr | calls (direct/indirect) | text | data+bss | retained sub0 (B) | added deps | verdict |
|---|---|---|---|---|---|---|---|---|---|---|---|
| handwritten | ok | 24.0 (+0.0) | 5 (+0) | 2 (+0) | 17 (+0) | 3/0 | 139038 (+0) | 5704 (+0) | 0 | - | reference |
| route_dynamic (vs today_dynamic) | ok | 48.0 (+0.0) | 42 (+0) | 96 (+0) | 20 (+0) | 0/1 | 140130 (+0) | 6072 (+0) | 584 | - | PASS |
| route_static | ok | 24.0 (+0.0) | 5 (+0) | 2 (+0) | 17 (+0) | 3/0 | 139038 (+0) | 5704 (+0) | 0 | - | PASS |
| today_dynamic | ok | 48.0 (+24.0) | 42 (+37) | 96 (+94) | 20 (+3) | 0/1 | 140130 (+1092) | 6072 (+368) | 584 | pure virtual | FAIL: publish instr, setup instr, teardown instr, publish path, no extra indirect calls, no extra RAM, no Sub0Pub retained, no extra dependencies |
| today_static | ok | 24.0 (+0.0) | 5 (+0) | 2 (+0) | 17 (+0) | 3/0 | 139038 (+0) | 5704 (+0) | 0 | - | PASS |

<details><summary>msvc-O2: largest symbols added by route_dynamic (bytes)</summary>

- 24 `struct app::LoggerSlot app::logger`
- 24 `struct app::ControllerSlot app::controllerB`
- 24 `struct app::ControllerSlot app::controllerA`
- 16 `class collapse::Slot<struct `anonymous namespace'::Sensor> `anonymous namespace'::sensor`

</details>

<details><summary>msvc-O2: largest symbols added by route_static (bytes)</summary>

- 8 `struct app::LoggerSlot app::logger`
- 4 `struct app::ControllerSlot app::controllerB`
- 4 `struct app::ControllerSlot app::controllerA`

</details>

<details><summary>msvc-O2: largest symbols added by today_dynamic (bytes)</summary>

- 1188 `__volatile_metadata`
- 256 `collapse_setup`
- 128 `protected: __cdecl sub0::Subscribe<struct app::Sample>::~Subscribe<struct app::Sample>(void) __ptr64`
- 104 `SetSmallXmm`
- 80 `class sub0::detail::SubscriberInterface<struct app::Sample,0> `RTTI Type Descriptor'`
- 72 `private: static struct sub0::detail::Table<struct app::Sample,struct sub0::detail::BuiltinT<8,1,2,0,struct sub0::NoLock> > sub0::detail::BrokerImpl<struct app::Sample,struct sub0::detail::BuiltinT<8,1,2,0,struct sub0::NoLock> >::global_`
- 56 `class sub0::Subscribe<struct app::Sample> `RTTI Type Descriptor'`
- 56 ``string'`

</details>

<details><summary>msvc-O2: largest symbols added by today_static (bytes)</summary>

- 8 `class collapse::Slot<struct app::Logger> `anonymous namespace'::logger`
- 4 `class collapse::Slot<struct app::Controller> `anonymous namespace'::controllerB`
- 4 `class collapse::Slot<struct app::Controller> `anonymous namespace'::controllerA`

</details>

### msvc-O2-lto, observable work

| variant | checksum | publish instr | setup instr | teardown instr | path instr | calls (direct/indirect) | text | data+bss | retained sub0 (B) | added deps | verdict |
|---|---|---|---|---|---|---|---|---|---|---|---|
| handwritten | ok | 19.0 (+0.0) | 5 (+0) | 2 (+0) | 13 (+0) | 0/0 | 138954 (+0) | 5704 (+0) | 0 | - | reference |
| route_dynamic (vs today_dynamic) | ok | 63.0 (+0.0) | 42 (+0) | 75 (+0) | 20 (+0) | 0/1 | 140326 (+0) | 6080 (+0) | 456 | - | PASS |
| route_static | ok | 19.0 (+0.0) | 5 (+0) | 2 (+0) | 13 (+0) | 0/0 | 138954 (+0) | 5712 (+8) | 0 | - | FAIL: no extra RAM |
| today_dynamic | ok | 63.0 (+44.0) | 42 (+37) | 75 (+73) | 20 (+7) | 0/1 | 140326 (+1372) | 6080 (+376) | 456 | pure virtual | FAIL: publish instr, setup instr, teardown instr, publish path, no extra indirect calls, no extra RAM, no Sub0Pub retained, no extra dependencies |
| today_static | ok | 19.0 (+0.0) | 5 (+0) | 2 (+0) | 13 (+0) | 0/0 | 138954 (+0) | 5712 (+8) | 0 | - | FAIL: no extra RAM |

### msvc-O2-lto, removable work

| variant | checksum | publish instr | setup instr | teardown instr | path instr | calls (direct/indirect) | text | data+bss | retained sub0 (B) | added deps | verdict |
|---|---|---|---|---|---|---|---|---|---|---|---|
| handwritten | ok | 9.0 (+0.0) | 5 (+0) | 2 (+0) | 3 (+0) | 0/0 | 138890 (+0) | 5704 (+0) | 0 | - | reference |
| route_dynamic (vs today_dynamic) | ok | 48.0 (+0.0) | 42 (+0) | 75 (+0) | 20 (+0) | 0/1 | 140294 (+0) | 6080 (+0) | 456 | - | PASS |
| route_static | ok | 9.0 (+0.0) | 5 (+0) | 2 (+0) | 3 (+0) | 0/0 | 138890 (+0) | 5712 (+8) | 0 | - | FAIL: no extra RAM |
| today_dynamic | ok | 48.0 (+39.0) | 42 (+37) | 75 (+73) | 20 (+17) | 0/1 | 140294 (+1404) | 6080 (+376) | 456 | pure virtual | FAIL: publish instr, setup instr, teardown instr, publish path, no extra indirect calls, no extra RAM, no Sub0Pub retained, no extra dependencies |
| today_static | ok | 9.0 (+0.0) | 5 (+0) | 2 (+0) | 3 (+0) | 0/0 | 138890 (+0) | 5712 (+8) | 0 | - | FAIL: no extra RAM |

<details><summary>msvc-O2-lto: largest symbols added by route_dynamic (bytes)</summary>

- 24 `struct app::LoggerSlot app::logger`
- 24 `struct app::ControllerSlot app::controllerB`
- 24 `struct app::ControllerSlot app::controllerA`
- 16 `class collapse::Slot<struct `anonymous namespace'::Sensor> `anonymous namespace'::sensor`

</details>

<details><summary>msvc-O2-lto: largest symbols added by route_static (bytes)</summary>

- 8 `__scrt_ucrt_dll_is_in_use`
- 4 `class collapse::Slot<struct `anonymous namespace'::Sensor> `anonymous namespace'::sensor`
- 4 `struct app::LoggerSlot app::logger`
- 4 `struct app::ControllerSlot app::controllerB`
- 4 `struct app::ControllerSlot app::controllerA`

</details>

<details><summary>msvc-O2-lto: largest symbols added by today_dynamic (bytes)</summary>

- 1188 `__volatile_metadata`
- 368 `collapse_teardown`
- 256 `collapse_setup`
- 120 `SetSmallXmm`
- 80 `collapse_publish`
- 80 `class sub0::detail::SubscriberInterface<struct app::Sample,0> `RTTI Type Descriptor'`
- 72 `private: static struct sub0::detail::Table<struct app::Sample,struct sub0::detail::BuiltinT<8,1,2,0,struct sub0::NoLock> > sub0::detail::BrokerImpl<struct app::Sample,struct sub0::detail::BuiltinT<8,1,2,0,struct sub0::NoLock> >::global_`
- 56 `class sub0::Subscribe<struct app::Sample> `RTTI Type Descriptor'`

</details>

<details><summary>msvc-O2-lto: largest symbols added by today_static (bytes)</summary>

- 8 `__scrt_ucrt_dll_is_in_use`
- 4 `class collapse::Slot<struct `anonymous namespace'::Sensor<struct sub0::StaticWiring<&class collapse::Slot<struct app::Controller> `anonymous namespace'::controllerA,&class collapse::Slot<struct app::Controller> A0x3ff54c27::controllerB,&class collapse::Slot<struct app::Logger> A0x3ff54c27::logger> > > `anonymous namespace'::sensor`
- 4 `class collapse::Slot<struct app::Logger> `anonymous namespace'::logger`
- 4 `class collapse::Slot<struct app::Controller> `anonymous namespace'::controllerB`
- 4 `class collapse::Slot<struct app::Controller> `anonymous namespace'::controllerA`

</details>

### clangcl-O2, observable work

| variant | checksum | publish instr | setup instr | teardown instr | path instr | calls (direct/indirect) | text | data+bss | retained sub0 (B) | added deps | verdict |
|---|---|---|---|---|---|---|---|---|---|---|---|
| handwritten | ok | 53.0 (+0.0) | 5 (+0) | 2 (+0) | 38 (+0) | 3/0 | 143629 (+0) | 5701 (+0) | 0 | - | reference |
| route_dynamic (vs today_dynamic) | ok | 76.0 (+0.0) | 38 (+0) | 89 (+0) | 25 (+0) | 0/1 | 145695 (+0) | 6025 (+0) | 416 | - | PASS |
| route_static | ok | 53.0 (+0.0) | 5 (+0) | 2 (+0) | 38 (+0) | 3/0 | 143629 (+0) | 5701 (+0) | 0 | - | PASS |
| today_dynamic | ok | 76.0 (+23.0) | 38 (+33) | 89 (+87) | 25 (-13) | 0/1 | 145695 (+2066) | 6025 (+324) | 416 | pure virtual | FAIL: publish instr, setup instr, teardown instr, no extra indirect calls, no extra RAM, no Sub0Pub retained, no extra dependencies |
| today_static | ok | 53.0 (+0.0) | 5 (+0) | 2 (+0) | 38 (+0) | 3/0 | 143629 (+0) | 5701 (+0) | 0 | - | PASS |

### clangcl-O2, removable work

| variant | checksum | publish instr | setup instr | teardown instr | path instr | calls (direct/indirect) | text | data+bss | retained sub0 (B) | added deps | verdict |
|---|---|---|---|---|---|---|---|---|---|---|---|
| handwritten | ok | 28.0 (+0.0) | 5 (+0) | 2 (+0) | 21 (+0) | 3/0 | 143586 (+0) | 5701 (+0) | 0 | - | reference |
| route_dynamic (vs today_dynamic) | ok | 51.0 (+0.0) | 38 (+0) | 89 (+0) | 25 (+0) | 0/1 | 145651 (+0) | 6025 (+0) | 416 | - | PASS |
| route_static | ok | 28.0 (+0.0) | 5 (+0) | 2 (+0) | 21 (+0) | 3/0 | 143586 (+0) | 5701 (+0) | 0 | - | PASS |
| today_dynamic | ok | 51.0 (+23.0) | 38 (+33) | 89 (+87) | 25 (+4) | 0/1 | 145651 (+2065) | 6025 (+324) | 416 | pure virtual | FAIL: publish instr, setup instr, teardown instr, publish path, no extra indirect calls, no extra RAM, no Sub0Pub retained, no extra dependencies |
| today_static | ok | 28.0 (+0.0) | 5 (+0) | 2 (+0) | 21 (+0) | 3/0 | 143586 (+0) | 5701 (+0) | 0 | - | PASS |

<details><summary>clangcl-O2: largest symbols added by route_dynamic (bytes)</summary>

- 24 `struct app::LoggerSlot app::logger`
- 24 `struct app::ControllerSlot app::controllerB`
- 24 `struct app::ControllerSlot app::controllerA`

</details>

<details><summary>clangcl-O2: largest symbols added by route_static (bytes)</summary>

- 8 `struct app::LoggerSlot app::logger`
- 4 `struct app::ControllerSlot app::controllerB`
- 4 `struct app::ControllerSlot app::controllerA`

</details>

<details><summary>clangcl-O2: largest symbols added by today_dynamic (bytes)</summary>

- 1232 `collapse_teardown`
- 444 `__acrt_SetEnvironmentVariableA`
- 304 `collapse_setup`
- 96 `collapse_publish`
- 84 `__rtc_tzz`
- 80 `class sub0::detail::SubscriberInterface<struct app::Sample,0> `RTTI Type Descriptor'`
- 72 `private: static struct sub0::detail::Table<struct app::Sample,struct sub0::detail::BuiltinT<8,1,2,0,struct sub0::NoLock> > sub0::detail::BrokerImpl<struct app::Sample,struct sub0::detail::BuiltinT<8,1,2,0,struct sub0::NoLock> >::global_`
- 64 `class sub0::Subscribe<struct app::Sample> `RTTI Type Descriptor'`

</details>

<details><summary>clangcl-O2: largest symbols added by today_static (bytes)</summary>

- 8 `class collapse::Slot<struct app::Logger> `anonymous namespace'::logger`
- 4 `class collapse::Slot<struct app::Controller> `anonymous namespace'::controllerB`
- 4 `class collapse::Slot<struct app::Controller> `anonymous namespace'::controllerA`

</details>

### icx-O2, observable work

| variant | checksum | publish instr | setup instr | teardown instr | path instr | calls (direct/indirect) | text | data+bss | retained sub0 (B) | added deps | verdict |
|---|---|---|---|---|---|---|---|---|---|---|---|
| handwritten | ok | 53.0 (+0.0) | 5 (+0) | 2 (+0) | 38 (+0) | 3/0 | 139510 (+0) | 5688 (+0) | 0 | - | reference |
| route_dynamic (vs today_dynamic) | ok | 76.0 (+0.0) | 39 (+0) | 72 (+0) | 25 (+0) | 0/1 | 140702 (+0) | 6072 (+0) | 416 | - | PASS |
| route_static | ok | 53.0 (+0.0) | 5 (+0) | 2 (+0) | 38 (+0) | 3/0 | 139510 (+0) | 5688 (+0) | 0 | - | PASS |
| today_dynamic | ok | 76.0 (+23.0) | 39 (+34) | 72 (+70) | 25 (-13) | 0/1 | 140702 (+1192) | 6072 (+384) | 416 | pure virtual | FAIL: publish instr, setup instr, teardown instr, no extra indirect calls, no extra RAM, no Sub0Pub retained, no extra dependencies |
| today_static | ok | 53.0 (+0.0) | 5 (+0) | 2 (+0) | 38 (+0) | 3/0 | 139510 (+0) | 5688 (+0) | 0 | - | PASS |

### icx-O2, removable work

| variant | checksum | publish instr | setup instr | teardown instr | path instr | calls (direct/indirect) | text | data+bss | retained sub0 (B) | added deps | verdict |
|---|---|---|---|---|---|---|---|---|---|---|---|
| handwritten | ok | 28.0 (+0.0) | 5 (+0) | 2 (+0) | 21 (+0) | 3/0 | 139478 (+0) | 5688 (+0) | 0 | - | reference |
| route_dynamic (vs today_dynamic) | ok | 51.0 (+0.0) | 39 (+0) | 72 (+0) | 25 (+0) | 0/1 | 140670 (+0) | 6072 (+0) | 416 | - | PASS |
| route_static | ok | 28.0 (+0.0) | 5 (+0) | 2 (+0) | 21 (+0) | 3/0 | 139478 (+0) | 5688 (+0) | 0 | - | PASS |
| today_dynamic | ok | 51.0 (+23.0) | 39 (+34) | 72 (+70) | 25 (+4) | 0/1 | 140670 (+1192) | 6072 (+384) | 416 | pure virtual | FAIL: publish instr, setup instr, teardown instr, publish path, no extra indirect calls, no extra RAM, no Sub0Pub retained, no extra dependencies |
| today_static | ok | 28.0 (+0.0) | 5 (+0) | 2 (+0) | 21 (+0) | 3/0 | 139478 (+0) | 5688 (+0) | 0 | - | PASS |

<details><summary>icx-O2: largest symbols added by route_dynamic (bytes)</summary>

- 24 `struct app::LoggerSlot app::logger`
- 24 `struct app::ControllerSlot app::controllerB`
- 24 `struct app::ControllerSlot app::controllerA`

</details>

<details><summary>icx-O2: largest symbols added by route_static (bytes)</summary>

- 8 `struct app::LoggerSlot app::logger`
- 4 `struct app::ControllerSlot app::controllerB`
- 4 `struct app::ControllerSlot app::controllerA`

</details>

<details><summary>icx-O2: largest symbols added by today_dynamic (bytes)</summary>

- 1204 `__volatile_metadata`
- 352 `collapse_teardown`
- 288 `collapse_setup`
- 104 `SetSmallXmm`
- 96 `collapse_publish`
- 80 `class sub0::detail::SubscriberInterface<struct app::Sample,0> `RTTI Type Descriptor'`
- 72 `private: static struct sub0::detail::Table<struct app::Sample,struct sub0::detail::BuiltinT<8,1,2,0,struct sub0::NoLock> > sub0::detail::BrokerImpl<struct app::Sample,struct sub0::detail::BuiltinT<8,1,2,0,struct sub0::NoLock> >::global_`
- 64 `class sub0::Subscribe<struct app::Sample> `RTTI Type Descriptor'`

</details>

<details><summary>icx-O2: largest symbols added by today_static (bytes)</summary>

- 8 `class collapse::Slot<struct app::Logger> `anonymous namespace'::logger`
- 4 `class collapse::Slot<struct app::Controller> `anonymous namespace'::controllerB`
- 4 `class collapse::Slot<struct app::Controller> `anonymous namespace'::controllerA`

</details>


## Summary: every variant against its reference (observable work)

Each cell: instructions per publication (delta), then the deltas of the static publish path, indirect calls on it, RAM and image text. `=` marks a variant that meets every criterion of docs/EVIDENCE.md against that reference; `!` one that does not.

### cross_file

| variant | judged against | msvc-O2 | msvc-O2-lto | clangcl-O2 | icx-O2 |
|---|---|---|---|---|---|
| handwritten | reference | 39.0 instr; path 27 | 19.0 instr; path 13 | 53.0 instr; path 38 | 53.0 instr; path 38 |
| route_dynamic | today_dynamic | = 63.0 (+0.0); path +0; indirect +0; RAM +0; text +0 | = 63.0 (+0.0); path +0; indirect +0; RAM +0; text +0 | = 76.0 (+0.0); path +0; indirect +0; RAM +0; text +0 | = 76.0 (+0.0); path +0; indirect +0; RAM +0; text +0 |
| route_static | handwritten | = 39.0 (+0.0); path +0; indirect +0; RAM +0; text +0 | ! 19.0 (+0.0); path +0; indirect +0; RAM +8; text +0 | = 53.0 (+0.0); path +0; indirect +0; RAM +0; text +0 | = 53.0 (+0.0); path +0; indirect +0; RAM +0; text +0 |
| today_dynamic | handwritten | ! 63.0 (+24.0); path -7; indirect +1; RAM +368; text +1092 | ! 63.0 (+44.0); path +7; indirect +1; RAM +376; text +1372 | ! 76.0 (+23.0); path -13; indirect +1; RAM +324; text +2066 | ! 76.0 (+23.0); path -13; indirect +1; RAM +384; text +1192 |
| today_static | handwritten | = 39.0 (+0.0); path +0; indirect +0; RAM +0; text +0 | ! 19.0 (+0.0); path +0; indirect +0; RAM +8; text +0 | = 53.0 (+0.0); path +0; indirect +0; RAM +0; text +0 | = 53.0 (+0.0); path +0; indirect +0; RAM +0; text +0 |
