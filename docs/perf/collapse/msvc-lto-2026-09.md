# Collapse evidence

Final-link evidence per case, build and form; every variant is compared with `handwritten` (equal-work reference, same build and form), or with the extra reference it names, shown as `variant (vs handwritten_<kind>)`: e.g. `handwritten_runtime`, hand-written code that reaches its receivers through addresses stored at setup. Deltas in parentheses. instr = callgrind instructions (publish: per publication of 1000). path = static instructions of `collapse_publish` plus directly reachable functions.

- **msvc-O2-lto**: `Microsoft (R) C/C++ Optimizing Compiler Version 19.51.36246 for x64` `/O2 /GL`

## Case: cancellation

## Case: cancellation_filtered

## Case: cross_file

### msvc-O2-lto, observable work

| variant | checksum | publish instr | setup instr | teardown instr | path instr | calls (direct/indirect) | text | data+bss | retained sub0 (B) | added deps | verdict |
|---|---|---|---|---|---|---|---|---|---|---|---|
| handwritten | ok | - | - | - | 13 (+0) | 0/0 | 135932 (+0) | 5688 (+0) | 0 | - | reference |
| handwritten_erased | ok | - | - | - | 9 (-4) | 0/1 | 148464 (+12532) | 5728 (+40) | 0 | - | reference; FAIL: no extra indirect calls, no extra RAM |
| handwritten_runtime | ok | - | - | - | 16 (+3) | 0/0 | 135996 (+64) | 5712 (+24) | 0 | - | reference; FAIL: publish path, no extra RAM |
| sub0_b1_wire (vs handwritten_runtime) | ok | - | - | - | 16 (+0) | 0/0 | 135996 (+0) | 5712 (+0) | 0 | - | PASS |
| sub0_b2_static | ok | - | - | - | 13 (+0) | 0/0 | 135932 (+0) | 5696 (+8) | 0 | - | FAIL: no extra RAM |
| sub0_b3_sink (vs handwritten_erased) | ok | - | - | - | 9 (+0) | 0/1 | 148464 (+0) | 5728 (+0) | 0 | - | PASS |
| sub0pub_virtual | ok | - | - | - | 20 (+7) | 0/1 | 137312 (+1380) | 6064 (+376) | 456 | pure virtual | FAIL: publish path, no extra indirect calls, no extra RAM, no Sub0Pub retained, no extra dependencies |
| sub0pub_virtual_lean | ok | - | - | - | 20 (+7) | 0/1 | 137312 (+1380) | 6064 (+376) | 456 | pure virtual | FAIL: publish path, no extra indirect calls, no extra RAM, no Sub0Pub retained, no extra dependencies |

### msvc-O2-lto, removable work

| variant | checksum | publish instr | setup instr | teardown instr | path instr | calls (direct/indirect) | text | data+bss | retained sub0 (B) | added deps | verdict |
|---|---|---|---|---|---|---|---|---|---|---|---|
| handwritten | ok | - | - | - | 3 (+0) | 0/0 | 135868 (+0) | 5688 (+0) | 0 | - | reference |
| handwritten_erased | ok | - | - | - | 9 (+6) | 0/1 | 148416 (+12548) | 5728 (+40) | 0 | - | reference; FAIL: publish path, no extra indirect calls, no extra RAM |
| handwritten_runtime | ok | - | - | - | 4 (+1) | 0/0 | 135916 (+48) | 5712 (+24) | 0 | - | reference; FAIL: no extra RAM |
| sub0_b1_wire (vs handwritten_runtime) | ok | - | - | - | 4 (+0) | 0/0 | 135916 (+0) | 5712 (+0) | 0 | - | PASS |
| sub0_b2_static | ok | - | - | - | 3 (+0) | 0/0 | 135868 (+0) | 5696 (+8) | 0 | - | FAIL: no extra RAM |
| sub0_b3_sink (vs handwritten_erased) | ok | - | - | - | 9 (+0) | 0/1 | 148416 (+0) | 5728 (+0) | 0 | - | PASS |
| sub0pub_virtual | ok | - | - | - | 20 (+17) | 0/1 | 137280 (+1412) | 6064 (+376) | 456 | pure virtual | FAIL: publish path, no extra indirect calls, no extra RAM, no Sub0Pub retained, no extra dependencies |
| sub0pub_virtual_lean | ok | - | - | - | 20 (+17) | 0/1 | 137280 (+1412) | 6064 (+376) | 456 | pure virtual | FAIL: publish path, no extra indirect calls, no extra RAM, no Sub0Pub retained, no extra dependencies |

