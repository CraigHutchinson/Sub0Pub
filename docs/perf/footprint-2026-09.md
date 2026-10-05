# Sub0Pub footprint report

- **host**: `g++ (Ubuntu 13.3.0-6ubuntu2~24.04.1) 13.3.0` `-std=c++17 -Os -fno-exceptions -fno-rtti`
- **cm33**: `arm-none-eabi-g++ (15:13.2.rel1-2) 13.2.1 20231009` `-std=c++17 -Os -fno-exceptions -fno-rtti -mcpu=cortex-m33 -mthumb -mfloat-abi=hard -mfpu=fpv5-sp-d16`

## Target: host

### Object size (text / data / bss bytes)

| Scenario | Direct (default) | Direct + check | Full (snapshot, cancel, filter) | ThreadSafe |
|---|---:|---:|---:|---:|
| 1 type: 1 publisher, 1 subscriber, 1 publish site | 485 / 72 / 106 | 841 / 72 / 114 | 770 / 72 / 114 | 1103 / 72 / 162 |
| +1 subscriber of the same type | 589 / 72 / 105 | 931 / 72 / 113 | 866 / 72 / 113 | 1181 / 72 / 161 |
| +1 publish call site of the same type | 622 / 72 / 89 | 1035 / 72 / 97 | 950 / 72 / 97 | 1301 / 72 / 145 |
| +1 Data type (publisher, subscriber, site) | 911 / 136 / 178 | 1593 / 136 / 194 | 1473 / 136 / 194 | 2232 / 136 / 290 |
| 1 type forwarded to StreamSerializer | 695 / 72 / 105 | 1049 / 72 / 113 | 980 / 72 / 113 | 1313 / 72 / 161 |

### Marginal text bytes

| Added usage | Direct (default) | Direct + check | Full (snapshot, cancel, filter) | ThreadSafe |
|---|---:|---:|---:|---:|
| +1 subscriber of the same type | +104 | +90 | +96 | +78 |
| +1 publish call site of the same type | +137 | +194 | +180 | +198 |
| +1 Data type (publisher, subscriber, site) | +426 | +752 | +703 | +1129 |

### Sub0Pub symbols, 1 type (bytes)

| Symbol | Direct (default) | Direct + check | Full (snapshot, cancel, filter) | ThreadSafe |
|---|---:|---:|---:|---:|
| `SubscriberInterface<MsgA, false>::filter()` | 5 | 5 | - | 5 |
| `vtable for Subscribe<MsgA>` | 32 | 32 | 32 | 32 |
| `BrokerImpl<MsgA, BuiltinT<8u, (Dispatch)1, (Context)2, false, NoLock> >::global_` | 72 | - | - | - |
| `PublishContext<MsgA, (Context)0>::top_` | - | 8 | 8 | 8 |
| `BrokerImpl<MsgA, BuiltinT<8u, ()` | - | 47 | - | - |
| `BrokerImpl<MsgA, BuiltinT<8u, (Dispatch)2, (Context)2, false, NoLock> >::global_` | - | 72 | - | - |
| `SubscriberInterface<MsgA, true>::filter()` | - | - | 7 | - |
| `BrokerImpl<MsgA, BuiltinT<8u, (Dispatch)0, (Context)0, true, NoLock> >::global_` | - | - | 72 | - |
| `void BrokerImpl<MsgA, BuiltinT<8u, ()` | - | - | - | 116 |
| `BrokerImpl<MsgA, BuiltinT<8u, (Dispatch)0, (Context)0, false, StdMutexLock> >::global_` | - | - | - | 120 |

### sizeof (bytes)

- `Publish<T>`: 1
- `Subscribe<T>`: 16

### Link-time dependencies, 1 type (undefined symbols)

- **Direct (default)**: `__cxa_atexit`, `__cxa_pure_virtual`, `__dso_handle`, `__stack_chk_fail`
- **Direct + check**: `_GLOBAL_OFFSET_TABLE_`, `__cxa_atexit`, `__cxa_pure_virtual`, `__dso_handle`, `__stack_chk_fail`, `abort`
- **Full (snapshot, cancel, filter)**: `_GLOBAL_OFFSET_TABLE_`, `__cxa_atexit`, `__cxa_pure_virtual`, `__dso_handle`, `__stack_chk_fail`
- **ThreadSafe**: `_GLOBAL_OFFSET_TABLE_`, `__cxa_atexit`, `__cxa_pure_virtual`, `__dso_handle`, `__stack_chk_fail`, `pthread_mutex_lock`, `pthread_mutex_unlock`, `pthread_self`, `sched_yield`, `std::__throw_system_error(int)`

