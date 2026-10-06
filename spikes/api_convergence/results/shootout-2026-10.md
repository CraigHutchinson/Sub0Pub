# API convergence shootout

Measured with `spikes/api_convergence/tools/shootout.py`: the collapse-evidence method of docs/EVIDENCE.md applied to the spike cases. On Windows the `instr` columns are exact executed-instruction counts from the spike driver's single-step mode (`--instr`), not callgrind; the counted region is the same (the driver's loop, the call and the publication).

Final-link evidence per case, build and form; every variant is compared with `handwritten` (equal-work reference, same build and form), or with the extra reference it names, shown as `variant (vs handwritten_<kind>)`: e.g. `handwritten_runtime`, hand-written code that reaches its receivers through addresses stored at setup. Deltas in parentheses. instr = callgrind instructions (publish: per publication of 1000). path = static instructions of `collapse_publish` plus directly reachable functions.

- **msvc-O2**: `Microsoft (R) C/C++ Optimizing Compiler Version 19.51.36246 for x64` `/O2`
- **msvc-O2-lto**: `Microsoft (R) C/C++ Optimizing Compiler Version 19.51.36246 for x64` `/O2 /GL`
- **clangcl-O2**: `clang version 22.1.8 (https://github.com/llvm/llvm-project ca7933e47d3a3451d81e72ac174dcb5aa28b59d1)` `/O2`
- **icx-O2**: `Intel(R) oneAPI DPC++/C++ Compiler 2026.1.1 (2026.1.1.20260724)` `/O2`

## Case: batch

### msvc-O2, observable work

| variant | checksum | publish instr | setup instr | teardown instr | path instr | calls (direct/indirect) | text | data+bss | retained sub0 (B) | added deps | verdict |
|---|---|---|---|---|---|---|---|---|---|---|---|
| handwritten | ok | 201.0 (+0.0) | 219 (+0) | 2 (+0) | 63 (+0) | 0/0 | 139762 (+0) | 5960 (+0) | 0 | - | reference |
| handwritten_runtime | ok | 417.0 (+216.0) | 222 (+3) | 2 (+0) | 117 (+54) | 1/0 | 139958 (+196) | 5976 (+16) | 0 | - | reference; FAIL: publish instr, setup instr, publish path, no extra RAM |
| route_bridged | ok | 732.0 (+531.0) | 249 (+30) | 11 (+9) | 39 (-24) | 0/1 | 140578 (+816) | 6360 (+400) | 800 | pure virtual | FAIL: publish instr, setup instr, teardown instr, no extra indirect calls, no extra RAM, no Sub0Pub retained, no extra dependencies |
| route_dynamic (vs today_dynamic) | ok | 1369.0 (+0.0) | 232 (+0) | 25 (+0) | 34 (+0) | 0/1 | 140298 (+0) | 6264 (+0) | 608 | - | PASS |
| route_static | ok | 205.0 (+4.0) | 219 (+0) | 2 (+0) | 67 (+4) | 1/0 | 139798 (+36) | 5976 (+16) | 0 | - | FAIL: publish instr, publish path, no extra RAM |
| today_dynamic | ok | 1369.0 (+1168.0) | 232 (+13) | 25 (+23) | 34 (-29) | 0/1 | 140298 (+536) | 6264 (+304) | 608 | pure virtual | FAIL: publish instr, setup instr, teardown instr, no extra indirect calls, no extra RAM, no Sub0Pub retained, no extra dependencies |
| today_static | ok | 205.0 (+4.0) | 219 (+0) | 2 (+0) | 67 (+4) | 1/0 | 139798 (+36) | 5976 (+16) | 192 | - | FAIL: publish instr, publish path, no extra RAM, no Sub0Pub retained |
| today_wire (vs handwritten_runtime) | ok | 417.0 (+0.0) | 222 (+0) | 2 (+0) | 117 (+0) | 1/0 | 139958 (+0) | 5976 (+0) | 368 | - | PASS |

### msvc-O2, removable work

| variant | checksum | publish instr | setup instr | teardown instr | path instr | calls (direct/indirect) | text | data+bss | retained sub0 (B) | added deps | verdict |
|---|---|---|---|---|---|---|---|---|---|---|---|
| handwritten | ok | 198.0 (+0.0) | 219 (+0) | 2 (+0) | 60 (+0) | 0/0 | 139746 (+0) | 5960 (+0) | 0 | - | reference |
| handwritten_runtime | ok | 411.0 (+213.0) | 222 (+3) | 2 (+0) | 111 (+51) | 1/0 | 139922 (+176) | 5976 (+16) | 0 | - | reference; FAIL: publish instr, setup instr, publish path, no extra RAM |
| route_bridged | ok | 790.0 (+592.0) | 249 (+30) | 11 (+9) | 34 (-26) | 0/1 | 140546 (+800) | 6360 (+400) | 800 | pure virtual | FAIL: publish instr, setup instr, teardown instr, no extra indirect calls, no extra RAM, no Sub0Pub retained, no extra dependencies |
| route_dynamic (vs today_dynamic) | ok | 1366.0 (+0.0) | 232 (+0) | 25 (+0) | 31 (+0) | 0/1 | 140282 (+0) | 6264 (+0) | 608 | - | PASS |
| route_static | ok | 199.0 (+1.0) | 219 (+0) | 2 (+0) | 61 (+1) | 1/0 | 139762 (+16) | 5976 (+16) | 0 | - | FAIL: no extra RAM |
| today_dynamic | ok | 1366.0 (+1168.0) | 232 (+13) | 25 (+23) | 31 (-29) | 0/1 | 140282 (+536) | 6264 (+304) | 608 | pure virtual | FAIL: publish instr, setup instr, teardown instr, no extra indirect calls, no extra RAM, no Sub0Pub retained, no extra dependencies |
| today_static | ok | 199.0 (+1.0) | 219 (+0) | 2 (+0) | 61 (+1) | 1/0 | 139762 (+16) | 5976 (+16) | 192 | - | FAIL: no extra RAM, no Sub0Pub retained |
| today_wire (vs handwritten_runtime) | ok | 411.0 (+0.0) | 222 (+0) | 2 (+0) | 111 (+0) | 1/0 | 139922 (+0) | 5976 (+0) | 368 | - | PASS |

<details><summary>msvc-O2: largest symbols added by handwritten_runtime (bytes)</summary>

- 1188 `__volatile_metadata`
- 368 `public: void __cdecl `anonymous namespace'::Sensor::sendBurst(unsigned int) __ptr64`
- 256 `unsigned int * `anonymous namespace'::block`
- 32 `struct _onexit_table_t module_local_at_quick_exit_table`
- 12 `$unwind$?sendBurst@Sensor@?A0x0f519126@@QEAAXI@Z`
- 8 `class collapse::Slot<struct `anonymous namespace'::Sensor> `anonymous namespace'::sensor`
- 8 `class collapse::Slot<struct `anonymous namespace'::Accumulator> `anonymous namespace'::accumulator`

</details>

<details><summary>msvc-O2: largest symbols added by route_bridged (bytes)</summary>

- 608 `collapse_setup`
- 256 `unsigned int * `anonymous namespace'::block`
- 128 `public: void __cdecl sub0::Subscribe<struct `anonymous namespace'::Sample>::disconnect(void) __ptr64`
- 88 `class sub0::detail::SubscriberInterface<struct `anonymous namespace'::Sample,0> `RTTI Type Descriptor'`
- 88 `class sub0::spike::detail::BridgedSubscribe<struct `anonymous namespace'::Sample> `RTTI Type Descriptor'`
- 80 `private: static struct sub0::detail::Table<struct `anonymous namespace'::Sample,struct sub0::with<struct sub0::detail::BuiltinT<8,1,2,0,struct sub0::NoLock>,struct sub0::spike::StaticFirst<&struct A0x0a8fb149::AccumulatorSlot A0x0a8fb149::accumulator> > > sub0::detail::BrokerImpl<struct `anonymous namespace'::Sample,struct sub0::with<struct sub0::detail::BuiltinT<8,1,2,0,struct sub0::NoLock>,struct sub0::spike::StaticFirst<&struct A0x0a8fb149::AccumulatorSlot A0x0a8fb149::accumulator> > >::global_`
- 72 `class sub0::Subscribe<struct `anonymous namespace'::Sample> `RTTI Type Descriptor'`
- 56 ``string'`

</details>

<details><summary>msvc-O2: largest symbols added by route_dynamic (bytes)</summary>

- 256 `unsigned int * `anonymous namespace'::block`
- 128 `protected: __cdecl sub0::Subscribe<struct `anonymous namespace'::Sample>::~Subscribe<struct `anonymous namespace'::Sample>(void) __ptr64`
- 88 `class sub0::detail::SubscriberInterface<struct `anonymous namespace'::Sample,0> `RTTI Type Descriptor'`
- 80 `private: static struct sub0::detail::Table<struct `anonymous namespace'::Sample,struct sub0::detail::BuiltinT<8,1,2,0,struct sub0::NoLock> > sub0::detail::BrokerImpl<struct `anonymous namespace'::Sample,struct sub0::detail::BuiltinT<8,1,2,0,struct sub0::NoLock> >::global_`
- 64 `class sub0::Subscribe<struct `anonymous namespace'::Sample> `RTTI Type Descriptor'`
- 48 `struct `anonymous namespace'::Accumulator `RTTI Type Descriptor'`
- 40 `const `anonymous namespace'::Accumulator::`RTTI Complete Object Locator'`
- 40 `const sub0::Subscribe<struct `anonymous namespace'::Sample>::`RTTI Complete Object Locator'`

</details>

<details><summary>msvc-O2: largest symbols added by route_static (bytes)</summary>

- 256 `unsigned int * `anonymous namespace'::block`
- 192 `public: void __cdecl `anonymous namespace'::Sensor::sendBurst(unsigned int) __ptr64`
- 32 `struct _onexit_table_t module_local_at_quick_exit_table`
- 12 `$unwind$?sendBurst@Sensor@?A0x9924a2ac@@QEAAXI@Z`
- 8 `class collapse::Slot<struct `anonymous namespace'::Sensor> `anonymous namespace'::sensor`
- 8 `struct A0x9924a2ac::AccumulatorSlot `anonymous namespace'::accumulator`

</details>

<details><summary>msvc-O2: largest symbols added by today_dynamic (bytes)</summary>

- 480 `collapse_setup`
- 256 `unsigned int * `anonymous namespace'::block`
- 128 `protected: __cdecl sub0::Subscribe<struct `anonymous namespace'::Sample>::~Subscribe<struct `anonymous namespace'::Sample>(void) __ptr64`
- 88 `class sub0::detail::SubscriberInterface<struct `anonymous namespace'::Sample,0> `RTTI Type Descriptor'`
- 80 `private: static struct sub0::detail::Table<struct `anonymous namespace'::Sample,struct sub0::detail::BuiltinT<8,1,2,0,struct sub0::NoLock> > sub0::detail::BrokerImpl<struct `anonymous namespace'::Sample,struct sub0::detail::BuiltinT<8,1,2,0,struct sub0::NoLock> >::global_`
- 64 `class sub0::Subscribe<struct `anonymous namespace'::Sample> `RTTI Type Descriptor'`
- 56 ``string'`
- 48 `struct `anonymous namespace'::Accumulator `RTTI Type Descriptor'`

</details>

<details><summary>msvc-O2: largest symbols added by today_static (bytes)</summary>

- 256 `unsigned int * `anonymous namespace'::block`
- 192 `public: void __cdecl `anonymous namespace'::Sensor<struct sub0::StaticWiring<&class collapse::Slot<struct `anonymous namespace'::Accumulator> `anonymous namespace'::accumulator> >::sendBurst(unsigned int) __ptr64`
- 32 `struct _onexit_table_t module_local_at_quick_exit_table`
- 12 `$unwind$?sendBurst@?$Sensor@U?$StaticWiring@$MPEAV?$Slot@UAccumulator@?A0x0f251c23@@@collapse@@1?accumulator@?A0x0f251c23@@3V12@A@sub0@@@?A0x0f251c23@@QEAAXI@Z`
- 8 `class collapse::Slot<struct `anonymous namespace'::Sensor<struct sub0::StaticWiring<&class collapse::Slot<struct `anonymous namespace'::Accumulator> `anonymous namespace'::accumulator> > > `anonymous namespace'::sensor`
- 8 `class collapse::Slot<struct `anonymous namespace'::Accumulator> `anonymous namespace'::accumulator`

</details>

<details><summary>msvc-O2: largest symbols added by today_wire (bytes)</summary>

- 368 `public: void __cdecl `anonymous namespace'::Sensor<class sub0::Wiring<struct `anonymous namespace'::Accumulator> >::sendBurst(unsigned int) __ptr64`
- 256 `unsigned int * `anonymous namespace'::block`
- 12 `$unwind$?sendBurst@?$Sensor@V?$Wiring@UAccumulator@?A0x13307c14@@@sub0@@@?A0x13307c14@@QEAAXI@Z`
- 8 `class collapse::Slot<struct `anonymous namespace'::Sensor<class sub0::Wiring<struct `anonymous namespace'::Accumulator> > > `anonymous namespace'::sensor`
- 8 `class collapse::Slot<struct `anonymous namespace'::Accumulator> `anonymous namespace'::accumulator`

</details>

### clangcl-O2, observable work

| variant | checksum | publish instr | setup instr | teardown instr | path instr | calls (direct/indirect) | text | data+bss | retained sub0 (B) | added deps | verdict |
|---|---|---|---|---|---|---|---|---|---|---|---|
| handwritten | ok | 136.0 (+0.0) | 36 (+0) | 2 (+0) | 130 (+0) | 0/0 | 144608 (+0) | 5941 (+0) | 0 | - | reference |
| handwritten_runtime | ok | 373.0 (+237.0) | 37 (+1) | 2 (+0) | 26 (-104) | 0/0 | 144044 (-564) | 5957 (+16) | 0 | - | reference; FAIL: publish instr, setup instr, no extra RAM |
| route_bridged | ok | 149.0 (+13.0) | 60 (+24) | 12 (+10) | 174 (+44) | 0/1 | 146332 (+1724) | 6297 (+356) | 584 | pure virtual | FAIL: publish instr, setup instr, teardown instr, publish path, no extra indirect calls, no extra RAM, no Sub0Pub retained, no extra dependencies |
| route_dynamic (vs today_dynamic) | ok | 1573.0 (+0.0) | 47 (+0) | 28 (+0) | 52 (+0) | 0/1 | 145032 (+0) | 6233 (+0) | 424 | - | PASS |
| route_static | ok | 136.0 (+0.0) | 36 (+0) | 2 (+0) | 130 (+0) | 0/0 | 144608 (+0) | 5941 (+0) | 0 | - | PASS |
| today_dynamic | ok | 1573.0 (+1437.0) | 47 (+11) | 28 (+26) | 52 (-78) | 0/1 | 145032 (+424) | 6233 (+292) | 424 | pure virtual | FAIL: publish instr, setup instr, teardown instr, no extra indirect calls, no extra RAM, no Sub0Pub retained, no extra dependencies |
| today_static | ok | 136.0 (+0.0) | 36 (+0) | 2 (+0) | 130 (+0) | 0/0 | 144608 (+0) | 5941 (+0) | 0 | - | PASS |
| today_wire (vs handwritten_runtime) | ok | 373.0 (+0.0) | 37 (+0) | 2 (+0) | 26 (+0) | 0/0 | 144044 (+0) | 5957 (+0) | 0 | - | PASS |

### clangcl-O2, removable work

| variant | checksum | publish instr | setup instr | teardown instr | path instr | calls (direct/indirect) | text | data+bss | retained sub0 (B) | added deps | verdict |
|---|---|---|---|---|---|---|---|---|---|---|---|
| handwritten | ok | 129.0 (+0.0) | 36 (+0) | 2 (+0) | 123 (+0) | 0/0 | 144576 (+0) | 5941 (+0) | 0 | - | reference |
| handwritten_runtime | ok | 367.0 (+238.0) | 37 (+1) | 2 (+0) | 20 (-103) | 0/0 | 144012 (-564) | 5957 (+16) | 0 | - | reference; FAIL: publish instr, setup instr, no extra RAM |
| route_bridged | ok | 142.0 (+13.0) | 60 (+24) | 12 (+10) | 164 (+41) | 0/1 | 146300 (+1724) | 6297 (+356) | 584 | pure virtual | FAIL: publish instr, setup instr, teardown instr, publish path, no extra indirect calls, no extra RAM, no Sub0Pub retained, no extra dependencies |
| route_dynamic (vs today_dynamic) | ok | 1567.0 (+0.0) | 47 (+0) | 28 (+0) | 46 (+0) | 0/1 | 145000 (+0) | 6233 (+0) | 424 | - | PASS |
| route_static | ok | 129.0 (+0.0) | 36 (+0) | 2 (+0) | 123 (+0) | 0/0 | 144576 (+0) | 5941 (+0) | 0 | - | PASS |
| today_dynamic | ok | 1567.0 (+1438.0) | 47 (+11) | 28 (+26) | 46 (-77) | 0/1 | 145000 (+424) | 6233 (+292) | 424 | pure virtual | FAIL: publish instr, setup instr, teardown instr, no extra indirect calls, no extra RAM, no Sub0Pub retained, no extra dependencies |
| today_static | ok | 129.0 (+0.0) | 36 (+0) | 2 (+0) | 123 (+0) | 0/0 | 144576 (+0) | 5941 (+0) | 0 | - | PASS |
| today_wire (vs handwritten_runtime) | ok | 367.0 (+0.0) | 37 (+0) | 2 (+0) | 20 (+0) | 0/0 | 144012 (+0) | 5957 (+0) | 0 | - | PASS |

<details><summary>clangcl-O2: largest symbols added by handwritten_runtime (bytes)</summary>

- 256 `unsigned int * `anonymous namespace'::block`
- 168 `std::bad_exception::`RTTI Base Class Descriptor at (0,-1,0,64)'`
- 16 `class collapse::Slot<struct `anonymous namespace'::Sensor> `anonymous namespace'::sensor`
- 8 `class collapse::Slot<struct `anonymous namespace'::Accumulator> `anonymous namespace'::accumulator`

</details>

<details><summary>clangcl-O2: largest symbols added by route_bridged (bytes)</summary>

- 880 `collapse_setup`
- 784 `collapse_publish`
- 444 `__acrt_SetEnvironmentVariableA`
- 416 `collapse_teardown`
- 256 `unsigned int * `anonymous namespace'::block`
- 112 `__rtc_tzz`
- 96 `class sub0::spike::detail::BridgedSubscribe<struct `anonymous namespace'::Sample> `RTTI Type Descriptor'`
- 88 `class sub0::detail::SubscriberInterface<struct `anonymous namespace'::Sample,0> `RTTI Type Descriptor'`

</details>

<details><summary>clangcl-O2: largest symbols added by route_dynamic (bytes)</summary>

- 256 `unsigned int * `anonymous namespace'::block`
- 88 `class sub0::detail::SubscriberInterface<struct `anonymous namespace'::Sample,0> `RTTI Type Descriptor'`
- 72 `private: static struct sub0::detail::Table<struct `anonymous namespace'::Sample,struct sub0::detail::BuiltinT<8,1,2,0,struct sub0::NoLock> > sub0::detail::BrokerImpl<struct `anonymous namespace'::Sample,struct sub0::detail::BuiltinT<8,1,2,0,struct sub0::NoLock> >::global_`
- 64 `class sub0::Subscribe<struct `anonymous namespace'::Sample> `RTTI Type Descriptor'`
- 48 `struct `anonymous namespace'::Accumulator `RTTI Type Descriptor'`
- 32 `const sub0::Subscribe<struct `anonymous namespace'::Sample>::`RTTI Complete Object Locator'`
- 32 ``anonymous namespace'::Accumulator::`RTTI Base Class Descriptor at (0,-1,0,64)'`
- 32 `sub0::detail::SubscriberInterface<struct `anonymous namespace'::Sample,0>::`RTTI Base Class Descriptor at (0,-1,0,64)'`

</details>

<details><summary>clangcl-O2: largest symbols added by route_static (bytes)</summary>

- 256 `unsigned int * `anonymous namespace'::block`
- 4 `?accumulator@?A0x78E10B86@@3UAccumulatorSlot@?A0x78E10B86@@A.1`
- 4 `?accumulator@?A0x78E10B86@@3UAccumulatorSlot@?A0x78E10B86@@A.0`