<details><summary>msvc-O2-lto: largest symbols added by handwritten_erased (bytes)</summary>

- 1456 `void __cdecl FindHandler<class __FrameHandler4>(struct EHExceptionRecord * __ptr64,unsigned __int64 * __ptr64,struct _CONTEXT * __ptr64,struct _xDISPATCHER_CONTEXT * __ptr64,struct FH4::FuncInfo4 * __ptr64,unsigned char,int,unsigned __int64 * __ptr64)`
- 1212 `__volatile_metadata`
- 992 `public: static void __cdecl __FrameHandler4::FrameUnwindToState(unsigned __int64 * __ptr64,struct _xDISPATCHER_CONTEXT * __ptr64,struct FH4::FuncInfo4 * __ptr64,int)`
- 880 `void __cdecl FindHandlerForForeignException<class __FrameHandler4>(struct EHExceptionRecord * __ptr64,unsigned __int64 * __ptr64,struct _CONTEXT * __ptr64,struct _xDISPATCHER_CONTEXT * __ptr64,struct FH4::FuncInfo4 * __ptr64,int,int,unsigned __int64 * __ptr64)`
- 614 `$$000000`
- 608 `public: static void * __ptr64 __cdecl __FrameHandler4::CxxCallCatchBlock(struct _EXCEPTION_RECORD * __ptr64)`
- 608 `enum _EXCEPTION_DISPOSITION __cdecl __InternalCxxFrameHandler<class __FrameHandler4>(struct EHExceptionRecord * __ptr64,unsigned __int64 * __ptr64,struct _CONTEXT * __ptr64,struct _xDISPATCHER_CONTEXT * __ptr64,struct FH4::FuncInfo4 * __ptr64,int,unsigned __int64 * __ptr64,unsigned char)`
- 496 `public: void __cdecl FH4::TryBlockMap4::setBuffer(class FH4::TryBlockMap4::iterator) __ptr64`

</details>

<details><summary>msvc-O2-lto: largest symbols added by handwritten_runtime (bytes)</summary>

- 1180 `__volatile_metadata`
- 80 `collapse_setup`
- 80 `collapse_publish`
- 24 `class collapse::Slot<struct `anonymous namespace'::Sensor> `anonymous namespace'::sensor`
- 4 `class collapse::Slot<struct app::Logger> `anonymous namespace'::logger`
- 4 `class collapse::Slot<struct app::Controller> `anonymous namespace'::controllerB`
- 4 `class collapse::Slot<struct app::Controller> `anonymous namespace'::controllerA`

</details>

<details><summary>msvc-O2-lto: largest symbols added by sub0_b1_wire (bytes)</summary>

- 24 `class collapse::Slot<struct `anonymous namespace'::Sensor<class sub0::Wiring<struct app::Controller,struct app::Controller,struct app::Logger> > > `anonymous namespace'::sensor`
- 4 `class collapse::Slot<struct app::Logger> `anonymous namespace'::logger`
- 4 `class collapse::Slot<struct app::Controller> `anonymous namespace'::controllerB`
- 4 `class collapse::Slot<struct app::Controller> `anonymous namespace'::controllerA`

</details>