## Target: cm33

### Object size (text / data / bss bytes)

| Scenario | Direct (default) | Direct + check | Full (snapshot, cancel, filter) | ThreadSafe |
|---|---:|---:|---:|---:|
| 1 type: 1 publisher, 1 subscriber, 1 publish site | 224 / 4 / 58 | 392 / 4 / 62 | 390 / 4 / 62 | n/a |
| +1 subscriber of the same type | 272 / 4 / 61 | 432 / 4 / 65 | 438 / 4 / 65 | n/a |
| +1 publish call site of the same type | 258 / 4 / 49 | 442 / 4 / 53 | 444 / 4 / 53 | n/a |
| +1 Data type (publisher, subscriber, site) | 448 / 4 / 98 | 772 / 4 / 106 | 776 / 4 / 106 | n/a |
| 1 type forwarded to StreamSerializer | 326 / 4 / 53 | 486 / 4 / 57 | 492 / 4 / 57 | n/a |

### Marginal text bytes

| Added usage | Direct (default) | Direct + check | Full (snapshot, cancel, filter) | ThreadSafe |
|---|---:|---:|---:|---:|
| +1 subscriber of the same type | +48 | +40 | +48 | n/a |
| +1 publish call site of the same type | +34 | +50 | +54 | n/a |
| +1 Data type (publisher, subscriber, site) | +224 | +380 | +386 | n/a |

### Sub0Pub symbols, 1 type (bytes)

| Symbol | Direct (default) | Direct + check | Full (snapshot, cancel, filter) | ThreadSafe |
|---|---:|---:|---:|---:|
| `SubscriberInterface<MsgA, false>::filter()` | 2 | 2 | - | - |
| `vtable for Subscribe<MsgA>` | 16 | 16 | 16 | - |
| `BrokerImpl<MsgA, BuiltinT<8ul, (Dispatch)1, (Context)2, false, NoLock> >::global_` | 36 | - | - | - |
| `PublishContext<MsgA, (Context)0>::top_` | - | 4 | 4 | - |
| `BrokerImpl<MsgA, BuiltinT<8ul, (Dispatch)2, (Context)2, false, NoLock> >::global_` | - | 36 | - | - |
| `BrokerImpl<MsgA, BuiltinT<8ul, ()` | - | 40 | - | - |
| `SubscriberInterface<MsgA, true>::filter()` | - | - | 4 | - |
| `BrokerImpl<MsgA, BuiltinT<8ul, (Dispatch)0, (Context)0, true, NoLock> >::global_` | - | - | 36 | - |

### sizeof (bytes)

- `Publish<T>`: 1
- `Subscribe<T>`: 8

### Link-time dependencies, 1 type (undefined symbols)

- **Direct (default)**: `__aeabi_atexit`, `__cxa_pure_virtual`, `__dso_handle`, `memmove`
- **Direct + check**: `__aeabi_atexit`, `__aeabi_read_tp`, `__cxa_pure_virtual`, `__dso_handle`, `abort`, `memmove`
- **Full (snapshot, cancel, filter)**: `__aeabi_atexit`, `__aeabi_read_tp`, `__cxa_pure_virtual`, `__dso_handle`, `memcpy`, `memmove`
- **ThreadSafe**: does not compile: `include/sub0pub/config.hpp:54:14: error: 'mutex' in namespace 'std' does not name a type`

## Runtime broker configuration, one option at a time

Same usage as `fp_1type` (1 type, 1 publisher, 1 subscriber, 1 publish site); compare with the Direct (default) column above.

### Target: host