</details>

<details><summary>clangcl-O2: largest symbols added by today_dynamic (bytes)</summary>

- 444 `__acrt_SetEnvironmentVariableA`
- 416 `collapse_teardown`
- 304 `collapse_setup`
- 256 `unsigned int * `anonymous namespace'::block`
- 88 `__rtc_tzz`
- 88 `class sub0::detail::SubscriberInterface<struct `anonymous namespace'::Sample,0> `RTTI Type Descriptor'`
- 72 `private: static struct sub0::detail::Table<struct `anonymous namespace'::Sample,struct sub0::detail::BuiltinT<8,1,2,0,struct sub0::NoLock> > sub0::detail::BrokerImpl<struct `anonymous namespace'::Sample,struct sub0::detail::BuiltinT<8,1,2,0,struct sub0::NoLock> >::global_`
- 64 `class sub0::Subscribe<struct `anonymous namespace'::Sample> `RTTI Type Descriptor'`

</details>

<details><summary>clangcl-O2: largest symbols added by today_static (bytes)</summary>

- 256 `unsigned int * `anonymous namespace'::block`
- 4 `?accumulator@?A0x7646892B@@3V?$Slot@UAccumulator@?A0x7646892B@@@collapse@@A.1`
- 4 `?accumulator@?A0x7646892B@@3V?$Slot@UAccumulator@?A0x7646892B@@@collapse@@A.0`

</details>

<details><summary>clangcl-O2: largest symbols added by today_wire (bytes)</summary>

- 256 `unsigned int * `anonymous namespace'::block`
- 16 `class collapse::Slot<struct `anonymous namespace'::Sensor<class sub0::Wiring<struct `anonymous namespace'::Accumulator> > > `anonymous namespace'::sensor`
- 8 `class collapse::Slot<struct `anonymous namespace'::Accumulator> `anonymous namespace'::accumulator`

</details>

### icx-O2, observable work

| variant | checksum | publish instr | setup instr | teardown instr | path instr | calls (direct/indirect) | text | data+bss | retained sub0 (B) | added deps | verdict |
|---|---|---|---|---|---|---|---|---|---|---|---|
| handwritten | ok | 250.0 (+0.0) | 128 (+0) | 2 (+0) | 48 (+0) | 0/0 | 139794 (+0) | 5944 (+0) | 0 | - | reference |
| handwritten_runtime | ok | 248.0 (-2.0) | 129 (+1) | 2 (+0) | 46 (-2) | 0/0 | 139778 (-16) | 5944 (+0) | 0 | - | reference; FAIL: setup instr |
| route_bridged | ok | 739.0 (+489.0) | 146 (+18) | 7 (+5) | 52 (+4) | 0/1 | 140502 (+708) | 6360 (+416) | 584 | pure virtual | FAIL: publish instr, setup instr, teardown instr, publish path, no extra indirect calls, no extra RAM, no Sub0Pub retained, no extra dependencies |
| route_dynamic (vs today_dynamic) | ok | 1571.0 (+0.0) | 139 (+0) | 21 (+0) | 50 (+0) | 0/1 | 140294 (+0) | 6232 (+0) | 424 | - | PASS |
| route_static | ok | 250.0 (+0.0) | 128 (+0) | 2 (+0) | 48 (+0) | 0/0 | 139794 (+0) | 5944 (+0) | 0 | - | PASS |
| today_dynamic | ok | 1571.0 (+1321.0) | 139 (+11) | 21 (+19) | 50 (+2) | 0/1 | 140294 (+500) | 6232 (+288) | 424 | pure virtual | FAIL: publish instr, setup instr, teardown instr, no extra indirect calls, no extra RAM, no Sub0Pub retained, no extra dependencies |
| today_static | ok | 250.0 (+0.0) | 128 (+0) | 2 (+0) | 48 (+0) | 0/0 | 139794 (+0) | 5944 (+0) | 0 | - | PASS |
| today_wire (vs handwritten_runtime) | ok | 248.0 (+0.0) | 129 (+0) | 2 (+0) | 46 (+0) | 0/0 | 139778 (+0) | 5944 (+0) | 0 | - | PASS |

### icx-O2, removable work

| variant | checksum | publish instr | setup instr | teardown instr | path instr | calls (direct/indirect) | text | data+bss | retained sub0 (B) | added deps | verdict |
|---|---|---|---|---|---|---|---|---|---|---|---|
| handwritten | ok | 244.0 (+0.0) | 128 (+0) | 2 (+0) | 42 (+0) | 0/0 | 139778 (+0) | 5944 (+0) | 0 | - | reference |
| handwritten_runtime | ok | 242.0 (-2.0) | 129 (+1) | 2 (+0) | 40 (-2) | 0/0 | 139762 (-16) | 5944 (+0) | 0 | - | reference; FAIL: setup instr |
| route_bridged | ok | 733.0 (+489.0) | 146 (+18) | 7 (+5) | 46 (+4) | 0/1 | 140470 (+692) | 6360 (+416) | 584 | pure virtual | FAIL: publish instr, setup instr, teardown instr, publish path, no extra indirect calls, no extra RAM, no Sub0Pub retained, no extra dependencies |
| route_dynamic (vs today_dynamic) | ok | 1565.0 (+0.0) | 139 (+0) | 21 (+0) | 44 (+0) | 0/1 | 140262 (+0) | 6232 (+0) | 424 | - | PASS |
| route_static | ok | 244.0 (+0.0) | 128 (+0) | 2 (+0) | 42 (+0) | 0/0 | 139778 (+0) | 5944 (+0) | 0 | - | PASS |
| today_dynamic | ok | 1565.0 (+1321.0) | 139 (+11) | 21 (+19) | 44 (+2) | 0/1 | 140262 (+484) | 6232 (+288) | 424 | pure virtual | FAIL: publish instr, setup instr, teardown instr, no extra indirect calls, no extra RAM, no Sub0Pub retained, no extra dependencies |
| today_static | ok | 244.0 (+0.0) | 128 (+0) | 2 (+0) | 42 (+0) | 0/0 | 139778 (+0) | 5944 (+0) | 0 | - | PASS |
| today_wire (vs handwritten_runtime) | ok | 242.0 (+0.0) | 129 (+0) | 2 (+0) | 40 (+0) | 0/0 | 139762 (+0) | 5944 (+0) | 0 | - | PASS |

<details><summary>icx-O2: largest symbols added by handwritten_runtime (bytes)</summary>

- 1204 `__volatile_metadata`
- 256 `unsigned int * `anonymous namespace'::block`
- 8 `class collapse::Slot<struct `anonymous namespace'::Sensor> `anonymous namespace'::sensor`
- 8 `class collapse::Slot<struct `anonymous namespace'::Accumulator> `anonymous namespace'::accumulator`

</details>

<details><summary>icx-O2: largest symbols added by route_bridged (bytes)</summary>

- 256 `collapse_setup`
- 256 `unsigned int * `anonymous namespace'::block`
- 128 `collapse_teardown`
- 96 `class sub0::spike::detail::BridgedSubscribe<struct `anonymous namespace'::Sample> `RTTI Type Descriptor'`
- 88 `class sub0::detail::SubscriberInterface<struct `anonymous namespace'::Sample,0> `RTTI Type Descriptor'`
- 72 `private: static struct sub0::detail::Table<struct `anonymous namespace'::Sample,struct sub0::with<struct sub0::detail::BuiltinT<8,1,2,0,struct sub0::NoLock>,struct sub0::spike::StaticFirst<&struct `anonymous namespace'::AccumulatorSlot `anonymous namespace'::accumulator> > > sub0::detail::BrokerImpl<struct `anonymous namespace'::Sample,struct sub0::with<struct sub0::detail::BuiltinT<8,1,2,0,struct sub0::NoLock>,struct sub0::spike::StaticFirst<&struct `anonymous namespace'::AccumulatorSlot `anonymous namespace'::accumulator> > >::global_`
- 68 `__rtc_tzz`
- 64 `class sub0::Subscribe<struct `anonymous namespace'::Sample> `RTTI Type Descriptor'`

</details>

<details><summary>icx-O2: largest symbols added by route_dynamic (bytes)</summary>

- 256 `unsigned int * `anonymous namespace'::block`
- 88 `class sub0::detail::SubscriberInterface<struct `anonymous namespace'::Sample,0> `RTTI Type Descriptor'`
- 72 `private: static struct sub0::detail::Table<struct `anonymous namespace'::Sample,struct sub0::detail::BuiltinT<8,1,2,0,struct sub0::NoLock> > sub0::detail::BrokerImpl<struct `anonymous namespace'::Sample,struct sub0::detail::BuiltinT<8,1,2,0,struct sub0::NoLock> >::global_`
- 64 `class sub0::Subscribe<struct `anonymous namespace'::Sample> `RTTI Type Descriptor'`
- 48 `struct `anonymous namespace'::Accumulator `RTTI Type Descriptor'`
- 32 `struct `anonymous namespace'::AccumulatorSlot `anonymous namespace'::accumulator`
- 32 `const sub0::Subscribe<struct `anonymous namespace'::Sample>::`RTTI Complete Object Locator'`
- 32 ``anonymous namespace'::Accumulator::`RTTI Base Class Descriptor at (0,-1,0,64)'`

</details>

<details><summary>icx-O2: largest symbols added by route_static (bytes)</summary>

- 256 `unsigned int * `anonymous namespace'::block`
- 12 `?accumulator@?A0x78E10B86@@3UAccumulatorSlot@?A0x78E10B86@@A.1`
- 4 `?accumulator@?A0x78E10B86@@3UAccumulatorSlot@?A0x78E10B86@@A.0`

</details>

<details><summary>icx-O2: largest symbols added by today_dynamic (bytes)</summary>

- 256 `unsigned int * `anonymous namespace'::block`
- 192 `collapse_setup`
- 128 `collapse_teardown`
- 88 `class sub0::detail::SubscriberInterface<struct `anonymous namespace'::Sample,0> `RTTI Type Descriptor'`
- 72 `private: static struct sub0::detail::Table<struct `anonymous namespace'::Sample,struct sub0::detail::BuiltinT<8,1,2,0,struct sub0::NoLock> > sub0::detail::BrokerImpl<struct `anonymous namespace'::Sample,struct sub0::detail::BuiltinT<8,1,2,0,struct sub0::NoLock> >::global_`
- 68 `__rtc_tzz`
- 64 `class sub0::Subscribe<struct `anonymous namespace'::Sample> `RTTI Type Descriptor'`
- 48 `struct `anonymous namespace'::Accumulator `RTTI Type Descriptor'`

</details>

<details><summary>icx-O2: largest symbols added by today_static (bytes)</summary>

- 256 `unsigned int * `anonymous namespace'::block`
- 12 `?accumulator@?A0x7646892B@@3V?$Slot@UAccumulator@?A0x7646892B@@@collapse@@A.1`
- 4 `?accumulator@?A0x7646892B@@3V?$Slot@UAccumulator@?A0x7646892B@@@collapse@@A.0`

</details>

<details><summary>icx-O2: largest symbols added by today_wire (bytes)</summary>

- 256 `unsigned int * `anonymous namespace'::block`
- 8 `class collapse::Slot<struct `anonymous namespace'::Sensor<class sub0::Wiring<struct `anonymous namespace'::Accumulator> > > `anonymous namespace'::sensor`
- 8 `class collapse::Slot<struct `anonymous namespace'::Accumulator> `anonymous namespace'::accumulator`

</details>

## Case: cross_file

### msvc-O2, observable work

| variant | checksum | publish instr | setup instr | teardown instr | path instr | calls (direct/indirect) | text | data+bss | retained sub0 (B) | added deps | verdict |
|---|---|---|---|---|---|---|---|---|---|---|---|
| handwritten | ok | 39.0 (+0.0) | 5 (+0) | 2 (+0) | 27 (+0) | 3/0 | 139070 (+0) | 5704 (+0) | 0 | - | reference |
| route_dynamic (vs today_dynamic) | ok | 63.0 (+0.0) | 42 (+0) | 96 (+0) | 20 (+0) | 0/1 | 140162 (+0) | 6072 (+0) | 584 | - | PASS |
| route_static | ok | 39.0 (+0.0) | 5 (+0) | 2 (+0) | 27 (+0) | 3/0 | 139070 (+0) | 5704 (+0) | 0 | - | PASS |
| route_static_out_of_line | ok | 46.0 (+7.0) | 5 (+0) | 2 (+0) | 34 (+7) | 4/0 | 139122 (+52) | 5704 (+0) | 0 | - | FAIL: publish instr, publish path |
| today_dynamic | ok | 63.0 (+24.0) | 42 (+37) | 96 (+94) | 20 (-7) | 0/1 | 140162 (+1092) | 6072 (+368) | 584 | pure virtual | FAIL: publish instr, setup instr, teardown instr, no extra indirect calls, no extra RAM, no Sub0Pub retained, no extra dependencies |
| today_static | ok | 39.0 (+0.0) | 5 (+0) | 2 (+0) | 27 (+0) | 3/0 | 139070 (+0) | 5704 (+0) | 0 | - | PASS |

### msvc-O2, removable work

| variant | checksum | publish instr | setup instr | teardown instr | path instr | calls (direct/indirect) | text | data+bss | retained sub0 (B) | added deps | verdict |
|---|---|---|---|---|---|---|---|---|---|---|---|
| handwritten | ok | 24.0 (+0.0) | 5 (+0) | 2 (+0) | 17 (+0) | 3/0 | 139038 (+0) | 5704 (+0) | 0 | - | reference |
| route_dynamic (vs today_dynamic) | ok | 48.0 (+0.0) | 42 (+0) | 96 (+0) | 20 (+0) | 0/1 | 140130 (+0) | 6072 (+0) | 584 | - | PASS |
| route_static | ok | 24.0 (+0.0) | 5 (+0) | 2 (+0) | 17 (+0) | 3/0 | 139038 (+0) | 5704 (+0) | 0 | - | PASS |
| route_static_out_of_line | ok | 31.0 (+7.0) | 5 (+0) | 2 (+0) | 24 (+7) | 4/0 | 139090 (+52) | 5704 (+0) | 0 | - | FAIL: publish instr, publish path |
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

<details><summary>msvc-O2: largest symbols added by route_static_out_of_line (bytes)</summary>

- 1196 `__volatile_metadata`
- 64 `public: void __cdecl app::SampleRoute::receive(struct app::Sample const & __ptr64) __ptr64`
- 8 `$unwind$?receive@SampleRoute@app@@QEAAXAEBUSample@2@@Z`
- 4 `struct app::SampleRoute app::sampleRoute`
- 4 `struct app::LoggerSlot app::logger`
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
| route_static_out_of_line | ok | 19.0 (+0.0) | 5 (+0) | 2 (+0) | 13 (+0) | 0/0 | 138954 (+0) | 5712 (+8) | 0 | - | FAIL: no extra RAM |
| today_dynamic | ok | 63.0 (+44.0) | 42 (+37) | 75 (+73) | 20 (+7) | 0/1 | 140326 (+1372) | 6080 (+376) | 456 | pure virtual | FAIL: publish instr, setup instr, teardown instr, publish path, no extra indirect calls, no extra RAM, no Sub0Pub retained, no extra dependencies |
| today_static | ok | 19.0 (+0.0) | 5 (+0) | 2 (+0) | 13 (+0) | 0/0 | 138954 (+0) | 5712 (+8) | 0 | - | FAIL: no extra RAM |

### msvc-O2-lto, removable work

| variant | checksum | publish instr | setup instr | teardown instr | path instr | calls (direct/indirect) | text | data+bss | retained sub0 (B) | added deps | verdict |
|---|---|---|---|---|---|---|---|---|---|---|---|
| handwritten | ok | 9.0 (+0.0) | 5 (+0) | 2 (+0) | 3 (+0) | 0/0 | 138890 (+0) | 5704 (+0) | 0 | - | reference |
| route_dynamic (vs today_dynamic) | ok | 48.0 (+0.0) | 42 (+0) | 75 (+0) | 20 (+0) | 0/1 | 140294 (+0) | 6080 (+0) | 456 | - | PASS |
| route_static | ok | 9.0 (+0.0) | 5 (+0) | 2 (+0) | 3 (+0) | 0/0 | 138890 (+0) | 5712 (+8) | 0 | - | FAIL: no extra RAM |
| route_static_out_of_line | ok | 9.0 (+0.0) | 5 (+0) | 2 (+0) | 3 (+0) | 0/0 | 138890 (+0) | 5712 (+8) | 0 | - | FAIL: no extra RAM |
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

<details><summary>msvc-O2-lto: largest symbols added by route_static_out_of_line (bytes)</summary>

- 1196 `__volatile_metadata`
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
| route_static_out_of_line | ok | 57.0 (+4.0) | 5 (+0) | 2 (+0) | 42 (+4) | 4/0 | 143674 (+45) | 5701 (+0) | 0 | - | FAIL: publish instr, publish path |
| today_dynamic | ok | 76.0 (+23.0) | 38 (+33) | 89 (+87) | 25 (-13) | 0/1 | 145695 (+2066) | 6025 (+324) | 416 | pure virtual | FAIL: publish instr, setup instr, teardown instr, no extra indirect calls, no extra RAM, no Sub0Pub retained, no extra dependencies |
| today_static | ok | 53.0 (+0.0) | 5 (+0) | 2 (+0) | 38 (+0) | 3/0 | 143629 (+0) | 5701 (+0) | 0 | - | PASS |

### clangcl-O2, removable work

| variant | checksum | publish instr | setup instr | teardown instr | path instr | calls (direct/indirect) | text | data+bss | retained sub0 (B) | added deps | verdict |
|---|---|---|---|---|---|---|---|---|---|---|---|
| handwritten | ok | 28.0 (+0.0) | 5 (+0) | 2 (+0) | 21 (+0) | 3/0 | 143586 (+0) | 5701 (+0) | 0 | - | reference |
| route_dynamic (vs today_dynamic) | ok | 51.0 (+0.0) | 38 (+0) | 89 (+0) | 25 (+0) | 0/1 | 145651 (+0) | 6025 (+0) | 416 | - | PASS |
| route_static | ok | 28.0 (+0.0) | 5 (+0) | 2 (+0) | 21 (+0) | 3/0 | 143586 (+0) | 5701 (+0) | 0 | - | PASS |
| route_static_out_of_line | ok | 32.0 (+4.0) | 5 (+0) | 2 (+0) | 25 (+4) | 4/0 | 143642 (+56) | 5701 (+0) | 0 | - | FAIL: publish instr, publish path |
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

<details><summary>clangcl-O2: largest symbols added by route_static_out_of_line (bytes)</summary>

- 176 `std::bad_exception::`RTTI Base Class Descriptor at (0,-1,0,64)'`
- 76 `__rtc_tzz`
- 64 `public: void __cdecl app::SampleRoute::receive(struct app::Sample const & __ptr64) __ptr64`
- 4 `struct app::SampleRoute app::sampleRoute`
- 4 `struct app::LoggerSlot app::logger`
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
| route_static_out_of_line | ok | 57.0 (+4.0) | 5 (+0) | 2 (+0) | 42 (+4) | 4/0 | 139562 (+52) | 5688 (+0) | 0 | - | FAIL: publish instr, publish path |
| today_dynamic | ok | 76.0 (+23.0) | 39 (+34) | 72 (+70) | 25 (-13) | 0/1 | 140702 (+1192) | 6072 (+384) | 416 | pure virtual | FAIL: publish instr, setup instr, teardown instr, no extra indirect calls, no extra RAM, no Sub0Pub retained, no extra dependencies |
| today_static | ok | 53.0 (+0.0) | 5 (+0) | 2 (+0) | 38 (+0) | 3/0 | 139510 (+0) | 5688 (+0) | 0 | - | PASS |

### icx-O2, removable work