<details><summary>msvc-O2-lto: largest symbols added by sub0_b2_static (bytes)</summary>

- 1180 `__volatile_metadata`
- 8 `__scrt_ucrt_dll_is_in_use`
- 4 `class collapse::Slot<struct `anonymous namespace'::Sensor<struct sub0::StaticWiring<&class collapse::Slot<struct app::Controller> `anonymous namespace'::controllerA,&class collapse::Slot<struct app::Controller> A0xff0ae43e::controllerB,&class collapse::Slot<struct app::Logger> A0xff0ae43e::logger> > > `anonymous namespace'::sensor`
- 4 `class collapse::Slot<struct app::Logger> `anonymous namespace'::logger`
- 4 `class collapse::Slot<struct app::Controller> `anonymous namespace'::controllerB`
- 4 `class collapse::Slot<struct app::Controller> `anonymous namespace'::controllerA`

</details>

<details><summary>msvc-O2-lto: largest symbols added by sub0_b3_sink (bytes)</summary>

- 64 `private: static __cdecl <lambda_5176b4e72f099bdab8ee92f0dbd06ba6>::<lambda_invoker_cdecl>(void const * __ptr64,struct app::Sample const & __ptr64)`
- 24 `class collapse::Slot<class sub0::Wiring<struct app::Controller,struct app::Controller,struct app::Logger> > `anonymous namespace'::bus`
- 16 `class collapse::Slot<struct app::Sensor> `anonymous namespace'::sensor`
- 4 `class collapse::Slot<struct app::Logger> `anonymous namespace'::logger`
- 4 `class collapse::Slot<struct app::Controller> `anonymous namespace'::controllerB`
- 4 `class collapse::Slot<struct app::Controller> `anonymous namespace'::controllerA`

</details>

<details><summary>msvc-O2-lto: largest symbols added by sub0pub_virtual (bytes)</summary>

- 1180 `__volatile_metadata`
- 368 `collapse_teardown`
- 256 `collapse_setup`
- 152 `SetSmallXmm`
- 80 `collapse_publish`
- 80 `class sub0::detail::SubscriberInterface<struct app::Sample,0> `RTTI Type Descriptor'`
- 72 `private: static struct sub0::detail::Table<struct app::Sample,struct sub0::detail::BuiltinT<8,1,2,0,struct sub0::NoLock> > sub0::detail::BrokerImpl<struct app::Sample,struct sub0::detail::BuiltinT<8,1,2,0,struct sub0::NoLock> >::global_`
- 56 `class sub0::Subscribe<struct app::Sample> `RTTI Type Descriptor'`

</details>

<details><summary>msvc-O2-lto: largest symbols added by sub0pub_virtual_lean (bytes)</summary>

- 1188 `__volatile_metadata`
- 368 `collapse_teardown`
- 256 `collapse_setup`
- 152 `SetSmallXmm`
- 80 `collapse_publish`
- 80 `class sub0::detail::SubscriberInterface<struct app::Sample,0> `RTTI Type Descriptor'`
- 72 `private: static struct sub0::detail::Table<struct app::Sample,struct sub0::detail::BuiltinT<8,1,2,0,struct sub0::NoLock> > sub0::detail::BrokerImpl<struct app::Sample,struct sub0::detail::BuiltinT<8,1,2,0,struct sub0::NoLock> >::global_`
- 56 `class sub0::Subscribe<struct app::Sample> `RTTI Type Descriptor'`

</details>

## Case: dynamic_subscriptions

## Case: filters

## Case: large_payload

## Case: many_receivers

## Case: multi_receivers

## Case: multi_types

## Case: nested_publish

## Case: one_receiver

## Case: publisher_ergonomics

## Case: static_dynamic_bridge

## Case: static_dynamic_bridge_churn

## Case: static_dynamic_bridge_empty

## Case: transport_endpoint

## Case: transport_two_links

## Case: two_domains

## Case: zero_receivers