| Configuration | text / data / bss | `Broker::publish()` | sizeof Subscribe / Publish | Link-time dependencies |
|---|---:|---:|---:|---|
| Lean: Direct, NoContext, NoFilter (the library default) | 485 / 72 / 106 | inlined | 16 / 1 | `__cxa_atexit`, `__cxa_pure_virtual`, `__dso_handle`, `__stack_chk_fail` |
| Full: Snapshot, ThreadLocal context, filter | 770 / 72 / 114 | inlined | 16 / 1 | `_GLOBAL_OFFSET_TABLE_`, `__cxa_atexit`, `__cxa_pure_virtual`, `__dso_handle`, `__stack_chk_fail` |
| Full, Dispatch=Direct | 709 / 72 / 114 | inlined | 16 / 1 | `_GLOBAL_OFFSET_TABLE_`, `__cxa_atexit`, `__cxa_pure_virtual`, `__dso_handle`, `__stack_chk_fail` |
| Full, Dispatch=Direct, Context=Static (no TLS) | 701 / 72 / 114 | inlined | 16 / 1 | `__cxa_atexit`, `__cxa_pure_virtual`, `__dso_handle`, `__stack_chk_fail` |
| Full, Dispatch=DirectChecked | 901 / 72 / 114 | inlined | 16 / 1 | `_GLOBAL_OFFSET_TABLE_`, `__cxa_atexit`, `__cxa_pure_virtual`, `__dso_handle`, `__stack_chk_fail`, `abort` |
| Full, Context=Static | 762 / 72 / 114 | inlined | 16 / 1 | `__cxa_atexit`, `__cxa_pure_virtual`, `__dso_handle`, `__stack_chk_fail` |
| Full, Context=None (must be rejected: Snapshot needs a context) | n/a: `include/sub0pub/broker/broker_impl.hpp:62:68: error: static assertion failed: sub0pub: Snapshot needs a publish context (StaticContext or ThreadLocalContext): a subscriber disconnected during a dispatch is removed from that dispatch's snapshot through its frame` | | | |
| Full, Filter=off | 732 / 72 / 114 | inlined | 16 / 1 | `_GLOBAL_OFFSET_TABLE_`, `__cxa_atexit`, `__cxa_pure_virtual`, `__dso_handle`, `__stack_chk_fail` |
| Full, Lock=spin (RTOS-style yield hook) | 1285 / 72 / 130 | inlined | 16 / 1 | `_GLOBAL_OFFSET_TABLE_`, `__cxa_atexit`, `__cxa_pure_virtual`, `__dso_handle`, `__stack_chk_fail`, `pthread_self` |
| Full, Lock=spin + StaticContext (must be rejected) | n/a: `include/sub0pub/broker/broker_impl.hpp:67:48: error: static assertion failed: sub0pub: a Lock requires ThreadLocalContext (disconnect must not wait on its own dispatch, and a StaticContext frame stack shared by concurrent publishers lets one thread's cancel() and frames act on another thread's dispatch)` | | | |
| Full, Storage=Scoped | 1203 / 72 / 232 | inlined | 24 / 8, Domain 80 | `_GLOBAL_OFFSET_TABLE_`, `__cxa_atexit`, `__cxa_pure_virtual`, `__dso_handle`, `__stack_chk_fail`, `abort`, `memmove` |
| Full, Capacity=64 | 868 / 72 / 562 | 282 | 16 / 1 | `_GLOBAL_OFFSET_TABLE_`, `__cxa_atexit`, `__cxa_pure_virtual`, `__dso_handle`, `__stack_chk_fail` |
| Full, Implementation=SingleSubscriberBroker | 600 / 72 / 50 | inlined | 16 / 1 | `_GLOBAL_OFFSET_TABLE_`, `__cxa_atexit`, `__cxa_pure_virtual`, `__dso_handle`, `__stack_chk_fail` |
| Full + 1 Route | 1058 / 104 / 166 | inlined | 16 / 1, Route 24 | `_GLOBAL_OFFSET_TABLE_`, `__cxa_atexit`, `__cxa_pure_virtual`, `__dso_handle`, `__stack_chk_fail` |
| Full, 2 Data types (marginal cost of a type) | 1465 / 136 / 211 | inlined | 16 / 1 | `_GLOBAL_OFFSET_TABLE_`, `__cxa_atexit`, `__cxa_pure_virtual`, `__dso_handle`, `__stack_chk_fail` |
| Lean, 2 Data types (marginal cost of a type) | 903 / 136 / 195 | inlined | 16 / 1 | `__cxa_atexit`, `__cxa_pure_virtual`, `__dso_handle`, `__stack_chk_fail` |

### Target: cm33