| variant | checksum | publish instr | setup instr | teardown instr | path instr | calls (direct/indirect) | text | data+bss | retained sub0 (B) | added deps | verdict |
|---|---|---|---|---|---|---|---|---|---|---|---|
| handwritten | ok | 28.0 (+0.0) | 5 (+0) | 2 (+0) | 21 (+0) | 3/0 | 139478 (+0) | 5688 (+0) | 0 | - | reference |
| route_dynamic (vs today_dynamic) | ok | 51.0 (+0.0) | 39 (+0) | 72 (+0) | 25 (+0) | 0/1 | 140670 (+0) | 6072 (+0) | 416 | - | PASS |
| route_static | ok | 28.0 (+0.0) | 5 (+0) | 2 (+0) | 21 (+0) | 3/0 | 139478 (+0) | 5688 (+0) | 0 | - | PASS |
| route_static_out_of_line | ok | 32.0 (+4.0) | 5 (+0) | 2 (+0) | 25 (+4) | 4/0 | 139530 (+52) | 5688 (+0) | 0 | - | FAIL: publish instr, publish path |
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

<details><summary>icx-O2: largest symbols added by route_static_out_of_line (bytes)</summary>

- 1212 `__volatile_metadata`
- 64 `__rtc_tzz`
- 64 `public: void __cdecl app::SampleRoute::receive(struct app::Sample const & __ptr64) __ptr64`
- 4 `struct app::SampleRoute app::sampleRoute`
- 4 `struct app::LoggerSlot app::logger`
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

## Case: one_receiver

### msvc-O2, observable work

| variant | checksum | publish instr | setup instr | teardown instr | path instr | calls (direct/indirect) | text | data+bss | retained sub0 (B) | added deps | verdict |
|---|---|---|---|---|---|---|---|---|---|---|---|
| handwritten | ok | 12.0 (+0.0) | 3 (+0) | 2 (+0) | 6 (+0) | 0/0 | 138922 (+0) | 5688 (+0) | 0 | - | reference |
| handwritten_erased | ok | 22.0 (+10.0) | 9 (+6) | 2 (+0) | 9 (+3) | 0/1 | 151398 (+12476) | 5720 (+32) | 0 | - | reference; FAIL: publish instr, setup instr, publish path, no extra indirect calls, no extra RAM |
| bus_dynamic (vs today_dynamic) | ok | 35.0 (+1.0) | 16 (+0) | 25 (+0) | 20 (+0) | 0/1 | 140006 (+224) | 6200 (+192) | 1256 | - | FAIL: no extra RAM, no Sub0Pub retained |
| bus_static | ok | 12.0 (+0.0) | 3 (+0) | 2 (+0) | 6 (+0) | 0/0 | 138922 (+0) | 5688 (+0) | 0 | - | PASS |
| port_dynamic (vs today_dynamic) | ok | 45.0 (+11.0) | 20 (+4) | 25 (+0) | 9 (-11) | 0/1 | 152354 (+12572) | 6216 (+208) | 1336 | - | FAIL: publish instr, setup instr, no extra RAM, no Sub0Pub retained |
| port_static (vs handwritten_erased) | ok | 22.0 (+0.0) | 9 (+0) | 2 (+0) | 9 (+0) | 0/1 | 151398 (+0) | 5720 (+0) | 32 | - | PASS |
| route_bridged | ok | 23.0 (+11.0) | 32 (+29) | 11 (+9) | 23 (+17) | 0/1 | 139998 (+1076) | 6104 (+416) | 792 | pure virtual | FAIL: publish instr, setup instr, teardown instr, publish path, no extra indirect calls, no extra RAM, no Sub0Pub retained, no extra dependencies |
| route_dynamic (vs today_dynamic) | ok | 34.0 (+0.0) | 16 (+0) | 25 (+0) | 20 (+0) | 0/1 | 139782 (+0) | 6008 (+0) | 600 | - | PASS |
| route_static | ok | 12.0 (+0.0) | 3 (+0) | 2 (+0) | 6 (+0) | 0/0 | 138922 (+0) | 5688 (+0) | 0 | - | PASS |
| route_static_virtual | ok | 12.0 (+0.0) | 5 (+2) | 2 (+0) | 6 (+0) | 0/0 | 139162 (+240) | 5848 (+160) | 168 | - | FAIL: setup instr, no extra RAM, no Sub0Pub retained |
| today_dynamic | ok | 34.0 (+22.0) | 16 (+13) | 25 (+23) | 20 (+14) | 0/1 | 139782 (+860) | 6008 (+320) | 600 | pure virtual | FAIL: publish instr, setup instr, teardown instr, publish path, no extra indirect calls, no extra RAM, no Sub0Pub retained, no extra dependencies |
| today_static | ok | 12.0 (+0.0) | 3 (+0) | 2 (+0) | 6 (+0) | 0/0 | 138922 (+0) | 5688 (+0) | 0 | - | PASS |

### msvc-O2, removable work

| variant | checksum | publish instr | setup instr | teardown instr | path instr | calls (direct/indirect) | text | data+bss | retained sub0 (B) | added deps | verdict |
|---|---|---|---|---|---|---|---|---|---|---|---|
| handwritten | ok | 8.0 (+0.0) | 3 (+0) | 2 (+0) | 2 (+0) | 0/0 | 138906 (+0) | 5688 (+0) | 0 | - | reference |
| handwritten_erased | ok | 16.0 (+8.0) | 9 (+6) | 2 (+0) | 9 (+7) | 0/1 | 151382 (+12476) | 5720 (+32) | 0 | - | reference; FAIL: publish instr, setup instr, publish path, no extra indirect calls, no extra RAM |
| bus_dynamic (vs today_dynamic) | ok | 29.0 (+0.0) | 16 (+0) | 25 (+0) | 20 (+0) | 0/1 | 139990 (+224) | 6200 (+192) | 1240 | - | FAIL: no extra RAM, no Sub0Pub retained |
| bus_static | ok | 8.0 (+0.0) | 3 (+0) | 2 (+0) | 2 (+0) | 0/0 | 138906 (+0) | 5688 (+0) | 0 | - | PASS |
| port_dynamic (vs today_dynamic) | ok | 39.0 (+10.0) | 20 (+4) | 25 (+0) | 9 (-11) | 0/1 | 152338 (+12572) | 6216 (+208) | 1320 | - | FAIL: publish instr, setup instr, no extra RAM, no Sub0Pub retained |
| port_static (vs handwritten_erased) | ok | 16.0 (+0.0) | 9 (+0) | 2 (+0) | 9 (+0) | 0/1 | 151382 (+0) | 5720 (+0) | 16 | - | PASS |
| route_bridged | ok | 20.0 (+12.0) | 32 (+29) | 11 (+9) | 20 (+18) | 0/1 | 139966 (+1060) | 6104 (+416) | 792 | pure virtual | FAIL: publish instr, setup instr, teardown instr, publish path, no extra indirect calls, no extra RAM, no Sub0Pub retained, no extra dependencies |
| route_dynamic (vs today_dynamic) | ok | 29.0 (+0.0) | 16 (+0) | 25 (+0) | 20 (+0) | 0/1 | 139766 (+0) | 6008 (+0) | 600 | - | PASS |
| route_static | ok | 8.0 (+0.0) | 3 (+0) | 2 (+0) | 2 (+0) | 0/0 | 138906 (+0) | 5688 (+0) | 0 | - | PASS |
| route_static_virtual | ok | 8.0 (+0.0) | 5 (+2) | 2 (+0) | 2 (+0) | 0/0 | 139130 (+224) | 5848 (+160) | 168 | - | FAIL: setup instr, no extra RAM, no Sub0Pub retained |
| today_dynamic | ok | 29.0 (+21.0) | 16 (+13) | 25 (+23) | 20 (+18) | 0/1 | 139766 (+860) | 6008 (+320) | 600 | pure virtual | FAIL: publish instr, setup instr, teardown instr, publish path, no extra indirect calls, no extra RAM, no Sub0Pub retained, no extra dependencies |
| today_static | ok | 8.0 (+0.0) | 3 (+0) | 2 (+0) | 2 (+0) | 0/0 | 138906 (+0) | 5688 (+0) | 0 | - | PASS |

<details><summary>msvc-O2: largest symbols added by handwritten_erased (bytes)</summary>

- 1456 `void __cdecl FindHandler<class __FrameHandler4>(struct EHExceptionRecord * __ptr64,unsigned __int64 * __ptr64,struct _CONTEXT * __ptr64,struct _xDISPATCHER_CONTEXT * __ptr64,struct FH4::FuncInfo4 * __ptr64,unsigned char,int,unsigned __int64 * __ptr64)`
- 1220 `__volatile_metadata`
- 992 `public: static void __cdecl __FrameHandler4::FrameUnwindToState(unsigned __int64 * __ptr64,struct _xDISPATCHER_CONTEXT * __ptr64,struct FH4::FuncInfo4 * __ptr64,int)`
- 880 `void __cdecl FindHandlerForForeignException<class __FrameHandler4>(struct EHExceptionRecord * __ptr64,unsigned __int64 * __ptr64,struct _CONTEXT * __ptr64,struct _xDISPATCHER_CONTEXT * __ptr64,struct FH4::FuncInfo4 * __ptr64,int,int,unsigned __int64 * __ptr64)`
- 614 `$$000000`
- 608 `public: static void * __ptr64 __cdecl __FrameHandler4::CxxCallCatchBlock(struct _EXCEPTION_RECORD * __ptr64)`
- 608 `enum _EXCEPTION_DISPOSITION __cdecl __InternalCxxFrameHandler<class __FrameHandler4>(struct EHExceptionRecord * __ptr64,unsigned __int64 * __ptr64,struct _CONTEXT * __ptr64,struct _xDISPATCHER_CONTEXT * __ptr64,struct FH4::FuncInfo4 * __ptr64,int,unsigned __int64 * __ptr64,unsigned char)`
- 496 `public: void __cdecl FH4::TryBlockMap4::setBuffer(class FH4::TryBlockMap4::iterator) __ptr64`

</details>

<details><summary>msvc-O2: largest symbols added by bus_dynamic (bytes)</summary>

- 144 `class sub0::spike::detail::Forwarding<class sub0::spike::Subscription<struct `anonymous namespace'::Controller,struct A0x0297b4e5::Sample>,struct `anonymous namespace'::Sample> `RTTI Type Descriptor'`
- 128 `protected: __cdecl sub0::Subscribe<struct `anonymous namespace'::Sample>::~Subscribe<struct `anonymous namespace'::Sample>(void) __ptr64`
- 96 `class sub0::spike::Subscription<struct `anonymous namespace'::Controller,struct A0x0297b4e5::Sample> `RTTI Type Descriptor'`
- 88 `class sub0::detail::SubscriberInterface<struct `anonymous namespace'::Sample,0> `RTTI Type Descriptor'`
- 72 `private: static struct sub0::detail::Table<struct `anonymous namespace'::Sample,struct sub0::detail::BuiltinT<8,1,2,0,struct sub0::NoLock> > sub0::detail::BrokerImpl<struct `anonymous namespace'::Sample,struct sub0::detail::BuiltinT<8,1,2,0,struct sub0::NoLock> >::global_`
- 64 `class sub0::Subscribe<struct `anonymous namespace'::Sample> `RTTI Type Descriptor'`
- 40 `const sub0::spike::Subscription<struct `anonymous namespace'::Controller,struct A0x0297b4e5::Sample>::`RTTI Complete Object Locator'`
- 40 `const sub0::Subscribe<struct `anonymous namespace'::Sample>::`RTTI Complete Object Locator'`

</details>

<details><summary>msvc-O2: largest symbols added by bus_static (bytes)</summary>

- 8 `class collapse::Slot<struct `anonymous namespace'::Controller> `anonymous namespace'::controller`

</details>

<details><summary>msvc-O2: largest symbols added by port_dynamic (bytes)</summary>

- 1456 `void __cdecl FindHandler<class __FrameHandler4>(struct EHExceptionRecord * __ptr64,unsigned __int64 * __ptr64,struct _CONTEXT * __ptr64,struct _xDISPATCHER_CONTEXT * __ptr64,struct FH4::FuncInfo4 * __ptr64,unsigned char,int,unsigned __int64 * __ptr64)`
- 1220 `__volatile_metadata`
- 992 `public: static void __cdecl __FrameHandler4::FrameUnwindToState(unsigned __int64 * __ptr64,struct _xDISPATCHER_CONTEXT * __ptr64,struct FH4::FuncInfo4 * __ptr64,int)`
- 880 `void __cdecl FindHandlerForForeignException<class __FrameHandler4>(struct EHExceptionRecord * __ptr64,unsigned __int64 * __ptr64,struct _CONTEXT * __ptr64,struct _xDISPATCHER_CONTEXT * __ptr64,struct FH4::FuncInfo4 * __ptr64,int,int,unsigned __int64 * __ptr64)`
- 614 `$$000000`
- 608 `public: static void * __ptr64 __cdecl __FrameHandler4::CxxCallCatchBlock(struct _EXCEPTION_RECORD * __ptr64)`
- 608 `enum _EXCEPTION_DISPOSITION __cdecl __InternalCxxFrameHandler<class __FrameHandler4>(struct EHExceptionRecord * __ptr64,unsigned __int64 * __ptr64,struct _CONTEXT * __ptr64,struct _xDISPATCHER_CONTEXT * __ptr64,struct FH4::FuncInfo4 * __ptr64,int,unsigned __int64 * __ptr64,unsigned char)`
- 496 `public: void __cdecl FH4::TryBlockMap4::setBuffer(class FH4::TryBlockMap4::iterator) __ptr64`

</details>

<details><summary>msvc-O2: largest symbols added by port_static (bytes)</summary>

- 32 `public: static __cdecl `public: __cdecl sub0::Sink<struct `anonymous namespace'::Sample>::Sink<struct `anonymous namespace'::Sample><class sub0::Wiring<struct `anonymous namespace'::Controller> >(class sub0::Wiring<struct `anonymous namespace'::Controller> & __ptr64) __ptr64'::`1'::<lambda_1_>::<lambda_invoker_cdecl>(void const * __ptr64,struct `anonymous namespace'::Sample const & __ptr64)`
- 16 `class collapse::Slot<struct `anonymous namespace'::Sensor> `anonymous namespace'::sensor`
- 8 `class collapse::Slot<struct `anonymous namespace'::Controller> `anonymous namespace'::controller`
- 8 `class collapse::Slot<class sub0::Wiring<struct `anonymous namespace'::Controller> > `anonymous namespace'::bus`

</details>

<details><summary>msvc-O2: largest symbols added by route_bridged (bytes)</summary>

- 176 `collapse_setup`
- 136 `SetSmallXmm`
- 128 `public: void __cdecl sub0::Subscribe<struct `anonymous namespace'::Sample>::disconnect(void) __ptr64`
- 96 `collapse_publish`
- 88 `class sub0::detail::SubscriberInterface<struct `anonymous namespace'::Sample,0> `RTTI Type Descriptor'`
- 88 `class sub0::spike::detail::BridgedSubscribe<struct `anonymous namespace'::Sample> `RTTI Type Descriptor'`
- 72 `private: static struct sub0::detail::Table<struct `anonymous namespace'::Sample,struct sub0::with<struct sub0::detail::BuiltinT<8,1,2,0,struct sub0::NoLock>,struct sub0::spike::StaticFirst<&struct A0xc457c397::ControllerSlot A0xc457c397::controller> > > sub0::detail::BrokerImpl<struct `anonymous namespace'::Sample,struct sub0::with<struct sub0::detail::BuiltinT<8,1,2,0,struct sub0::NoLock>,struct sub0::spike::StaticFirst<&struct A0xc457c397::ControllerSlot A0xc457c397::controller> > >::global_`
- 72 `class sub0::Subscribe<struct `anonymous namespace'::Sample> `RTTI Type Descriptor'`

</details>

<details><summary>msvc-O2: largest symbols added by route_dynamic (bytes)</summary>

- 128 `protected: __cdecl sub0::Subscribe<struct `anonymous namespace'::Sample>::~Subscribe<struct `anonymous namespace'::Sample>(void) __ptr64`
- 88 `class sub0::detail::SubscriberInterface<struct `anonymous namespace'::Sample,0> `RTTI Type Descriptor'`
- 72 `private: static struct sub0::detail::Table<struct `anonymous namespace'::Sample,struct sub0::detail::BuiltinT<8,1,2,0,struct sub0::NoLock> > sub0::detail::BrokerImpl<struct `anonymous namespace'::Sample,struct sub0::detail::BuiltinT<8,1,2,0,struct sub0::NoLock> >::global_`
- 64 `class sub0::Subscribe<struct `anonymous namespace'::Sample> `RTTI Type Descriptor'`
- 48 `struct `anonymous namespace'::Controller `RTTI Type Descriptor'`
- 40 `const `anonymous namespace'::Controller::`RTTI Complete Object Locator'`
- 40 `const sub0::Subscribe<struct `anonymous namespace'::Sample>::`RTTI Complete Object Locator'`
- 40 ``anonymous namespace'::Controller::`RTTI Base Class Descriptor at (0,-1,0,64)'`

</details>

<details><summary>msvc-O2: largest symbols added by route_static (bytes)</summary>

- 8 `struct A0x20b2e3db::ControllerSlot `anonymous namespace'::controller`

</details>

<details><summary>msvc-O2: largest symbols added by route_static_virtual (bytes)</summary>

- 1196 `__volatile_metadata`
- 88 `class sub0::spike::detail::StaticSubscribe<struct `anonymous namespace'::Sample> `RTTI Type Descriptor'`
- 56 ``string'`
- 48 `struct `anonymous namespace'::Controller `RTTI Type Descriptor'`
- 40 `const `anonymous namespace'::Controller::`RTTI Complete Object Locator'`
- 40 `type_info::`RTTI Base Class Descriptor at (0,-1,0,64)'`
- 40 ``anonymous namespace'::Controller::`RTTI Base Class Descriptor at (0,-1,0,64)'`
- 40 `sub0::spike::detail::StaticSubscribe<struct `anonymous namespace'::Sample>::`RTTI Base Class Descriptor at (0,-1,0,64)'`

</details>

<details><summary>msvc-O2: largest symbols added by today_dynamic (bytes)</summary>

- 136 `SetSmallXmm`
- 128 `protected: __cdecl sub0::Subscribe<struct `anonymous namespace'::Sample>::~Subscribe<struct `anonymous namespace'::Sample>(void) __ptr64`
- 96 `collapse_setup`
- 88 `class sub0::detail::SubscriberInterface<struct `anonymous namespace'::Sample,0> `RTTI Type Descriptor'`
- 80 `collapse_publish`
- 72 `private: static struct sub0::detail::Table<struct `anonymous namespace'::Sample,struct sub0::detail::BuiltinT<8,1,2,0,struct sub0::NoLock> > sub0::detail::BrokerImpl<struct `anonymous namespace'::Sample,struct sub0::detail::BuiltinT<8,1,2,0,struct sub0::NoLock> >::global_`
- 64 `class sub0::Subscribe<struct `anonymous namespace'::Sample> `RTTI Type Descriptor'`
- 56 ``string'`

</details>

<details><summary>msvc-O2: largest symbols added by today_static (bytes)</summary>