| Configuration | text / data / bss | `Broker::publish()` | sizeof Subscribe / Publish | Link-time dependencies |
|---|---:|---:|---:|---|
| Lean: Direct, NoContext, NoFilter (the library default) | 224 / 4 / 58 | inlined | 8 / 1 | `__aeabi_atexit`, `__cxa_pure_virtual`, `__dso_handle`, `memmove` |
| Full: Snapshot, ThreadLocal context, filter | 390 / 4 / 62 | inlined | 8 / 1 | `__aeabi_atexit`, `__aeabi_read_tp`, `__cxa_pure_virtual`, `__dso_handle`, `memcpy`, `memmove` |
| Full, Dispatch=Direct | 358 / 4 / 62 | inlined | 8 / 1 | `__aeabi_atexit`, `__aeabi_read_tp`, `__cxa_pure_virtual`, `__dso_handle`, `memmove` |
| Full, Dispatch=Direct, Context=Static (no TLS) | 346 / 4 / 62 | inlined | 8 / 1 | `__aeabi_atexit`, `__cxa_pure_virtual`, `__dso_handle`, `memmove` |
| Full, Dispatch=DirectChecked | 422 / 4 / 62 | inlined | 8 / 1 | `__aeabi_atexit`, `__aeabi_read_tp`, `__cxa_pure_virtual`, `__dso_handle`, `abort`, `memmove` |
| Full, Context=Static | 378 / 4 / 62 | inlined | 8 / 1 | `__aeabi_atexit`, `__cxa_pure_virtual`, `__dso_handle`, `memcpy`, `memmove` |
| Full, Context=None (must be rejected: Snapshot needs a context) | n/a: `include/sub0pub/broker/broker_impl.hpp:62:68: error: static assertion failed: sub0pub: Snapshot needs a publish context (StaticContext or ThreadLocalContext): a subscriber disconnected during a dispatch is removed from that dispatch's snapshot through its frame` | | | |
| Full, Filter=off | 360 / 4 / 62 | inlined | 8 / 1 | `__aeabi_atexit`, `__aeabi_read_tp`, `__cxa_pure_virtual`, `__dso_handle`, `memcpy`, `memmove` |
| Full, Lock=spin (RTOS-style yield hook) | 630 / 4 / 70 | inlined | 8 / 1 | `__aeabi_atexit`, `__aeabi_read_tp`, `__cxa_pure_virtual`, `__dso_handle`, `memmove`, `memset` |
| Full, Lock=spin + StaticContext (must be rejected) | n/a: `include/sub0pub/broker/broker_impl.hpp:67:48: error: static assertion failed: sub0pub: a Lock requires ThreadLocalContext (disconnect must not wait on its own dispatch, and a StaticContext frame stack shared by concurrent publishers lets one thread's cancel() and frames act on another thread's dispatch)` | | | |
| Full, Storage=Scoped | 636 / 4 / 128 | inlined | 12 / 4, Domain 44 | `__aeabi_atexit`, `__aeabi_read_tp`, `__cxa_pure_virtual`, `__dso_handle`, `abort`, `memmove` |
| Full, Capacity=64 | 426 / 4 / 286 | 164 | 8 / 1 | `__aeabi_atexit`, `__aeabi_read_tp`, `__cxa_pure_virtual`, `__dso_handle`, `memcpy`, `memmove` |
| Full, Implementation=SingleSubscriberBroker | 294 / 4 / 30 | inlined | 8 / 1 | `__aeabi_atexit`, `__aeabi_read_tp`, `__cxa_pure_virtual`, `__dso_handle` |
| Full + 1 Route | 558 / 4 / 90 | inlined | 8 / 1, Route 12 | `__aeabi_atexit`, `__aeabi_read_tp`, `__cxa_pure_virtual`, `__dso_handle`, `memcpy`, `memmove` |
| Full, 2 Data types (marginal cost of a type) | 776 / 4 / 115 | inlined | 8 / 1 | `__aeabi_atexit`, `__aeabi_read_tp`, `__cxa_pure_virtual`, `__dso_handle`, `memcpy`, `memmove` |
| Lean, 2 Data types (marginal cost of a type) | 444 / 4 / 107 | inlined | 8 / 1 | `__aeabi_atexit`, `__cxa_pure_virtual`, `__dso_handle`, `memmove` |