- 8 `class collapse::Slot<struct `anonymous namespace'::Controller> `anonymous namespace'::controller`

</details>

### clangcl-O2, observable work

| variant | checksum | publish instr | setup instr | teardown instr | path instr | calls (direct/indirect) | text | data+bss | retained sub0 (B) | added deps | verdict |
|---|---|---|---|---|---|---|---|---|---|---|---|
| handwritten | ok | 17.0 (+0.0) | 3 (+0) | 2 (+0) | 11 (+0) | 0/0 | 143484 (+0) | 5685 (+0) | 0 | - | reference |
| handwritten_erased | ok | 25.0 (+8.0) | 7 (+4) | 2 (+0) | 19 (+8) | 1/0 | 143568 (+84) | 5701 (+16) | 0 | - | reference; FAIL: publish instr, setup instr, publish path, no extra RAM |
| bus_dynamic (vs today_dynamic) | ok | 41.0 (+1.0) | 17 (+2) | 28 (+0) | 25 (+0) | 0/1 | 144672 (+192) | 6169 (+192) | 1024 | - | FAIL: setup instr, no extra RAM, no Sub0Pub retained |
| bus_static | ok | 17.0 (+0.0) | 3 (+0) | 2 (+0) | 11 (+0) | 0/0 | 143484 (+0) | 5685 (+0) | 0 | - | PASS |
| port_dynamic (vs today_dynamic) | ok | 48.0 (+8.0) | 19 (+4) | 28 (+0) | 32 (+7) | 1/1 | 144742 (+262) | 6185 (+208) | 1104 | - | FAIL: publish instr, setup instr, publish path, no extra RAM, no Sub0Pub retained |
| port_static (vs handwritten_erased) | ok | 25.0 (+0.0) | 7 (+0) | 2 (+0) | 19 (+0) | 1/0 | 143581 (+13) | 5701 (+0) | 32 | - | FAIL: no Sub0Pub retained |
| route_bridged | ok | 26.0 (+9.0) | 28 (+25) | 12 (+10) | 32 (+21) | 0/1 | 145188 (+1704) | 6041 (+356) | 584 | pure virtual | FAIL: publish instr, setup instr, teardown instr, publish path, no extra indirect calls, no extra RAM, no Sub0Pub retained, no extra dependencies |
| route_dynamic (vs today_dynamic) | ok | 40.0 (+0.0) | 15 (+0) | 28 (+0) | 25 (+0) | 0/1 | 144480 (+0) | 5977 (+0) | 424 | - | PASS |
| route_static | ok | 17.0 (+0.0) | 3 (+0) | 2 (+0) | 11 (+0) | 0/0 | 143484 (+0) | 5685 (+0) | 0 | - | PASS |
| route_static_virtual | ok | 17.0 (+0.0) | 3 (+0) | 2 (+0) | 11 (+0) | 0/0 | 143484 (+0) | 5685 (+0) | 0 | - | PASS |
| today_dynamic | ok | 40.0 (+23.0) | 15 (+12) | 28 (+26) | 25 (+14) | 0/1 | 144480 (+996) | 5977 (+292) | 424 | pure virtual | FAIL: publish instr, setup instr, teardown instr, publish path, no extra indirect calls, no extra RAM, no Sub0Pub retained, no extra dependencies |
| today_static | ok | 17.0 (+0.0) | 3 (+0) | 2 (+0) | 11 (+0) | 0/0 | 143484 (+0) | 5685 (+0) | 0 | - | PASS |

### clangcl-O2, removable work

| variant | checksum | publish instr | setup instr | teardown instr | path instr | calls (direct/indirect) | text | data+bss | retained sub0 (B) | added deps | verdict |
|---|---|---|---|---|---|---|---|---|---|---|---|
| handwritten | ok | 8.0 (+0.0) | 2 (+0) | 2 (+0) | 2 (+0) | 0/0 | 143452 (+0) | 5685 (+0) | 0 | - | reference |
| handwritten_erased | ok | 8.0 (+0.0) | 7 (+5) | 2 (+0) | 2 (+0) | 0/0 | 143484 (+32) | 5701 (+16) | 0 | - | reference; FAIL: setup instr, no extra RAM |
| bus_dynamic (vs today_dynamic) | ok | 32.0 (+0.0) | 17 (+2) | 28 (+0) | 25 (+0) | 0/1 | 144656 (+192) | 6169 (+192) | 1008 | - | FAIL: setup instr, no extra RAM, no Sub0Pub retained |
| bus_static | ok | 8.0 (+0.0) | 2 (+0) | 2 (+0) | 2 (+0) | 0/0 | 143452 (+0) | 5685 (+0) | 0 | - | PASS |
| port_dynamic (vs today_dynamic) | ok | 39.0 (+7.0) | 19 (+4) | 28 (+0) | 32 (+7) | 1/1 | 144726 (+262) | 6185 (+208) | 1088 | - | FAIL: publish instr, setup instr, publish path, no extra RAM, no Sub0Pub retained |
| port_static (vs handwritten_erased) | ok | 8.0 (+0.0) | 7 (+0) | 2 (+0) | 2 (+0) | 0/0 | 143484 (+0) | 5701 (+0) | 0 | - | PASS |
| route_bridged | ok | 19.0 (+11.0) | 28 (+26) | 12 (+10) | 25 (+23) | 0/1 | 145140 (+1688) | 6041 (+356) | 584 | pure virtual | FAIL: publish instr, setup instr, teardown instr, publish path, no extra indirect calls, no extra RAM, no Sub0Pub retained, no extra dependencies |
| route_dynamic (vs today_dynamic) | ok | 32.0 (+0.0) | 15 (+0) | 28 (+0) | 25 (+0) | 0/1 | 144464 (+0) | 5977 (+0) | 424 | - | PASS |
| route_static | ok | 8.0 (+0.0) | 2 (+0) | 2 (+0) | 2 (+0) | 0/0 | 143452 (+0) | 5685 (+0) | 0 | - | PASS |
| route_static_virtual | ok | 8.0 (+0.0) | 2 (+0) | 2 (+0) | 2 (+0) | 0/0 | 143452 (+0) | 5685 (+0) | 0 | - | PASS |
| today_dynamic | ok | 32.0 (+24.0) | 15 (+13) | 28 (+26) | 25 (+23) | 0/1 | 144464 (+1012) | 5977 (+292) | 424 | pure virtual | FAIL: publish instr, setup instr, teardown instr, publish path, no extra indirect calls, no extra RAM, no Sub0Pub retained, no extra dependencies |
| today_static | ok | 8.0 (+0.0) | 2 (+0) | 2 (+0) | 2 (+0) | 0/0 | 143452 (+0) | 5685 (+0) | 0 | - | PASS |

<details><summary>clangcl-O2: largest symbols added by handwritten_erased (bytes)</summary>

- 176 `std::bad_exception::`RTTI Base Class Descriptor at (0,-1,0,64)'`
- 68 `__rtc_tzz`
- 48 `collapse_setup`
- 32 `void __cdecl `anonymous namespace'::deliverNode(void const * __ptr64,struct `anonymous namespace'::Sample const & __ptr64)`
- 8 `?sensor@?A0x94129378@@3V?$Slot@USensor@?A0x94129378@@@collapse@@A.0`
- 8 `class collapse::Slot<struct `anonymous namespace'::Node> `anonymous namespace'::node`
- 8 `class collapse::Slot<struct `anonymous namespace'::Controller> `anonymous namespace'::controller`

</details>

<details><summary>clangcl-O2: largest symbols added by bus_dynamic (bytes)</summary>

- 160 `class sub0::spike::detail::Forwarding<class sub0::spike::Subscription<struct `anonymous namespace'::Controller,struct `anonymous namespace'::Sample>,struct `anonymous namespace'::Sample> `RTTI Type Descriptor'`
- 112 `class sub0::spike::Subscription<struct `anonymous namespace'::Controller,struct `anonymous namespace'::Sample> `RTTI Type Descriptor'`
- 96 `collapse_setup`
- 88 `class sub0::detail::SubscriberInterface<struct `anonymous namespace'::Sample,0> `RTTI Type Descriptor'`
- 72 `private: static struct sub0::detail::Table<struct `anonymous namespace'::Sample,struct sub0::detail::BuiltinT<8,1,2,0,struct sub0::NoLock> > sub0::detail::BrokerImpl<struct `anonymous namespace'::Sample,struct sub0::detail::BuiltinT<8,1,2,0,struct sub0::NoLock> >::global_`
- 64 `class sub0::Subscribe<struct `anonymous namespace'::Sample> `RTTI Type Descriptor'`
- 32 `private: virtual void __cdecl sub0::spike::detail::Forwarding<class sub0::spike::Subscription<struct `anonymous namespace'::Controller,struct `anonymous namespace'::Sample>,struct `anonymous namespace'::Sample>::receive(struct `anonymous namespace'::Sample const & __ptr64) __ptr64`
- 32 `const sub0::Subscribe<struct `anonymous namespace'::Sample>::`RTTI Complete Object Locator'`

</details>

<details><summary>clangcl-O2: largest symbols added by bus_static (bytes)</summary>

- 8 `?controller@?A0x93F297C7@@3V?$Slot@UController@?A0x93F297C7@@@collapse@@A.0`

</details>

<details><summary>clangcl-O2: largest symbols added by port_dynamic (bytes)</summary>

- 160 `class sub0::spike::detail::Forwarding<class sub0::spike::Subscription<struct `anonymous namespace'::Controller,struct `anonymous namespace'::Sample>,struct `anonymous namespace'::Sample> `RTTI Type Descriptor'`
- 112 `collapse_setup`
- 112 `class sub0::spike::Subscription<struct `anonymous namespace'::Controller,struct `anonymous namespace'::Sample> `RTTI Type Descriptor'`
- 88 `__rtc_tzz`
- 88 `class sub0::detail::SubscriberInterface<struct `anonymous namespace'::Sample,0> `RTTI Type Descriptor'`
- 80 `private: static <auto> __cdecl `public: __cdecl sub0::Sink<struct `anonymous namespace'::Sample>::Sink<struct `anonymous namespace'::Sample><struct sub0::spike::BrokerBus>(struct sub0::spike::BrokerBus & __ptr64) __ptr64'::`1'::<lambda_1>::__invoke(void const * __ptr64,struct `anonymous namespace'::Sample const & __ptr64)`
- 72 `private: static struct sub0::detail::Table<struct `anonymous namespace'::Sample,struct sub0::detail::BuiltinT<8,1,2,0,struct sub0::NoLock> > sub0::detail::BrokerImpl<struct `anonymous namespace'::Sample,struct sub0::detail::BuiltinT<8,1,2,0,struct sub0::NoLock> >::global_`
- 64 `class sub0::Subscribe<struct `anonymous namespace'::Sample> `RTTI Type Descriptor'`

</details>

<details><summary>clangcl-O2: largest symbols added by port_static (bytes)</summary>

- 32 `private: static <auto> __cdecl `public: __cdecl sub0::Sink<struct `anonymous namespace'::Sample>::Sink<struct `anonymous namespace'::Sample><class sub0::Wiring<struct `anonymous namespace'::Controller> >(class sub0::Wiring<struct `anonymous namespace'::Controller> & __ptr64) __ptr64'::`1'::<lambda_1>::__invoke(void const * __ptr64,struct `anonymous namespace'::Sample const & __ptr64)`
- 8 `?sensor@?A0xBF1ABE84@@3V?$Slot@USensor@?A0xBF1ABE84@@@collapse@@A.0`
- 8 `class collapse::Slot<struct `anonymous namespace'::Controller> `anonymous namespace'::controller`
- 8 `class collapse::Slot<class sub0::Wiring<struct `anonymous namespace'::Controller> > `anonymous namespace'::bus`

</details>

<details><summary>clangcl-O2: largest symbols added by route_bridged (bytes)</summary>

- 656 `collapse_setup`
- 444 `__acrt_SetEnvironmentVariableA`
- 416 `collapse_teardown`
- 128 `collapse_publish`
- 96 `class sub0::spike::detail::BridgedSubscribe<struct `anonymous namespace'::Sample> `RTTI Type Descriptor'`
- 92 `__rtc_tzz`
- 88 `class sub0::detail::SubscriberInterface<struct `anonymous namespace'::Sample,0> `RTTI Type Descriptor'`
- 72 `private: static struct sub0::detail::Table<struct `anonymous namespace'::Sample,struct sub0::with<struct sub0::detail::BuiltinT<8,1,2,0,struct sub0::NoLock>,struct sub0::spike::StaticFirst<&struct `anonymous namespace'::ControllerSlot `anonymous namespace'::controller> > > sub0::detail::BrokerImpl<struct `anonymous namespace'::Sample,struct sub0::with<struct sub0::detail::BuiltinT<8,1,2,0,struct sub0::NoLock>,struct sub0::spike::StaticFirst<&struct `anonymous namespace'::ControllerSlot `anonymous namespace'::controller> > >::global_`

</details>

<details><summary>clangcl-O2: largest symbols added by route_dynamic (bytes)</summary>

- 88 `class sub0::detail::SubscriberInterface<struct `anonymous namespace'::Sample,0> `RTTI Type Descriptor'`
- 72 `private: static struct sub0::detail::Table<struct `anonymous namespace'::Sample,struct sub0::detail::BuiltinT<8,1,2,0,struct sub0::NoLock> > sub0::detail::BrokerImpl<struct `anonymous namespace'::Sample,struct sub0::detail::BuiltinT<8,1,2,0,struct sub0::NoLock> >::global_`
- 64 `class sub0::Subscribe<struct `anonymous namespace'::Sample> `RTTI Type Descriptor'`
- 48 `struct `anonymous namespace'::Controller `RTTI Type Descriptor'`
- 32 `public: virtual void __cdecl `anonymous namespace'::Controller::receive(struct `anonymous namespace'::Sample const & __ptr64) __ptr64`
- 32 `const sub0::Subscribe<struct `anonymous namespace'::Sample>::`RTTI Complete Object Locator'`
- 32 ``anonymous namespace'::Controller::`RTTI Base Class Descriptor at (0,-1,0,64)'`
- 32 `sub0::detail::SubscriberInterface<struct `anonymous namespace'::Sample,0>::`RTTI Base Class Descriptor at (0,-1,0,64)'`

</details>

<details><summary>clangcl-O2: largest symbols added by route_static (bytes)</summary>

- 8 `?controller@?A0x74AB06AE@@3UControllerSlot@?A0x74AB06AE@@A.0`

</details>

<details><summary>clangcl-O2: largest symbols added by route_static_virtual (bytes)</summary>

- 176 `std::bad_exception::`RTTI Base Class Descriptor at (0,-1,0,64)'`
- 8 `?controller@?A0xAECE3977@@3UControllerSlot@?A0xAECE3977@@A.1`

</details>

<details><summary>clangcl-O2: largest symbols added by today_dynamic (bytes)</summary>

- 444 `__acrt_SetEnvironmentVariableA`
- 416 `collapse_teardown`
- 96 `collapse_publish`
- 88 `class sub0::detail::SubscriberInterface<struct `anonymous namespace'::Sample,0> `RTTI Type Descriptor'`
- 80 `collapse_setup`
- 80 `__rtc_tzz`
- 72 `private: static struct sub0::detail::Table<struct `anonymous namespace'::Sample,struct sub0::detail::BuiltinT<8,1,2,0,struct sub0::NoLock> > sub0::detail::BrokerImpl<struct `anonymous namespace'::Sample,struct sub0::detail::BuiltinT<8,1,2,0,struct sub0::NoLock> >::global_`
- 64 `class sub0::Subscribe<struct `anonymous namespace'::Sample> `RTTI Type Descriptor'`

</details>

<details><summary>clangcl-O2: largest symbols added by today_static (bytes)</summary>

- 8 `?controller@?A0xE4D9E086@@3V?$Slot@UController@?A0xE4D9E086@@@collapse@@A.0`

</details>

### icx-O2, observable work

| variant | checksum | publish instr | setup instr | teardown instr | path instr | calls (direct/indirect) | text | data+bss | retained sub0 (B) | added deps | verdict |
|---|---|---|---|---|---|---|---|---|---|---|---|
| handwritten | ok | 17.0 (+0.0) | 3 (+0) | 2 (+0) | 11 (+0) | 0/0 | 139378 (+0) | 5688 (+0) | 0 | - | reference |
| handwritten_erased | ok | 26.0 (+9.0) | 9 (+6) | 2 (+0) | 10 (-1) | 0/1 | 139478 (+100) | 5704 (+16) | 0 | - | reference; FAIL: publish instr, setup instr, no extra indirect calls, no extra RAM |
| bus_dynamic (vs today_dynamic) | ok | 41.0 (+1.0) | 17 (+2) | 21 (+0) | 25 (+0) | 0/1 | 140254 (+144) | 6232 (+272) | 1024 | - | FAIL: setup instr, no extra RAM, no Sub0Pub retained |
| bus_static | ok | 17.0 (+0.0) | 3 (+0) | 2 (+0) | 11 (+0) | 0/0 | 139378 (+0) | 5688 (+0) | 0 | - | PASS |
| port_dynamic (vs today_dynamic) | ok | 48.0 (+8.0) | 21 (+6) | 21 (+0) | 9 (-16) | 0/1 | 140338 (+228) | 6248 (+288) | 1104 | - | FAIL: publish instr, setup instr, no extra RAM, no Sub0Pub retained |
| port_static (vs handwritten_erased) | ok | 25.0 (-1.0) | 9 (+0) | 2 (+0) | 9 (-1) | 0/1 | 139478 (+0) | 5704 (+0) | 32 | - | PASS |
| route_bridged | ok | 26.0 (+9.0) | 22 (+19) | 7 (+5) | 32 (+21) | 0/1 | 140350 (+972) | 6088 (+400) | 584 | pure virtual | FAIL: publish instr, setup instr, teardown instr, publish path, no extra indirect calls, no extra RAM, no Sub0Pub retained, no extra dependencies |
| route_dynamic (vs today_dynamic) | ok | 40.0 (+0.0) | 15 (+0) | 21 (+0) | 25 (+0) | 0/1 | 140110 (+0) | 5960 (+0) | 424 | - | PASS |
| route_static | ok | 17.0 (+0.0) | 3 (+0) | 2 (+0) | 11 (+0) | 0/0 | 139378 (+0) | 5688 (+0) | 0 | - | PASS |
| route_static_virtual | ok | 17.0 (+0.0) | 3 (+0) | 2 (+0) | 11 (+0) | 0/0 | 139378 (+0) | 5688 (+0) | 0 | - | PASS |
| today_dynamic | ok | 40.0 (+23.0) | 15 (+12) | 21 (+19) | 25 (+14) | 0/1 | 140110 (+732) | 5960 (+272) | 424 | pure virtual | FAIL: publish instr, setup instr, teardown instr, publish path, no extra indirect calls, no extra RAM, no Sub0Pub retained, no extra dependencies |
| today_static | ok | 17.0 (+0.0) | 3 (+0) | 2 (+0) | 11 (+0) | 0/0 | 139378 (+0) | 5688 (+0) | 0 | - | PASS |

### icx-O2, removable work

| variant | checksum | publish instr | setup instr | teardown instr | path instr | calls (direct/indirect) | text | data+bss | retained sub0 (B) | added deps | verdict |
|---|---|---|---|---|---|---|---|---|---|---|---|
| handwritten | ok | 8.0 (+0.0) | 2 (+0) | 2 (+0) | 2 (+0) | 0/0 | 139346 (+0) | 5672 (+0) | 0 | - | reference |
| handwritten_erased | ok | 17.0 (+9.0) | 9 (+7) | 2 (+0) | 10 (+8) | 0/1 | 139462 (+116) | 5704 (+32) | 0 | - | reference; FAIL: publish instr, setup instr, publish path, no extra indirect calls, no extra RAM |
| bus_dynamic (vs today_dynamic) | ok | 32.0 (+0.0) | 17 (+2) | 21 (+0) | 25 (+0) | 0/1 | 140238 (+144) | 6232 (+272) | 1008 | - | FAIL: setup instr, no extra RAM, no Sub0Pub retained |
| bus_static | ok | 8.0 (+0.0) | 2 (+0) | 2 (+0) | 2 (+0) | 0/0 | 139346 (+0) | 5672 (+0) | 0 | - | PASS |
| port_dynamic (vs today_dynamic) | ok | 39.0 (+7.0) | 21 (+6) | 21 (+0) | 9 (-16) | 0/1 | 140322 (+228) | 6248 (+288) | 1088 | - | FAIL: publish instr, setup instr, no extra RAM, no Sub0Pub retained |
| port_static (vs handwritten_erased) | ok | 16.0 (-1.0) | 9 (+0) | 2 (+0) | 9 (-1) | 0/1 | 139462 (+0) | 5704 (+0) | 16 | - | PASS |
| route_bridged | ok | 19.0 (+11.0) | 22 (+20) | 7 (+5) | 25 (+23) | 0/1 | 140302 (+956) | 6088 (+416) | 584 | pure virtual | FAIL: publish instr, setup instr, teardown instr, publish path, no extra indirect calls, no extra RAM, no Sub0Pub retained, no extra dependencies |
| route_dynamic (vs today_dynamic) | ok | 32.0 (+0.0) | 15 (+0) | 21 (+0) | 25 (+0) | 0/1 | 140094 (+0) | 5960 (+0) | 424 | - | PASS |
| route_static | ok | 8.0 (+0.0) | 2 (+0) | 2 (+0) | 2 (+0) | 0/0 | 139346 (+0) | 5672 (+0) | 0 | - | PASS |
| route_static_virtual | ok | 8.0 (+0.0) | 2 (+0) | 2 (+0) | 2 (+0) | 0/0 | 139346 (+0) | 5672 (+0) | 0 | - | PASS |
| today_dynamic | ok | 32.0 (+24.0) | 15 (+13) | 21 (+19) | 25 (+23) | 0/1 | 140094 (+748) | 5960 (+288) | 424 | pure virtual | FAIL: publish instr, setup instr, teardown instr, publish path, no extra indirect calls, no extra RAM, no Sub0Pub retained, no extra dependencies |
| today_static | ok | 8.0 (+0.0) | 2 (+0) | 2 (+0) | 2 (+0) | 0/0 | 139346 (+0) | 5672 (+0) | 0 | - | PASS |

<details><summary>icx-O2: largest symbols added by handwritten_erased (bytes)</summary>

- 64 `collapse_setup`
- 56 `__rtc_tzz`
- 32 `void __cdecl `anonymous namespace'::deliverNode(void const * __ptr64,struct `anonymous namespace'::Sample const & __ptr64)`
- 16 `class collapse::Slot<struct `anonymous namespace'::Sensor> `anonymous namespace'::sensor`
- 8 `class collapse::Slot<struct `anonymous namespace'::Node> `anonymous namespace'::node`
- 8 `class collapse::Slot<struct `anonymous namespace'::Controller> `anonymous namespace'::controller`

</details>

<details><summary>icx-O2: largest symbols added by bus_dynamic (bytes)</summary>

- 160 `class sub0::spike::detail::Forwarding<class sub0::spike::Subscription<struct `anonymous namespace'::Controller,struct `anonymous namespace'::Sample>,struct `anonymous namespace'::Sample> `RTTI Type Descriptor'`
- 112 `class sub0::spike::Subscription<struct `anonymous namespace'::Controller,struct `anonymous namespace'::Sample> `RTTI Type Descriptor'`
- 96 `collapse_setup`
- 88 `class sub0::detail::SubscriberInterface<struct `anonymous namespace'::Sample,0> `RTTI Type Descriptor'`
- 72 `private: static struct sub0::detail::Table<struct `anonymous namespace'::Sample,struct sub0::detail::BuiltinT<8,1,2,0,struct sub0::NoLock> > sub0::detail::BrokerImpl<struct `anonymous namespace'::Sample,struct sub0::detail::BuiltinT<8,1,2,0,struct sub0::NoLock> >::global_`
- 64 `class sub0::Subscribe<struct `anonymous namespace'::Sample> `RTTI Type Descriptor'`
- 48 `__isa_inverted`
- 32 `private: virtual void __cdecl sub0::spike::detail::Forwarding<class sub0::spike::Subscription<struct `anonymous namespace'::Controller,struct `anonymous namespace'::Sample>,struct `anonymous namespace'::Sample>::receive(struct `anonymous namespace'::Sample const & __ptr64) __ptr64`

</details>

<details><summary>icx-O2: largest symbols added by bus_static (bytes)</summary>

- 8 `?controller@?A0x93F297C7@@3V?$Slot@UController@?A0x93F297C7@@@collapse@@A.0`

</details>

<details><summary>icx-O2: largest symbols added by port_dynamic (bytes)</summary>

- 160 `class sub0::spike::detail::Forwarding<class sub0::spike::Subscription<struct `anonymous namespace'::Controller,struct `anonymous namespace'::Sample>,struct `anonymous namespace'::Sample> `RTTI Type Descriptor'`
- 128 `collapse_setup`
- 112 `class sub0::spike::Subscription<struct `anonymous namespace'::Controller,struct `anonymous namespace'::Sample> `RTTI Type Descriptor'`
- 88 `class sub0::detail::SubscriberInterface<struct `anonymous namespace'::Sample,0> `RTTI Type Descriptor'`
- 80 `private: static <auto> __cdecl `public: __cdecl sub0::Sink<struct `anonymous namespace'::Sample>::Sink<struct `anonymous namespace'::Sample><struct sub0::spike::BrokerBus>(struct sub0::spike::BrokerBus & __ptr64) __ptr64'::`1'::<lambda_1>::__invoke(void const * __ptr64,struct `anonymous namespace'::Sample const & __ptr64)`
- 72 `private: static struct sub0::detail::Table<struct `anonymous namespace'::Sample,struct sub0::detail::BuiltinT<8,1,2,0,struct sub0::NoLock> > sub0::detail::BrokerImpl<struct `anonymous namespace'::Sample,struct sub0::detail::BuiltinT<8,1,2,0,struct sub0::NoLock> >::global_`
- 68 `__rtc_tzz`
- 64 `class sub0::Subscribe<struct `anonymous namespace'::Sample> `RTTI Type Descriptor'`

</details>

<details><summary>icx-O2: largest symbols added by port_static (bytes)</summary>

- 32 `private: static <auto> __cdecl `public: __cdecl sub0::Sink<struct `anonymous namespace'::Sample>::Sink<struct `anonymous namespace'::Sample><class sub0::Wiring<struct `anonymous namespace'::Controller> >(class sub0::Wiring<struct `anonymous namespace'::Controller> & __ptr64) __ptr64'::`1'::<lambda_1>::__invoke(void const * __ptr64,struct `anonymous namespace'::Sample const & __ptr64)`
- 16 `class collapse::Slot<struct `anonymous namespace'::Sensor> `anonymous namespace'::sensor`
- 8 `class collapse::Slot<struct `anonymous namespace'::Controller> `anonymous namespace'::controller`
- 8 `class collapse::Slot<class sub0::Wiring<struct `anonymous namespace'::Controller> > `anonymous namespace'::bus`

</details>

<details><summary>icx-O2: largest symbols added by route_bridged (bytes)</summary>

- 160 `collapse_setup`
- 152 `SetSmallXmm`
- 128 `collapse_teardown`
- 128 `collapse_publish`
- 96 `class sub0::spike::detail::BridgedSubscribe<struct `anonymous namespace'::Sample> `RTTI Type Descriptor'`
- 88 `class sub0::detail::SubscriberInterface<struct `anonymous namespace'::Sample,0> `RTTI Type Descriptor'`
- 72 `private: static struct sub0::detail::Table<struct `anonymous namespace'::Sample,struct sub0::with<struct sub0::detail::BuiltinT<8,1,2,0,struct sub0::NoLock>,struct sub0::spike::StaticFirst<&struct `anonymous namespace'::ControllerSlot `anonymous namespace'::controller> > > sub0::detail::BrokerImpl<struct `anonymous namespace'::Sample,struct sub0::with<struct sub0::detail::BuiltinT<8,1,2,0,struct sub0::NoLock>,struct sub0::spike::StaticFirst<&struct `anonymous namespace'::ControllerSlot `anonymous namespace'::controller> > >::global_`
- 64 `class sub0::Subscribe<struct `anonymous namespace'::Sample> `RTTI Type Descriptor'`

</details>

<details><summary>icx-O2: largest symbols added by route_dynamic (bytes)</summary>

- 88 `class sub0::detail::SubscriberInterface<struct `anonymous namespace'::Sample,0> `RTTI Type Descriptor'`
- 72 `private: static struct sub0::detail::Table<struct `anonymous namespace'::Sample,struct sub0::detail::BuiltinT<8,1,2,0,struct sub0::NoLock> > sub0::detail::BrokerImpl<struct `anonymous namespace'::Sample,struct sub0::detail::BuiltinT<8,1,2,0,struct sub0::NoLock> >::global_`
- 64 `class sub0::Subscribe<struct `anonymous namespace'::Sample> `RTTI Type Descriptor'`
- 48 `struct `anonymous namespace'::Controller `RTTI Type Descriptor'`
- 32 `public: virtual void __cdecl `anonymous namespace'::Controller::receive(struct `anonymous namespace'::Sample const & __ptr64) __ptr64`
- 32 `const sub0::Subscribe<struct `anonymous namespace'::Sample>::`RTTI Complete Object Locator'`
- 32 ``anonymous namespace'::Controller::`RTTI Base Class Descriptor at (0,-1,0,64)'`
- 32 `sub0::detail::SubscriberInterface<struct `anonymous namespace'::Sample,0>::`RTTI Base Class Descriptor at (0,-1,0,64)'`

</details>

<details><summary>icx-O2: largest symbols added by route_static (bytes)</summary>

- 8 `?controller@?A0x74AB06AE@@3UControllerSlot@?A0x74AB06AE@@A.0`

</details>

<details><summary>icx-O2: largest symbols added by route_static_virtual (bytes)</summary>

- 1212 `__volatile_metadata`
- 8 `?controller@?A0xAECE3977@@3UControllerSlot@?A0xAECE3977@@A.1`

</details>

<details><summary>icx-O2: largest symbols added by today_dynamic (bytes)</summary>

- 128 `collapse_teardown`
- 104 `SetSmallXmm`
- 96 `collapse_publish`
- 88 `class sub0::detail::SubscriberInterface<struct `anonymous namespace'::Sample,0> `RTTI Type Descriptor'`
- 80 `collapse_setup`
- 72 `private: static struct sub0::detail::Table<struct `anonymous namespace'::Sample,struct sub0::detail::BuiltinT<8,1,2,0,struct sub0::NoLock> > sub0::detail::BrokerImpl<struct `anonymous namespace'::Sample,struct sub0::detail::BuiltinT<8,1,2,0,struct sub0::NoLock> >::global_`
- 64 `class sub0::Subscribe<struct `anonymous namespace'::Sample> `RTTI Type Descriptor'`
- 60 `__rtc_tzz`

</details>

<details><summary>icx-O2: largest symbols added by today_static (bytes)</summary>

- 8 `?controller@?A0xE4D9E086@@3V?$Slot@UController@?A0xE4D9E086@@@collapse@@A.0`

</details>

## Case: station

### msvc-O2, observable work

| variant | checksum | publish instr | setup instr | teardown instr | path instr | calls (direct/indirect) | text | data+bss | retained sub0 (B) | added deps | verdict |
|---|---|---|---|---|---|---|---|---|---|---|---|
| handwritten | ok | 21.0 (+0.0) | 3 (+0) | 2 (+0) | 15 (+0) | 0/0 | 138954 (+0) | 5688 (+0) | 0 | - | reference |
| handwritten_erased | ok | 46.0 (+25.0) | 16 (+13) | 2 (+0) | 17 (+2) | 0/2 | 151558 (+12604) | 5752 (+64) | 0 | - | reference; FAIL: publish instr, setup instr, no extra indirect calls, no extra RAM |
| bus_dynamic (vs today_dynamic) | ok | 84.0 (+1.0) | 56 (+0) | 116 (+0) | 34 (+0) | 0/2 | 141714 (+752) | 7160 (+768) | 3624 | - | FAIL: no extra RAM, no Sub0Pub retained |
| bus_static | ok | 21.0 (+0.0) | 3 (+0) | 2 (+0) | 15 (+0) | 0/0 | 138954 (+0) | 5688 (+0) | 0 | - | PASS |
| port_dynamic (vs today_dynamic) | ok | 110.0 (+27.0) | 63 (+7) | 116 (+0) | 18 (-16) | 0/2 | 154314 (+13352) | 7192 (+800) | 3784 | - | FAIL: publish instr, setup instr, no extra RAM, no Sub0Pub retained |
| port_static (vs handwritten_erased) | ok | 47.0 (+1.0) | 16 (+0) | 2 (+0) | 18 (+1) | 0/2 | 151558 (+0) | 5752 (+0) | 112 | - | PASS |
| route_bridged | ok | 42.0 (+21.0) | 182 (+179) | 47 (+45) | 48 (+33) | 0/2 | 141846 (+2892) | 6584 (+896) | 2376 | pure virtual | FAIL: publish instr, setup instr, teardown instr, publish path, no extra indirect calls, no extra RAM, no Sub0Pub retained, no extra dependencies |
| route_dynamic (vs today_dynamic) | ok | 83.0 (+0.0) | 56 (+0) | 116 (+0) | 34 (+0) | 0/2 | 140962 (+0) | 6392 (+0) | 1424 | - | PASS |
| route_static | ok | 21.0 (+0.0) | 6 (+3) | 2 (+0) | 15 (+0) | 0/0 | 138970 (+16) | 5688 (+0) | 0 | - | FAIL: setup instr |
| route_static_empty_bases | ok | 21.0 (+0.0) | 3 (+0) | 2 (+0) | 15 (+0) | 0/0 | 138954 (+0) | 5688 (+0) | 0 | - | PASS |
| route_static_virtual | ok | 21.0 (+0.0) | 14 (+11) | 2 (+0) | 15 (+0) | 0/0 | 139890 (+936) | 6072 (+384) | 496 | - | FAIL: setup instr, no extra RAM, no Sub0Pub retained |
| today_dynamic | ok | 83.0 (+62.0) | 56 (+53) | 116 (+114) | 34 (+19) | 0/2 | 140962 (+2008) | 6392 (+704) | 1424 | pure virtual | FAIL: publish instr, setup instr, teardown instr, publish path, no extra indirect calls, no extra RAM, no Sub0Pub retained, no extra dependencies |
| today_static | ok | 21.0 (+0.0) | 3 (+0) | 2 (+0) | 15 (+0) | 0/0 | 138954 (+0) | 5688 (+0) | 0 | - | PASS |

### msvc-O2, removable work

| variant | checksum | publish instr | setup instr | teardown instr | path instr | calls (direct/indirect) | text | data+bss | retained sub0 (B) | added deps | verdict |
|---|---|---|---|---|---|---|---|---|---|---|---|
| handwritten | ok | 9.0 (+0.0) | 3 (+0) | 2 (+0) | 3 (+0) | 0/0 | 138906 (+0) | 5688 (+0) | 0 | - | reference |
| handwritten_erased | ok | 27.0 (+18.0) | 16 (+13) | 2 (+0) | 17 (+14) | 0/2 | 151478 (+12572) | 5752 (+64) | 0 | - | reference; FAIL: publish instr, setup instr, publish path, no extra indirect calls, no extra RAM |
| bus_dynamic (vs today_dynamic) | ok | 66.0 (+1.0) | 56 (+0) | 116 (+0) | 34 (+0) | 0/2 | 141650 (+752) | 7160 (+768) | 3560 | - | FAIL: no extra RAM, no Sub0Pub retained |
| bus_static | ok | 9.0 (+0.0) | 3 (+0) | 2 (+0) | 3 (+0) | 0/0 | 138906 (+0) | 5688 (+0) | 0 | - | PASS |
| port_dynamic (vs today_dynamic) | ok | 92.0 (+27.0) | 63 (+7) | 116 (+0) | 18 (-16) | 0/2 | 154250 (+13352) | 7192 (+800) | 3720 | - | FAIL: publish instr, setup instr, no extra RAM, no Sub0Pub retained |
| port_static (vs handwritten_erased) | ok | 28.0 (+1.0) | 16 (+0) | 2 (+0) | 18 (+1) | 0/2 | 151478 (+0) | 5752 (+0) | 32 | - | PASS |
| route_bridged | ok | 29.0 (+20.0) | 182 (+179) | 47 (+45) | 35 (+32) | 0/2 | 141734 (+2828) | 6584 (+896) | 2376 | pure virtual | FAIL: publish instr, setup instr, teardown instr, publish path, no extra indirect calls, no extra RAM, no Sub0Pub retained, no extra dependencies |
| route_dynamic (vs today_dynamic) | ok | 65.0 (+0.0) | 56 (+0) | 116 (+0) | 34 (+0) | 0/2 | 140898 (+0) | 6392 (+0) | 1424 | - | PASS |
| route_static | ok | 9.0 (+0.0) | 6 (+3) | 2 (+0) | 3 (+0) | 0/0 | 138922 (+16) | 5688 (+0) | 0 | - | FAIL: setup instr |
| route_static_empty_bases | ok | 9.0 (+0.0) | 3 (+0) | 2 (+0) | 3 (+0) | 0/0 | 138906 (+0) | 5688 (+0) | 0 | - | PASS |
| route_static_virtual | ok | 9.0 (+0.0) | 14 (+11) | 2 (+0) | 3 (+0) | 0/0 | 139778 (+872) | 6072 (+384) | 496 | - | FAIL: setup instr, no extra RAM, no Sub0Pub retained |
| today_dynamic | ok | 65.0 (+56.0) | 56 (+53) | 116 (+114) | 34 (+31) | 0/2 | 140898 (+1992) | 6392 (+704) | 1424 | pure virtual | FAIL: publish instr, setup instr, teardown instr, publish path, no extra indirect calls, no extra RAM, no Sub0Pub retained, no extra dependencies |
| today_static | ok | 9.0 (+0.0) | 3 (+0) | 2 (+0) | 3 (+0) | 0/0 | 138906 (+0) | 5688 (+0) | 0 | - | PASS |

<details><summary>msvc-O2: largest symbols added by handwritten_erased (bytes)</summary>

- 1456 `void __cdecl FindHandler<class __FrameHandler4>(struct EHExceptionRecord * __ptr64,unsigned __int64 * __ptr64,struct _CONTEXT * __ptr64,struct _xDISPATCHER_CONTEXT * __ptr64,struct FH4::FuncInfo4 * __ptr64,unsigned char,int,unsigned __int64 * __ptr64)`
- 1220 `__volatile_metadata`
- 992 `public: static void __cdecl __FrameHandler4::FrameUnwindToState(unsigned __int64 * __ptr64,struct _xDISPATCHER_CONTEXT * __ptr64,struct FH4::FuncInfo4 * __ptr64,int)`
- 880 `void __cdecl FindHandlerForForeignException<class __FrameHandler4>(struct EHExceptionRecord * __ptr64,unsigned __int64 * __ptr64,struct _CONTEXT * __ptr64,struct _xDISPATCHER_CONTEXT * __ptr64,struct FH4::FuncInfo4 * __ptr64,int,int,unsigned __int64 * __ptr64)`
- 614 `$$000000`
- 608 `public: static void * __ptr64 __cdecl __FrameHandler4::CxxCallCatchBlock(struct _EXCEPTION_RECORD * __ptr64)`
- 608 `enum _EXCEPTION_DISPOSITION __cdecl __InternalCxxFrameHandler<class __FrameHandler4>(struct EHExceptionRecord * __ptr64,unsigned __int64 * __ptr64,struct _CONTEXT * __ptr64,struct _xDISPATCHER_CONTEXT * __ptr64,struct FH4::FuncInfo4 * __ptr64,int,unsigned __int64 * __ptr64,unsigned char)`
- 496 `public: void __cdecl FH4::TryBlockMap4::setBuffer(class FH4::TryBlockMap4::iterator) __ptr64`

</details>

<details><summary>msvc-O2: largest symbols added by bus_dynamic (bytes)</summary>

- 320 `collapse_setup`
- 160 `class sub0::spike::detail::Forwarding<class sub0::spike::Subscription<struct `anonymous namespace'::Controller,struct A0x16433ac2::Sample,struct A0x16433ac2::Command>,struct `anonymous namespace'::Command> `RTTI Type Descriptor'`
- 152 `class sub0::spike::detail::Forwarding<class sub0::spike::Subscription<struct `anonymous namespace'::Controller,struct A0x16433ac2::Sample,struct A0x16433ac2::Command>,struct `anonymous namespace'::Sample> `RTTI Type Descriptor'`
- 144 `class sub0::spike::detail::Forwarding<class sub0::spike::Subscription<struct `anonymous namespace'::Logger,struct A0x16433ac2::Sample>,struct `anonymous namespace'::Sample> `RTTI Type Descriptor'`
- 144 `class sub0::spike::detail::Forwarding<class sub0::spike::Subscription<struct `anonymous namespace'::Actuator,struct A0x16433ac2::Command>,struct `anonymous namespace'::Command> `RTTI Type Descriptor'`
- 128 `protected: __cdecl sub0::Subscribe<struct `anonymous namespace'::Sample>::~Subscribe<struct `anonymous namespace'::Sample>(void) __ptr64`
- 128 `protected: __cdecl sub0::Subscribe<struct `anonymous namespace'::Command>::~Subscribe<struct `anonymous namespace'::Command>(void) __ptr64`
- 96 `class sub0::spike::Subscription<struct `anonymous namespace'::Logger,struct A0x16433ac2::Sample> `RTTI Type Descriptor'`

</details>

<details><summary>msvc-O2: largest symbols added by bus_static (bytes)</summary>

- 8 `class collapse::Slot<struct `anonymous namespace'::Logger> `anonymous namespace'::logger`

</details>

<details><summary>msvc-O2: largest symbols added by port_dynamic (bytes)</summary>

- 1456 `void __cdecl FindHandler<class __FrameHandler4>(struct EHExceptionRecord * __ptr64,unsigned __int64 * __ptr64,struct _CONTEXT * __ptr64,struct _xDISPATCHER_CONTEXT * __ptr64,struct FH4::FuncInfo4 * __ptr64,unsigned char,int,unsigned __int64 * __ptr64)`
- 1220 `__volatile_metadata`
- 992 `public: static void __cdecl __FrameHandler4::FrameUnwindToState(unsigned __int64 * __ptr64,struct _xDISPATCHER_CONTEXT * __ptr64,struct FH4::FuncInfo4 * __ptr64,int)`
- 880 `void __cdecl FindHandlerForForeignException<class __FrameHandler4>(struct EHExceptionRecord * __ptr64,unsigned __int64 * __ptr64,struct _CONTEXT * __ptr64,struct _xDISPATCHER_CONTEXT * __ptr64,struct FH4::FuncInfo4 * __ptr64,int,int,unsigned __int64 * __ptr64)`
- 614 `$$000000`
- 608 `public: static void * __ptr64 __cdecl __FrameHandler4::CxxCallCatchBlock(struct _EXCEPTION_RECORD * __ptr64)`
- 608 `enum _EXCEPTION_DISPOSITION __cdecl __InternalCxxFrameHandler<class __FrameHandler4>(struct EHExceptionRecord * __ptr64,unsigned __int64 * __ptr64,struct _CONTEXT * __ptr64,struct _xDISPATCHER_CONTEXT * __ptr64,struct FH4::FuncInfo4 * __ptr64,int,unsigned __int64 * __ptr64,unsigned char)`
- 496 `public: void __cdecl FH4::TryBlockMap4::setBuffer(class FH4::TryBlockMap4::iterator) __ptr64`

</details>

<details><summary>msvc-O2: largest symbols added by port_static (bytes)</summary>

- 64 `public: static __cdecl `public: __cdecl sub0::Sink<struct `anonymous namespace'::Sample>::Sink<struct `anonymous namespace'::Sample><class sub0::Wiring<struct `anonymous namespace'::Controller,struct A0x497d27e9::Logger,struct A0x497d27e9::Actuator> >(class sub0::Wiring<struct `anonymous namespace'::Controller,struct A0x497d27e9::Logger,struct A0x497d27e9::Actuator> & __ptr64) __ptr64'::`1'::<lambda_1_>::<lambda_invoker_cdecl>(void const * __ptr64,struct `anonymous namespace'::Sample const & __ptr64)`
- 48 `public: static __cdecl `public: __cdecl sub0::Sink<struct `anonymous namespace'::Command>::Sink<struct `anonymous namespace'::Command><class sub0::Wiring<struct `anonymous namespace'::Controller,struct A0x497d27e9::Logger,struct A0x497d27e9::Actuator> >(class sub0::Wiring<struct `anonymous namespace'::Controller,struct A0x497d27e9::Logger,struct A0x497d27e9::Actuator> & __ptr64) __ptr64'::`1'::<lambda_2_>::<lambda_invoker_cdecl>(void const * __ptr64,struct `anonymous namespace'::Command const & __ptr64)`
- 32 `class collapse::Slot<struct `anonymous namespace'::Sensor> `anonymous namespace'::sensor`
- 24 `class collapse::Slot<class sub0::Wiring<struct `anonymous namespace'::Controller,struct A0x497d27e9::Logger,struct A0x497d27e9::Actuator> > `anonymous namespace'::bus`
- 4 `class collapse::Slot<struct `anonymous namespace'::Logger> `anonymous namespace'::logger`
- 3 `class collapse::Slot<struct `anonymous namespace'::Actuator> `anonymous namespace'::actuator`
- 1 `class collapse::Slot<struct `anonymous namespace'::Controller> `anonymous namespace'::controller`

</details>

<details><summary>msvc-O2: largest symbols added by route_bridged (bytes)</summary>

- 1188 `__volatile_metadata`
- 208 `public: __cdecl sub0::spike::detail::BridgedSubscribe<struct `anonymous namespace'::Command>::BridgedSubscribe<struct `anonymous namespace'::Command>(void) __ptr64`
- 192 `collapse_publish`
- 192 `public: __cdecl sub0::spike::detail::BridgedSubscribe<struct `anonymous namespace'::Sample>::BridgedSubscribe<struct `anonymous namespace'::Sample>(void) __ptr64`
- 176 `collapse_setup`
- 128 `public: void __cdecl sub0::Subscribe<struct `anonymous namespace'::Sample>::disconnect(void) __ptr64`
- 128 `public: void __cdecl sub0::Subscribe<struct `anonymous namespace'::Command>::disconnect(void) __ptr64`
- 120 `SetSmallXmm`

</details>

<details><summary>msvc-O2: largest symbols added by route_dynamic (bytes)</summary>

- 128 `protected: __cdecl sub0::Subscribe<struct `anonymous namespace'::Sample>::~Subscribe<struct `anonymous namespace'::Sample>(void) __ptr64`
- 128 `protected: __cdecl sub0::Subscribe<struct `anonymous namespace'::Command>::~Subscribe<struct `anonymous namespace'::Command>(void) __ptr64`
- 88 `class sub0::detail::SubscriberInterface<struct `anonymous namespace'::Sample,0> `RTTI Type Descriptor'`
- 88 `class sub0::detail::SubscriberInterface<struct `anonymous namespace'::Command,0> `RTTI Type Descriptor'`
- 80 `private: static struct sub0::detail::Table<struct `anonymous namespace'::Sample,struct sub0::detail::BuiltinT<8,1,2,0,struct sub0::NoLock> > sub0::detail::BrokerImpl<struct `anonymous namespace'::Sample,struct sub0::detail::BuiltinT<8,1,2,0,struct sub0::NoLock> >::global_`
- 72 `private: static struct sub0::detail::Table<struct `anonymous namespace'::Command,struct sub0::detail::BuiltinT<8,1,2,0,struct sub0::NoLock> > sub0::detail::BrokerImpl<struct `anonymous namespace'::Command,struct sub0::detail::BuiltinT<8,1,2,0,struct sub0::NoLock> >::global_`
- 72 `class sub0::Subscribe<struct `anonymous namespace'::Command> `RTTI Type Descriptor'`
- 64 `class sub0::Subscribe<struct `anonymous namespace'::Sample> `RTTI Type Descriptor'`

</details>

<details><summary>msvc-O2: largest symbols added by route_static (bytes)</summary>

- 32 `collapse_setup`
- 4 `struct A0x85ac823e::LoggerSlot `anonymous namespace'::logger`
- 3 `class collapse::Slot<struct `anonymous namespace'::Sensor> `anonymous namespace'::sensor`
- 1 `struct A0x85ac823e::ControllerSlot `anonymous namespace'::controller`

</details>

<details><summary>msvc-O2: largest symbols added by route_static_empty_bases (bytes)</summary>

- 1196 `__volatile_metadata`
- 8 `struct A0xa975d0c3::LoggerSlot `anonymous namespace'::logger`

</details>

<details><summary>msvc-O2: largest symbols added by route_static_virtual (bytes)</summary>

- 1188 `__volatile_metadata`
- 120 `SetSmallXmm`
- 96 `class sub0::spike::detail::StaticSubscribe<struct `anonymous namespace'::Sample> `RTTI Type Descriptor'`
- 88 `class sub0::spike::detail::StaticSubscribe<struct `anonymous namespace'::Command> `RTTI Type Descriptor'`
- 80 `collapse_setup`
- 56 ``string'`
- 48 `struct `anonymous namespace'::Logger `RTTI Type Descriptor'`
- 48 `struct `anonymous namespace'::Controller `RTTI Type Descriptor'`

</details>

<details><summary>msvc-O2: largest symbols added by today_dynamic (bytes)</summary>

- 1188 `__volatile_metadata`
- 304 `collapse_setup`
- 128 `collapse_publish`
- 128 `protected: __cdecl sub0::Subscribe<struct `anonymous namespace'::Sample>::~Subscribe<struct `anonymous namespace'::Sample>(void) __ptr64`
- 128 `protected: __cdecl sub0::Subscribe<struct `anonymous namespace'::Command>::~Subscribe<struct `anonymous namespace'::Command>(void) __ptr64`
- 88 `class sub0::detail::SubscriberInterface<struct `anonymous namespace'::Sample,0> `RTTI Type Descriptor'`
- 88 `class sub0::detail::SubscriberInterface<struct `anonymous namespace'::Command,0> `RTTI Type Descriptor'`
- 80 `private: static struct sub0::detail::Table<struct `anonymous namespace'::Sample,struct sub0::detail::BuiltinT<8,1,2,0,struct sub0::NoLock> > sub0::detail::BrokerImpl<struct `anonymous namespace'::Sample,struct sub0::detail::BuiltinT<8,1,2,0,struct sub0::NoLock> >::global_`

</details>

<details><summary>msvc-O2: largest symbols added by today_static (bytes)</summary>

- 8 `class collapse::Slot<struct `anonymous namespace'::Logger> `anonymous namespace'::logger`

</details>

### clangcl-O2, observable work

| variant | checksum | publish instr | setup instr | teardown instr | path instr | calls (direct/indirect) | text | data+bss | retained sub0 (B) | added deps | verdict |
|---|---|---|---|---|---|---|---|---|---|---|---|
| handwritten | ok | 33.0 (+0.0) | 3 (+0) | 2 (+0) | 27 (+0) | 0/0 | 143532 (+0) | 5685 (+0) | 0 | - | reference |
| handwritten_erased | ok | 58.0 (+25.0) | 12 (+9) | 2 (+0) | 52 (+25) | 2/0 | 143728 (+196) | 5733 (+48) | 0 | - | reference; FAIL: publish instr, setup instr, publish path, no extra RAM |
| bus_dynamic (vs today_dynamic) | ok | 104.0 (+1.0) | 59 (+4) | 107 (+0) | 44 (+0) | 0/2 | 147312 (+576) | 7209 (+864) | 3064 | - | FAIL: setup instr, no extra RAM, no Sub0Pub retained |
| bus_static | ok | 33.0 (+0.0) | 3 (+0) | 2 (+0) | 27 (+0) | 0/0 | 143532 (+0) | 5685 (+0) | 0 | - | PASS |
| port_dynamic (vs today_dynamic) | ok | 123.0 (+20.0) | 62 (+7) | 107 (+0) | 63 (+19) | 2/2 | 147458 (+722) | 7225 (+880) | 3224 | - | FAIL: publish instr, setup instr, publish path, no extra RAM, no Sub0Pub retained |
| port_static (vs handwritten_erased) | ok | 58.0 (+0.0) | 12 (+0) | 2 (+0) | 52 (+0) | 2/0 | 143743 (+15) | 5733 (+0) | 128 | - | FAIL: no Sub0Pub retained |
| route_bridged | ok | 51.0 (+18.0) | 92 (+89) | 32 (+30) | 70 (+43) | 0/2 | 149220 (+5688) | 6537 (+852) | 1384 | pure virtual | FAIL: publish instr, setup instr, teardown instr, publish path, no extra indirect calls, no extra RAM, no Sub0Pub retained, no extra dependencies |
| route_dynamic (vs today_dynamic) | ok | 103.0 (+0.0) | 55 (+0) | 107 (+0) | 44 (+0) | 0/2 | 146736 (+0) | 6345 (+0) | 1032 | - | PASS |
| route_static | ok | 33.0 (+0.0) | 3 (+0) | 2 (+0) | 27 (+0) | 0/0 | 143532 (+0) | 5685 (+0) | 0 | - | PASS |
| route_static_empty_bases | ok | 33.0 (+0.0) | 3 (+0) | 2 (+0) | 27 (+0) | 0/0 | 143532 (+0) | 5685 (+0) | 0 | - | PASS |
| route_static_virtual | ok | 33.0 (+0.0) | 5 (+2) | 2 (+0) | 27 (+0) | 0/0 | 143771 (+239) | 5829 (+144) | 152 | - | FAIL: setup instr, no extra RAM, no Sub0Pub retained |
| today_dynamic | ok | 103.0 (+70.0) | 55 (+52) | 107 (+105) | 44 (+17) | 0/2 | 146736 (+3204) | 6345 (+660) | 1032 | pure virtual | FAIL: publish instr, setup instr, teardown instr, publish path, no extra indirect calls, no extra RAM, no Sub0Pub retained, no extra dependencies |
| today_static | ok | 33.0 (+0.0) | 3 (+0) | 2 (+0) | 27 (+0) | 0/0 | 143532 (+0) | 5685 (+0) | 0 | - | PASS |

### clangcl-O2, removable work

| variant | checksum | publish instr | setup instr | teardown instr | path instr | calls (direct/indirect) | text | data+bss | retained sub0 (B) | added deps | verdict |
|---|---|---|---|---|---|---|---|---|---|---|---|
| handwritten | ok | 9.0 (+0.0) | 3 (+0) | 2 (+0) | 3 (+0) | 0/0 | 143452 (+0) | 5685 (+0) | 0 | - | reference |
| handwritten_erased | ok | 18.0 (+9.0) | 12 (+9) | 2 (+0) | 12 (+9) | 1/0 | 143584 (+132) | 5733 (+48) | 0 | - | reference; FAIL: publish instr, setup instr, publish path, no extra RAM |
| bus_dynamic (vs today_dynamic) | ok | 72.0 (+1.0) | 59 (+4) | 107 (+0) | 44 (+0) | 0/2 | 147224 (+560) | 7209 (+864) | 2984 | - | FAIL: setup instr, no extra RAM, no Sub0Pub retained |
| bus_static | ok | 9.0 (+0.0) | 3 (+0) | 2 (+0) | 3 (+0) | 0/0 | 143452 (+0) | 5685 (+0) | 0 | - | PASS |
| port_dynamic (vs today_dynamic) | ok | 91.0 (+20.0) | 62 (+7) | 107 (+0) | 63 (+19) | 2/2 | 147378 (+714) | 7225 (+880) | 3144 | - | FAIL: publish instr, setup instr, publish path, no extra RAM, no Sub0Pub retained |
| port_static (vs handwritten_erased) | ok | 18.0 (+0.0) | 12 (+0) | 2 (+0) | 12 (+0) | 1/0 | 143590 (+6) | 5733 (+0) | 16 | - | FAIL: no Sub0Pub retained |
| route_bridged | ok | 27.0 (+18.0) | 92 (+89) | 32 (+30) | 45 (+42) | 0/2 | 149092 (+5640) | 6537 (+852) | 1384 | pure virtual | FAIL: publish instr, setup instr, teardown instr, publish path, no extra indirect calls, no extra RAM, no Sub0Pub retained, no extra dependencies |
| route_dynamic (vs today_dynamic) | ok | 71.0 (+0.0) | 55 (+0) | 107 (+0) | 44 (+0) | 0/2 | 146664 (+0) | 6345 (+0) | 1032 | - | PASS |
| route_static | ok | 9.0 (+0.0) | 3 (+0) | 2 (+0) | 3 (+0) | 0/0 | 143452 (+0) | 5685 (+0) | 0 | - | PASS |
| route_static_empty_bases | ok | 9.0 (+0.0) | 3 (+0) | 2 (+0) | 3 (+0) | 0/0 | 143452 (+0) | 5685 (+0) | 0 | - | PASS |
| route_static_virtual | ok | 9.0 (+0.0) | 5 (+2) | 2 (+0) | 3 (+0) | 0/0 | 143663 (+211) | 5829 (+144) | 152 | - | FAIL: setup instr, no extra RAM, no Sub0Pub retained |
| today_dynamic | ok | 71.0 (+62.0) | 55 (+52) | 107 (+105) | 44 (+41) | 0/2 | 146664 (+3212) | 6345 (+660) | 1032 | pure virtual | FAIL: publish instr, setup instr, teardown instr, publish path, no extra indirect calls, no extra RAM, no Sub0Pub retained, no extra dependencies |
| today_static | ok | 9.0 (+0.0) | 3 (+0) | 2 (+0) | 3 (+0) | 0/0 | 143452 (+0) | 5685 (+0) | 0 | - | PASS |

<details><summary>clangcl-O2: largest symbols added by handwritten_erased (bytes)</summary>

- 168 `std::bad_exception::`RTTI Base Class Descriptor at (0,-1,0,64)'`
- 80 `collapse_setup`
- 80 `void __cdecl `anonymous namespace'::deliverSample(void const * __ptr64,struct `anonymous namespace'::Sample const & __ptr64)`
- 68 `__rtc_tzz`
- 48 `void __cdecl `anonymous namespace'::deliverCommand(void const * __ptr64,struct `anonymous namespace'::Command const & __ptr64)`
- 24 `class collapse::Slot<struct `anonymous namespace'::Node> `anonymous namespace'::node`
- 8 `?sensor@?A0xBCBA4CC4@@3V?$Slot@USensor@?A0xBCBA4CC4@@@collapse@@A.2`
- 8 `?sensor@?A0xBCBA4CC4@@3V?$Slot@USensor@?A0xBCBA4CC4@@@collapse@@A.0`

</details>

<details><summary>clangcl-O2: largest symbols added by bus_dynamic (bytes)</summary>

- 320 `collapse_setup`
- 192 `class sub0::spike::detail::Forwarding<class sub0::spike::Subscription<struct `anonymous namespace'::Controller,struct `anonymous namespace'::Sample,struct `anonymous namespace'::Command>,struct `anonymous namespace'::Command> `RTTI Type Descriptor'`
- 176 `class sub0::spike::detail::Forwarding<class sub0::spike::Subscription<struct `anonymous namespace'::Controller,struct `anonymous namespace'::Sample,struct `anonymous namespace'::Command>,struct `anonymous namespace'::Sample> `RTTI Type Descriptor'`
- 160 `class sub0::spike::detail::Forwarding<class sub0::spike::Subscription<struct `anonymous namespace'::Logger,struct `anonymous namespace'::Sample>,struct `anonymous namespace'::Sample> `RTTI Type Descriptor'`
- 160 `class sub0::spike::detail::Forwarding<class sub0::spike::Subscription<struct `anonymous namespace'::Actuator,struct `anonymous namespace'::Command>,struct `anonymous namespace'::Command> `RTTI Type Descriptor'`
- 128 `class sub0::spike::Subscription<struct `anonymous namespace'::Controller,struct `anonymous namespace'::Sample,struct `anonymous namespace'::Command> `RTTI Type Descriptor'`
- 96 `class sub0::spike::Subscription<struct `anonymous namespace'::Logger,struct `anonymous namespace'::Sample> `RTTI Type Descriptor'`
- 96 `class sub0::spike::Subscription<struct `anonymous namespace'::Actuator,struct `anonymous namespace'::Command> `RTTI Type Descriptor'`

</details>

<details><summary>clangcl-O2: largest symbols added by bus_static (bytes)</summary>

- 8 `?logger@?A0xE056B48A@@3V?$Slot@ULogger@?A0xE056B48A@@@collapse@@A.0`

</details>

<details><summary>clangcl-O2: largest symbols added by port_dynamic (bytes)</summary>

- 352 `collapse_setup`
- 192 `class sub0::spike::detail::Forwarding<class sub0::spike::Subscription<struct `anonymous namespace'::Controller,struct `anonymous namespace'::Sample,struct `anonymous namespace'::Command>,struct `anonymous namespace'::Command> `RTTI Type Descriptor'`
- 176 `class sub0::spike::detail::Forwarding<class sub0::spike::Subscription<struct `anonymous namespace'::Controller,struct `anonymous namespace'::Sample,struct `anonymous namespace'::Command>,struct `anonymous namespace'::Sample> `RTTI Type Descriptor'`
- 160 `class sub0::spike::detail::Forwarding<class sub0::spike::Subscription<struct `anonymous namespace'::Logger,struct `anonymous namespace'::Sample>,struct `anonymous namespace'::Sample> `RTTI Type Descriptor'`
- 160 `class sub0::spike::detail::Forwarding<class sub0::spike::Subscription<struct `anonymous namespace'::Actuator,struct `anonymous namespace'::Command>,struct `anonymous namespace'::Command> `RTTI Type Descriptor'`
- 128 `class sub0::spike::Subscription<struct `anonymous namespace'::Controller,struct `anonymous namespace'::Sample,struct `anonymous namespace'::Command> `RTTI Type Descriptor'`
- 104 `__rtc_tzz`
- 96 `class sub0::spike::Subscription<struct `anonymous namespace'::Logger,struct `anonymous namespace'::Sample> `RTTI Type Descriptor'`

</details>

<details><summary>clangcl-O2: largest symbols added by port_static (bytes)</summary>

- 80 `private: static <auto> __cdecl `public: __cdecl sub0::Sink<struct `anonymous namespace'::Sample>::Sink<struct `anonymous namespace'::Sample><class sub0::Wiring<struct `anonymous namespace'::Controller,struct `anonymous namespace'::Logger,struct `anonymous namespace'::Actuator> >(class sub0::Wiring<struct `anonymous namespace'::Controller,struct `anonymous namespace'::Logger,struct `anonymous namespace'::Actuator> & __ptr64) __ptr64'::`1'::<lambda_1>::__invoke(void const * __ptr64,struct `anonymous namespace'::Sample const & __ptr64)`
- 48 `private: static <auto> __cdecl `public: __cdecl sub0::Sink<struct `anonymous namespace'::Command>::Sink<struct `anonymous namespace'::Command><class sub0::Wiring<struct `anonymous namespace'::Controller,struct `anonymous namespace'::Logger,struct `anonymous namespace'::Actuator> >(class sub0::Wiring<struct `anonymous namespace'::Controller,struct `anonymous namespace'::Logger,struct `anonymous namespace'::Actuator> & __ptr64) __ptr64'::`1'::<lambda_1>::__invoke(void const * __ptr64,struct `anonymous namespace'::Command const & __ptr64)`
- 24 `class collapse::Slot<class sub0::Wiring<struct `anonymous namespace'::Controller,struct `anonymous namespace'::Logger,struct `anonymous namespace'::Actuator> > `anonymous namespace'::bus`
- 8 `?sensor@?A0xC5383E38@@3V?$Slot@USensor@?A0xC5383E38@@@collapse@@A.2`
- 8 `?sensor@?A0xC5383E38@@3V?$Slot@USensor@?A0xC5383E38@@@collapse@@A.0`
- 8 `class collapse::Slot<struct `anonymous namespace'::Actuator> `anonymous namespace'::actuator`
- 4 `class collapse::Slot<struct `anonymous namespace'::Logger> `anonymous namespace'::logger`
- 4 `class collapse::Slot<struct `anonymous namespace'::Controller> `anonymous namespace'::controller`

</details>

<details><summary>clangcl-O2: largest symbols added by route_bridged (bytes)</summary>

- 2496 `collapse_setup`
- 1664 `collapse_teardown`
- 444 `__acrt_SetEnvironmentVariableA`
- 240 `collapse_publish`
- 168 `std::bad_exception::`RTTI Base Class Descriptor at (0,-1,0,64)'`
- 104 `__rtc_tzz`
- 96 `class sub0::detail::SubscriberInterface<struct `anonymous namespace'::Sample,0> `RTTI Type Descriptor'`
- 96 `class sub0::detail::SubscriberInterface<struct `anonymous namespace'::Command,0> `RTTI Type Descriptor'`

</details>

<details><summary>clangcl-O2: largest symbols added by route_dynamic (bytes)</summary>

- 96 `class sub0::detail::SubscriberInterface<struct `anonymous namespace'::Sample,0> `RTTI Type Descriptor'`
- 96 `class sub0::detail::SubscriberInterface<struct `anonymous namespace'::Command,0> `RTTI Type Descriptor'`
- 72 `private: static struct sub0::detail::Table<struct `anonymous namespace'::Sample,struct sub0::detail::BuiltinT<8,1,2,0,struct sub0::NoLock> > sub0::detail::BrokerImpl<struct `anonymous namespace'::Sample,struct sub0::detail::BuiltinT<8,1,2,0,struct sub0::NoLock> >::global_`
- 72 `private: static struct sub0::detail::Table<struct `anonymous namespace'::Command,struct sub0::detail::BuiltinT<8,1,2,0,struct sub0::NoLock> > sub0::detail::BrokerImpl<struct `anonymous namespace'::Command,struct sub0::detail::BuiltinT<8,1,2,0,struct sub0::NoLock> >::global_`
- 64 `class sub0::Subscribe<struct `anonymous namespace'::Sample> `RTTI Type Descriptor'`
- 64 `class sub0::Subscribe<struct `anonymous namespace'::Command> `RTTI Type Descriptor'`
- 48 `struct `anonymous namespace'::Logger `RTTI Type Descriptor'`
- 48 `struct `anonymous namespace'::Controller `RTTI Type Descriptor'`

</details>

<details><summary>clangcl-O2: largest symbols added by route_static (bytes)</summary>

- 168 `std::bad_exception::`RTTI Base Class Descriptor at (0,-1,0,64)'`
- 8 `?logger@?A0x3AD4E5D5@@3ULoggerSlot@?A0x3AD4E5D5@@A.0`

</details>

<details><summary>clangcl-O2: largest symbols added by route_static_empty_bases (bytes)</summary>

- 176 `std::bad_exception::`RTTI Base Class Descriptor at (0,-1,0,64)'`
- 8 `?logger@?A0x66CD793C@@3ULoggerSlot@?A0x66CD793C@@A.0`

</details>

<details><summary>clangcl-O2: largest symbols added by route_static_virtual (bytes)</summary>

- 444 `__acrt_SetEnvironmentVariableA`
- 176 `std::bad_exception::`RTTI Base Class Descriptor at (0,-1,0,64)'`
- 96 `class sub0::spike::detail::StaticSubscribe<struct `anonymous namespace'::Sample> `RTTI Type Descriptor'`
- 48 `struct `anonymous namespace'::Logger `RTTI Type Descriptor'`
- 40 ``anonymous namespace'::Logger::`RTTI Base Class Descriptor at (0,-1,0,64)'`
- 36 `$pdata$__vcrt_getptd`
- 32 `collapse_setup`
- 32 `public: virtual void __cdecl `anonymous namespace'::Logger::receive(struct `anonymous namespace'::Sample const & __ptr64) __ptr64`

</details>

<details><summary>clangcl-O2: largest symbols added by today_dynamic (bytes)</summary>

- 1664 `collapse_teardown`
- 444 `__acrt_SetEnvironmentVariableA`
- 304 `collapse_setup`
- 168 `std::bad_exception::`RTTI Base Class Descriptor at (0,-1,0,64)'`
- 160 `collapse_publish`
- 96 `class sub0::detail::SubscriberInterface<struct `anonymous namespace'::Sample,0> `RTTI Type Descriptor'`
- 96 `class sub0::detail::SubscriberInterface<struct `anonymous namespace'::Command,0> `RTTI Type Descriptor'`
- 88 `__rtc_tzz`

</details>

<details><summary>clangcl-O2: largest symbols added by today_static (bytes)</summary>

- 168 `std::bad_exception::`RTTI Base Class Descriptor at (0,-1,0,64)'`
- 8 `?logger@?A0xB5EA942A@@3V?$Slot@ULogger@?A0xB5EA942A@@@collapse@@A.0`

</details>

### icx-O2, observable work

| variant | checksum | publish instr | setup instr | teardown instr | path instr | calls (direct/indirect) | text | data+bss | retained sub0 (B) | added deps | verdict |
|---|---|---|---|---|---|---|---|---|---|---|---|
| handwritten | ok | 33.0 (+0.0) | 3 (+0) | 2 (+0) | 27 (+0) | 0/0 | 139426 (+0) | 5688 (+0) | 0 | - | reference |
| handwritten_erased | ok | 54.0 (+21.0) | 12 (+9) | 2 (+0) | 48 (+21) | 2/0 | 139606 (+180) | 5736 (+48) | 0 | - | reference; FAIL: publish instr, setup instr, publish path, no extra RAM |
| bus_dynamic (vs today_dynamic) | ok | 105.0 (+2.0) | 59 (+4) | 88 (+0) | 44 (+0) | 0/2 | 142074 (+544) | 7272 (+928) | 3064 | - | FAIL: setup instr, no extra RAM, no Sub0Pub retained |
| bus_static | ok | 33.0 (+0.0) | 3 (+0) | 2 (+0) | 27 (+0) | 0/0 | 139426 (+0) | 5688 (+0) | 0 | - | PASS |
| port_dynamic (vs today_dynamic) | ok | 124.0 (+21.0) | 66 (+11) | 88 (+0) | 17 (-27) | 0/2 | 142242 (+712) | 7304 (+960) | 3224 | - | FAIL: publish instr, setup instr, no extra RAM, no Sub0Pub retained |
| port_static (vs handwritten_erased) | ok | 59.0 (+5.0) | 16 (+4) | 2 (+0) | 17 (-31) | 0/2 | 139654 (+48) | 5752 (+16) | 128 | - | FAIL: publish instr, setup instr, no extra indirect calls, no extra RAM, no Sub0Pub retained |
| route_bridged | ok | 51.0 (+18.0) | 82 (+79) | 22 (+20) | 70 (+43) | 0/2 | 141994 (+2568) | 6536 (+848) | 1384 | pure virtual | FAIL: publish instr, setup instr, teardown instr, publish path, no extra indirect calls, no extra RAM, no Sub0Pub retained, no extra dependencies |
| route_dynamic (vs today_dynamic) | ok | 103.0 (+0.0) | 55 (+0) | 88 (+0) | 44 (+0) | 0/2 | 141530 (+0) | 6344 (+0) | 1032 | - | PASS |
| route_static | ok | 33.0 (+0.0) | 3 (+0) | 2 (+0) | 27 (+0) | 0/0 | 139426 (+0) | 5688 (+0) | 0 | - | PASS |
| route_static_empty_bases | ok | 33.0 (+0.0) | 3 (+0) | 2 (+0) | 27 (+0) | 0/0 | 139426 (+0) | 5688 (+0) | 0 | - | PASS |
| route_static_virtual | ok | 33.0 (+0.0) | 5 (+2) | 2 (+0) | 27 (+0) | 0/0 | 139730 (+304) | 5816 (+128) | 152 | - | FAIL: setup instr, no extra RAM, no Sub0Pub retained |
| today_dynamic | ok | 103.0 (+70.0) | 55 (+52) | 88 (+86) | 44 (+17) | 0/2 | 141530 (+2104) | 6344 (+656) | 1032 | pure virtual | FAIL: publish instr, setup instr, teardown instr, publish path, no extra indirect calls, no extra RAM, no Sub0Pub retained, no extra dependencies |
| today_static | ok | 33.0 (+0.0) | 3 (+0) | 2 (+0) | 27 (+0) | 0/0 | 139426 (+0) | 5688 (+0) | 0 | - | PASS |

### icx-O2, removable work

| variant | checksum | publish instr | setup instr | teardown instr | path instr | calls (direct/indirect) | text | data+bss | retained sub0 (B) | added deps | verdict |
|---|---|---|---|---|---|---|---|---|---|---|---|
| handwritten | ok | 9.0 (+0.0) | 3 (+0) | 2 (+0) | 3 (+0) | 0/0 | 139346 (+0) | 5688 (+0) | 0 | - | reference |
| handwritten_erased | ok | 17.0 (+8.0) | 12 (+9) | 2 (+0) | 11 (+8) | 1/0 | 139478 (+132) | 5736 (+48) | 0 | - | reference; FAIL: publish instr, setup instr, publish path, no extra RAM |
| bus_dynamic (vs today_dynamic) | ok | 72.0 (+1.0) | 59 (+4) | 88 (+0) | 44 (+0) | 0/2 | 141994 (+528) | 7272 (+928) | 2984 | - | FAIL: setup instr, no extra RAM, no Sub0Pub retained |
| bus_static | ok | 9.0 (+0.0) | 3 (+0) | 2 (+0) | 3 (+0) | 0/0 | 139346 (+0) | 5688 (+0) | 0 | - | PASS |
| port_dynamic (vs today_dynamic) | ok | 91.0 (+20.0) | 66 (+11) | 88 (+0) | 17 (-27) | 0/2 | 142162 (+696) | 7304 (+960) | 3144 | - | FAIL: publish instr, setup instr, no extra RAM, no Sub0Pub retained |
| port_static (vs handwritten_erased) | ok | 27.0 (+10.0) | 16 (+4) | 2 (+0) | 17 (+6) | 0/2 | 139558 (+80) | 5752 (+16) | 32 | - | FAIL: publish instr, setup instr, publish path, no extra indirect calls, no extra RAM, no Sub0Pub retained |
| route_bridged | ok | 27.0 (+18.0) | 82 (+79) | 22 (+20) | 45 (+42) | 0/2 | 141866 (+2520) | 6536 (+848) | 1384 | pure virtual | FAIL: publish instr, setup instr, teardown instr, publish path, no extra indirect calls, no extra RAM, no Sub0Pub retained, no extra dependencies |
| route_dynamic (vs today_dynamic) | ok | 71.0 (+0.0) | 55 (+0) | 88 (+0) | 44 (+0) | 0/2 | 141466 (+0) | 6344 (+0) | 1032 | - | PASS |
| route_static | ok | 9.0 (+0.0) | 3 (+0) | 2 (+0) | 3 (+0) | 0/0 | 139346 (+0) | 5688 (+0) | 0 | - | PASS |
| route_static_empty_bases | ok | 9.0 (+0.0) | 3 (+0) | 2 (+0) | 3 (+0) | 0/0 | 139346 (+0) | 5688 (+0) | 0 | - | PASS |
| route_static_virtual | ok | 9.0 (+0.0) | 5 (+2) | 2 (+0) | 3 (+0) | 0/0 | 139634 (+288) | 5816 (+128) | 152 | - | FAIL: setup instr, no extra RAM, no Sub0Pub retained |
| today_dynamic | ok | 71.0 (+62.0) | 55 (+52) | 88 (+86) | 44 (+41) | 0/2 | 141466 (+2120) | 6344 (+656) | 1032 | pure virtual | FAIL: publish instr, setup instr, teardown instr, publish path, no extra indirect calls, no extra RAM, no Sub0Pub retained, no extra dependencies |
| today_static | ok | 9.0 (+0.0) | 3 (+0) | 2 (+0) | 3 (+0) | 0/0 | 139346 (+0) | 5688 (+0) | 0 | - | PASS |

<details><summary>icx-O2: largest symbols added by handwritten_erased (bytes)</summary>

- 1204 `__volatile_metadata`
- 80 `collapse_setup`
- 64 `void __cdecl `anonymous namespace'::deliverSample(void const * __ptr64,struct `anonymous namespace'::Sample const & __ptr64)`
- 56 `__rtc_tzz`
- 48 `void __cdecl `anonymous namespace'::deliverCommand(void const * __ptr64,struct `anonymous namespace'::Command const & __ptr64)`
- 24 `class collapse::Slot<struct `anonymous namespace'::Node> `anonymous namespace'::node`
- 8 `?sensor@?A0xBCBA4CC4@@3V?$Slot@USensor@?A0xBCBA4CC4@@@collapse@@A.2`
- 8 `?sensor@?A0xBCBA4CC4@@3V?$Slot@USensor@?A0xBCBA4CC4@@@collapse@@A.0`

</details>

<details><summary>icx-O2: largest symbols added by bus_dynamic (bytes)</summary>

- 320 `collapse_setup`
- 192 `class sub0::spike::detail::Forwarding<class sub0::spike::Subscription<struct `anonymous namespace'::Controller,struct `anonymous namespace'::Sample,struct `anonymous namespace'::Command>,struct `anonymous namespace'::Command> `RTTI Type Descriptor'`
- 176 `class sub0::spike::detail::Forwarding<class sub0::spike::Subscription<struct `anonymous namespace'::Controller,struct `anonymous namespace'::Sample,struct `anonymous namespace'::Command>,struct `anonymous namespace'::Sample> `RTTI Type Descriptor'`
- 160 `class sub0::spike::detail::Forwarding<class sub0::spike::Subscription<struct `anonymous namespace'::Logger,struct `anonymous namespace'::Sample>,struct `anonymous namespace'::Sample> `RTTI Type Descriptor'`
- 160 `class sub0::spike::detail::Forwarding<class sub0::spike::Subscription<struct `anonymous namespace'::Actuator,struct `anonymous namespace'::Command>,struct `anonymous namespace'::Command> `RTTI Type Descriptor'`
- 128 `class sub0::spike::Subscription<struct `anonymous namespace'::Controller,struct `anonymous namespace'::Sample,struct `anonymous namespace'::Command> `RTTI Type Descriptor'`
- 96 `class sub0::spike::Subscription<struct `anonymous namespace'::Logger,struct `anonymous namespace'::Sample> `RTTI Type Descriptor'`
- 96 `class sub0::spike::Subscription<struct `anonymous namespace'::Actuator,struct `anonymous namespace'::Command> `RTTI Type Descriptor'`

</details>

<details><summary>icx-O2: largest symbols added by bus_static (bytes)</summary>

- 8 `?logger@?A0xE056B48A@@3V?$Slot@ULogger@?A0xE056B48A@@@collapse@@A.0`

</details>

<details><summary>icx-O2: largest symbols added by port_dynamic (bytes)</summary>

- 368 `collapse_setup`
- 192 `class sub0::spike::detail::Forwarding<class sub0::spike::Subscription<struct `anonymous namespace'::Controller,struct `anonymous namespace'::Sample,struct `anonymous namespace'::Command>,struct `anonymous namespace'::Command> `RTTI Type Descriptor'`
- 176 `class sub0::spike::detail::Forwarding<class sub0::spike::Subscription<struct `anonymous namespace'::Controller,struct `anonymous namespace'::Sample,struct `anonymous namespace'::Command>,struct `anonymous namespace'::Sample> `RTTI Type Descriptor'`
- 160 `class sub0::spike::detail::Forwarding<class sub0::spike::Subscription<struct `anonymous namespace'::Logger,struct `anonymous namespace'::Sample>,struct `anonymous namespace'::Sample> `RTTI Type Descriptor'`
- 160 `class sub0::spike::detail::Forwarding<class sub0::spike::Subscription<struct `anonymous namespace'::Actuator,struct `anonymous namespace'::Command>,struct `anonymous namespace'::Command> `RTTI Type Descriptor'`
- 128 `class sub0::spike::Subscription<struct `anonymous namespace'::Controller,struct `anonymous namespace'::Sample,struct `anonymous namespace'::Command> `RTTI Type Descriptor'`
- 96 `class sub0::spike::Subscription<struct `anonymous namespace'::Logger,struct `anonymous namespace'::Sample> `RTTI Type Descriptor'`
- 96 `class sub0::spike::Subscription<struct `anonymous namespace'::Actuator,struct `anonymous namespace'::Command> `RTTI Type Descriptor'`

</details>

<details><summary>icx-O2: largest symbols added by port_static (bytes)</summary>

- 112 `collapse_setup`
- 80 `private: static <auto> __cdecl `public: __cdecl sub0::Sink<struct `anonymous namespace'::Sample>::Sink<struct `anonymous namespace'::Sample><class sub0::Wiring<struct `anonymous namespace'::Controller,struct `anonymous namespace'::Logger,struct `anonymous namespace'::Actuator> >(class sub0::Wiring<struct `anonymous namespace'::Controller,struct `anonymous namespace'::Logger,struct `anonymous namespace'::Actuator> & __ptr64) __ptr64'::`1'::<lambda_1>::__invoke(void const * __ptr64,struct `anonymous namespace'::Sample const & __ptr64)`
- 48 `private: static <auto> __cdecl `public: __cdecl sub0::Sink<struct `anonymous namespace'::Command>::Sink<struct `anonymous namespace'::Command><class sub0::Wiring<struct `anonymous namespace'::Controller,struct `anonymous namespace'::Logger,struct `anonymous namespace'::Actuator> >(class sub0::Wiring<struct `anonymous namespace'::Controller,struct `anonymous namespace'::Logger,struct `anonymous namespace'::Actuator> & __ptr64) __ptr64'::`1'::<lambda_1>::__invoke(void const * __ptr64,struct `anonymous namespace'::Command const & __ptr64)`
- 32 `class collapse::Slot<struct `anonymous namespace'::Sensor> `anonymous namespace'::sensor`
- 24 `class collapse::Slot<class sub0::Wiring<struct `anonymous namespace'::Controller,struct `anonymous namespace'::Logger,struct `anonymous namespace'::Actuator> > `anonymous namespace'::bus`
- 8 `class collapse::Slot<struct `anonymous namespace'::Actuator> `anonymous namespace'::actuator`
- 4 `class collapse::Slot<struct `anonymous namespace'::Logger> `anonymous namespace'::logger`
- 4 `class collapse::Slot<struct `anonymous namespace'::Controller> `anonymous namespace'::controller`

</details>

<details><summary>icx-O2: largest symbols added by route_bridged (bytes)</summary>

- 1204 `__volatile_metadata`
- 560 `collapse_setup`
- 480 `collapse_teardown`
- 240 `collapse_publish`
- 96 `class sub0::detail::SubscriberInterface<struct `anonymous namespace'::Sample,0> `RTTI Type Descriptor'`
- 96 `class sub0::detail::SubscriberInterface<struct `anonymous namespace'::Command,0> `RTTI Type Descriptor'`
- 96 `class sub0::spike::detail::BridgedSubscribe<struct `anonymous namespace'::Sample> `RTTI Type Descriptor'`
- 96 `class sub0::spike::detail::BridgedSubscribe<struct `anonymous namespace'::Command> `RTTI Type Descriptor'`

</details>

<details><summary>icx-O2: largest symbols added by route_dynamic (bytes)</summary>

- 96 `class sub0::detail::SubscriberInterface<struct `anonymous namespace'::Sample,0> `RTTI Type Descriptor'`
- 96 `class sub0::detail::SubscriberInterface<struct `anonymous namespace'::Command,0> `RTTI Type Descriptor'`
- 72 `private: static struct sub0::detail::Table<struct `anonymous namespace'::Sample,struct sub0::detail::BuiltinT<8,1,2,0,struct sub0::NoLock> > sub0::detail::BrokerImpl<struct `anonymous namespace'::Sample,struct sub0::detail::BuiltinT<8,1,2,0,struct sub0::NoLock> >::global_`
- 72 `private: static struct sub0::detail::Table<struct `anonymous namespace'::Command,struct sub0::detail::BuiltinT<8,1,2,0,struct sub0::NoLock> > sub0::detail::BrokerImpl<struct `anonymous namespace'::Command,struct sub0::detail::BuiltinT<8,1,2,0,struct sub0::NoLock> >::global_`
- 64 `class sub0::Subscribe<struct `anonymous namespace'::Sample> `RTTI Type Descriptor'`
- 64 `class sub0::Subscribe<struct `anonymous namespace'::Command> `RTTI Type Descriptor'`
- 48 `struct `anonymous namespace'::Logger `RTTI Type Descriptor'`
- 48 `struct `anonymous namespace'::Controller `RTTI Type Descriptor'`

</details>

<details><summary>icx-O2: largest symbols added by route_static (bytes)</summary>

- 8 `?logger@?A0x3AD4E5D5@@3ULoggerSlot@?A0x3AD4E5D5@@A.0`

</details>

<details><summary>icx-O2: largest symbols added by route_static_empty_bases (bytes)</summary>

- 1212 `__volatile_metadata`
- 8 `?logger@?A0x66CD793C@@3ULoggerSlot@?A0x66CD793C@@A.0`

</details>

<details><summary>icx-O2: largest symbols added by route_static_virtual (bytes)</summary>

- 1204 `__volatile_metadata`
- 120 `SetSmallXmm`
- 96 `class sub0::spike::detail::StaticSubscribe<struct `anonymous namespace'::Sample> `RTTI Type Descriptor'`
- 48 `struct `anonymous namespace'::Logger `RTTI Type Descriptor'`
- 40 `type_info::`RTTI Base Class Descriptor at (0,-1,0,64)'`
- 40 ``anonymous namespace'::Logger::`RTTI Base Class Descriptor at (0,-1,0,64)'`
- 32 `collapse_setup`
- 32 `public: virtual void __cdecl `anonymous namespace'::Logger::receive(struct `anonymous namespace'::Sample const & __ptr64) __ptr64`

</details>

<details><summary>icx-O2: largest symbols added by today_dynamic (bytes)</summary>

- 1204 `__volatile_metadata`
- 480 `collapse_teardown`
- 304 `collapse_setup`
- 160 `collapse_publish`
- 152 `SetSmallXmm`
- 96 `class sub0::detail::SubscriberInterface<struct `anonymous namespace'::Sample,0> `RTTI Type Descriptor'`
- 96 `class sub0::detail::SubscriberInterface<struct `anonymous namespace'::Command,0> `RTTI Type Descriptor'`
- 72 `__rtc_tzz`

</details>

<details><summary>icx-O2: largest symbols added by today_static (bytes)</summary>

- 8 `?logger@?A0xB5EA942A@@3V?$Slot@ULogger@?A0xB5EA942A@@@collapse@@A.0`

</details>


## Summary: every variant against its reference (observable work)

Each cell: instructions per publication (delta), then the deltas of the static publish path, indirect calls on it, RAM and image text. `=` marks a variant that meets every criterion of docs/EVIDENCE.md against that reference; `!` one that does not.

### batch

| variant | judged against | msvc-O2 | msvc-O2-lto | clangcl-O2 | icx-O2 |
|---|---|---|---|---|---|
| handwritten | reference | 201.0 instr; path 63 | - | 136.0 instr; path 130 | 250.0 instr; path 48 |
| handwritten_runtime | handwritten | ! 417.0 (+216.0); path +54; indirect +0; RAM +16; text +196 | - | ! 373.0 (+237.0); path -104; indirect +0; RAM +16; text -564 | ! 248.0 (-2.0); path -2; indirect +0; RAM +0; text -16 |
| route_bridged | handwritten | ! 732.0 (+531.0); path -24; indirect +1; RAM +400; text +816 | - | ! 149.0 (+13.0); path +44; indirect +1; RAM +356; text +1724 | ! 739.0 (+489.0); path +4; indirect +1; RAM +416; text +708 |
| route_dynamic | today_dynamic | = 1369.0 (+0.0); path +0; indirect +0; RAM +0; text +0 | - | = 1573.0 (+0.0); path +0; indirect +0; RAM +0; text +0 | = 1571.0 (+0.0); path +0; indirect +0; RAM +0; text +0 |
| route_static | handwritten | ! 205.0 (+4.0); path +4; indirect +0; RAM +16; text +36 | - | = 136.0 (+0.0); path +0; indirect +0; RAM +0; text +0 | = 250.0 (+0.0); path +0; indirect +0; RAM +0; text +0 |
| today_dynamic | handwritten | ! 1369.0 (+1168.0); path -29; indirect +1; RAM +304; text +536 | - | ! 1573.0 (+1437.0); path -78; indirect +1; RAM +292; text +424 | ! 1571.0 (+1321.0); path +2; indirect +1; RAM +288; text +500 |
| today_static | handwritten | ! 205.0 (+4.0); path +4; indirect +0; RAM +16; text +36 | - | = 136.0 (+0.0); path +0; indirect +0; RAM +0; text +0 | = 250.0 (+0.0); path +0; indirect +0; RAM +0; text +0 |
| today_wire | handwritten_runtime | = 417.0 (+0.0); path +0; indirect +0; RAM +0; text +0 | - | = 373.0 (+0.0); path +0; indirect +0; RAM +0; text +0 | = 248.0 (+0.0); path +0; indirect +0; RAM +0; text +0 |

### cross_file

| variant | judged against | msvc-O2 | msvc-O2-lto | clangcl-O2 | icx-O2 |
|---|---|---|---|---|---|
| handwritten | reference | 39.0 instr; path 27 | 19.0 instr; path 13 | 53.0 instr; path 38 | 53.0 instr; path 38 |
| route_dynamic | today_dynamic | = 63.0 (+0.0); path +0; indirect +0; RAM +0; text +0 | = 63.0 (+0.0); path +0; indirect +0; RAM +0; text +0 | = 76.0 (+0.0); path +0; indirect +0; RAM +0; text +0 | = 76.0 (+0.0); path +0; indirect +0; RAM +0; text +0 |
| route_static | handwritten | = 39.0 (+0.0); path +0; indirect +0; RAM +0; text +0 | ! 19.0 (+0.0); path +0; indirect +0; RAM +8; text +0 | = 53.0 (+0.0); path +0; indirect +0; RAM +0; text +0 | = 53.0 (+0.0); path +0; indirect +0; RAM +0; text +0 |
| route_static_out_of_line | handwritten | ! 46.0 (+7.0); path +7; indirect +0; RAM +0; text +52 | ! 19.0 (+0.0); path +0; indirect +0; RAM +8; text +0 | ! 57.0 (+4.0); path +4; indirect +0; RAM +0; text +45 | ! 57.0 (+4.0); path +4; indirect +0; RAM +0; text +52 |
| today_dynamic | handwritten | ! 63.0 (+24.0); path -7; indirect +1; RAM +368; text +1092 | ! 63.0 (+44.0); path +7; indirect +1; RAM +376; text +1372 | ! 76.0 (+23.0); path -13; indirect +1; RAM +324; text +2066 | ! 76.0 (+23.0); path -13; indirect +1; RAM +384; text +1192 |
| today_static | handwritten | = 39.0 (+0.0); path +0; indirect +0; RAM +0; text +0 | ! 19.0 (+0.0); path +0; indirect +0; RAM +8; text +0 | = 53.0 (+0.0); path +0; indirect +0; RAM +0; text +0 | = 53.0 (+0.0); path +0; indirect +0; RAM +0; text +0 |

### one_receiver

| variant | judged against | msvc-O2 | msvc-O2-lto | clangcl-O2 | icx-O2 |
|---|---|---|---|---|---|
| handwritten | reference | 12.0 instr; path 6 | - | 17.0 instr; path 11 | 17.0 instr; path 11 |
| handwritten_erased | handwritten | ! 22.0 (+10.0); path +3; indirect +1; RAM +32; text +12476 | - | ! 25.0 (+8.0); path +8; indirect +0; RAM +16; text +84 | ! 26.0 (+9.0); path -1; indirect +1; RAM +16; text +100 |
| bus_dynamic | today_dynamic | ! 35.0 (+1.0); path +0; indirect +0; RAM +192; text +224 | - | ! 41.0 (+1.0); path +0; indirect +0; RAM +192; text +192 | ! 41.0 (+1.0); path +0; indirect +0; RAM +272; text +144 |
| bus_static | handwritten | = 12.0 (+0.0); path +0; indirect +0; RAM +0; text +0 | - | = 17.0 (+0.0); path +0; indirect +0; RAM +0; text +0 | = 17.0 (+0.0); path +0; indirect +0; RAM +0; text +0 |
| port_dynamic | today_dynamic | ! 45.0 (+11.0); path -11; indirect +0; RAM +208; text +12572 | - | ! 48.0 (+8.0); path +7; indirect +0; RAM +208; text +262 | ! 48.0 (+8.0); path -16; indirect +0; RAM +288; text +228 |
| port_static | handwritten_erased | = 22.0 (+0.0); path +0; indirect +0; RAM +0; text +0 | - | ! 25.0 (+0.0); path +0; indirect +0; RAM +0; text +13 | = 25.0 (-1.0); path -1; indirect +0; RAM +0; text +0 |
| route_bridged | handwritten | ! 23.0 (+11.0); path +17; indirect +1; RAM +416; text +1076 | - | ! 26.0 (+9.0); path +21; indirect +1; RAM +356; text +1704 | ! 26.0 (+9.0); path +21; indirect +1; RAM +400; text +972 |
| route_dynamic | today_dynamic | = 34.0 (+0.0); path +0; indirect +0; RAM +0; text +0 | - | = 40.0 (+0.0); path +0; indirect +0; RAM +0; text +0 | = 40.0 (+0.0); path +0; indirect +0; RAM +0; text +0 |
| route_static | handwritten | = 12.0 (+0.0); path +0; indirect +0; RAM +0; text +0 | - | = 17.0 (+0.0); path +0; indirect +0; RAM +0; text +0 | = 17.0 (+0.0); path +0; indirect +0; RAM +0; text +0 |
| route_static_virtual | handwritten | ! 12.0 (+0.0); path +0; indirect +0; RAM +160; text +240 | - | = 17.0 (+0.0); path +0; indirect +0; RAM +0; text +0 | = 17.0 (+0.0); path +0; indirect +0; RAM +0; text +0 |
| today_dynamic | handwritten | ! 34.0 (+22.0); path +14; indirect +1; RAM +320; text +860 | - | ! 40.0 (+23.0); path +14; indirect +1; RAM +292; text +996 | ! 40.0 (+23.0); path +14; indirect +1; RAM +272; text +732 |
| today_static | handwritten | = 12.0 (+0.0); path +0; indirect +0; RAM +0; text +0 | - | = 17.0 (+0.0); path +0; indirect +0; RAM +0; text +0 | = 17.0 (+0.0); path +0; indirect +0; RAM +0; text +0 |

### station

| variant | judged against | msvc-O2 | msvc-O2-lto | clangcl-O2 | icx-O2 |
|---|---|---|---|---|---|
| handwritten | reference | 21.0 instr; path 15 | - | 33.0 instr; path 27 | 33.0 instr; path 27 |
| handwritten_erased | handwritten | ! 46.0 (+25.0); path +2; indirect +2; RAM +64; text +12604 | - | ! 58.0 (+25.0); path +25; indirect +0; RAM +48; text +196 | ! 54.0 (+21.0); path +21; indirect +0; RAM +48; text +180 |
| bus_dynamic | today_dynamic | ! 84.0 (+1.0); path +0; indirect +0; RAM +768; text +752 | - | ! 104.0 (+1.0); path +0; indirect +0; RAM +864; text +576 | ! 105.0 (+2.0); path +0; indirect +0; RAM +928; text +544 |
| bus_static | handwritten | = 21.0 (+0.0); path +0; indirect +0; RAM +0; text +0 | - | = 33.0 (+0.0); path +0; indirect +0; RAM +0; text +0 | = 33.0 (+0.0); path +0; indirect +0; RAM +0; text +0 |
| port_dynamic | today_dynamic | ! 110.0 (+27.0); path -16; indirect +0; RAM +800; text +13352 | - | ! 123.0 (+20.0); path +19; indirect +0; RAM +880; text +722 | ! 124.0 (+21.0); path -27; indirect +0; RAM +960; text +712 |
| port_static | handwritten_erased | = 47.0 (+1.0); path +1; indirect +0; RAM +0; text +0 | - | ! 58.0 (+0.0); path +0; indirect +0; RAM +0; text +15 | ! 59.0 (+5.0); path -31; indirect +2; RAM +16; text +48 |
| route_bridged | handwritten | ! 42.0 (+21.0); path +33; indirect +2; RAM +896; text +2892 | - | ! 51.0 (+18.0); path +43; indirect +2; RAM +852; text +5688 | ! 51.0 (+18.0); path +43; indirect +2; RAM +848; text +2568 |
| route_dynamic | today_dynamic | = 83.0 (+0.0); path +0; indirect +0; RAM +0; text +0 | - | = 103.0 (+0.0); path +0; indirect +0; RAM +0; text +0 | = 103.0 (+0.0); path +0; indirect +0; RAM +0; text +0 |
| route_static | handwritten | ! 21.0 (+0.0); path +0; indirect +0; RAM +0; text +16 | - | = 33.0 (+0.0); path +0; indirect +0; RAM +0; text +0 | = 33.0 (+0.0); path +0; indirect +0; RAM +0; text +0 |
| route_static_empty_bases | handwritten | = 21.0 (+0.0); path +0; indirect +0; RAM +0; text +0 | - | = 33.0 (+0.0); path +0; indirect +0; RAM +0; text +0 | = 33.0 (+0.0); path +0; indirect +0; RAM +0; text +0 |
| route_static_virtual | handwritten | ! 21.0 (+0.0); path +0; indirect +0; RAM +384; text +936 | - | ! 33.0 (+0.0); path +0; indirect +0; RAM +144; text +239 | ! 33.0 (+0.0); path +0; indirect +0; RAM +128; text +304 |
| today_dynamic | handwritten | ! 83.0 (+62.0); path +19; indirect +2; RAM +704; text +2008 | - | ! 103.0 (+70.0); path +17; indirect +2; RAM +660; text +3204 | ! 103.0 (+70.0); path +17; indirect +2; RAM +656; text +2104 |
| today_static | handwritten | = 21.0 (+0.0); path +0; indirect +0; RAM +0; text +0 | - | = 33.0 (+0.0); path +0; indirect +0; RAM +0; text +0 | = 33.0 (+0.0); path +0; indirect +0; RAM +0; text +0 |

## Wall-clock time (supplemental)

Best of 9 epochs of 5,000,000 publications, ns per publication. Machine- and noise-dependent: the instruction counts above are the comparison of record.

### batch

| variant | msvc-O2 | msvc-O2-lto | clangcl-O2 | icx-O2 |
|---|---|---|---|---|
| handwritten | 8.35 | - | 5.17 | 11.81 |
| handwritten_runtime | 81.02 | - | 15.46 | 9.62 |
| route_bridged | 89.80 | - | 6.23 | 103.25 |
| route_dynamic | 94.56 | - | 91.59 | 82.85 |
| route_static | 8.76 | - | 5.81 | 10.40 |
| today_dynamic | 95.23 | - | 88.66 | 83.82 |
| today_static | 8.61 | - | 5.20 | 9.45 |
| today_wire | 82.43 | - | 15.19 | 10.55 |

### cross_file

| variant | msvc-O2 | msvc-O2-lto | clangcl-O2 | icx-O2 |
|---|---|---|---|---|
| handwritten | 5.29 | 2.69 | 4.51 | 4.96 |
| route_dynamic | 5.53 | 5.48 | 5.71 | 5.42 |
| route_static | 5.29 | 2.45 | 4.99 | 5.17 |
| route_static_out_of_line | 5.73 | 1.87 | 4.92 | 5.65 |
| today_dynamic | 6.48 | 5.56 | 5.53 | 5.19 |
| today_static | 5.27 | 2.57 | 4.96 | 5.22 |

### one_receiver

| variant | msvc-O2 | msvc-O2-lto | clangcl-O2 | icx-O2 |
|---|---|---|---|---|
| handwritten | 1.79 | - | 1.65 | 1.64 |
| handwritten_erased | 1.88 | - | 1.69 | 1.63 |
| bus_dynamic | 2.01 | - | 1.79 | 1.85 |
| bus_static | 1.82 | - | 1.68 | 1.67 |
| port_dynamic | 2.42 | - | 1.82 | 1.85 |
| port_static | 1.84 | - | 1.97 | 1.69 |
| route_bridged | 1.88 | - | 1.74 | 1.63 |
| route_dynamic | 2.47 | - | 1.93 | 1.88 |
| route_static | 1.75 | - | 1.65 | 1.65 |
| route_static_virtual | 2.00 | - | 1.66 | 1.66 |
| today_dynamic | 2.40 | - | 1.97 | 1.90 |
| today_static | 1.75 | - | 1.84 | 1.64 |

### station

| variant | msvc-O2 | msvc-O2-lto | clangcl-O2 | icx-O2 |
|---|---|---|---|---|
| handwritten | 2.86 | - | 3.79 | 3.67 |
| handwritten_erased | 6.39 | - | 6.28 | 4.69 |
| bus_dynamic | 8.47 | - | 7.23 | 7.19 |
| bus_static | 2.78 | - | 3.67 | 3.67 |
| port_dynamic | 8.31 | - | 7.75 | 7.35 |
| port_static | 6.75 | - | 5.66 | 5.72 |
| route_bridged | 4.10 | - | 3.68 | 4.39 |
| route_dynamic | 7.93 | - | 7.32 | 7.24 |
| route_static | 2.79 | - | 3.95 | 3.69 |
| route_static_empty_bases | 2.77 | - | 3.68 | 3.69 |
| route_static_virtual | 2.72 | - | 3.67 | 3.69 |
| today_dynamic | 8.21 | - | 8.37 | 7.23 |
| today_static | 2.76 | - | 3.67 | 3.62 |
