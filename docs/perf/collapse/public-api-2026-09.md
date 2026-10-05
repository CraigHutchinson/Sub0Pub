# Collapse evidence

Final-link evidence per case, build and form; every variant is compared with `handwritten` (equal-work reference, same build and form), or with the extra reference it names, shown as `variant (vs handwritten_<kind>)`: e.g. `handwritten_runtime`, hand-written code that reaches its receivers through addresses stored at setup. Deltas in parentheses. instr = callgrind instructions (publish: per publication of 1000). path = static instructions of `collapse_publish` plus directly reachable functions.

- **gcc-O2**: `g++ (Ubuntu 13.3.0-6ubuntu2~24.04.1) 13.3.0` `-O2`
- **clang-O2**: `Ubuntu clang version 18.1.3 (1ubuntu1)` `-O2`
- **cm33-gcc-Os**: `arm-none-eabi-g++ (15:13.2.rel1-2) 13.2.1 20231009` `-Os -mcpu=cortex-m33 -mthumb -mfloat-abi=hard -mfpu=fpv5-sp-d16 -fno-exceptions -fno-rtti -DCOLLAPSE_NO_STDIO`
- **gcc-O2-lto**: `g++ (Ubuntu 13.3.0-6ubuntu2~24.04.1) 13.3.0` `-O2 -flto`
- **clang-O2-lto**: `Ubuntu clang version 18.1.3 (1ubuntu1)` `-O2 -flto`
- **cm33-gcc-Os-lto**: `arm-none-eabi-g++ (15:13.2.rel1-2) 13.2.1 20231009` `-Os -mcpu=cortex-m33 -mthumb -mfloat-abi=hard -mfpu=fpv5-sp-d16 -fno-exceptions -fno-rtti -DCOLLAPSE_NO_STDIO -flto`

## Case: cancellation

### gcc-O2, observable work

| variant | checksum | publish instr | setup instr | teardown instr | path instr | calls (direct/indirect) | text | data+bss | retained sub0 (B) | added deps | verdict |
|---|---|---|---|---|---|---|---|---|---|---|---|
| handwritten | ok | 27.0 (+0.0) | 20 (+0) | 16 (+0) | 29 (+0) | 0/0 | 2431 (+0) | 624 (+0) | 0 | - | reference |
| handwritten_runtime | ok | 31.3 (+4.3) | 26 (+6) | 16 (+0) | 34 (+5) | 0/0 | 2495 (+64) | 656 (+32) | 0 | - | reference; FAIL: publish instr, setup instr, publish path, no extra RAM |
| sub0_b1_cancelable (vs handwritten_runtime) | ok | 30.7 (-0.7) | 26 (+0) | 16 (+0) | 34 (+0) | 0/0 | 2495 (+0) | 656 (+0) | 0 | - | PASS |
| sub0_b2_cancelable | ok | 27.0 (+0.0) | 20 (+0) | 16 (+0) | 29 (+0) | 0/0 | 2431 (+0) | 624 (+0) | 0 | - | PASS |

### gcc-O2, removable work

| variant | checksum | publish instr | setup instr | teardown instr | path instr | calls (direct/indirect) | text | data+bss | retained sub0 (B) | added deps | verdict |
|---|---|---|---|---|---|---|---|---|---|---|---|
| handwritten | ok | 11.7 (+0.0) | 20 (+0) | 16 (+0) | 9 (+0) | 0/0 | 2367 (+0) | 624 (+0) | 0 | - | reference |
| handwritten_runtime | ok | 12.3 (+0.7) | 26 (+6) | 16 (+0) | 10 (+1) | 0/0 | 2415 (+48) | 656 (+32) | 0 | - | reference; FAIL: setup instr, no extra RAM |
| sub0_b1_cancelable (vs handwritten_runtime) | ok | 12.3 (+0.0) | 26 (+0) | 16 (+0) | 10 (+0) | 0/0 | 2415 (+0) | 656 (+0) | 0 | - | PASS |
| sub0_b2_cancelable | ok | 11.7 (+0.0) | 20 (+0) | 16 (+0) | 9 (+0) | 0/0 | 2367 (+0) | 624 (+0) | 0 | - | PASS |

<details><summary>gcc-O2: largest symbols added by handwritten_runtime (bytes)</summary>

- 114 `collapse_publish`
- 67 `collapse_setup`
- 24 `(anonymous namespace)::sensor`
- 1 `(anonymous namespace)::gate`

</details>

### clang-O2, observable work

| variant | checksum | publish instr | setup instr | teardown instr | path instr | calls (direct/indirect) | text | data+bss | retained sub0 (B) | added deps | verdict |
|---|---|---|---|---|---|---|---|---|---|---|---|
| handwritten | ok | 28.7 (+0.0) | 18 (+0) | 14 (+0) | 30 (+0) | 0/0 | 2170 (+0) | 664 (+0) | 0 | - | reference |
| handwritten_runtime | ok | 28.7 (+0.0) | 18 (+0) | 14 (+0) | 30 (+0) | 0/0 | 2170 (+0) | 664 (+0) | 0 | - | reference; PASS |
| sub0_b1_cancelable (vs handwritten_runtime) | ok | 28.7 (+0.0) | 18 (+0) | 14 (+0) | 30 (+0) | 0/0 | 2170 (+0) | 664 (+0) | 0 | - | PASS |
| sub0_b2_cancelable | ok | 28.7 (+0.0) | 18 (+0) | 14 (+0) | 30 (+0) | 0/0 | 2170 (+0) | 664 (+0) | 0 | - | PASS |

### clang-O2, removable work

| variant | checksum | publish instr | setup instr | teardown instr | path instr | calls (direct/indirect) | text | data+bss | retained sub0 (B) | added deps | verdict |
|---|---|---|---|---|---|---|---|---|---|---|---|
| handwritten | ok | 11.7 (+0.0) | 17 (+0) | 14 (+0) | 8 (+0) | 0/0 | 2090 (+0) | 664 (+0) | 0 | - | reference |
| handwritten_runtime | ok | 11.7 (+0.0) | 18 (+1) | 14 (+0) | 8 (+0) | 0/0 | 2106 (+16) | 664 (+0) | 0 | - | reference; FAIL: setup instr |
| sub0_b1_cancelable (vs handwritten_runtime) | ok | 11.7 (+0.0) | 18 (+0) | 14 (+0) | 8 (+0) | 0/0 | 2106 (+0) | 664 (+0) | 0 | - | PASS |
| sub0_b2_cancelable | ok | 11.7 (+0.0) | 17 (+0) | 14 (+0) | 8 (+0) | 0/0 | 2090 (+0) | 664 (+0) | 0 | - | PASS |

### cm33-gcc-Os, observable work

| variant | checksum | publish instr | setup instr | teardown instr | path instr | calls (direct/indirect) | text | data+bss | retained sub0 (B) | added deps | verdict |
|---|---|---|---|---|---|---|---|---|---|---|---|
| handwritten | - | - | - | - | 32 (+0) | 0/0 | 1184 (+0) | 516 (+0) | 0 | - | reference |
| handwritten_runtime | - | - | - | - | 35 (+3) | 0/0 | 1204 (+20) | 532 (+16) | 0 | - | reference; FAIL: publish path, no extra RAM |
| sub0_b1_cancelable (vs handwritten_runtime) | - | - | - | - | 35 (+0) | 0/0 | 1204 (+0) | 532 (+0) | 0 | - | PASS |
| sub0_b2_cancelable | - | - | - | - | 32 (+0) | 0/0 | 1184 (+0) | 516 (+0) | 0 | - | PASS |

### cm33-gcc-Os, removable work

| variant | checksum | publish instr | setup instr | teardown instr | path instr | calls (direct/indirect) | text | data+bss | retained sub0 (B) | added deps | verdict |
|---|---|---|---|---|---|---|---|---|---|---|---|
| handwritten | - | - | - | - | 16 (+0) | 0/0 | 1140 (+0) | 516 (+0) | 0 | - | reference |
| handwritten_runtime | - | - | - | - | 18 (+2) | 0/0 | 1160 (+20) | 532 (+16) | 0 | - | reference; FAIL: no extra RAM |
| sub0_b1_cancelable (vs handwritten_runtime) | - | - | - | - | 18 (+0) | 0/0 | 1160 (+0) | 532 (+0) | 0 | - | PASS |
| sub0_b2_cancelable | - | - | - | - | 16 (+0) | 0/0 | 1140 (+0) | 516 (+0) | 0 | - | PASS |

<details><summary>cm33-gcc-Os: largest symbols added by handwritten_runtime (bytes)</summary>

- 88 `collapse_publish`
- 40 `collapse_setup`
- 12 `(anonymous namespace)::sensor`
- 1 `(anonymous namespace)::gate`

</details>

## Case: cancellation_filtered

### gcc-O2, observable work

| variant | checksum | publish instr | setup instr | teardown instr | path instr | calls (direct/indirect) | text | data+bss | retained sub0 (B) | added deps | verdict |
|---|---|---|---|---|---|---|---|---|---|---|---|
| handwritten | ok | 26.8 (+0.0) | 20 (+0) | 16 (+0) | 31 (+0) | 0/0 | 2447 (+0) | 624 (+0) | 0 | - | reference |
| sub0_b2_cancelable | ok | 27.2 (+0.3) | 20 (+0) | 16 (+0) | 34 (+3) | 0/0 | 2447 (+0) | 624 (+0) | 0 | - | FAIL: publish path |

### gcc-O2, removable work

| variant | checksum | publish instr | setup instr | teardown instr | path instr | calls (direct/indirect) | text | data+bss | retained sub0 (B) | added deps | verdict |
|---|---|---|---|---|---|---|---|---|---|---|---|
| handwritten | ok | 12.3 (+0.0) | 20 (+0) | 16 (+0) | 10 (+0) | 0/0 | 2367 (+0) | 624 (+0) | 0 | - | reference |
| sub0_b2_cancelable | ok | 12.3 (+0.0) | 20 (+0) | 16 (+0) | 10 (+0) | 0/0 | 2367 (+0) | 624 (+0) | 0 | - | PASS |

<details><summary>gcc-O2: largest symbols added by sub0_b2_cancelable (bytes)</summary>

- 121 `collapse_publish`

</details>

### clang-O2, observable work

| variant | checksum | publish instr | setup instr | teardown instr | path instr | calls (direct/indirect) | text | data+bss | retained sub0 (B) | added deps | verdict |
|---|---|---|---|---|---|---|---|---|---|---|---|
| handwritten | ok | 28.8 (+0.0) | 18 (+0) | 14 (+0) | 37 (+0) | 0/0 | 2202 (+0) | 664 (+0) | 0 | - | reference |
| sub0_b2_cancelable | ok | 28.0 (-0.8) | 18 (+0) | 14 (+0) | 34 (-3) | 0/0 | 2186 (-16) | 664 (+0) | 0 | - | PASS |

### clang-O2, removable work

| variant | checksum | publish instr | setup instr | teardown instr | path instr | calls (direct/indirect) | text | data+bss | retained sub0 (B) | added deps | verdict |
|---|---|---|---|---|---|---|---|---|---|---|---|
| handwritten | ok | 14.3 (+0.0) | 17 (+0) | 14 (+0) | 15 (+0) | 0/0 | 2106 (+0) | 664 (+0) | 0 | - | reference |
| sub0_b2_cancelable | ok | 14.3 (+0.0) | 17 (+0) | 14 (+0) | 15 (+0) | 0/0 | 2106 (+0) | 664 (+0) | 0 | - | PASS |

### cm33-gcc-Os, observable work

| variant | checksum | publish instr | setup instr | teardown instr | path instr | calls (direct/indirect) | text | data+bss | retained sub0 (B) | added deps | verdict |
|---|---|---|---|---|---|---|---|---|---|---|---|
| handwritten | - | - | - | - | 36 (+0) | 0/0 | 1192 (+0) | 516 (+0) | 0 | - | reference |
| sub0_b2_cancelable | - | - | - | - | 40 (+4) | 0/0 | 1204 (+12) | 516 (+0) | 0 | - | FAIL: publish path |

### cm33-gcc-Os, removable work

| variant | checksum | publish instr | setup instr | teardown instr | path instr | calls (direct/indirect) | text | data+bss | retained sub0 (B) | added deps | verdict |
|---|---|---|---|---|---|---|---|---|---|---|---|
| handwritten | - | - | - | - | 18 (+0) | 0/0 | 1144 (+0) | 516 (+0) | 0 | - | reference |
| sub0_b2_cancelable | - | - | - | - | 18 (+0) | 0/0 | 1144 (+0) | 516 (+0) | 0 | - | PASS |

<details><summary>cm33-gcc-Os: largest symbols added by sub0_b2_cancelable (bytes)</summary>

- 104 `collapse_publish`

</details>

## Case: cross_file

### gcc-O2, observable work

| variant | checksum | publish instr | setup instr | teardown instr | path instr | calls (direct/indirect) | text | data+bss | retained sub0 (B) | added deps | verdict |
|---|---|---|---|---|---|---|---|---|---|---|---|
| handwritten | ok | 61.0 (+0.0) | 21 (+0) | 16 (+0) | 50 (+0) | 4/0 | 2617 (+0) | 632 (+0) | 0 | - | reference |
| handwritten_erased | ok | 74.0 (+13.0) | 31 (+10) | 16 (+0) | 22 (-28) | 2/1 | 2798 (+181) | 672 (+40) | 0 | - | reference; FAIL: publish instr, setup instr, no extra indirect calls, no extra RAM |
| handwritten_runtime | ok | 61.0 (+0.0) | 27 (+6) | 16 (+0) | 50 (+0) | 4/0 | 2649 (+32) | 656 (+24) | 0 | - | reference; FAIL: setup instr, no extra RAM |
| sub0_b1_wire (vs handwritten_runtime) | ok | 61.0 (+0.0) | 27 (+0) | 16 (+0) | 50 (+0) | 4/0 | 2649 (+0) | 656 (+0) | 0 | - | PASS |
| sub0_b2_static | ok | 61.0 (+0.0) | 21 (+0) | 16 (+0) | 50 (+0) | 4/0 | 2617 (+0) | 632 (+0) | 0 | - | PASS |
| sub0_b3_sink (vs handwritten_erased) | ok | 74.0 (+0.0) | 31 (+0) | 16 (+0) | 22 (+0) | 2/1 | 2798 (+0) | 672 (+0) | 55 | - | PASS |
| sub0pub_virtual | ok | 86.0 (+25.0) | 53 (+32) | 100 (+84) | 37 (-13) | 1/1 | 4487 (+1870) | 944 (+312) | 237 | - | FAIL: publish instr, setup instr, teardown instr, no extra indirect calls, no extra RAM, no Sub0Pub retained |
| sub0pub_virtual_lean | ok | 86.0 (+25.0) | 53 (+32) | 100 (+84) | 37 (-13) | 1/1 | 4487 (+1870) | 944 (+312) | 237 | - | FAIL: publish instr, setup instr, teardown instr, no extra indirect calls, no extra RAM, no Sub0Pub retained |

### gcc-O2, removable work

| variant | checksum | publish instr | setup instr | teardown instr | path instr | calls (direct/indirect) | text | data+bss | retained sub0 (B) | added deps | verdict |
|---|---|---|---|---|---|---|---|---|---|---|---|
| handwritten | ok | 36.0 (+0.0) | 21 (+0) | 16 (+0) | 34 (+0) | 4/0 | 2574 (+0) | 632 (+0) | 0 | - | reference |
| handwritten_erased | ok | 49.0 (+13.0) | 31 (+10) | 16 (+0) | 22 (-12) | 2/1 | 2750 (+176) | 672 (+40) | 0 | - | reference; FAIL: publish instr, setup instr, no extra indirect calls, no extra RAM |
| handwritten_runtime | ok | 36.0 (+0.0) | 27 (+6) | 16 (+0) | 34 (+0) | 4/0 | 2606 (+32) | 656 (+24) | 0 | - | reference; FAIL: setup instr, no extra RAM |
| sub0_b1_wire (vs handwritten_runtime) | ok | 36.0 (+0.0) | 27 (+0) | 16 (+0) | 34 (+0) | 4/0 | 2606 (+0) | 656 (+0) | 0 | - | PASS |
| sub0_b2_static | ok | 36.0 (+0.0) | 21 (+0) | 16 (+0) | 34 (+0) | 4/0 | 2574 (+0) | 632 (+0) | 0 | - | PASS |
| sub0_b3_sink (vs handwritten_erased) | ok | 49.0 (+0.0) | 31 (+0) | 16 (+0) | 22 (+0) | 2/1 | 2750 (+0) | 672 (+0) | 55 | - | PASS |
| sub0pub_virtual | ok | 61.0 (+25.0) | 53 (+32) | 100 (+84) | 37 (+3) | 1/1 | 4443 (+1869) | 944 (+312) | 237 | - | FAIL: publish instr, setup instr, teardown instr, publish path, no extra indirect calls, no extra RAM, no Sub0Pub retained |
| sub0pub_virtual_lean | ok | 61.0 (+25.0) | 53 (+32) | 100 (+84) | 37 (+3) | 1/1 | 4443 (+1869) | 944 (+312) | 237 | - | FAIL: publish instr, setup instr, teardown instr, publish path, no extra indirect calls, no extra RAM, no Sub0Pub retained |

<details><summary>gcc-O2: largest symbols added by handwritten_erased (bytes)</summary>

- 105 `collapse_setup`
- 68 `app::Sensor::send(unsigned int)`
- 54 `(anonymous namespace)::deliverNode(void const*, app::Sample const&)`
- 24 `(anonymous namespace)::node`
- 16 `(anonymous namespace)::sensor`

</details>

<details><summary>gcc-O2: largest symbols added by handwritten_runtime (bytes)</summary>

- 77 `collapse_setup`
- 24 `(anonymous namespace)::sensor`

</details>

<details><summary>gcc-O2: largest symbols added by sub0_b3_sink (bytes)</summary>

- 55 `sub0::Sink<app::Sample>::Sink<sub0::Wiring<app::Controller, app::Controller, app::Logger>, 0>(sub0::Wiring<app::Controller, app::Controller, app::Logger>&)::{lambda(void const*, app::Sample const&)#1}::_FUN(void const*, app::Sample const&)`
- 24 `(anonymous namespace)::bus`

</details>

<details><summary>gcc-O2: largest symbols added by sub0pub_virtual (bytes)</summary>

- 483 `collapse_teardown`
- 343 `collapse_setup`
- 126 `collapse_publish`
- 72 `sub0::detail::BrokerImpl<app::Sample, sub0::detail::BuiltinT<8u, (sub0::Dispatch)1, (sub0::Context)2, false, sub0::NoLock> >::global_`
- 55 `typeinfo name for sub0::detail::SubscriberInterface<app::Sample, false>`
- 37 `app::Logger::receive(app::Sample const&)`
- 33 `typeinfo name for sub0::Subscribe<app::Sample>`
- 32 `vtable for sub0::Subscribe<app::Sample>`

</details>

<details><summary>gcc-O2: largest symbols added by sub0pub_virtual_lean (bytes)</summary>

- 483 `collapse_teardown`
- 343 `collapse_setup`
- 126 `collapse_publish`
- 72 `sub0::detail::BrokerImpl<app::Sample, sub0::detail::BuiltinT<8u, (sub0::Dispatch)1, (sub0::Context)2, false, sub0::NoLock> >::global_`
- 55 `typeinfo name for sub0::detail::SubscriberInterface<app::Sample, false>`
- 37 `app::Logger::receive(app::Sample const&)`
- 33 `typeinfo name for sub0::Subscribe<app::Sample>`
- 32 `vtable for sub0::Subscribe<app::Sample>`

</details>

### clang-O2, observable work

| variant | checksum | publish instr | setup instr | teardown instr | path instr | calls (direct/indirect) | text | data+bss | retained sub0 (B) | added deps | verdict |
|---|---|---|---|---|---|---|---|---|---|---|---|
| handwritten | ok | 55.0 (+0.0) | 19 (+0) | 14 (+0) | 42 (+0) | 3/0 | 2298 (+0) | 672 (+0) | 0 | - | reference |
| handwritten_erased | ok | 67.0 (+12.0) | 29 (+10) | 14 (+0) | 15 (-27) | 1/1 | 2493 (+195) | 712 (+40) | 0 | - | reference; FAIL: publish instr, setup instr, no extra indirect calls, no extra RAM |
| handwritten_runtime | ok | 55.0 (+0.0) | 25 (+6) | 14 (+0) | 42 (+0) | 3/0 | 2346 (+48) | 696 (+24) | 0 | - | reference; FAIL: setup instr, no extra RAM |
| sub0_b1_wire (vs handwritten_runtime) | ok | 55.0 (+0.0) | 25 (+0) | 14 (+0) | 42 (+0) | 3/0 | 2346 (+0) | 696 (+0) | 0 | - | PASS |
| sub0_b2_static | ok | 55.0 (+0.0) | 19 (+0) | 14 (+0) | 42 (+0) | 3/0 | 2298 (+0) | 672 (+0) | 0 | - | PASS |
| sub0_b3_sink (vs handwritten_erased) | ok | 66.0 (-1.0) | 29 (+0) | 14 (+0) | 14 (-1) | 1/1 | 2490 (-3) | 712 (+0) | 0 | - | PASS |
| sub0pub_virtual | ok | 79.0 (+24.0) | 54 (+35) | 95 (+81) | 28 (-14) | 0/1 | 4560 (+2262) | 960 (+288) | 233 | - | FAIL: publish instr, setup instr, teardown instr, no extra indirect calls, no extra RAM, no Sub0Pub retained |
| sub0pub_virtual_lean | ok | 79.0 (+24.0) | 54 (+35) | 95 (+81) | 28 (-14) | 0/1 | 4560 (+2262) | 960 (+288) | 233 | - | FAIL: publish instr, setup instr, teardown instr, no extra indirect calls, no extra RAM, no Sub0Pub retained |

### clang-O2, removable work

| variant | checksum | publish instr | setup instr | teardown instr | path instr | calls (direct/indirect) | text | data+bss | retained sub0 (B) | added deps | verdict |
|---|---|---|---|---|---|---|---|---|---|---|---|
| handwritten | ok | 27.0 (+0.0) | 19 (+0) | 14 (+0) | 24 (+0) | 3/0 | 2256 (+0) | 672 (+0) | 0 | - | reference |
| handwritten_erased | ok | 39.0 (+12.0) | 29 (+10) | 14 (+0) | 15 (-9) | 1/1 | 2461 (+205) | 712 (+40) | 0 | - | reference; FAIL: publish instr, setup instr, no extra indirect calls, no extra RAM |
| handwritten_runtime | ok | 27.0 (+0.0) | 25 (+6) | 14 (+0) | 24 (+0) | 3/0 | 2304 (+48) | 696 (+24) | 0 | - | reference; FAIL: setup instr, no extra RAM |
| sub0_b1_wire (vs handwritten_runtime) | ok | 27.0 (+0.0) | 25 (+0) | 14 (+0) | 24 (+0) | 3/0 | 2304 (+0) | 696 (+0) | 0 | - | PASS |
| sub0_b2_static | ok | 27.0 (+0.0) | 19 (+0) | 14 (+0) | 24 (+0) | 3/0 | 2256 (+0) | 672 (+0) | 0 | - | PASS |
| sub0_b3_sink (vs handwritten_erased) | ok | 38.0 (-1.0) | 29 (+0) | 14 (+0) | 14 (-1) | 1/1 | 2458 (-3) | 712 (+0) | 0 | - | PASS |
| sub0pub_virtual | ok | 51.0 (+24.0) | 54 (+35) | 95 (+81) | 28 (+4) | 0/1 | 4517 (+2261) | 960 (+288) | 233 | - | FAIL: publish instr, setup instr, teardown instr, publish path, no extra indirect calls, no extra RAM, no Sub0Pub retained |
| sub0pub_virtual_lean | ok | 51.0 (+24.0) | 54 (+35) | 95 (+81) | 28 (+4) | 0/1 | 4517 (+2261) | 960 (+288) | 233 | - | FAIL: publish instr, setup instr, teardown instr, publish path, no extra indirect calls, no extra RAM, no Sub0Pub retained |

<details><summary>clang-O2: largest symbols added by handwritten_erased (bytes)</summary>

- 101 `collapse_setup`
- 49 `(anonymous namespace)::deliverNode(void const*, app::Sample const&)`
- 24 `app::Sensor::send(unsigned int)`
- 24 `(anonymous namespace)::node`
- 16 `(anonymous namespace)::sensor`

</details>

<details><summary>clang-O2: largest symbols added by handwritten_runtime (bytes)</summary>

- 73 `collapse_setup`
- 8 `_ZN12_GLOBAL__N_16sensorE.2`
- 8 `_ZN12_GLOBAL__N_16sensorE.1`
- 8 `_ZN12_GLOBAL__N_16sensorE.0`

</details>

<details><summary>clang-O2: largest symbols added by sub0_b3_sink (bytes)</summary>

- 49 `_ZZN4sub04SinkIN3app6SampleEEC1INS_6WiringIJNS1_10ControllerES6_NS1_6LoggerEEEETnNSt9enable_ifIXntsr3stdE9is_same_vINSt9remove_cvIT_E4typeES3_EEiE4typeELi0EEERSB_ENUlPKvRKS2_E_8__invokeESI_SK_`
- 24 `(anonymous namespace)::bus`

</details>

<details><summary>clang-O2: largest symbols added by sub0pub_virtual (bytes)</summary>

- 1078 `collapse_teardown`
- 292 `collapse_setup`
- 85 `collapse_publish`
- 72 `sub0::detail::BrokerImpl<app::Sample, sub0::detail::BuiltinT<8u, (sub0::Dispatch)1, (sub0::Context)2, false, sub0::NoLock> >::global_`
- 55 `typeinfo name for sub0::detail::SubscriberInterface<app::Sample, false>`
- 33 `typeinfo name for sub0::Subscribe<app::Sample>`
- 32 `vtable for sub0::Subscribe<app::Sample>`
- 32 `vtable for app::Logger`

</details>

<details><summary>clang-O2: largest symbols added by sub0pub_virtual_lean (bytes)</summary>

- 1078 `collapse_teardown`
- 292 `collapse_setup`
- 85 `collapse_publish`
- 72 `sub0::detail::BrokerImpl<app::Sample, sub0::detail::BuiltinT<8u, (sub0::Dispatch)1, (sub0::Context)2, false, sub0::NoLock> >::global_`
- 55 `typeinfo name for sub0::detail::SubscriberInterface<app::Sample, false>`
- 33 `typeinfo name for sub0::Subscribe<app::Sample>`
- 32 `vtable for sub0::Subscribe<app::Sample>`
- 32 `vtable for app::Logger`

</details>

### cm33-gcc-Os, observable work

| variant | checksum | publish instr | setup instr | teardown instr | path instr | calls (direct/indirect) | text | data+bss | retained sub0 (B) | added deps | verdict |
|---|---|---|---|---|---|---|---|---|---|---|---|
| handwritten | - | - | - | - | 43 (+0) | 3/0 | 1220 (+0) | 520 (+0) | 0 | - | reference |
| handwritten_erased | - | - | - | - | 17 (-26) | 1/1 | 1264 (+44) | 540 (+20) | 0 | - | reference; FAIL: no extra indirect calls, no extra RAM |
| handwritten_runtime | - | - | - | - | 42 (-1) | 3/0 | 1224 (+4) | 532 (+12) | 0 | - | reference; FAIL: no extra RAM |
| sub0_b1_wire (vs handwritten_runtime) | - | - | - | - | 42 (+0) | 3/0 | 1224 (+0) | 532 (+0) | 0 | - | PASS |
| sub0_b2_static | - | - | - | - | 43 (+0) | 3/0 | 1220 (+0) | 520 (+0) | 0 | - | PASS |
| sub0_b3_sink (vs handwritten_erased) | - | - | - | - | 17 (+0) | 1/1 | 1264 (+0) | 540 (+0) | 32 | - | PASS |
| sub0pub_virtual | - | - | - | - | 23 (-20) | 0/1 | 1712 (+492) | 580 (+60) | 198 | - | FAIL: no extra indirect calls, no extra RAM, no Sub0Pub retained |
| sub0pub_virtual_lean | - | - | - | - | 23 (-20) | 0/1 | 1712 (+492) | 580 (+60) | 198 | - | FAIL: no extra indirect calls, no extra RAM, no Sub0Pub retained |

### cm33-gcc-Os, removable work

| variant | checksum | publish instr | setup instr | teardown instr | path instr | calls (direct/indirect) | text | data+bss | retained sub0 (B) | added deps | verdict |
|---|---|---|---|---|---|---|---|---|---|---|---|
| handwritten | - | - | - | - | 27 (+0) | 3/0 | 1176 (+0) | 520 (+0) | 0 | - | reference |
| handwritten_erased | - | - | - | - | 17 (-10) | 1/1 | 1220 (+44) | 540 (+20) | 0 | - | reference; FAIL: no extra indirect calls, no extra RAM |
| handwritten_runtime | - | - | - | - | 26 (-1) | 3/0 | 1180 (+4) | 532 (+12) | 0 | - | reference; FAIL: no extra RAM |
| sub0_b1_wire (vs handwritten_runtime) | - | - | - | - | 26 (+0) | 3/0 | 1180 (+0) | 532 (+0) | 0 | - | PASS |
| sub0_b2_static | - | - | - | - | 27 (+0) | 3/0 | 1176 (+0) | 520 (+0) | 0 | - | PASS |
| sub0_b3_sink (vs handwritten_erased) | - | - | - | - | 17 (+0) | 1/1 | 1220 (+0) | 540 (+0) | 32 | - | PASS |
| sub0pub_virtual | - | - | - | - | 23 (-4) | 0/1 | 1672 (+496) | 580 (+60) | 198 | - | FAIL: no extra indirect calls, no extra RAM, no Sub0Pub retained |
| sub0pub_virtual_lean | - | - | - | - | 23 (-4) | 0/1 | 1672 (+496) | 580 (+60) | 198 | - | FAIL: no extra indirect calls, no extra RAM, no Sub0Pub retained |

<details><summary>cm33-gcc-Os: largest symbols added by handwritten_erased (bytes)</summary>

- 60 `collapse_setup`
- 32 `(anonymous namespace)::deliverNode(void const*, app::Sample const&)`
- 18 `app::Sensor::send(unsigned long)`
- 12 `(anonymous namespace)::node`
- 8 `(anonymous namespace)::sensor`

</details>

<details><summary>cm33-gcc-Os: largest symbols added by handwritten_runtime (bytes)</summary>

- 44 `collapse_setup`
- 12 `(anonymous namespace)::sensor`

</details>

<details><summary>cm33-gcc-Os: largest symbols added by sub0_b3_sink (bytes)</summary>

- 32 `sub0::Sink<app::Sample>::Sink<sub0::Wiring<app::Controller, app::Controller, app::Logger>, 0>(sub0::Wiring<app::Controller, app::Controller, app::Logger>&)::{lambda(void const*, app::Sample const&)#1}::_FUN(void const*, app::Sample const&)`
- 12 `(anonymous namespace)::bus`

</details>

<details><summary>cm33-gcc-Os: largest symbols added by sub0pub_virtual (bytes)</summary>

- 254 `memmove`
- 80 `sub0::Subscribe<app::Sample>::Subscribe<sub0::detail::BuiltinT<8ul, (sub0::Dispatch)1, (sub0::Context)2, false, sub0::NoLock>, 0>()`
- 76 `collapse_setup`
- 64 `sub0::Subscribe<app::Sample>::disconnect()`
- 48 `collapse_teardown`
- 36 `sub0::detail::BrokerImpl<app::Sample, sub0::detail::BuiltinT<8ul, (sub0::Dispatch)1, (sub0::Context)2, false, sub0::NoLock> >::global_`
- 16 `vtable for sub0::Subscribe<app::Sample>`
- 16 `vtable for app::Logger`

</details>

<details><summary>cm33-gcc-Os: largest symbols added by sub0pub_virtual_lean (bytes)</summary>

- 254 `memmove`
- 80 `sub0::Subscribe<app::Sample>::Subscribe<sub0::detail::BuiltinT<8ul, (sub0::Dispatch)1, (sub0::Context)2, false, sub0::NoLock>, 0>()`
- 76 `collapse_setup`
- 64 `sub0::Subscribe<app::Sample>::disconnect()`
- 48 `collapse_teardown`
- 36 `sub0::detail::BrokerImpl<app::Sample, sub0::detail::BuiltinT<8ul, (sub0::Dispatch)1, (sub0::Context)2, false, sub0::NoLock> >::global_`
- 16 `vtable for sub0::Subscribe<app::Sample>`
- 16 `vtable for app::Logger`

</details>

### gcc-O2-lto, observable work

| variant | checksum | publish instr | setup instr | teardown instr | path instr | calls (direct/indirect) | text | data+bss | retained sub0 (B) | added deps | verdict |
|---|---|---|---|---|---|---|---|---|---|---|---|
| handwritten | ok | 30.0 (+0.0) | 21 (+0) | 11 (+0) | 25 (+0) | 0/0 | 2388 (+0) | 632 (+0) | 0 | - | reference |
| handwritten_erased | ok | 48.0 (+18.0) | 31 (+10) | 11 (+0) | 16 (-9) | 1/1 | 2561 (+173) | 680 (+48) | 0 | - | reference; FAIL: publish instr, setup instr, no extra indirect calls, no extra RAM |
| handwritten_runtime | ok | 33.0 (+3.0) | 27 (+6) | 11 (+0) | 28 (+3) | 0/0 | 2425 (+37) | 664 (+32) | 0 | - | reference; FAIL: publish instr, setup instr, publish path, no extra RAM |
| sub0_b1_wire (vs handwritten_runtime) | ok | 33.0 (+0.0) | 27 (+0) | 11 (+0) | 28 (+0) | 0/0 | 2425 (+0) | 664 (+0) | 0 | - | PASS |
| sub0_b2_static | ok | 30.0 (+0.0) | 21 (+0) | 11 (+0) | 25 (+0) | 0/0 | 2388 (+0) | 632 (+0) | 0 | - | PASS |
| sub0_b3_sink (vs handwritten_erased) | ok | 48.0 (+0.0) | 31 (+0) | 11 (+0) | 16 (+0) | 1/1 | 2561 (+0) | 680 (+0) | 76 | - | PASS |
| sub0pub_virtual | ok | 86.0 (+56.0) | 53 (+32) | 100 (+89) | 37 (+12) | 1/1 | 4492 (+2104) | 984 (+352) | 237 | - | FAIL: publish instr, setup instr, teardown instr, publish path, no extra indirect calls, no extra RAM, no Sub0Pub retained |
| sub0pub_virtual_lean | ok | 86.0 (+56.0) | 53 (+32) | 100 (+89) | 37 (+12) | 1/1 | 4492 (+2104) | 984 (+352) | 237 | - | FAIL: publish instr, setup instr, teardown instr, publish path, no extra indirect calls, no extra RAM, no Sub0Pub retained |

### gcc-O2-lto, removable work

| variant | checksum | publish instr | setup instr | teardown instr | path instr | calls (direct/indirect) | text | data+bss | retained sub0 (B) | added deps | verdict |
|---|---|---|---|---|---|---|---|---|---|---|---|
| handwritten | ok | 9.0 (+0.0) | 21 (+0) | 11 (+0) | 4 (+0) | 0/0 | 2321 (+0) | 632 (+0) | 0 | - | reference |
| handwritten_erased | ok | 24.0 (+15.0) | 31 (+10) | 11 (+0) | 16 (+12) | 1/1 | 2497 (+176) | 680 (+48) | 0 | - | reference; FAIL: publish instr, setup instr, publish path, no extra indirect calls, no extra RAM |
| handwritten_runtime | ok | 10.0 (+1.0) | 27 (+6) | 11 (+0) | 5 (+1) | 0/0 | 2356 (+35) | 664 (+32) | 0 | - | reference; FAIL: setup instr, no extra RAM |
| sub0_b1_wire (vs handwritten_runtime) | ok | 10.0 (+0.0) | 27 (+0) | 11 (+0) | 5 (+0) | 0/0 | 2356 (+0) | 664 (+0) | 0 | - | PASS |
| sub0_b2_static | ok | 9.0 (+0.0) | 21 (+0) | 11 (+0) | 4 (+0) | 0/0 | 2321 (+0) | 632 (+0) | 0 | - | PASS |
| sub0_b3_sink (vs handwritten_erased) | ok | 24.0 (+0.0) | 31 (+0) | 11 (+0) | 16 (+0) | 1/1 | 2497 (+0) | 680 (+0) | 11 | - | PASS |
| sub0pub_virtual | ok | 61.0 (+52.0) | 53 (+32) | 100 (+89) | 37 (+33) | 1/1 | 4444 (+2123) | 984 (+352) | 237 | - | FAIL: publish instr, setup instr, teardown instr, publish path, no extra indirect calls, no extra RAM, no Sub0Pub retained |
| sub0pub_virtual_lean | ok | 61.0 (+52.0) | 53 (+32) | 100 (+89) | 37 (+33) | 1/1 | 4444 (+2123) | 984 (+352) | 237 | - | FAIL: publish instr, setup instr, teardown instr, publish path, no extra indirect calls, no extra RAM, no Sub0Pub retained |

<details><summary>gcc-O2-lto: largest symbols added by handwritten_erased (bytes)</summary>

- 105 `collapse_setup`
- 76 `(anonymous namespace)::deliverNode(void const*, app::Sample const&)`
- 24 `(anonymous namespace)::node`
- 16 `(anonymous namespace)::sensor`

</details>

<details><summary>gcc-O2-lto: largest symbols added by handwritten_runtime (bytes)</summary>

- 91 `collapse_publish`
- 77 `collapse_setup`
- 24 `(anonymous namespace)::sensor`

</details>

<details><summary>gcc-O2-lto: largest symbols added by sub0_b3_sink (bytes)</summary>

- 76 `sub0::Sink<app::Sample>::Sink<sub0::Wiring<app::Controller, app::Controller, app::Logger>, 0>(sub0::Wiring<app::Controller, app::Controller, app::Logger>&)::{lambda(void const*, app::Sample const&)#1}::_FUN(void const*, app::Sample const&)`
- 24 `(anonymous namespace)::bus`

</details>

<details><summary>gcc-O2-lto: largest symbols added by sub0pub_virtual (bytes)</summary>

- 620 `main`
- 483 `collapse_teardown`
- 343 `collapse_setup`
- 126 `collapse_publish`
- 72 `sub0::detail::BrokerImpl<app::Sample, sub0::detail::BuiltinT<8u, (sub0::Dispatch)1, (sub0::Context)2, false, sub0::NoLock> >::global_`
- 55 `typeinfo name for sub0::detail::SubscriberInterface<app::Sample, false>`
- 37 `app::Logger::receive(app::Sample const&)`
- 33 `typeinfo name for sub0::Subscribe<app::Sample>`

</details>

<details><summary>gcc-O2-lto: largest symbols added by sub0pub_virtual_lean (bytes)</summary>

- 620 `main`
- 483 `collapse_teardown`
- 343 `collapse_setup`
- 126 `collapse_publish`
- 72 `sub0::detail::BrokerImpl<app::Sample, sub0::detail::BuiltinT<8u, (sub0::Dispatch)1, (sub0::Context)2, false, sub0::NoLock> >::global_`
- 55 `typeinfo name for sub0::detail::SubscriberInterface<app::Sample, false>`
- 37 `app::Logger::receive(app::Sample const&)`
- 33 `typeinfo name for sub0::Subscribe<app::Sample>`

</details>

### clang-O2-lto, observable work

| variant | checksum | publish instr | setup instr | teardown instr | path instr | calls (direct/indirect) | text | data+bss | retained sub0 (B) | added deps | verdict |
|---|---|---|---|---|---|---|---|---|---|---|---|
| handwritten | ok | 31.0 (+0.0) | 18 (+0) | 10 (+0) | 27 (+0) | 0/0 | 2114 (+0) | 664 (+0) | 0 | - | reference |
| handwritten_erased | ok | 41.0 (+10.0) | 26 (+8) | 10 (+0) | 11 (-16) | 0/1 | 2246 (+132) | 696 (+32) | 0 | - | reference; FAIL: publish instr, setup instr, no extra indirect calls, no extra RAM |
| handwritten_runtime | ok | 31.0 (+0.0) | 18 (+0) | 10 (+0) | 27 (+0) | 0/0 | 2114 (+0) | 664 (+0) | 0 | - | reference; PASS |
| sub0_b1_wire (vs handwritten_runtime) | ok | 31.0 (+0.0) | 18 (+0) | 10 (+0) | 27 (+0) | 0/0 | 2114 (+0) | 664 (+0) | 0 | - | PASS |
| sub0_b2_static | ok | 31.0 (+0.0) | 18 (+0) | 10 (+0) | 27 (+0) | 0/0 | 2114 (+0) | 664 (+0) | 0 | - | PASS |
| sub0_b3_sink (vs handwritten_erased) | ok | 40.0 (-1.0) | 26 (+0) | 10 (+0) | 36 (+25) | 1/0 | 2224 (-22) | 696 (+0) | 0 | - | FAIL: publish path |
| sub0pub_virtual | ok | 75.0 (+44.0) | 51 (+33) | 89 (+79) | 27 (+0) | 0/1 | 4441 (+2327) | 960 (+296) | 233 | - | FAIL: publish instr, setup instr, teardown instr, no extra indirect calls, no extra RAM, no Sub0Pub retained |
| sub0pub_virtual_lean | ok | 75.0 (+44.0) | 51 (+33) | 89 (+79) | 27 (+0) | 0/1 | 4441 (+2327) | 960 (+296) | 233 | - | FAIL: publish instr, setup instr, teardown instr, no extra indirect calls, no extra RAM, no Sub0Pub retained |

### clang-O2-lto, removable work

| variant | checksum | publish instr | setup instr | teardown instr | path instr | calls (direct/indirect) | text | data+bss | retained sub0 (B) | added deps | verdict |
|---|---|---|---|---|---|---|---|---|---|---|---|
| handwritten | ok | 8.0 (+0.0) | 15 (+0) | 10 (+0) | 4 (+0) | 0/0 | 2004 (+0) | 656 (+0) | 0 | - | reference |
| handwritten_erased | ok | 11.0 (+3.0) | 25 (+10) | 10 (+0) | 7 (+3) | 1/0 | 2136 (+132) | 696 (+40) | 0 | - | reference; FAIL: publish instr, setup instr, publish path, no extra RAM |
| handwritten_runtime | ok | 8.0 (+0.0) | 17 (+2) | 10 (+0) | 4 (+0) | 0/0 | 2024 (+20) | 664 (+8) | 0 | - | reference; FAIL: setup instr, no extra RAM |
| sub0_b1_wire (vs handwritten_runtime) | ok | 8.0 (+0.0) | 17 (+0) | 10 (+0) | 4 (+0) | 0/0 | 2024 (+0) | 664 (+0) | 0 | - | PASS |
| sub0_b2_static | ok | 8.0 (+0.0) | 15 (+0) | 10 (+0) | 4 (+0) | 0/0 | 2004 (+0) | 656 (+0) | 0 | - | PASS |
| sub0_b3_sink (vs handwritten_erased) | ok | 11.0 (+0.0) | 25 (+0) | 10 (+0) | 8 (+1) | 1/0 | 2136 (+0) | 696 (+0) | 0 | - | PASS |
| sub0pub_virtual | ok | 50.0 (+42.0) | 50 (+35) | 89 (+79) | 27 (+23) | 0/1 | 4397 (+2393) | 960 (+304) | 233 | - | FAIL: publish instr, setup instr, teardown instr, publish path, no extra indirect calls, no extra RAM, no Sub0Pub retained |
| sub0pub_virtual_lean | ok | 50.0 (+42.0) | 50 (+35) | 89 (+79) | 27 (+23) | 0/1 | 4397 (+2393) | 960 (+304) | 233 | - | FAIL: publish instr, setup instr, teardown instr, publish path, no extra indirect calls, no extra RAM, no Sub0Pub retained |

<details><summary>clang-O2-lto: largest symbols added by handwritten_erased (bytes)</summary>

- 87 `collapse_setup`
- 77 `(anonymous namespace)::deliverNode(void const*, app::Sample const&)`
- 24 `(anonymous namespace)::node`
- 8 `_ZN12_GLOBAL__N_16sensorE.0`
- 4 `(anonymous namespace)::logger`
- 4 `(anonymous namespace)::controllerB`
- 4 `(anonymous namespace)::controllerA`

</details>

<details><summary>clang-O2-lto: largest symbols added by sub0_b3_sink (bytes)</summary>

- 71 `_ZZN4sub04SinkIN3app6SampleEEC1INS_6WiringIJNS1_10ControllerES6_NS1_6LoggerEEEETnNSt9enable_ifIXntsr3stdE9is_same_vINSt9remove_cvIT_E4typeES3_EEiE4typeELi0EEERSB_ENUlPKvRKS2_E_8__invokeESI_SK_`
- 24 `(anonymous namespace)::bus`

</details>

<details><summary>clang-O2-lto: largest symbols added by sub0pub_virtual (bytes)</summary>

- 1008 `collapse_teardown`
- 554 `main`
- 277 `collapse_setup`
- 72 `sub0::detail::BrokerImpl<app::Sample, sub0::detail::BuiltinT<8u, (sub0::Dispatch)1, (sub0::Context)2, false, sub0::NoLock> >::global_`
- 55 `typeinfo name for sub0::detail::SubscriberInterface<app::Sample, false>`
- 33 `typeinfo name for sub0::Subscribe<app::Sample>`
- 32 `vtable for sub0::Subscribe<app::Sample>`
- 32 `vtable for app::Logger`

</details>

<details><summary>clang-O2-lto: largest symbols added by sub0pub_virtual_lean (bytes)</summary>

- 1008 `collapse_teardown`
- 554 `main`
- 277 `collapse_setup`
- 72 `sub0::detail::BrokerImpl<app::Sample, sub0::detail::BuiltinT<8u, (sub0::Dispatch)1, (sub0::Context)2, false, sub0::NoLock> >::global_`
- 55 `typeinfo name for sub0::detail::SubscriberInterface<app::Sample, false>`
- 33 `typeinfo name for sub0::Subscribe<app::Sample>`
- 32 `vtable for sub0::Subscribe<app::Sample>`
- 32 `vtable for app::Logger`

</details>

### cm33-gcc-Os-lto, observable work

| variant | checksum | publish instr | setup instr | teardown instr | path instr | calls (direct/indirect) | text | data+bss | retained sub0 (B) | added deps | verdict |
|---|---|---|---|---|---|---|---|---|---|---|---|
| handwritten | - | - | - | - | 30 (+0) | 0/0 | 1172 (+0) | 520 (+0) | 0 | - | reference |
| handwritten_erased | - | - | - | - | 15 (-15) | 0/1 | 1216 (+44) | 540 (+20) | 0 | - | reference; FAIL: no extra indirect calls, no extra RAM |
| handwritten_runtime | - | - | - | - | 28 (-2) | 0/0 | 1176 (+4) | 532 (+12) | 0 | - | reference; FAIL: no extra RAM |
| sub0_b1_wire (vs handwritten_runtime) | - | - | - | - | 28 (+0) | 0/0 | 1176 (+0) | 532 (+0) | 0 | - | PASS |
| sub0_b2_static | - | - | - | - | 30 (+0) | 0/0 | 1172 (+0) | 520 (+0) | 0 | - | PASS |
| sub0_b3_sink (vs handwritten_erased) | - | - | - | - | 15 (+0) | 0/1 | 1216 (+0) | 540 (+0) | 56 | - | PASS |
| sub0pub_virtual | - | - | - | - | 23 (-7) | 0/1 | 1700 (+528) | 580 (+60) | 150 | - | FAIL: no extra indirect calls, no extra RAM, no Sub0Pub retained |
| sub0pub_virtual_lean | - | - | - | - | 23 (-7) | 0/1 | 1700 (+528) | 580 (+60) | 150 | - | FAIL: no extra indirect calls, no extra RAM, no Sub0Pub retained |

### cm33-gcc-Os-lto, removable work

| variant | checksum | publish instr | setup instr | teardown instr | path instr | calls (direct/indirect) | text | data+bss | retained sub0 (B) | added deps | verdict |
|---|---|---|---|---|---|---|---|---|---|---|---|
| handwritten | - | - | - | - | 12 (+0) | 0/0 | 1116 (+0) | 520 (+0) | 0 | - | reference |
| handwritten_erased | - | - | - | - | 15 (+3) | 0/1 | 1172 (+56) | 540 (+20) | 0 | - | reference; FAIL: publish path, no extra indirect calls, no extra RAM |
| handwritten_runtime | - | - | - | - | 12 (+0) | 0/0 | 1128 (+12) | 532 (+12) | 0 | - | reference; FAIL: no extra RAM |
| sub0_b1_wire (vs handwritten_runtime) | - | - | - | - | 12 (+0) | 0/0 | 1128 (+0) | 532 (+0) | 0 | - | PASS |
| sub0_b2_static | - | - | - | - | 12 (+0) | 0/0 | 1116 (+0) | 520 (+0) | 0 | - | PASS |
| sub0_b3_sink (vs handwritten_erased) | - | - | - | - | 15 (+0) | 0/1 | 1172 (+0) | 540 (+0) | 10 | - | PASS |
| sub0pub_virtual | - | - | - | - | 23 (+11) | 0/1 | 1656 (+540) | 580 (+60) | 150 | - | FAIL: publish path, no extra indirect calls, no extra RAM, no Sub0Pub retained |
| sub0pub_virtual_lean | - | - | - | - | 23 (+11) | 0/1 | 1656 (+540) | 580 (+60) | 150 | - | FAIL: publish path, no extra indirect calls, no extra RAM, no Sub0Pub retained |

<details><summary>cm33-gcc-Os-lto: largest symbols added by handwritten_erased (bytes)</summary>

- 76 `main`
- 60 `collapse_setup`
- 56 `(anonymous namespace)::deliverNode(void const*, app::Sample const&)`
- 12 `(anonymous namespace)::node`
- 8 `(anonymous namespace)::sensor`

</details>

<details><summary>cm33-gcc-Os-lto: largest symbols added by handwritten_runtime (bytes)</summary>

- 44 `collapse_setup`
- 12 `(anonymous namespace)::sensor`

</details>

<details><summary>cm33-gcc-Os-lto: largest symbols added by sub0_b3_sink (bytes)</summary>

- 56 `sub0::Sink<app::Sample>::Sink<sub0::Wiring<app::Controller, app::Controller, app::Logger>, 0>(sub0::Wiring<app::Controller, app::Controller, app::Logger>&)::{lambda(void const*, app::Sample const&)#1}::_FUN(void const*, app::Sample const&)`
- 12 `(anonymous namespace)::bus`

</details>

<details><summary>cm33-gcc-Os-lto: largest symbols added by sub0pub_virtual (bytes)</summary>

- 254 `memmove`
- 84 `main`
- 72 `collapse_setup`
- 64 `sub0::Subscribe<app::Sample>::disconnect()`
- 48 `collapse_teardown`
- 36 `sub0::detail::BrokerImpl<app::Sample, sub0::detail::BuiltinT<8ul, (sub0::Dispatch)1, (sub0::Context)2, false, sub0::NoLock> >::global_`
- 32 `sub0::Subscribe<app::Sample>::trySubscribe() [clone .isra.0]`
- 28 `app::Logger::receive(app::Sample const&)`

</details>

<details><summary>cm33-gcc-Os-lto: largest symbols added by sub0pub_virtual_lean (bytes)</summary>

- 254 `memmove`
- 84 `main`
- 72 `collapse_setup`
- 64 `sub0::Subscribe<app::Sample>::disconnect()`
- 48 `collapse_teardown`
- 36 `sub0::detail::BrokerImpl<app::Sample, sub0::detail::BuiltinT<8ul, (sub0::Dispatch)1, (sub0::Context)2, false, sub0::NoLock> >::global_`
- 32 `sub0::Subscribe<app::Sample>::trySubscribe() [clone .isra.0]`
- 28 `app::Logger::receive(app::Sample const&)`

</details>

## Case: dynamic_subscriptions

### gcc-O2, observable work

| variant | checksum | publish instr | setup instr | teardown instr | path instr | calls (direct/indirect) | text | data+bss | retained sub0 (B) | added deps | verdict |
|---|---|---|---|---|---|---|---|---|---|---|---|
| handwritten | ok | 60.8 (+0.0) | 30 (+0) | 32 (+0) | 87 (+0) | 2/2 | 3676 (+0) | 840 (+0) | 0 | - | reference |
| sub0pub_virtual | ok | 64.3 (+3.5) | 30 (+0) | 36 (+4) | 89 (+2) | 2/2 | 3968 (+292) | 904 (+64) | 227 | - | FAIL: publish instr, teardown instr, no extra RAM, no Sub0Pub retained |
| sub0pub_virtual_lean | ok | 64.3 (+3.5) | 30 (+0) | 36 (+4) | 89 (+2) | 2/2 | 3968 (+292) | 904 (+64) | 227 | - | FAIL: publish instr, teardown instr, no extra RAM, no Sub0Pub retained |

### gcc-O2, removable work

| variant | checksum | publish instr | setup instr | teardown instr | path instr | calls (direct/indirect) | text | data+bss | retained sub0 (B) | added deps | verdict |
|---|---|---|---|---|---|---|---|---|---|---|---|
| handwritten | ok | 51.0 (+0.0) | 30 (+0) | 32 (+0) | 87 (+0) | 2/2 | 3600 (+0) | 840 (+0) | 0 | - | reference |
| sub0pub_virtual | ok | 54.5 (+3.5) | 30 (+0) | 36 (+4) | 89 (+2) | 2/2 | 3848 (+248) | 904 (+64) | 227 | - | FAIL: publish instr, teardown instr, no extra RAM, no Sub0Pub retained |
| sub0pub_virtual_lean | ok | 54.5 (+3.5) | 30 (+0) | 36 (+4) | 89 (+2) | 2/2 | 3848 (+248) | 904 (+64) | 227 | - | FAIL: publish instr, teardown instr, no extra RAM, no Sub0Pub retained |

<details><summary>gcc-O2: largest symbols added by sub0pub_virtual (bytes)</summary>

- 145 `collapse_teardown`
- 72 `sub0::detail::BrokerImpl<(anonymous namespace)::Sample, sub0::detail::BuiltinT<8u, (sub0::Dispatch)1, (sub0::Context)2, false, sub0::NoLock> >::global_`
- 66 `typeinfo name for sub0::detail::SubscriberInterface<(anonymous namespace)::Sample, false>`
- 44 `typeinfo name for sub0::Subscribe<(anonymous namespace)::Sample>`
- 32 `vtable for (anonymous namespace)::Probe`
- 32 `vtable for (anonymous namespace)::Controller`
- 24 `typeinfo for sub0::Subscribe<(anonymous namespace)::Sample>`
- 16 `typeinfo for sub0::detail::SubscriberInterface<(anonymous namespace)::Sample, false>`

</details>

<details><summary>gcc-O2: largest symbols added by sub0pub_virtual_lean (bytes)</summary>

- 145 `collapse_teardown`
- 72 `sub0::detail::BrokerImpl<(anonymous namespace)::Sample, sub0::detail::BuiltinT<8u, (sub0::Dispatch)1, (sub0::Context)2, false, sub0::NoLock> >::global_`
- 66 `typeinfo name for sub0::detail::SubscriberInterface<(anonymous namespace)::Sample, false>`
- 44 `typeinfo name for sub0::Subscribe<(anonymous namespace)::Sample>`
- 32 `vtable for (anonymous namespace)::Probe`
- 32 `vtable for (anonymous namespace)::Controller`
- 24 `typeinfo for sub0::Subscribe<(anonymous namespace)::Sample>`
- 16 `typeinfo for sub0::detail::SubscriberInterface<(anonymous namespace)::Sample, false>`

</details>

### clang-O2, observable work

| variant | checksum | publish instr | setup instr | teardown instr | path instr | calls (direct/indirect) | text | data+bss | retained sub0 (B) | added deps | verdict |
|---|---|---|---|---|---|---|---|---|---|---|---|
| handwritten | ok | 57.0 (+0.0) | 26 (+0) | 30 (+0) | 142 (+0) | 0/2 | 3671 (+0) | 848 (+0) | 0 | - | reference |
| sub0pub_virtual | ok | 55.5 (-1.5) | 28 (+2) | 34 (+4) | 138 (-4) | 0/2 | 4034 (+363) | 928 (+80) | 253 | - | FAIL: setup instr, teardown instr, no extra RAM, no Sub0Pub retained |
| sub0pub_virtual_lean | ok | 55.5 (-1.5) | 28 (+2) | 34 (+4) | 138 (-4) | 0/2 | 4034 (+363) | 928 (+80) | 253 | - | FAIL: setup instr, teardown instr, no extra RAM, no Sub0Pub retained |

### clang-O2, removable work

| variant | checksum | publish instr | setup instr | teardown instr | path instr | calls (direct/indirect) | text | data+bss | retained sub0 (B) | added deps | verdict |
|---|---|---|---|---|---|---|---|---|---|---|---|
| handwritten | ok | 45.8 (+0.0) | 26 (+0) | 30 (+0) | 142 (+0) | 0/2 | 3630 (+0) | 848 (+0) | 0 | - | reference |
| sub0pub_virtual | ok | 44.3 (-1.5) | 28 (+2) | 34 (+4) | 138 (-4) | 0/2 | 3993 (+363) | 928 (+80) | 253 | - | FAIL: setup instr, teardown instr, no extra RAM, no Sub0Pub retained |
| sub0pub_virtual_lean | ok | 44.3 (-1.5) | 28 (+2) | 34 (+4) | 138 (-4) | 0/2 | 3993 (+363) | 928 (+80) | 253 | - | FAIL: setup instr, teardown instr, no extra RAM, no Sub0Pub retained |

<details><summary>clang-O2: largest symbols added by sub0pub_virtual (bytes)</summary>

- 341 `collapse_teardown`
- 73 `collapse_setup`
- 72 `sub0::detail::BrokerImpl<(anonymous namespace)::Sample, sub0::detail::BuiltinT<8u, (sub0::Dispatch)1, (sub0::Context)2, false, sub0::NoLock> >::global_`
- 65 `typeinfo name for sub0::detail::SubscriberInterface<(anonymous namespace)::Sample, false>`
- 43 `typeinfo name for sub0::Subscribe<(anonymous namespace)::Sample>`
- 32 `vtable for sub0::Subscribe<(anonymous namespace)::Sample>`
- 32 `vtable for (anonymous namespace)::Probe`
- 32 `vtable for (anonymous namespace)::Controller`

</details>

<details><summary>clang-O2: largest symbols added by sub0pub_virtual_lean (bytes)</summary>

- 341 `collapse_teardown`
- 73 `collapse_setup`
- 72 `sub0::detail::BrokerImpl<(anonymous namespace)::Sample, sub0::detail::BuiltinT<8u, (sub0::Dispatch)1, (sub0::Context)2, false, sub0::NoLock> >::global_`
- 65 `typeinfo name for sub0::detail::SubscriberInterface<(anonymous namespace)::Sample, false>`
- 43 `typeinfo name for sub0::Subscribe<(anonymous namespace)::Sample>`
- 32 `vtable for sub0::Subscribe<(anonymous namespace)::Sample>`
- 32 `vtable for (anonymous namespace)::Probe`
- 32 `vtable for (anonymous namespace)::Controller`

</details>

### cm33-gcc-Os, observable work

| variant | checksum | publish instr | setup instr | teardown instr | path instr | calls (direct/indirect) | text | data+bss | retained sub0 (B) | added deps | verdict |
|---|---|---|---|---|---|---|---|---|---|---|---|
| handwritten | - | - | - | - | 162 (+0) | 4/1 | 1636 (+0) | 548 (+0) | 0 | - | reference |
| sub0pub_virtual | - | - | - | - | 59 (-103) | 3/2 | 1668 (+32) | 552 (+4) | 166 | - | FAIL: no extra indirect calls, no extra RAM, no Sub0Pub retained |
| sub0pub_virtual_lean | - | - | - | - | 59 (-103) | 3/2 | 1668 (+32) | 552 (+4) | 166 | - | FAIL: no extra indirect calls, no extra RAM, no Sub0Pub retained |

### cm33-gcc-Os, removable work

| variant | checksum | publish instr | setup instr | teardown instr | path instr | calls (direct/indirect) | text | data+bss | retained sub0 (B) | added deps | verdict |
|---|---|---|---|---|---|---|---|---|---|---|---|
| handwritten | - | - | - | - | 138 (+0) | 2/0 | 1520 (+0) | 548 (+0) | 0 | - | reference |
| sub0pub_virtual | - | - | - | - | 35 (-103) | 1/1 | 1548 (+28) | 552 (+4) | 126 | - | FAIL: no extra indirect calls, no extra RAM, no Sub0Pub retained |
| sub0pub_virtual_lean | - | - | - | - | 35 (-103) | 1/1 | 1548 (+28) | 552 (+4) | 126 | - | FAIL: no extra indirect calls, no extra RAM, no Sub0Pub retained |

<details><summary>cm33-gcc-Os: largest symbols added by sub0pub_virtual (bytes)</summary>

- 56 `sub0::detail::BrokerImpl<(anonymous namespace)::Sample, sub0::detail::BuiltinT<8ul, (sub0::Dispatch)1, (sub0::Context)2, false, sub0::NoLock> >::disconnect(sub0::Subscribe<(anonymous namespace)::Sample>*) [clone .isra.0]`
- 40 `void sub0::publish<(anonymous namespace)::Sensor, (anonymous namespace)::Sample>((anonymous namespace)::Sensor&, (anonymous namespace)::Sample const&) [clone .isra.0]`
- 36 `sub0::detail::BrokerImpl<(anonymous namespace)::Sample, sub0::detail::BuiltinT<8ul, (sub0::Dispatch)1, (sub0::Context)2, false, sub0::NoLock> >::global_`
- 32 `sub0::Subscribe<(anonymous namespace)::Sample>::trySubscribe() [clone .isra.0]`
- 24 `collapse_teardown`
- 16 `vtable for (anonymous namespace)::Probe`
- 16 `vtable for (anonymous namespace)::Controller`
- 8 `(anonymous namespace)::controller`

</details>

<details><summary>cm33-gcc-Os: largest symbols added by sub0pub_virtual_lean (bytes)</summary>

- 56 `sub0::detail::BrokerImpl<(anonymous namespace)::Sample, sub0::detail::BuiltinT<8ul, (sub0::Dispatch)1, (sub0::Context)2, false, sub0::NoLock> >::disconnect(sub0::Subscribe<(anonymous namespace)::Sample>*) [clone .isra.0]`
- 40 `void sub0::publish<(anonymous namespace)::Sensor, (anonymous namespace)::Sample>((anonymous namespace)::Sensor&, (anonymous namespace)::Sample const&) [clone .isra.0]`
- 36 `sub0::detail::BrokerImpl<(anonymous namespace)::Sample, sub0::detail::BuiltinT<8ul, (sub0::Dispatch)1, (sub0::Context)2, false, sub0::NoLock> >::global_`
- 32 `sub0::Subscribe<(anonymous namespace)::Sample>::trySubscribe() [clone .isra.0]`
- 24 `collapse_teardown`
- 16 `vtable for (anonymous namespace)::Probe`
- 16 `vtable for (anonymous namespace)::Controller`
- 8 `(anonymous namespace)::controller`

</details>

## Case: filters

### gcc-O2, observable work

| variant | checksum | publish instr | setup instr | teardown instr | path instr | calls (direct/indirect) | text | data+bss | retained sub0 (B) | added deps | verdict |
|---|---|---|---|---|---|---|---|---|---|---|---|
| handwritten | ok | 19.0 (+0.0) | 18 (+0) | 16 (+0) | 18 (+0) | 0/0 | 2367 (+0) | 616 (+0) | 0 | - | reference |
| handwritten_erased | ok | 35.5 (+16.5) | 26 (+8) | 16 (+0) | 17 (-1) | 1/1 | 2531 (+164) | 656 (+40) | 0 | - | reference; FAIL: publish instr, setup instr, no extra indirect calls, no extra RAM |
| handwritten_runtime | ok | 19.0 (+0.0) | 22 (+4) | 16 (+0) | 18 (+0) | 0/0 | 2399 (+32) | 640 (+24) | 0 | - | reference; FAIL: setup instr, no extra RAM |
| sub0_b1_mixin (vs handwritten_runtime) | ok | 19.0 (+0.0) | 22 (+0) | 16 (+0) | 18 (+0) | 0/0 | 2399 (+0) | 640 (+0) | 0 | - | PASS |
| sub0_b1_wire (vs handwritten_runtime) | ok | 19.0 (+0.0) | 22 (+0) | 16 (+0) | 18 (+0) | 0/0 | 2399 (+0) | 640 (+0) | 0 | - | PASS |
| sub0_b2_static | ok | 19.0 (+0.0) | 18 (+0) | 16 (+0) | 18 (+0) | 0/0 | 2367 (+0) | 616 (+0) | 0 | - | PASS |
| sub0_b3_sink (vs handwritten_erased) | ok | 35.5 (+0.0) | 26 (+0) | 16 (+0) | 17 (+0) | 1/1 | 2531 (+0) | 656 (+0) | 55 | - | PASS |
| sub0pub_virtual | ok | 86.5 (+67.5) | 41 (+23) | 61 (+45) | 49 (+31) | 1/2 | 4109 (+1742) | 920 (+304) | 232 | - | FAIL: publish instr, setup instr, teardown instr, publish path, no extra indirect calls, no extra RAM, no Sub0Pub retained |
| sub0pub_virtual_lean | ok | 86.5 (+67.5) | 41 (+23) | 61 (+45) | 49 (+31) | 1/2 | 4109 (+1742) | 920 (+304) | 232 | - | FAIL: publish instr, setup instr, teardown instr, publish path, no extra indirect calls, no extra RAM, no Sub0Pub retained |

### gcc-O2, removable work

| variant | checksum | publish instr | setup instr | teardown instr | path instr | calls (direct/indirect) | text | data+bss | retained sub0 (B) | added deps | verdict |
|---|---|---|---|---|---|---|---|---|---|---|---|
| handwritten | ok | 8.0 (+0.0) | 18 (+0) | 16 (+0) | 4 (+0) | 0/0 | 2319 (+0) | 616 (+0) | 0 | - | reference |
| handwritten_erased | ok | 22.0 (+14.0) | 26 (+8) | 16 (+0) | 17 (+13) | 1/1 | 2483 (+164) | 656 (+40) | 0 | - | reference; FAIL: publish instr, setup instr, publish path, no extra indirect calls, no extra RAM |
| handwritten_runtime | ok | 8.0 (+0.0) | 22 (+4) | 16 (+0) | 4 (+0) | 0/0 | 2351 (+32) | 640 (+24) | 0 | - | reference; FAIL: setup instr, no extra RAM |
| sub0_b1_mixin (vs handwritten_runtime) | ok | 8.0 (+0.0) | 22 (+0) | 16 (+0) | 4 (+0) | 0/0 | 2351 (+0) | 640 (+0) | 0 | - | PASS |
| sub0_b1_wire (vs handwritten_runtime) | ok | 8.0 (+0.0) | 22 (+0) | 16 (+0) | 4 (+0) | 0/0 | 2351 (+0) | 640 (+0) | 0 | - | PASS |
| sub0_b2_static | ok | 8.0 (+0.0) | 18 (+0) | 16 (+0) | 4 (+0) | 0/0 | 2319 (+0) | 616 (+0) | 0 | - | PASS |
| sub0_b3_sink (vs handwritten_erased) | ok | 22.0 (+0.0) | 26 (+0) | 16 (+0) | 17 (+0) | 1/1 | 2483 (+0) | 656 (+0) | 5 | - | PASS |
| sub0pub_virtual | ok | 75.0 (+67.0) | 41 (+23) | 61 (+45) | 49 (+45) | 1/2 | 4033 (+1714) | 920 (+304) | 232 | - | FAIL: publish instr, setup instr, teardown instr, publish path, no extra indirect calls, no extra RAM, no Sub0Pub retained |
| sub0pub_virtual_lean | ok | 75.0 (+67.0) | 41 (+23) | 61 (+45) | 49 (+45) | 1/2 | 4033 (+1714) | 920 (+304) | 232 | - | FAIL: publish instr, setup instr, teardown instr, publish path, no extra indirect calls, no extra RAM, no Sub0Pub retained |

<details><summary>gcc-O2: largest symbols added by handwritten_erased (bytes)</summary>

- 79 `collapse_publish`
- 61 `collapse_setup`
- 55 `(anonymous namespace)::deliverNode(void const*, (anonymous namespace)::Sample const&)`
- 16 `(anonymous namespace)::sensor`
- 16 `(anonymous namespace)::node`
- 1 `(anonymous namespace)::monitor`
- 1 `(anonymous namespace)::controller`

</details>

<details><summary>gcc-O2: largest symbols added by handwritten_runtime (bytes)</summary>

- 33 `collapse_setup`
- 16 `(anonymous namespace)::sensor`
- 1 `(anonymous namespace)::monitor`
- 1 `(anonymous namespace)::controller`

</details>

<details><summary>gcc-O2: largest symbols added by sub0_b3_sink (bytes)</summary>

- 55 `sub0::Sink<(anonymous namespace)::Sample>::Sink<sub0::Wiring<(anonymous namespace)::Controller, (anonymous namespace)::EvenMonitor>, 0>(sub0::Wiring<(anonymous namespace)::Controller, (anonymous namespace)::EvenMonitor>&)::{lambda(void const*, (anonymous namespace)::Sample const&)#1}::_FUN(void const*, (anonymous namespace)::Sample const&)`
- 16 `(anonymous namespace)::bus`

</details>

<details><summary>gcc-O2: largest symbols added by sub0pub_virtual (bytes)</summary>

- 283 `collapse_teardown`
- 164 `collapse_setup`
- 162 `collapse_publish`
- 72 `sub0::detail::BrokerImpl<(anonymous namespace)::Sample, sub0::detail::BuiltinT<8u, (sub0::Dispatch)1, (sub0::Context)2, true, sub0::NoLock> >::global_`
- 66 `typeinfo name for sub0::detail::SubscriberInterface<(anonymous namespace)::Sample, true>`
- 44 `typeinfo name for sub0::Subscribe<(anonymous namespace)::Sample>`
- 32 `vtable for (anonymous namespace)::EvenMonitor`
- 32 `vtable for (anonymous namespace)::Controller`

</details>

<details><summary>gcc-O2: largest symbols added by sub0pub_virtual_lean (bytes)</summary>

- 283 `collapse_teardown`
- 164 `collapse_setup`
- 162 `collapse_publish`
- 72 `sub0::detail::BrokerImpl<(anonymous namespace)::Sample, sub0::detail::BuiltinT<8u, (sub0::Dispatch)1, (sub0::Context)2, true, sub0::NoLock> >::global_`
- 66 `typeinfo name for sub0::detail::SubscriberInterface<(anonymous namespace)::Sample, true>`
- 44 `typeinfo name for sub0::Subscribe<(anonymous namespace)::Sample>`
- 32 `vtable for (anonymous namespace)::EvenMonitor`
- 32 `vtable for (anonymous namespace)::Controller`

</details>

### clang-O2, observable work

| variant | checksum | publish instr | setup instr | teardown instr | path instr | calls (direct/indirect) | text | data+bss | retained sub0 (B) | added deps | verdict |
|---|---|---|---|---|---|---|---|---|---|---|---|
| handwritten | ok | 22.0 (+0.0) | 16 (+0) | 14 (+0) | 19 (+0) | 0/0 | 2122 (+0) | 656 (+0) | 0 | - | reference |
| handwritten_erased | ok | 31.0 (+9.0) | 22 (+6) | 14 (+0) | 12 (-7) | 0/1 | 2218 (+96) | 688 (+32) | 0 | - | reference; FAIL: publish instr, setup instr, no extra indirect calls, no extra RAM |
| handwritten_runtime | ok | 22.0 (+0.0) | 16 (+0) | 14 (+0) | 19 (+0) | 0/0 | 2122 (+0) | 656 (+0) | 0 | - | reference; PASS |
| sub0_b1_mixin (vs handwritten_runtime) | ok | 22.0 (+0.0) | 16 (+0) | 14 (+0) | 19 (+0) | 0/0 | 2122 (+0) | 656 (+0) | 0 | - | PASS |
| sub0_b1_wire (vs handwritten_runtime) | ok | 22.0 (+0.0) | 16 (+0) | 14 (+0) | 19 (+0) | 0/0 | 2122 (+0) | 656 (+0) | 0 | - | PASS |
| sub0_b2_static | ok | 22.0 (+0.0) | 16 (+0) | 14 (+0) | 19 (+0) | 0/0 | 2122 (+0) | 656 (+0) | 0 | - | PASS |
| sub0_b3_sink (vs handwritten_erased) | ok | 30.0 (-1.0) | 22 (+0) | 14 (+0) | 30 (+18) | 1/0 | 2233 (+15) | 688 (+0) | 0 | - | FAIL: publish path |
| sub0pub_virtual | ok | 77.5 (+55.5) | 39 (+23) | 61 (+47) | 41 (+22) | 0/2 | 4178 (+2056) | 944 (+288) | 255 | - | FAIL: publish instr, setup instr, teardown instr, publish path, no extra indirect calls, no extra RAM, no Sub0Pub retained |
| sub0pub_virtual_lean | ok | 77.5 (+55.5) | 39 (+23) | 61 (+47) | 41 (+22) | 0/2 | 4178 (+2056) | 944 (+288) | 255 | - | FAIL: publish instr, setup instr, teardown instr, publish path, no extra indirect calls, no extra RAM, no Sub0Pub retained |

### clang-O2, removable work

| variant | checksum | publish instr | setup instr | teardown instr | path instr | calls (direct/indirect) | text | data+bss | retained sub0 (B) | added deps | verdict |
|---|---|---|---|---|---|---|---|---|---|---|---|
| handwritten | ok | 8.0 (+0.0) | 16 (+0) | 14 (+0) | 4 (+0) | 0/0 | 2074 (+0) | 656 (+0) | 0 | - | reference |
| handwritten_erased | ok | 8.0 (+0.0) | 22 (+6) | 14 (+0) | 4 (+0) | 0/0 | 2106 (+32) | 688 (+32) | 0 | - | reference; FAIL: setup instr, no extra RAM |
| handwritten_runtime | ok | 8.0 (+0.0) | 16 (+0) | 14 (+0) | 4 (+0) | 0/0 | 2074 (+0) | 656 (+0) | 0 | - | reference; PASS |
| sub0_b1_mixin (vs handwritten_runtime) | ok | 8.0 (+0.0) | 16 (+0) | 14 (+0) | 4 (+0) | 0/0 | 2074 (+0) | 656 (+0) | 0 | - | PASS |
| sub0_b1_wire (vs handwritten_runtime) | ok | 8.0 (+0.0) | 16 (+0) | 14 (+0) | 4 (+0) | 0/0 | 2074 (+0) | 656 (+0) | 0 | - | PASS |
| sub0_b2_static | ok | 8.0 (+0.0) | 16 (+0) | 14 (+0) | 4 (+0) | 0/0 | 2074 (+0) | 656 (+0) | 0 | - | PASS |
| sub0_b3_sink (vs handwritten_erased) | ok | 8.0 (+0.0) | 22 (+0) | 14 (+0) | 4 (+0) | 0/0 | 2106 (+0) | 688 (+0) | 0 | - | PASS |
| sub0pub_virtual | ok | 64.0 (+56.0) | 39 (+23) | 61 (+47) | 41 (+37) | 0/2 | 4146 (+2072) | 944 (+288) | 255 | - | FAIL: publish instr, setup instr, teardown instr, publish path, no extra indirect calls, no extra RAM, no Sub0Pub retained |
| sub0pub_virtual_lean | ok | 64.0 (+56.0) | 39 (+23) | 61 (+47) | 41 (+37) | 0/2 | 4146 (+2072) | 944 (+288) | 255 | - | FAIL: publish instr, setup instr, teardown instr, publish path, no extra indirect calls, no extra RAM, no Sub0Pub retained |

<details><summary>clang-O2: largest symbols added by handwritten_erased (bytes)</summary>

- 48 `(anonymous namespace)::deliverNode(void const*, (anonymous namespace)::Sample const&)`
- 43 `collapse_setup`
- 16 `(anonymous namespace)::node`
- 8 `_ZN12_GLOBAL__N_16sensorE.0`
- 1 `(anonymous namespace)::monitor`
- 1 `(anonymous namespace)::controller`

</details>

<details><summary>clang-O2: largest symbols added by sub0_b3_sink (bytes)</summary>

- 48 `_ZZN4sub04SinkIN12_GLOBAL__N_16SampleEEC1INS_6WiringIJNS1_10ControllerENS1_11EvenMonitorEEEETnNSt9enable_ifIXntsr3stdE9is_same_vINSt9remove_cvIT_E4typeES3_EEiE4typeELi0EEERSB_ENUlPKvRKS2_E_8__invokeESI_SK_`
- 16 `(anonymous namespace)::bus`

</details>

<details><summary>clang-O2: largest symbols added by sub0pub_virtual (bytes)</summary>

- 709 `collapse_teardown`
- 161 `collapse_setup`
- 115 `collapse_publish`
- 72 `sub0::detail::BrokerImpl<(anonymous namespace)::Sample, sub0::detail::BuiltinT<8u, (sub0::Dispatch)1, (sub0::Context)2, true, sub0::NoLock> >::global_`
- 65 `typeinfo name for sub0::detail::SubscriberInterface<(anonymous namespace)::Sample, true>`
- 43 `typeinfo name for sub0::Subscribe<(anonymous namespace)::Sample>`
- 32 `vtable for sub0::Subscribe<(anonymous namespace)::Sample>`
- 32 `vtable for (anonymous namespace)::EvenMonitor`

</details>

<details><summary>clang-O2: largest symbols added by sub0pub_virtual_lean (bytes)</summary>

- 709 `collapse_teardown`
- 161 `collapse_setup`
- 115 `collapse_publish`
- 72 `sub0::detail::BrokerImpl<(anonymous namespace)::Sample, sub0::detail::BuiltinT<8u, (sub0::Dispatch)1, (sub0::Context)2, true, sub0::NoLock> >::global_`
- 65 `typeinfo name for sub0::detail::SubscriberInterface<(anonymous namespace)::Sample, true>`
- 43 `typeinfo name for sub0::Subscribe<(anonymous namespace)::Sample>`
- 32 `vtable for sub0::Subscribe<(anonymous namespace)::Sample>`
- 32 `vtable for (anonymous namespace)::EvenMonitor`

</details>

### cm33-gcc-Os, observable work

| variant | checksum | publish instr | setup instr | teardown instr | path instr | calls (direct/indirect) | text | data+bss | retained sub0 (B) | added deps | verdict |
|---|---|---|---|---|---|---|---|---|---|---|---|
| handwritten | - | - | - | - | 19 (+0) | 0/0 | 1128 (+0) | 508 (+0) | 0 | - | reference |
| handwritten_erased | - | - | - | - | 15 (-4) | 0/1 | 1192 (+64) | 528 (+20) | 0 | - | reference; FAIL: no extra indirect calls, no extra RAM |
| handwritten_runtime | - | - | - | - | 19 (+0) | 0/0 | 1148 (+20) | 520 (+12) | 0 | - | reference; FAIL: no extra RAM |
| sub0_b1_mixin (vs handwritten_runtime) | - | - | - | - | 19 (+0) | 0/0 | 1148 (+0) | 520 (+0) | 0 | - | PASS |
| sub0_b1_wire (vs handwritten_runtime) | - | - | - | - | 19 (+0) | 0/0 | 1148 (+0) | 520 (+0) | 0 | - | PASS |
| sub0_b2_static | - | - | - | - | 19 (+0) | 0/0 | 1128 (+0) | 508 (+0) | 0 | - | PASS |
| sub0_b3_sink (vs handwritten_erased) | - | - | - | - | 15 (+0) | 0/1 | 1192 (+0) | 528 (+0) | 40 | - | PASS |
| sub0pub_virtual | - | - | - | - | 34 (+15) | 0/2 | 1672 (+544) | 560 (+52) | 128 | - | FAIL: publish path, no extra indirect calls, no extra RAM, no Sub0Pub retained |
| sub0pub_virtual_lean | - | - | - | - | 34 (+15) | 0/2 | 1672 (+544) | 560 (+52) | 128 | - | FAIL: publish path, no extra indirect calls, no extra RAM, no Sub0Pub retained |

### cm33-gcc-Os, removable work

| variant | checksum | publish instr | setup instr | teardown instr | path instr | calls (direct/indirect) | text | data+bss | retained sub0 (B) | added deps | verdict |
|---|---|---|---|---|---|---|---|---|---|---|---|
| handwritten | - | - | - | - | 7 (+0) | 0/0 | 1096 (+0) | 508 (+0) | 0 | - | reference |
| handwritten_erased | - | - | - | - | 15 (+8) | 0/1 | 1156 (+60) | 528 (+20) | 0 | - | reference; FAIL: publish path, no extra indirect calls, no extra RAM |
| handwritten_runtime | - | - | - | - | 7 (+0) | 0/0 | 1116 (+20) | 520 (+12) | 0 | - | reference; FAIL: no extra RAM |
| sub0_b1_mixin (vs handwritten_runtime) | - | - | - | - | 7 (+0) | 0/0 | 1116 (+0) | 520 (+0) | 0 | - | PASS |
| sub0_b1_wire (vs handwritten_runtime) | - | - | - | - | 7 (+0) | 0/0 | 1116 (+0) | 520 (+0) | 0 | - | PASS |
| sub0_b2_static | - | - | - | - | 7 (+0) | 0/0 | 1096 (+0) | 508 (+0) | 0 | - | PASS |
| sub0_b3_sink (vs handwritten_erased) | - | - | - | - | 15 (+0) | 0/1 | 1156 (+0) | 528 (+0) | 2 | - | PASS |
| sub0pub_virtual | - | - | - | - | 34 (+27) | 0/2 | 1624 (+528) | 560 (+52) | 128 | - | FAIL: publish path, no extra indirect calls, no extra RAM, no Sub0Pub retained |
| sub0pub_virtual_lean | - | - | - | - | 34 (+27) | 0/2 | 1624 (+528) | 560 (+52) | 128 | - | FAIL: publish path, no extra indirect calls, no extra RAM, no Sub0Pub retained |

<details><summary>cm33-gcc-Os: largest symbols added by handwritten_erased (bytes)</summary>

- 40 `collapse_setup`
- 40 `(anonymous namespace)::deliverNode(void const*, (anonymous namespace)::Sample const&)`
- 8 `(anonymous namespace)::sensor`
- 8 `(anonymous namespace)::node`
- 1 `(anonymous namespace)::monitor`
- 1 `(anonymous namespace)::controller`

</details>

<details><summary>cm33-gcc-Os: largest symbols added by handwritten_runtime (bytes)</summary>

- 24 `collapse_setup`
- 8 `(anonymous namespace)::sensor`
- 1 `(anonymous namespace)::monitor`
- 1 `(anonymous namespace)::controller`

</details>

<details><summary>cm33-gcc-Os: largest symbols added by sub0_b3_sink (bytes)</summary>

- 40 `sub0::Sink<(anonymous namespace)::Sample>::Sink<sub0::Wiring<(anonymous namespace)::Controller, (anonymous namespace)::EvenMonitor>, 0>(sub0::Wiring<(anonymous namespace)::Controller, (anonymous namespace)::EvenMonitor>&)::{lambda(void const*, (anonymous namespace)::Sample const&)#1}::_FUN(void const*, (anonymous namespace)::Sample const&)`
- 8 `(anonymous namespace)::bus`

</details>

<details><summary>cm33-gcc-Os: largest symbols added by sub0pub_virtual (bytes)</summary>

- 254 `memmove`
- 72 `collapse_publish`
- 56 `sub0::detail::BrokerImpl<(anonymous namespace)::Sample, sub0::detail::BuiltinT<8ul, (sub0::Dispatch)1, (sub0::Context)2, true, sub0::NoLock> >::disconnect(sub0::Subscribe<(anonymous namespace)::Sample>*) [clone .isra.0]`
- 48 `collapse_setup`
- 40 `collapse_teardown`
- 36 `sub0::detail::BrokerImpl<(anonymous namespace)::Sample, sub0::detail::BuiltinT<8ul, (sub0::Dispatch)1, (sub0::Context)2, true, sub0::NoLock> >::global_`
- 32 `sub0::Subscribe<(anonymous namespace)::Sample>::trySubscribe() [clone .isra.0]`
- 24 `(anonymous namespace)::EvenMonitor::receive((anonymous namespace)::Sample const&)`

</details>

<details><summary>cm33-gcc-Os: largest symbols added by sub0pub_virtual_lean (bytes)</summary>

- 254 `memmove`
- 72 `collapse_publish`
- 56 `sub0::detail::BrokerImpl<(anonymous namespace)::Sample, sub0::detail::BuiltinT<8ul, (sub0::Dispatch)1, (sub0::Context)2, true, sub0::NoLock> >::disconnect(sub0::Subscribe<(anonymous namespace)::Sample>*) [clone .isra.0]`
- 48 `collapse_setup`
- 40 `collapse_teardown`
- 36 `sub0::detail::BrokerImpl<(anonymous namespace)::Sample, sub0::detail::BuiltinT<8ul, (sub0::Dispatch)1, (sub0::Context)2, true, sub0::NoLock> >::global_`
- 32 `sub0::Subscribe<(anonymous namespace)::Sample>::trySubscribe() [clone .isra.0]`
- 24 `(anonymous namespace)::EvenMonitor::receive((anonymous namespace)::Sample const&)`

</details>

## Case: large_payload

### gcc-O2, observable work

| variant | checksum | publish instr | setup instr | teardown instr | path instr | calls (direct/indirect) | text | data+bss | retained sub0 (B) | added deps | verdict |
|---|---|---|---|---|---|---|---|---|---|---|---|
| handwritten | ok | 67.0 (+0.0) | 19 (+0) | 16 (+0) | 43 (+0) | 1/0 | 2529 (+0) | 624 (+0) | 0 | - | reference |
| handwritten_erased | ok | 77.0 (+10.0) | 27 (+8) | 16 (+0) | 31 (-12) | 1/1 | 2669 (+140) | 656 (+32) | 0 | - | reference; FAIL: publish instr, setup instr, no extra indirect calls, no extra RAM |
| handwritten_runtime | ok | 70.0 (+3.0) | 23 (+4) | 16 (+0) | 46 (+3) | 1/0 | 2577 (+48) | 640 (+16) | 0 | - | reference; FAIL: publish instr, setup instr, publish path, no extra RAM |
| sub0_b1_mixin (vs handwritten_runtime) | ok | 70.0 (+0.0) | 23 (+0) | 16 (+0) | 46 (+0) | 1/0 | 2577 (+0) | 640 (+0) | 0 | - | PASS |
| sub0_b1_wire (vs handwritten_runtime) | ok | 70.0 (+0.0) | 23 (+0) | 16 (+0) | 46 (+0) | 1/0 | 2577 (+0) | 640 (+0) | 0 | - | PASS |
| sub0_b2_static | ok | 67.0 (+0.0) | 19 (+0) | 16 (+0) | 43 (+0) | 1/0 | 2529 (+0) | 624 (+0) | 0 | - | PASS |
| sub0_b3_sink (vs handwritten_erased) | ok | 76.0 (-1.0) | 27 (+0) | 16 (+0) | 30 (-1) | 1/1 | 2669 (+0) | 656 (+0) | 70 | - | PASS |
| sub0pub_virtual | ok | 103.0 (+36.0) | 42 (+23) | 61 (+45) | 49 (+6) | 1/1 | 4138 (+1609) | 920 (+296) | 225 | - | FAIL: publish instr, setup instr, teardown instr, publish path, no extra indirect calls, no extra RAM, no Sub0Pub retained |
| sub0pub_virtual_lean | ok | 103.0 (+36.0) | 42 (+23) | 61 (+45) | 49 (+6) | 1/1 | 4138 (+1609) | 920 (+296) | 225 | - | FAIL: publish instr, setup instr, teardown instr, publish path, no extra indirect calls, no extra RAM, no Sub0Pub retained |

### gcc-O2, removable work

| variant | checksum | publish instr | setup instr | teardown instr | path instr | calls (direct/indirect) | text | data+bss | retained sub0 (B) | added deps | verdict |
|---|---|---|---|---|---|---|---|---|---|---|---|
| handwritten | ok | 9.0 (+0.0) | 19 (+0) | 16 (+0) | 6 (+0) | 0/0 | 2335 (+0) | 624 (+0) | 0 | - | reference |
| handwritten_erased | ok | 59.0 (+50.0) | 27 (+8) | 16 (+0) | 31 (+25) | 1/1 | 2605 (+270) | 656 (+32) | 0 | - | reference; FAIL: publish instr, setup instr, publish path, no extra indirect calls, no extra RAM |
| handwritten_runtime | ok | 10.0 (+1.0) | 23 (+4) | 16 (+0) | 6 (+0) | 0/0 | 2367 (+32) | 640 (+16) | 0 | - | reference; FAIL: setup instr, no extra RAM |
| sub0_b1_mixin (vs handwritten_runtime) | ok | 10.0 (+0.0) | 23 (+0) | 16 (+0) | 6 (+0) | 0/0 | 2367 (+0) | 640 (+0) | 0 | - | PASS |
| sub0_b1_wire (vs handwritten_runtime) | ok | 10.0 (+0.0) | 23 (+0) | 16 (+0) | 6 (+0) | 0/0 | 2367 (+0) | 640 (+0) | 0 | - | PASS |
| sub0_b2_static | ok | 9.0 (+0.0) | 19 (+0) | 16 (+0) | 6 (+0) | 0/0 | 2335 (+0) | 624 (+0) | 0 | - | PASS |
| sub0_b3_sink (vs handwritten_erased) | ok | 57.0 (-2.0) | 27 (+0) | 16 (+0) | 30 (-1) | 1/1 | 2605 (+0) | 656 (+0) | 11 | - | PASS |
| sub0pub_virtual | ok | 85.0 (+76.0) | 42 (+23) | 61 (+45) | 49 (+43) | 1/1 | 4030 (+1695) | 920 (+296) | 225 | - | FAIL: publish instr, setup instr, teardown instr, publish path, no extra indirect calls, no extra RAM, no Sub0Pub retained |
| sub0pub_virtual_lean | ok | 85.0 (+76.0) | 42 (+23) | 61 (+45) | 49 (+43) | 1/1 | 4030 (+1695) | 920 (+296) | 225 | - | FAIL: publish instr, setup instr, teardown instr, publish path, no extra indirect calls, no extra RAM, no Sub0Pub retained |

<details><summary>gcc-O2: largest symbols added by handwritten_erased (bytes)</summary>

- 71 `collapse_setup`
- 69 `(anonymous namespace)::deliverNode(void const*, (anonymous namespace)::Frame const&)`
- 16 `(anonymous namespace)::sensor`
- 16 `(anonymous namespace)::node`
- 1 `(anonymous namespace)::controller`

</details>

<details><summary>gcc-O2: largest symbols added by handwritten_runtime (bytes)</summary>

- 185 `collapse_publish`
- 43 `collapse_setup`
- 16 `(anonymous namespace)::sensor`
- 1 `(anonymous namespace)::controller`

</details>

<details><summary>gcc-O2: largest symbols added by sub0_b2_static (bytes)</summary>

- 175 `collapse_publish`

</details>

<details><summary>gcc-O2: largest symbols added by sub0_b3_sink (bytes)</summary>

- 70 `sub0::Sink<(anonymous namespace)::Frame>::Sink<sub0::Wiring<(anonymous namespace)::Controller, (anonymous namespace)::Logger>, 0>(sub0::Wiring<(anonymous namespace)::Controller, (anonymous namespace)::Logger>&)::{lambda(void const*, (anonymous namespace)::Frame const&)#1}::_FUN(void const*, (anonymous namespace)::Frame const&)`
- 16 `(anonymous namespace)::bus`

</details>

<details><summary>gcc-O2: largest symbols added by sub0pub_virtual (bytes)</summary>

- 283 `collapse_teardown`
- 183 `collapse_publish`
- 167 `collapse_setup`
- 72 `sub0::detail::BrokerImpl<(anonymous namespace)::Frame, sub0::detail::BuiltinT<8u, (sub0::Dispatch)1, (sub0::Context)2, false, sub0::NoLock> >::global_`
- 65 `typeinfo name for sub0::detail::SubscriberInterface<(anonymous namespace)::Frame, false>`
- 43 `typeinfo name for sub0::Subscribe<(anonymous namespace)::Frame>`
- 38 `(anonymous namespace)::Logger::receive((anonymous namespace)::Frame const&)`
- 34 `(anonymous namespace)::Controller::receive((anonymous namespace)::Frame const&)`

</details>

<details><summary>gcc-O2: largest symbols added by sub0pub_virtual_lean (bytes)</summary>

- 283 `collapse_teardown`
- 183 `collapse_publish`
- 167 `collapse_setup`
- 72 `sub0::detail::BrokerImpl<(anonymous namespace)::Frame, sub0::detail::BuiltinT<8u, (sub0::Dispatch)1, (sub0::Context)2, false, sub0::NoLock> >::global_`
- 65 `typeinfo name for sub0::detail::SubscriberInterface<(anonymous namespace)::Frame, false>`
- 43 `typeinfo name for sub0::Subscribe<(anonymous namespace)::Frame>`
- 38 `(anonymous namespace)::Logger::receive((anonymous namespace)::Frame const&)`
- 34 `(anonymous namespace)::Controller::receive((anonymous namespace)::Frame const&)`

</details>

### clang-O2, observable work

| variant | checksum | publish instr | setup instr | teardown instr | path instr | calls (direct/indirect) | text | data+bss | retained sub0 (B) | added deps | verdict |
|---|---|---|---|---|---|---|---|---|---|---|---|
| handwritten | ok | 26.0 (+0.0) | 17 (+0) | 14 (+0) | 23 (+0) | 0/0 | 2138 (+0) | 664 (+0) | 0 | - | reference |
| handwritten_erased | ok | 53.0 (+27.0) | 23 (+6) | 14 (+0) | 27 (+4) | 0/1 | 2389 (+251) | 688 (+24) | 0 | - | reference; FAIL: publish instr, setup instr, publish path, no extra indirect calls, no extra RAM |
| handwritten_runtime | ok | 26.0 (+0.0) | 17 (+0) | 14 (+0) | 23 (+0) | 0/0 | 2138 (+0) | 664 (+0) | 0 | - | reference; PASS |
| sub0_b1_mixin (vs handwritten_runtime) | ok | 26.0 (+0.0) | 17 (+0) | 14 (+0) | 23 (+0) | 0/0 | 2138 (+0) | 664 (+0) | 0 | - | PASS |
| sub0_b1_wire (vs handwritten_runtime) | ok | 26.0 (+0.0) | 17 (+0) | 14 (+0) | 23 (+0) | 0/0 | 2138 (+0) | 664 (+0) | 0 | - | PASS |
| sub0_b2_static | ok | 26.0 (+0.0) | 17 (+0) | 14 (+0) | 23 (+0) | 0/0 | 2138 (+0) | 664 (+0) | 0 | - | PASS |
| sub0_b3_sink (vs handwritten_erased) | ok | 52.0 (-1.0) | 23 (+0) | 14 (+0) | 48 (+21) | 1/0 | 2399 (+10) | 688 (+0) | 0 | - | FAIL: publish path |
| sub0pub_virtual | ok | 78.0 (+52.0) | 40 (+23) | 61 (+47) | 44 (+21) | 0/1 | 4234 (+2096) | 944 (+280) | 251 | - | FAIL: publish instr, setup instr, teardown instr, publish path, no extra indirect calls, no extra RAM, no Sub0Pub retained |
| sub0pub_virtual_lean | ok | 78.0 (+52.0) | 40 (+23) | 61 (+47) | 44 (+21) | 0/1 | 4234 (+2096) | 944 (+280) | 251 | - | FAIL: publish instr, setup instr, teardown instr, publish path, no extra indirect calls, no extra RAM, no Sub0Pub retained |

### clang-O2, removable work

| variant | checksum | publish instr | setup instr | teardown instr | path instr | calls (direct/indirect) | text | data+bss | retained sub0 (B) | added deps | verdict |
|---|---|---|---|---|---|---|---|---|---|---|---|
| handwritten | ok | 9.0 (+0.0) | 17 (+0) | 14 (+0) | 4 (+0) | 0/0 | 2074 (+0) | 664 (+0) | 0 | - | reference |
| handwritten_erased | ok | 34.0 (+25.0) | 23 (+6) | 14 (+0) | 27 (+23) | 0/1 | 2341 (+267) | 688 (+24) | 0 | - | reference; FAIL: publish instr, setup instr, publish path, no extra indirect calls, no extra RAM |
| handwritten_runtime | ok | 9.0 (+0.0) | 17 (+0) | 14 (+0) | 4 (+0) | 0/0 | 2074 (+0) | 664 (+0) | 0 | - | reference; PASS |
| sub0_b1_mixin (vs handwritten_runtime) | ok | 9.0 (+0.0) | 17 (+0) | 14 (+0) | 4 (+0) | 0/0 | 2074 (+0) | 664 (+0) | 0 | - | PASS |
| sub0_b1_wire (vs handwritten_runtime) | ok | 9.0 (+0.0) | 17 (+0) | 14 (+0) | 4 (+0) | 0/0 | 2074 (+0) | 664 (+0) | 0 | - | PASS |
| sub0_b2_static | ok | 9.0 (+0.0) | 17 (+0) | 14 (+0) | 4 (+0) | 0/0 | 2074 (+0) | 664 (+0) | 0 | - | PASS |
| sub0_b3_sink (vs handwritten_erased) | ok | 33.0 (-1.0) | 23 (+0) | 14 (+0) | 29 (+2) | 1/0 | 2346 (+5) | 688 (+0) | 0 | - | PASS |
| sub0pub_virtual | ok | 58.0 (+49.0) | 40 (+23) | 61 (+47) | 44 (+40) | 0/1 | 4190 (+2116) | 944 (+280) | 251 | - | FAIL: publish instr, setup instr, teardown instr, publish path, no extra indirect calls, no extra RAM, no Sub0Pub retained |
| sub0pub_virtual_lean | ok | 58.0 (+49.0) | 40 (+23) | 61 (+47) | 44 (+40) | 0/1 | 4190 (+2116) | 944 (+280) | 251 | - | FAIL: publish instr, setup instr, teardown instr, publish path, no extra indirect calls, no extra RAM, no Sub0Pub retained |

<details><summary>clang-O2: largest symbols added by handwritten_erased (bytes)</summary>

- 122 `collapse_publish`
- 60 `(anonymous namespace)::deliverNode(void const*, (anonymous namespace)::Frame const&)`
- 53 `collapse_setup`
- 16 `(anonymous namespace)::node`
- 8 `_ZN12_GLOBAL__N_16sensorE.0`
- 4 `(anonymous namespace)::logger`
- 1 `(anonymous namespace)::controller`

</details>

<details><summary>clang-O2: largest symbols added by sub0_b3_sink (bytes)</summary>

- 59 `_ZZN4sub04SinkIN12_GLOBAL__N_15FrameEEC1INS_6WiringIJNS1_10ControllerENS1_6LoggerEEEETnNSt9enable_ifIXntsr3stdE9is_same_vINSt9remove_cvIT_E4typeES3_EEiE4typeELi0EEERSB_ENUlPKvRKS2_E_8__invokeESI_SK_`
- 16 `(anonymous namespace)::bus`

</details>

<details><summary>clang-O2: largest symbols added by sub0pub_virtual (bytes)</summary>

- 709 `collapse_teardown`
- 171 `collapse_setup`
- 165 `collapse_publish`
- 72 `sub0::detail::BrokerImpl<(anonymous namespace)::Frame, sub0::detail::BuiltinT<8u, (sub0::Dispatch)1, (sub0::Context)2, false, sub0::NoLock> >::global_`
- 64 `typeinfo name for sub0::detail::SubscriberInterface<(anonymous namespace)::Frame, false>`
- 42 `typeinfo name for sub0::Subscribe<(anonymous namespace)::Frame>`
- 32 `vtable for sub0::Subscribe<(anonymous namespace)::Frame>`
- 32 `vtable for (anonymous namespace)::Logger`

</details>

<details><summary>clang-O2: largest symbols added by sub0pub_virtual_lean (bytes)</summary>

- 709 `collapse_teardown`
- 171 `collapse_setup`
- 165 `collapse_publish`
- 72 `sub0::detail::BrokerImpl<(anonymous namespace)::Frame, sub0::detail::BuiltinT<8u, (sub0::Dispatch)1, (sub0::Context)2, false, sub0::NoLock> >::global_`
- 64 `typeinfo name for sub0::detail::SubscriberInterface<(anonymous namespace)::Frame, false>`
- 42 `typeinfo name for sub0::Subscribe<(anonymous namespace)::Frame>`
- 32 `vtable for sub0::Subscribe<(anonymous namespace)::Frame>`
- 32 `vtable for (anonymous namespace)::Logger`

</details>

### cm33-gcc-Os, observable work

| variant | checksum | publish instr | setup instr | teardown instr | path instr | calls (direct/indirect) | text | data+bss | retained sub0 (B) | added deps | verdict |
|---|---|---|---|---|---|---|---|---|---|---|---|
| handwritten | - | - | - | - | 35 (+0) | 0/0 | 1172 (+0) | 512 (+0) | 0 | - | reference |
| handwritten_erased | - | - | - | - | 22 (-13) | 0/1 | 1224 (+52) | 532 (+20) | 0 | - | reference; FAIL: no extra indirect calls, no extra RAM |
| handwritten_runtime | - | - | - | - | 37 (+2) | 0/0 | 1192 (+20) | 524 (+12) | 0 | - | reference; FAIL: no extra RAM |
| sub0_b1_mixin (vs handwritten_runtime) | - | - | - | - | 37 (+0) | 0/0 | 1192 (+0) | 524 (+0) | 0 | - | PASS |
| sub0_b1_wire (vs handwritten_runtime) | - | - | - | - | 37 (+0) | 0/0 | 1192 (+0) | 524 (+0) | 0 | - | PASS |
| sub0_b2_static | - | - | - | - | 35 (+0) | 0/0 | 1172 (+0) | 512 (+0) | 0 | - | PASS |
| sub0_b3_sink (vs handwritten_erased) | - | - | - | - | 22 (+0) | 0/1 | 1224 (+0) | 532 (+0) | 52 | - | PASS |
| sub0pub_virtual | - | - | - | - | 32 (-3) | 0/1 | 1668 (+496) | 564 (+52) | 126 | - | FAIL: no extra indirect calls, no extra RAM, no Sub0Pub retained |
| sub0pub_virtual_lean | - | - | - | - | 32 (-3) | 0/1 | 1668 (+496) | 564 (+52) | 126 | - | FAIL: no extra indirect calls, no extra RAM, no Sub0Pub retained |

### cm33-gcc-Os, removable work

| variant | checksum | publish instr | setup instr | teardown instr | path instr | calls (direct/indirect) | text | data+bss | retained sub0 (B) | added deps | verdict |
|---|---|---|---|---|---|---|---|---|---|---|---|
| handwritten | - | - | - | - | 12 (+0) | 0/0 | 1116 (+0) | 512 (+0) | 0 | - | reference |
| handwritten_erased | - | - | - | - | 22 (+10) | 0/1 | 1184 (+68) | 532 (+20) | 0 | - | reference; FAIL: publish path, no extra indirect calls, no extra RAM |
| handwritten_runtime | - | - | - | - | 12 (+0) | 0/0 | 1132 (+16) | 524 (+12) | 0 | - | reference; FAIL: no extra RAM |
| sub0_b1_mixin (vs handwritten_runtime) | - | - | - | - | 12 (+0) | 0/0 | 1132 (+0) | 524 (+0) | 0 | - | PASS |
| sub0_b1_wire (vs handwritten_runtime) | - | - | - | - | 12 (+0) | 0/0 | 1132 (+0) | 524 (+0) | 0 | - | PASS |
| sub0_b2_static | - | - | - | - | 12 (+0) | 0/0 | 1116 (+0) | 512 (+0) | 0 | - | PASS |
| sub0_b3_sink (vs handwritten_erased) | - | - | - | - | 22 (+0) | 0/1 | 1184 (+0) | 532 (+0) | 10 | - | PASS |
| sub0pub_virtual | - | - | - | - | 32 (+20) | 0/1 | 1620 (+504) | 564 (+52) | 126 | - | FAIL: publish path, no extra indirect calls, no extra RAM, no Sub0Pub retained |
| sub0pub_virtual_lean | - | - | - | - | 32 (+20) | 0/1 | 1620 (+504) | 564 (+52) | 126 | - | FAIL: publish path, no extra indirect calls, no extra RAM, no Sub0Pub retained |

<details><summary>cm33-gcc-Os: largest symbols added by handwritten_erased (bytes)</summary>

- 52 `(anonymous namespace)::deliverNode(void const*, (anonymous namespace)::Frame const&)`
- 44 `collapse_setup`
- 8 `(anonymous namespace)::sensor`
- 8 `(anonymous namespace)::node`
- 1 `(anonymous namespace)::controller`

</details>

<details><summary>cm33-gcc-Os: largest symbols added by handwritten_runtime (bytes)</summary>

- 88 `collapse_publish`
- 28 `collapse_setup`
- 8 `(anonymous namespace)::sensor`
- 1 `(anonymous namespace)::controller`

</details>

<details><summary>cm33-gcc-Os: largest symbols added by sub0_b3_sink (bytes)</summary>

- 52 `sub0::Sink<(anonymous namespace)::Frame>::Sink<sub0::Wiring<(anonymous namespace)::Controller, (anonymous namespace)::Logger>, 0>(sub0::Wiring<(anonymous namespace)::Controller, (anonymous namespace)::Logger>&)::{lambda(void const*, (anonymous namespace)::Frame const&)#1}::_FUN(void const*, (anonymous namespace)::Frame const&)`
- 8 `(anonymous namespace)::bus`

</details>

<details><summary>cm33-gcc-Os: largest symbols added by sub0pub_virtual (bytes)</summary>

- 254 `memmove`
- 56 `sub0::detail::BrokerImpl<(anonymous namespace)::Frame, sub0::detail::BuiltinT<8ul, (sub0::Dispatch)1, (sub0::Context)2, false, sub0::NoLock> >::disconnect(sub0::Subscribe<(anonymous namespace)::Frame>*) [clone .isra.0]`
- 48 `collapse_setup`
- 40 `collapse_teardown`
- 36 `sub0::detail::BrokerImpl<(anonymous namespace)::Frame, sub0::detail::BuiltinT<8ul, (sub0::Dispatch)1, (sub0::Context)2, false, sub0::NoLock> >::global_`
- 32 `sub0::Subscribe<(anonymous namespace)::Frame>::trySubscribe() [clone .isra.0]`
- 28 `(anonymous namespace)::Logger::receive((anonymous namespace)::Frame const&)`
- 28 `(anonymous namespace)::Controller::receive((anonymous namespace)::Frame const&)`

</details>

<details><summary>cm33-gcc-Os: largest symbols added by sub0pub_virtual_lean (bytes)</summary>

- 254 `memmove`
- 56 `sub0::detail::BrokerImpl<(anonymous namespace)::Frame, sub0::detail::BuiltinT<8ul, (sub0::Dispatch)1, (sub0::Context)2, false, sub0::NoLock> >::disconnect(sub0::Subscribe<(anonymous namespace)::Frame>*) [clone .isra.0]`
- 48 `collapse_setup`
- 40 `collapse_teardown`
- 36 `sub0::detail::BrokerImpl<(anonymous namespace)::Frame, sub0::detail::BuiltinT<8ul, (sub0::Dispatch)1, (sub0::Context)2, false, sub0::NoLock> >::global_`
- 32 `sub0::Subscribe<(anonymous namespace)::Frame>::trySubscribe() [clone .isra.0]`
- 28 `(anonymous namespace)::Logger::receive((anonymous namespace)::Frame const&)`
- 28 `(anonymous namespace)::Controller::receive((anonymous namespace)::Frame const&)`

</details>

## Case: many_receivers

### gcc-O2, observable work

| variant | checksum | publish instr | setup instr | teardown instr | path instr | calls (direct/indirect) | text | data+bss | retained sub0 (B) | added deps | verdict |
|---|---|---|---|---|---|---|---|---|---|---|---|
| handwritten | ok | 202.0 (+0.0) | 50 (+0) | 16 (+0) | 198 (+0) | 0/0 | 3231 (+0) | 744 (+0) | 0 | - | reference |
| handwritten_erased | ok | 282.0 (+80.0) | 118 (+68) | 16 (+0) | 17 (-181) | 1/1 | 4067 (+836) | 1016 (+272) | 0 | - | reference; FAIL: publish instr, setup instr, no extra indirect calls, no extra RAM |
| handwritten_loop | ok | 332.0 (+130.0) | 80 (+30) | 67 (+51) | 18 (-180) | 0/0 | 2518 (-713) | 760 (+16) | 0 | - | reference; FAIL: publish instr, setup instr, teardown instr, no extra RAM |
| handwritten_runtime | ok | 296.0 (+94.0) | 114 (+64) | 16 (+0) | 291 (+93) | 0/0 | 4015 (+784) | 1016 (+272) | 0 | - | reference; FAIL: publish instr, setup instr, publish path, no extra RAM |
| sub0_b1_mixin (vs handwritten_runtime) | ok | 296.0 (+0.0) | 114 (+0) | 16 (+0) | 291 (+0) | 0/0 | 4015 (+0) | 1016 (+0) | 0 | - | PASS |
| sub0_b1_wire (vs handwritten_runtime) | ok | 296.0 (+0.0) | 114 (+0) | 16 (+0) | 291 (+0) | 0/0 | 4015 (+0) | 1016 (+0) | 0 | - | PASS |
| sub0_b2_static | ok | 202.0 (+0.0) | 50 (+0) | 16 (+0) | 198 (+0) | 0/0 | 3231 (+0) | 744 (+0) | 0 | - | PASS |
| sub0_b3_sink (vs handwritten_erased) | ok | 282.0 (+0.0) | 118 (+0) | 16 (+0) | 17 (+0) | 1/1 | 4067 (+0) | 1016 (+0) | 834 | - | PASS |
| sub0pub_virtual | ok | 336.0 (+134.0) | 341 (+291) | 1656 (+1640) | 23 (-175) | 0/0 | 10776 (+7545) | 1536 (+792) | 419 | - | FAIL: publish instr, setup instr, teardown instr, no extra RAM, no Sub0Pub retained |
| sub0pub_virtual_lean | ok | 336.0 (+134.0) | 341 (+291) | 1656 (+1640) | 23 (-175) | 0/0 | 10776 (+7545) | 1536 (+792) | 419 | - | FAIL: publish instr, setup instr, teardown instr, no extra RAM, no Sub0Pub retained |

### gcc-O2, removable work

| variant | checksum | publish instr | setup instr | teardown instr | path instr | calls (direct/indirect) | text | data+bss | retained sub0 (B) | added deps | verdict |
|---|---|---|---|---|---|---|---|---|---|---|---|
| handwritten | ok | 8.0 (+0.0) | 50 (+0) | 16 (+0) | 4 (+0) | 0/0 | 2639 (+0) | 744 (+0) | 0 | - | reference |
| handwritten_erased | ok | 22.0 (+14.0) | 118 (+68) | 16 (+0) | 17 (+13) | 1/1 | 3235 (+596) | 1016 (+272) | 0 | - | reference; FAIL: publish instr, setup instr, publish path, no extra indirect calls, no extra RAM |
| handwritten_loop | ok | 59.0 (+51.0) | 80 (+30) | 67 (+51) | 10 (+6) | 0/0 | 2502 (-137) | 760 (+16) | 0 | - | reference; FAIL: publish instr, setup instr, teardown instr, publish path, no extra RAM |
| handwritten_runtime | ok | 8.0 (+0.0) | 114 (+64) | 16 (+0) | 4 (+0) | 0/0 | 3087 (+448) | 1016 (+272) | 0 | - | reference; FAIL: setup instr, no extra RAM |
| sub0_b1_mixin (vs handwritten_runtime) | ok | 8.0 (+0.0) | 114 (+0) | 16 (+0) | 4 (+0) | 0/0 | 3087 (+0) | 1016 (+0) | 0 | - | PASS |
| sub0_b1_wire (vs handwritten_runtime) | ok | 8.0 (+0.0) | 114 (+0) | 16 (+0) | 4 (+0) | 0/0 | 3087 (+0) | 1016 (+0) | 0 | - | PASS |
| sub0_b2_static | ok | 8.0 (+0.0) | 50 (+0) | 16 (+0) | 4 (+0) | 0/0 | 2639 (+0) | 744 (+0) | 0 | - | PASS |
| sub0_b3_sink (vs handwritten_erased) | ok | 22.0 (+0.0) | 118 (+0) | 16 (+0) | 17 (+0) | 1/1 | 3235 (+0) | 1016 (+0) | 5 | - | PASS |
| sub0pub_virtual | ok | 8.0 (+0.0) | 341 (+291) | 1656 (+1640) | 4 (+0) | 0/0 | 10636 (+7997) | 1536 (+792) | 419 | - | FAIL: setup instr, teardown instr, no extra RAM, no Sub0Pub retained |
| sub0pub_virtual_lean | ok | 8.0 (+0.0) | 341 (+291) | 1656 (+1640) | 4 (+0) | 0/0 | 10636 (+7997) | 1536 (+792) | 419 | - | FAIL: setup instr, teardown instr, no extra RAM, no Sub0Pub retained |

<details><summary>gcc-O2: largest symbols added by handwritten_erased (bytes)</summary>

- 834 `(anonymous namespace)::deliverNode(void const*, (anonymous namespace)::Sample const&)`
- 801 `collapse_setup`
- 256 `(anonymous namespace)::node`
- 16 `(anonymous namespace)::sensor`

</details>

<details><summary>gcc-O2: largest symbols added by handwritten_loop (bytes)</summary>

- 128 `(anonymous namespace)::controllers`
- 34 `collapse_teardown`

</details>

<details><summary>gcc-O2: largest symbols added by handwritten_runtime (bytes)</summary>

- 944 `collapse_publish`
- 773 `collapse_setup`
- 256 `(anonymous namespace)::sensor`

</details>

<details><summary>gcc-O2: largest symbols added by sub0_b3_sink (bytes)</summary>

- 834 `sub0::Sink<(anonymous namespace)::Sample>::Sink<sub0::Wiring<(anonymous namespace)::Controller, (anonymous namespace)::Controller, (anonymous namespace)::Controller, (anonymous namespace)::Controller, (anonymous namespace)::Controller, (anonymous namespace)::Controller, (anonymous namespace)::Controller, (anonymous namespace)::Controller, (anonymous namespace)::Controller, (anonymous namespace)::Controller, (anonymous namespace)::Controller, (anonymous namespace)::Controller, (anonymous namespace)::Controller, (anonymous namespace)::Controller, (anonymous namespace)::Controller, (anonymous namespace)::Controller, (anonymous namespace)::Controller, (anonymous namespace)::Controller, (anonymous namespace)::Controller, (anonymous namespace)::Controller, (anonymous namespace)::Controller, (anonymous namespace)::Controller, (anonymous namespace)::Controller, (anonymous namespace)::Controller, (anonymous namespace)::Controller, (anonymous namespace)::Controller, (anonymous namespace)::Controller, (anonymous namespace)::Controller, (anonymous namespace)::Controller, (anonymous namespace)::Controller, (anonymous namespace)::Controller, (anonymous namespace)::Controller>, 0>(sub0::Wiring<(anonymous namespace)::Controller, (anonymous namespace)::Controller, (anonymous namespace)::Controller, (anonymous namespace)::Controller, (anonymous namespace)::Controller, (anonymous namespace)::Controller, (anonymous namespace)::Controller, (anonymous namespace)::Controller, (anonymous namespace)::Controller, (anonymous namespace)::Controller, (anonymous namespace)::Controller, (anonymous namespace)::Controller, (anonymous namespace)::Controller, (anonymous namespace)::Controller, (anonymous namespace)::Controller, (anonymous namespace)::Controller, (anonymous namespace)::Controller, (anonymous namespace)::Controller, (anonymous namespace)::Controller, (anonymous namespace)::Controller, (anonymous namespace)::Controller, (anonymous namespace)::Controller, (anonymous namespace)::Controller, (anonymous namespace)::Controller, (anonymous namespace)::Controller, (anonymous namespace)::Controller, (anonymous namespace)::Controller, (anonymous namespace)::Controller, (anonymous namespace)::Controller, (anonymous namespace)::Controller, (anonymous namespace)::Controller, (anonymous namespace)::Controller>&)::{lambda(void const*, (anonymous namespace)::Sample const&)#1}::_FUN(void const*, (anonymous namespace)::Sample const&)`
- 256 `(anonymous namespace)::bus`

</details>

<details><summary>gcc-O2: largest symbols added by sub0pub_virtual (bytes)</summary>

- 4747 `collapse_teardown`
- 2773 `collapse_setup`
- 264 `sub0::detail::BrokerImpl<(anonymous namespace)::Sample, sub0::detail::BuiltinT<32u, (sub0::Dispatch)1, (sub0::Context)2, false, sub0::NoLock> >::global_`
- 66 `typeinfo name for sub0::detail::SubscriberInterface<(anonymous namespace)::Sample, false>`
- 44 `typeinfo name for sub0::Subscribe<(anonymous namespace)::Sample>`
- 32 `vtable for (anonymous namespace)::Controller`
- 32 `(anonymous namespace)::Controller::receive((anonymous namespace)::Sample const&)`
- 30 `typeinfo name for (anonymous namespace)::Controller`

</details>

<details><summary>gcc-O2: largest symbols added by sub0pub_virtual_lean (bytes)</summary>

- 4747 `collapse_teardown`
- 2773 `collapse_setup`
- 264 `sub0::detail::BrokerImpl<(anonymous namespace)::Sample, sub0::detail::BuiltinT<32u, (sub0::Dispatch)1, (sub0::Context)2, false, sub0::NoLock> >::global_`
- 66 `typeinfo name for sub0::detail::SubscriberInterface<(anonymous namespace)::Sample, false>`
- 44 `typeinfo name for sub0::Subscribe<(anonymous namespace)::Sample>`
- 32 `vtable for (anonymous namespace)::Controller`
- 32 `(anonymous namespace)::Controller::receive((anonymous namespace)::Sample const&)`
- 30 `typeinfo name for (anonymous namespace)::Controller`

</details>

### clang-O2, observable work

| variant | checksum | publish instr | setup instr | teardown instr | path instr | calls (direct/indirect) | text | data+bss | retained sub0 (B) | added deps | verdict |
|---|---|---|---|---|---|---|---|---|---|---|---|
| handwritten | ok | 256.0 (+0.0) | 48 (+0) | 14 (+0) | 251 (+0) | 0/0 | 3210 (+0) | 784 (+0) | 0 | - | reference |
| handwritten_erased | ok | 274.0 (+18.0) | 114 (+66) | 14 (+0) | 12 (-239) | 0/1 | 3722 (+512) | 1048 (+264) | 0 | - | reference; FAIL: publish instr, setup instr, no extra indirect calls, no extra RAM |
| handwritten_loop | ok | 230.0 (-26.0) | 32 (-16) | 14 (+0) | 38 (-213) | 0/0 | 2453 (-757) | 784 (+0) | 0 | - | reference; PASS |
| handwritten_runtime | ok | 265.0 (+9.0) | 112 (+64) | 14 (+0) | 262 (+11) | 0/0 | 3594 (+384) | 1040 (+256) | 0 | - | reference; FAIL: publish instr, setup instr, publish path, no extra RAM |
| sub0_b1_mixin (vs handwritten_runtime) | ok | 265.0 (+0.0) | 112 (+0) | 14 (+0) | 262 (+0) | 0/0 | 3594 (+0) | 1040 (+0) | 0 | - | PASS |
| sub0_b1_wire (vs handwritten_runtime) | ok | 265.0 (+0.0) | 112 (+0) | 14 (+0) | 262 (+0) | 0/0 | 3594 (+0) | 1040 (+0) | 0 | - | PASS |
| sub0_b2_static | ok | 256.0 (+0.0) | 48 (+0) | 14 (+0) | 251 (+0) | 0/0 | 3210 (+0) | 784 (+0) | 0 | - | PASS |
| sub0_b3_sink (vs handwritten_erased) | ok | 273.0 (-1.0) | 114 (+0) | 14 (+0) | 270 (+258) | 1/0 | 3730 (+8) | 1048 (+0) | 0 | - | FAIL: publish path |
| sub0pub_virtual | ok | 599.0 (+343.0) | 350 (+302) | 2809 (+2795) | 28 (-223) | 0/1 | 17490 (+14280) | 1560 (+776) | 445 | - | FAIL: publish instr, setup instr, teardown instr, no extra indirect calls, no extra RAM, no Sub0Pub retained |
| sub0pub_virtual_lean | ok | 599.0 (+343.0) | 350 (+302) | 2809 (+2795) | 28 (-223) | 0/1 | 17490 (+14280) | 1560 (+776) | 445 | - | FAIL: publish instr, setup instr, teardown instr, no extra indirect calls, no extra RAM, no Sub0Pub retained |

### clang-O2, removable work

| variant | checksum | publish instr | setup instr | teardown instr | path instr | calls (direct/indirect) | text | data+bss | retained sub0 (B) | added deps | verdict |
|---|---|---|---|---|---|---|---|---|---|---|---|
| handwritten | ok | 8.0 (+0.0) | 16 (+0) | 14 (+0) | 4 (+0) | 0/0 | 2074 (+0) | 656 (+0) | 0 | - | reference |
| handwritten_erased | ok | 8.0 (+0.0) | 114 (+98) | 14 (+0) | 4 (+0) | 0/0 | 2842 (+768) | 1048 (+392) | 0 | - | reference; FAIL: setup instr, no extra RAM |
| handwritten_loop | ok | 8.0 (+0.0) | 16 (+0) | 14 (+0) | 4 (+0) | 0/0 | 2074 (+0) | 656 (+0) | 0 | - | reference; PASS |
| handwritten_runtime | ok | 8.0 (+0.0) | 16 (+0) | 14 (+0) | 4 (+0) | 0/0 | 2074 (+0) | 656 (+0) | 0 | - | reference; PASS |
| sub0_b1_mixin (vs handwritten_runtime) | ok | 8.0 (+0.0) | 16 (+0) | 14 (+0) | 4 (+0) | 0/0 | 2074 (+0) | 656 (+0) | 0 | - | PASS |
| sub0_b1_wire (vs handwritten_runtime) | ok | 8.0 (+0.0) | 16 (+0) | 14 (+0) | 4 (+0) | 0/0 | 2074 (+0) | 656 (+0) | 0 | - | PASS |
| sub0_b2_static | ok | 8.0 (+0.0) | 16 (+0) | 14 (+0) | 4 (+0) | 0/0 | 2074 (+0) | 656 (+0) | 0 | - | PASS |
| sub0_b3_sink (vs handwritten_erased) | ok | 8.0 (+0.0) | 114 (+0) | 14 (+0) | 4 (+0) | 0/0 | 2842 (+0) | 1048 (+0) | 0 | - | PASS |
| sub0pub_virtual | ok | 311.0 (+303.0) | 350 (+334) | 2809 (+2795) | 28 (+24) | 0/1 | 17474 (+15400) | 1560 (+904) | 445 | - | FAIL: publish instr, setup instr, teardown instr, publish path, no extra indirect calls, no extra RAM, no Sub0Pub retained |
| sub0pub_virtual_lean | ok | 311.0 (+303.0) | 350 (+334) | 2809 (+2795) | 28 (+24) | 0/1 | 17474 (+15400) | 1560 (+904) | 445 | - | FAIL: publish instr, setup instr, teardown instr, publish path, no extra indirect calls, no extra RAM, no Sub0Pub retained |

<details><summary>clang-O2: largest symbols added by handwritten_erased (bytes)</summary>

- 809 `(anonymous namespace)::deliverNode(void const*, (anonymous namespace)::Sample const&)`
- 783 `collapse_setup`
- 256 `(anonymous namespace)::node`
- 8 `_ZN12_GLOBAL__N_16sensorE.0`
- 4 `(anonymous namespace)::c9`
- 4 `(anonymous namespace)::c8`
- 4 `(anonymous namespace)::c7`
- 4 `(anonymous namespace)::c6`

</details>

<details><summary>clang-O2: largest symbols added by handwritten_loop (bytes)</summary>

- 128 `(anonymous namespace)::controllers`

</details>

<details><summary>clang-O2: largest symbols added by handwritten_runtime (bytes)</summary>

- 769 `collapse_setup`
- 256 `(anonymous namespace)::sensor`
- 4 `(anonymous namespace)::c9`
- 4 `(anonymous namespace)::c8`
- 4 `(anonymous namespace)::c7`
- 4 `(anonymous namespace)::c6`
- 4 `(anonymous namespace)::c5`
- 4 `(anonymous namespace)::c4`

</details>

<details><summary>clang-O2: largest symbols added by sub0_b3_sink (bytes)</summary>

- 809 `_ZZN4sub04SinkIN12_GLOBAL__N_16SampleEEC1INS_6WiringIJNS1_10ControllerES6_S6_S6_S6_S6_S6_S6_S6_S6_S6_S6_S6_S6_S6_S6_S6_S6_S6_S6_S6_S6_S6_S6_S6_S6_S6_S6_S6_S6_S6_S6_EEETnNSt9enable_ifIXntsr3stdE9is_same_vINSt9remove_cvIT_E4typeES3_EEiE4typeELi0EEERSA_ENUlPKvRKS2_E_8__invokeESH_SJ_`
- 256 `(anonymous namespace)::bus`

</details>

<details><summary>clang-O2: largest symbols added by sub0pub_virtual (bytes)</summary>

- 11750 `collapse_teardown`
- 2760 `collapse_setup`
- 264 `sub0::detail::BrokerImpl<(anonymous namespace)::Sample, sub0::detail::BuiltinT<32u, (sub0::Dispatch)1, (sub0::Context)2, false, sub0::NoLock> >::global_`
- 65 `typeinfo name for sub0::detail::SubscriberInterface<(anonymous namespace)::Sample, false>`
- 43 `typeinfo name for sub0::Subscribe<(anonymous namespace)::Sample>`
- 32 `vtable for sub0::Subscribe<(anonymous namespace)::Sample>`
- 32 `vtable for (anonymous namespace)::Controller`
- 29 `typeinfo name for (anonymous namespace)::Controller`

</details>

<details><summary>clang-O2: largest symbols added by sub0pub_virtual_lean (bytes)</summary>

- 11750 `collapse_teardown`
- 2760 `collapse_setup`
- 264 `sub0::detail::BrokerImpl<(anonymous namespace)::Sample, sub0::detail::BuiltinT<32u, (sub0::Dispatch)1, (sub0::Context)2, false, sub0::NoLock> >::global_`
- 65 `typeinfo name for sub0::detail::SubscriberInterface<(anonymous namespace)::Sample, false>`
- 43 `typeinfo name for sub0::Subscribe<(anonymous namespace)::Sample>`
- 32 `vtable for sub0::Subscribe<(anonymous namespace)::Sample>`
- 32 `vtable for (anonymous namespace)::Controller`
- 29 `typeinfo name for (anonymous namespace)::Controller`

</details>

### cm33-gcc-Os, observable work

| variant | checksum | publish instr | setup instr | teardown instr | path instr | calls (direct/indirect) | text | data+bss | retained sub0 (B) | added deps | verdict |
|---|---|---|---|---|---|---|---|---|---|---|---|
| handwritten | - | - | - | - | 143 (+0) | 32/0 | 1824 (+0) | 636 (+0) | 0 | - | reference |
| handwritten_erased | - | - | - | - | 15 (-128) | 0/1 | 1940 (+116) | 772 (+136) | 0 | - | reference; FAIL: no extra indirect calls, no extra RAM |
| handwritten_loop | - | - | - | - | 21 (-122) | 0/0 | 1156 (-668) | 636 (+0) | 0 | - | reference; PASS |
| handwritten_runtime | - | - | - | - | 114 (-29) | 32/0 | 1840 (+16) | 764 (+128) | 0 | - | reference; FAIL: no extra RAM |
| sub0_b1_mixin (vs handwritten_runtime) | - | - | - | - | 114 (+0) | 32/0 | 1840 (+0) | 764 (+0) | 0 | - | PASS |
| sub0_b1_wire (vs handwritten_runtime) | - | - | - | - | 114 (+0) | 32/0 | 1840 (+0) | 764 (+0) | 0 | - | PASS |
| sub0_b2_static | - | - | - | - | 143 (+0) | 32/0 | 1824 (+0) | 636 (+0) | 0 | - | PASS |
| sub0_b3_sink (vs handwritten_erased) | - | - | - | - | 15 (+0) | 0/1 | 1940 (+0) | 772 (+0) | 330 | - | PASS |
| sub0pub_virtual | - | - | - | - | 31 (-112) | 1/0 | 2392 (+568) | 1024 (+388) | 254 | - | FAIL: no extra RAM, no Sub0Pub retained |
| sub0pub_virtual_lean | - | - | - | - | 31 (-112) | 1/0 | 2392 (+568) | 1024 (+388) | 254 | - | FAIL: no extra RAM, no Sub0Pub retained |

### cm33-gcc-Os, removable work

| variant | checksum | publish instr | setup instr | teardown instr | path instr | calls (direct/indirect) | text | data+bss | retained sub0 (B) | added deps | verdict |
|---|---|---|---|---|---|---|---|---|---|---|---|
| handwritten | - | - | - | - | 7 (+0) | 0/0 | 1416 (+0) | 636 (+0) | 0 | - | reference |
| handwritten_erased | - | - | - | - | 15 (+8) | 0/1 | 1592 (+176) | 772 (+136) | 0 | - | reference; FAIL: publish path, no extra indirect calls, no extra RAM |
| handwritten_loop | - | - | - | - | 11 (+4) | 0/0 | 1128 (-288) | 636 (+0) | 0 | - | reference; FAIL: publish path |
| handwritten_runtime | - | - | - | - | 7 (+0) | 0/0 | 1552 (+136) | 764 (+128) | 0 | - | reference; FAIL: no extra RAM |
| sub0_b1_mixin (vs handwritten_runtime) | - | - | - | - | 7 (+0) | 0/0 | 1552 (+0) | 764 (+0) | 0 | - | PASS |
| sub0_b1_wire (vs handwritten_runtime) | - | - | - | - | 7 (+0) | 0/0 | 1552 (+0) | 764 (+0) | 0 | - | PASS |
| sub0_b2_static | - | - | - | - | 7 (+0) | 0/0 | 1416 (+0) | 636 (+0) | 0 | - | PASS |
| sub0_b3_sink (vs handwritten_erased) | - | - | - | - | 15 (+0) | 0/1 | 1592 (+0) | 772 (+0) | 2 | - | PASS |
| sub0pub_virtual | - | - | - | - | 7 (+0) | 0/0 | 2332 (+916) | 1024 (+388) | 254 | - | FAIL: no extra RAM, no Sub0Pub retained |
| sub0pub_virtual_lean | - | - | - | - | 7 (+0) | 0/0 | 2332 (+916) | 1024 (+388) | 254 | - | FAIL: no extra RAM, no Sub0Pub retained |

<details><summary>cm33-gcc-Os: largest symbols added by handwritten_erased (bytes)</summary>

- 476 `collapse_setup`
- 330 `(anonymous namespace)::deliverNode(void const*, (anonymous namespace)::Sample const&)`
- 128 `(anonymous namespace)::node`
- 8 `(anonymous namespace)::sensor`

</details>

<details><summary>cm33-gcc-Os: largest symbols added by handwritten_loop (bytes)</summary>

- 128 `(anonymous namespace)::controllers`
- 10 `collapse_teardown`

</details>

<details><summary>cm33-gcc-Os: largest symbols added by handwritten_runtime (bytes)</summary>

- 460 `collapse_setup`
- 128 `(anonymous namespace)::sensor`

</details>

<details><summary>cm33-gcc-Os: largest symbols added by sub0_b3_sink (bytes)</summary>

- 330 `sub0::Sink<(anonymous namespace)::Sample>::Sink<sub0::Wiring<(anonymous namespace)::Controller, (anonymous namespace)::Controller, (anonymous namespace)::Controller, (anonymous namespace)::Controller, (anonymous namespace)::Controller, (anonymous namespace)::Controller, (anonymous namespace)::Controller, (anonymous namespace)::Controller, (anonymous namespace)::Controller, (anonymous namespace)::Controller, (anonymous namespace)::Controller, (anonymous namespace)::Controller, (anonymous namespace)::Controller, (anonymous namespace)::Controller, (anonymous namespace)::Controller, (anonymous namespace)::Controller, (anonymous namespace)::Controller, (anonymous namespace)::Controller, (anonymous namespace)::Controller, (anonymous namespace)::Controller, (anonymous namespace)::Controller, (anonymous namespace)::Controller, (anonymous namespace)::Controller, (anonymous namespace)::Controller, (anonymous namespace)::Controller, (anonymous namespace)::Controller, (anonymous namespace)::Controller, (anonymous namespace)::Controller, (anonymous namespace)::Controller, (anonymous namespace)::Controller, (anonymous namespace)::Controller, (anonymous namespace)::Controller>, 0>(sub0::Wiring<(anonymous namespace)::Controller, (anonymous namespace)::Controller, (anonymous namespace)::Controller, (anonymous namespace)::Controller, (anonymous namespace)::Controller, (anonymous namespace)::Controller, (anonymous namespace)::Controller, (anonymous namespace)::Controller, (anonymous namespace)::Controller, (anonymous namespace)::Controller, (anonymous namespace)::Controller, (anonymous namespace)::Controller, (anonymous namespace)::Controller, (anonymous namespace)::Controller, (anonymous namespace)::Controller, (anonymous namespace)::Controller, (anonymous namespace)::Controller, (anonymous namespace)::Controller, (anonymous namespace)::Controller, (anonymous namespace)::Controller, (anonymous namespace)::Controller, (anonymous namespace)::Controller, (anonymous namespace)::Controller, (anonymous namespace)::Controller, (anonymous namespace)::Controller, (anonymous namespace)::Controller, (anonymous namespace)::Controller, (anonymous namespace)::Controller, (anonymous namespace)::Controller, (anonymous namespace)::Controller, (anonymous namespace)::Controller, (anonymous namespace)::Controller>&)::{lambda(void const*, (anonymous namespace)::Sample const&)#1}::_FUN(void const*, (anonymous namespace)::Sample const&)`
- 128 `(anonymous namespace)::bus`

</details>

<details><summary>cm33-gcc-Os: largest symbols added by sub0pub_virtual (bytes)</summary>

- 524 `collapse_setup`
- 324 `collapse_teardown`
- 254 `memmove`
- 132 `sub0::detail::BrokerImpl<(anonymous namespace)::Sample, sub0::detail::BuiltinT<32ul, (sub0::Dispatch)1, (sub0::Context)2, false, sub0::NoLock> >::global_`
- 64 `sub0::Subscribe<(anonymous namespace)::Sample>::disconnect()`
- 40 `sub0::Subscribe<(anonymous namespace)::Sample>::Subscribe<sub0::detail::BuiltinT<32ul, (sub0::Dispatch)1, (sub0::Context)2, false, sub0::NoLock>, 0>() [clone .isra.0]`
- 24 `(anonymous namespace)::Controller::receive((anonymous namespace)::Sample const&)`
- 16 `vtable for sub0::Subscribe<(anonymous namespace)::Sample>`

</details>

<details><summary>cm33-gcc-Os: largest symbols added by sub0pub_virtual_lean (bytes)</summary>

- 524 `collapse_setup`
- 324 `collapse_teardown`
- 254 `memmove`
- 132 `sub0::detail::BrokerImpl<(anonymous namespace)::Sample, sub0::detail::BuiltinT<32ul, (sub0::Dispatch)1, (sub0::Context)2, false, sub0::NoLock> >::global_`
- 64 `sub0::Subscribe<(anonymous namespace)::Sample>::disconnect()`
- 40 `sub0::Subscribe<(anonymous namespace)::Sample>::Subscribe<sub0::detail::BuiltinT<32ul, (sub0::Dispatch)1, (sub0::Context)2, false, sub0::NoLock>, 0>() [clone .isra.0]`
- 24 `(anonymous namespace)::Controller::receive((anonymous namespace)::Sample const&)`
- 16 `vtable for sub0::Subscribe<(anonymous namespace)::Sample>`

</details>

## Case: multi_receivers

### gcc-O2, observable work

| variant | checksum | publish instr | setup instr | teardown instr | path instr | calls (direct/indirect) | text | data+bss | retained sub0 (B) | added deps | verdict |
|---|---|---|---|---|---|---|---|---|---|---|---|
| handwritten | ok | 30.0 (+0.0) | 21 (+0) | 16 (+0) | 26 (+0) | 0/0 | 2431 (+0) | 632 (+0) | 0 | - | reference |
| handwritten_erased | ok | 52.0 (+22.0) | 31 (+10) | 16 (+0) | 17 (-9) | 1/1 | 2627 (+196) | 672 (+40) | 0 | - | reference; FAIL: publish instr, setup instr, no extra indirect calls, no extra RAM |
| handwritten_runtime | ok | 38.0 (+8.0) | 27 (+6) | 16 (+0) | 35 (+9) | 0/0 | 2495 (+64) | 656 (+24) | 0 | - | reference; FAIL: publish instr, setup instr, publish path, no extra RAM |
| sub0_b1_mixin (vs handwritten_runtime) | ok | 38.0 (+0.0) | 27 (+0) | 16 (+0) | 35 (+0) | 0/0 | 2495 (+0) | 656 (+0) | 0 | - | PASS |
| sub0_b1_wire (vs handwritten_runtime) | ok | 38.0 (+0.0) | 27 (+0) | 16 (+0) | 35 (+0) | 0/0 | 2495 (+0) | 656 (+0) | 0 | - | PASS |
| sub0_b2_static | ok | 30.0 (+0.0) | 21 (+0) | 16 (+0) | 26 (+0) | 0/0 | 2431 (+0) | 632 (+0) | 0 | - | PASS |
| sub0_b3_sink (vs handwritten_erased) | ok | 52.0 (+0.0) | 31 (+0) | 16 (+0) | 17 (+0) | 1/1 | 2627 (+0) | 672 (+0) | 99 | - | PASS |
| sub0pub_virtual | ok | 86.0 (+56.0) | 53 (+32) | 92 (+76) | 37 (+11) | 1/1 | 4307 (+1876) | 936 (+304) | 227 | - | FAIL: publish instr, setup instr, teardown instr, publish path, no extra indirect calls, no extra RAM, no Sub0Pub retained |
| sub0pub_virtual_lean | ok | 86.0 (+56.0) | 53 (+32) | 92 (+76) | 37 (+11) | 1/1 | 4307 (+1876) | 936 (+304) | 227 | - | FAIL: publish instr, setup instr, teardown instr, publish path, no extra indirect calls, no extra RAM, no Sub0Pub retained |

### gcc-O2, removable work

| variant | checksum | publish instr | setup instr | teardown instr | path instr | calls (direct/indirect) | text | data+bss | retained sub0 (B) | added deps | verdict |
|---|---|---|---|---|---|---|---|---|---|---|---|
| handwritten | ok | 9.0 (+0.0) | 21 (+0) | 16 (+0) | 6 (+0) | 0/0 | 2367 (+0) | 632 (+0) | 0 | - | reference |
| handwritten_erased | ok | 24.0 (+15.0) | 31 (+10) | 16 (+0) | 17 (+11) | 1/1 | 2531 (+164) | 672 (+40) | 0 | - | reference; FAIL: publish instr, setup instr, publish path, no extra indirect calls, no extra RAM |
| handwritten_runtime | ok | 10.0 (+1.0) | 27 (+6) | 16 (+0) | 6 (+0) | 0/0 | 2399 (+32) | 656 (+24) | 0 | - | reference; FAIL: setup instr, no extra RAM |
| sub0_b1_mixin (vs handwritten_runtime) | ok | 10.0 (+0.0) | 27 (+0) | 16 (+0) | 6 (+0) | 0/0 | 2399 (+0) | 656 (+0) | 0 | - | PASS |
| sub0_b1_wire (vs handwritten_runtime) | ok | 10.0 (+0.0) | 27 (+0) | 16 (+0) | 6 (+0) | 0/0 | 2399 (+0) | 656 (+0) | 0 | - | PASS |
| sub0_b2_static | ok | 9.0 (+0.0) | 21 (+0) | 16 (+0) | 6 (+0) | 0/0 | 2367 (+0) | 632 (+0) | 0 | - | PASS |
| sub0_b3_sink (vs handwritten_erased) | ok | 24.0 (+0.0) | 31 (+0) | 16 (+0) | 17 (+0) | 1/1 | 2531 (+0) | 672 (+0) | 11 | - | PASS |
| sub0pub_virtual | ok | 61.0 (+52.0) | 53 (+32) | 92 (+76) | 37 (+31) | 1/1 | 4215 (+1848) | 936 (+304) | 227 | - | FAIL: publish instr, setup instr, teardown instr, publish path, no extra indirect calls, no extra RAM, no Sub0Pub retained |
| sub0pub_virtual_lean | ok | 61.0 (+52.0) | 53 (+32) | 92 (+76) | 37 (+31) | 1/1 | 4215 (+1848) | 936 (+304) | 227 | - | FAIL: publish instr, setup instr, teardown instr, publish path, no extra indirect calls, no extra RAM, no Sub0Pub retained |

<details><summary>gcc-O2: largest symbols added by handwritten_erased (bytes)</summary>

- 105 `collapse_setup`
- 99 `(anonymous namespace)::deliverNode(void const*, (anonymous namespace)::Sample const&)`
- 24 `(anonymous namespace)::node`
- 16 `(anonymous namespace)::sensor`

</details>

<details><summary>gcc-O2: largest symbols added by handwritten_runtime (bytes)</summary>

- 114 `collapse_publish`
- 77 `collapse_setup`
- 24 `(anonymous namespace)::sensor`

</details>

<details><summary>gcc-O2: largest symbols added by sub0_b3_sink (bytes)</summary>

- 99 `sub0::Sink<(anonymous namespace)::Sample>::Sink<sub0::Wiring<(anonymous namespace)::Controller, (anonymous namespace)::Controller, (anonymous namespace)::Logger>, 0>(sub0::Wiring<(anonymous namespace)::Controller, (anonymous namespace)::Controller, (anonymous namespace)::Logger>&)::{lambda(void const*, (anonymous namespace)::Sample const&)#1}::_FUN(void const*, (anonymous namespace)::Sample const&)`
- 24 `(anonymous namespace)::bus`

</details>

<details><summary>gcc-O2: largest symbols added by sub0pub_virtual (bytes)</summary>

- 427 `collapse_teardown`
- 306 `collapse_setup`
- 126 `collapse_publish`
- 72 `sub0::detail::BrokerImpl<(anonymous namespace)::Sample, sub0::detail::BuiltinT<8u, (sub0::Dispatch)1, (sub0::Context)2, false, sub0::NoLock> >::global_`
- 66 `typeinfo name for sub0::detail::SubscriberInterface<(anonymous namespace)::Sample, false>`
- 44 `typeinfo name for sub0::Subscribe<(anonymous namespace)::Sample>`
- 37 `(anonymous namespace)::Logger::receive((anonymous namespace)::Sample const&)`
- 32 `vtable for (anonymous namespace)::Logger`

</details>

<details><summary>gcc-O2: largest symbols added by sub0pub_virtual_lean (bytes)</summary>

- 427 `collapse_teardown`
- 306 `collapse_setup`
- 126 `collapse_publish`
- 72 `sub0::detail::BrokerImpl<(anonymous namespace)::Sample, sub0::detail::BuiltinT<8u, (sub0::Dispatch)1, (sub0::Context)2, false, sub0::NoLock> >::global_`
- 66 `typeinfo name for sub0::detail::SubscriberInterface<(anonymous namespace)::Sample, false>`
- 44 `typeinfo name for sub0::Subscribe<(anonymous namespace)::Sample>`
- 37 `(anonymous namespace)::Logger::receive((anonymous namespace)::Sample const&)`
- 32 `vtable for (anonymous namespace)::Logger`

</details>

### clang-O2, observable work

| variant | checksum | publish instr | setup instr | teardown instr | path instr | calls (direct/indirect) | text | data+bss | retained sub0 (B) | added deps | verdict |
|---|---|---|---|---|---|---|---|---|---|---|---|
| handwritten | ok | 33.0 (+0.0) | 19 (+0) | 14 (+0) | 29 (+0) | 0/0 | 2170 (+0) | 672 (+0) | 0 | - | reference |
| handwritten_erased | ok | 45.0 (+12.0) | 27 (+8) | 14 (+0) | 12 (-17) | 0/1 | 2314 (+144) | 704 (+32) | 0 | - | reference; FAIL: publish instr, setup instr, no extra indirect calls, no extra RAM |
| handwritten_runtime | ok | 33.0 (+0.0) | 19 (+0) | 14 (+0) | 29 (+0) | 0/0 | 2170 (+0) | 672 (+0) | 0 | - | reference; PASS |
| sub0_b1_mixin (vs handwritten_runtime) | ok | 33.0 (+0.0) | 19 (+0) | 14 (+0) | 29 (+0) | 0/0 | 2170 (+0) | 672 (+0) | 0 | - | PASS |
| sub0_b1_wire (vs handwritten_runtime) | ok | 33.0 (+0.0) | 19 (+0) | 14 (+0) | 29 (+0) | 0/0 | 2170 (+0) | 672 (+0) | 0 | - | PASS |
| sub0_b2_static | ok | 33.0 (+0.0) | 19 (+0) | 14 (+0) | 29 (+0) | 0/0 | 2170 (+0) | 672 (+0) | 0 | - | PASS |
| sub0_b3_sink (vs handwritten_erased) | ok | 44.0 (-1.0) | 27 (+0) | 14 (+0) | 41 (+29) | 1/0 | 2314 (+0) | 704 (+0) | 0 | - | FAIL: publish path |
| sub0pub_virtual | ok | 79.0 (+46.0) | 52 (+33) | 95 (+81) | 28 (-1) | 0/1 | 4580 (+2410) | 960 (+288) | 253 | - | FAIL: publish instr, setup instr, teardown instr, no extra indirect calls, no extra RAM, no Sub0Pub retained |
| sub0pub_virtual_lean | ok | 79.0 (+46.0) | 52 (+33) | 95 (+81) | 28 (-1) | 0/1 | 4580 (+2410) | 960 (+288) | 253 | - | FAIL: publish instr, setup instr, teardown instr, no extra indirect calls, no extra RAM, no Sub0Pub retained |

### clang-O2, removable work

| variant | checksum | publish instr | setup instr | teardown instr | path instr | calls (direct/indirect) | text | data+bss | retained sub0 (B) | added deps | verdict |
|---|---|---|---|---|---|---|---|---|---|---|---|
| handwritten | ok | 9.0 (+0.0) | 17 (+0) | 14 (+0) | 4 (+0) | 0/0 | 2074 (+0) | 664 (+0) | 0 | - | reference |
| handwritten_erased | ok | 18.0 (+9.0) | 27 (+10) | 14 (+0) | 12 (+8) | 0/1 | 2234 (+160) | 704 (+40) | 0 | - | reference; FAIL: publish instr, setup instr, publish path, no extra indirect calls, no extra RAM |
| handwritten_runtime | ok | 9.0 (+0.0) | 19 (+2) | 14 (+0) | 4 (+0) | 0/0 | 2090 (+16) | 672 (+8) | 0 | - | reference; FAIL: setup instr, no extra RAM |
| sub0_b1_mixin (vs handwritten_runtime) | ok | 9.0 (+0.0) | 19 (+0) | 14 (+0) | 4 (+0) | 0/0 | 2090 (+0) | 672 (+0) | 0 | - | PASS |
| sub0_b1_wire (vs handwritten_runtime) | ok | 9.0 (+0.0) | 19 (+0) | 14 (+0) | 4 (+0) | 0/0 | 2090 (+0) | 672 (+0) | 0 | - | PASS |
| sub0_b2_static | ok | 9.0 (+0.0) | 17 (+0) | 14 (+0) | 4 (+0) | 0/0 | 2074 (+0) | 664 (+0) | 0 | - | PASS |
| sub0_b3_sink (vs handwritten_erased) | ok | 17.0 (-1.0) | 27 (+0) | 14 (+0) | 14 (+2) | 1/0 | 2239 (+5) | 704 (+0) | 0 | - | PASS |
| sub0pub_virtual | ok | 51.0 (+42.0) | 52 (+35) | 95 (+81) | 28 (+24) | 0/1 | 4537 (+2463) | 960 (+296) | 253 | - | FAIL: publish instr, setup instr, teardown instr, publish path, no extra indirect calls, no extra RAM, no Sub0Pub retained |
| sub0pub_virtual_lean | ok | 51.0 (+42.0) | 52 (+35) | 95 (+81) | 28 (+24) | 0/1 | 4537 (+2463) | 960 (+296) | 253 | - | FAIL: publish instr, setup instr, teardown instr, publish path, no extra indirect calls, no extra RAM, no Sub0Pub retained |

<details><summary>clang-O2: largest symbols added by handwritten_erased (bytes)</summary>

- 87 `collapse_setup`
- 81 `(anonymous namespace)::deliverNode(void const*, (anonymous namespace)::Sample const&)`
- 24 `(anonymous namespace)::node`
- 8 `_ZN12_GLOBAL__N_16sensorE.0`
- 4 `(anonymous namespace)::logger`
- 4 `(anonymous namespace)::controllerB`
- 4 `(anonymous namespace)::controllerA`

</details>

<details><summary>clang-O2: largest symbols added by sub0_b3_sink (bytes)</summary>

- 81 `_ZZN4sub04SinkIN12_GLOBAL__N_16SampleEEC1INS_6WiringIJNS1_10ControllerES6_NS1_6LoggerEEEETnNSt9enable_ifIXntsr3stdE9is_same_vINSt9remove_cvIT_E4typeES3_EEiE4typeELi0EEERSB_ENUlPKvRKS2_E_8__invokeESI_SK_`
- 24 `(anonymous namespace)::bus`

</details>

<details><summary>clang-O2: largest symbols added by sub0pub_virtual (bytes)</summary>

- 1078 `collapse_teardown`
- 276 `collapse_setup`
- 72 `sub0::detail::BrokerImpl<(anonymous namespace)::Sample, sub0::detail::BuiltinT<8u, (sub0::Dispatch)1, (sub0::Context)2, false, sub0::NoLock> >::global_`
- 65 `typeinfo name for sub0::detail::SubscriberInterface<(anonymous namespace)::Sample, false>`
- 43 `typeinfo name for sub0::Subscribe<(anonymous namespace)::Sample>`
- 32 `vtable for sub0::Subscribe<(anonymous namespace)::Sample>`
- 32 `vtable for (anonymous namespace)::Logger`
- 32 `vtable for (anonymous namespace)::Controller`

</details>

<details><summary>clang-O2: largest symbols added by sub0pub_virtual_lean (bytes)</summary>

- 1078 `collapse_teardown`
- 276 `collapse_setup`
- 72 `sub0::detail::BrokerImpl<(anonymous namespace)::Sample, sub0::detail::BuiltinT<8u, (sub0::Dispatch)1, (sub0::Context)2, false, sub0::NoLock> >::global_`
- 65 `typeinfo name for sub0::detail::SubscriberInterface<(anonymous namespace)::Sample, false>`
- 43 `typeinfo name for sub0::Subscribe<(anonymous namespace)::Sample>`
- 32 `vtable for sub0::Subscribe<(anonymous namespace)::Sample>`
- 32 `vtable for (anonymous namespace)::Logger`
- 32 `vtable for (anonymous namespace)::Controller`

</details>

### cm33-gcc-Os, observable work

| variant | checksum | publish instr | setup instr | teardown instr | path instr | calls (direct/indirect) | text | data+bss | retained sub0 (B) | added deps | verdict |
|---|---|---|---|---|---|---|---|---|---|---|---|
| handwritten | - | - | - | - | 30 (+0) | 0/0 | 1188 (+0) | 520 (+0) | 0 | - | reference |
| handwritten_erased | - | - | - | - | 15 (-15) | 0/1 | 1240 (+52) | 540 (+20) | 0 | - | reference; FAIL: no extra indirect calls, no extra RAM |
| handwritten_runtime | - | - | - | - | 32 (+2) | 0/0 | 1200 (+12) | 532 (+12) | 0 | - | reference; FAIL: no extra RAM |
| sub0_b1_mixin (vs handwritten_runtime) | - | - | - | - | 32 (+0) | 0/0 | 1200 (+0) | 532 (+0) | 0 | - | PASS |
| sub0_b1_wire (vs handwritten_runtime) | - | - | - | - | 32 (+0) | 0/0 | 1200 (+0) | 532 (+0) | 0 | - | PASS |
| sub0_b2_static | - | - | - | - | 30 (+0) | 0/0 | 1188 (+0) | 520 (+0) | 0 | - | PASS |
| sub0_b3_sink (vs handwritten_erased) | - | - | - | - | 15 (+0) | 0/1 | 1240 (+0) | 540 (+0) | 68 | - | PASS |
| sub0pub_virtual | - | - | - | - | 23 (-7) | 0/1 | 1672 (+484) | 580 (+60) | 134 | - | FAIL: no extra indirect calls, no extra RAM, no Sub0Pub retained |
| sub0pub_virtual_lean | - | - | - | - | 23 (-7) | 0/1 | 1672 (+484) | 580 (+60) | 134 | - | FAIL: no extra indirect calls, no extra RAM, no Sub0Pub retained |

### cm33-gcc-Os, removable work

| variant | checksum | publish instr | setup instr | teardown instr | path instr | calls (direct/indirect) | text | data+bss | retained sub0 (B) | added deps | verdict |
|---|---|---|---|---|---|---|---|---|---|---|---|
| handwritten | - | - | - | - | 12 (+0) | 0/0 | 1136 (+0) | 520 (+0) | 0 | - | reference |
| handwritten_erased | - | - | - | - | 15 (+3) | 0/1 | 1184 (+48) | 540 (+20) | 0 | - | reference; FAIL: publish path, no extra indirect calls, no extra RAM |
| handwritten_runtime | - | - | - | - | 12 (+0) | 0/0 | 1148 (+12) | 532 (+12) | 0 | - | reference; FAIL: no extra RAM |
| sub0_b1_mixin (vs handwritten_runtime) | - | - | - | - | 12 (+0) | 0/0 | 1148 (+0) | 532 (+0) | 0 | - | PASS |
| sub0_b1_wire (vs handwritten_runtime) | - | - | - | - | 12 (+0) | 0/0 | 1148 (+0) | 532 (+0) | 0 | - | PASS |
| sub0_b2_static | - | - | - | - | 12 (+0) | 0/0 | 1136 (+0) | 520 (+0) | 0 | - | PASS |
| sub0_b3_sink (vs handwritten_erased) | - | - | - | - | 15 (+0) | 0/1 | 1184 (+0) | 540 (+0) | 10 | - | PASS |
| sub0pub_virtual | - | - | - | - | 23 (+11) | 0/1 | 1628 (+492) | 580 (+60) | 134 | - | FAIL: publish path, no extra indirect calls, no extra RAM, no Sub0Pub retained |
| sub0pub_virtual_lean | - | - | - | - | 23 (+11) | 0/1 | 1628 (+492) | 580 (+60) | 134 | - | FAIL: publish path, no extra indirect calls, no extra RAM, no Sub0Pub retained |

<details><summary>cm33-gcc-Os: largest symbols added by handwritten_erased (bytes)</summary>

- 68 `(anonymous namespace)::deliverNode(void const*, (anonymous namespace)::Sample const&)`
- 60 `collapse_setup`
- 12 `(anonymous namespace)::node`
- 8 `(anonymous namespace)::sensor`

</details>

<details><summary>cm33-gcc-Os: largest symbols added by handwritten_runtime (bytes)</summary>

- 44 `collapse_setup`
- 12 `(anonymous namespace)::sensor`

</details>

<details><summary>cm33-gcc-Os: largest symbols added by sub0_b3_sink (bytes)</summary>

- 68 `sub0::Sink<(anonymous namespace)::Sample>::Sink<sub0::Wiring<(anonymous namespace)::Controller, (anonymous namespace)::Controller, (anonymous namespace)::Logger>, 0>(sub0::Wiring<(anonymous namespace)::Controller, (anonymous namespace)::Controller, (anonymous namespace)::Logger>&)::{lambda(void const*, (anonymous namespace)::Sample const&)#1}::_FUN(void const*, (anonymous namespace)::Sample const&)`
- 12 `(anonymous namespace)::bus`

</details>

<details><summary>cm33-gcc-Os: largest symbols added by sub0pub_virtual (bytes)</summary>

- 254 `memmove`
- 72 `collapse_setup`
- 64 `sub0::Subscribe<(anonymous namespace)::Sample>::disconnect()`
- 36 `sub0::detail::BrokerImpl<(anonymous namespace)::Sample, sub0::detail::BuiltinT<8ul, (sub0::Dispatch)1, (sub0::Context)2, false, sub0::NoLock> >::global_`
- 36 `collapse_teardown`
- 32 `sub0::Subscribe<(anonymous namespace)::Sample>::trySubscribe() [clone .isra.0]`
- 28 `(anonymous namespace)::Logger::receive((anonymous namespace)::Sample const&)`
- 24 `(anonymous namespace)::Controller::receive((anonymous namespace)::Sample const&)`

</details>

<details><summary>cm33-gcc-Os: largest symbols added by sub0pub_virtual_lean (bytes)</summary>

- 254 `memmove`
- 72 `collapse_setup`
- 64 `sub0::Subscribe<(anonymous namespace)::Sample>::disconnect()`
- 36 `sub0::detail::BrokerImpl<(anonymous namespace)::Sample, sub0::detail::BuiltinT<8ul, (sub0::Dispatch)1, (sub0::Context)2, false, sub0::NoLock> >::global_`
- 36 `collapse_teardown`
- 32 `sub0::Subscribe<(anonymous namespace)::Sample>::trySubscribe() [clone .isra.0]`
- 28 `(anonymous namespace)::Logger::receive((anonymous namespace)::Sample const&)`
- 24 `(anonymous namespace)::Controller::receive((anonymous namespace)::Sample const&)`

</details>

## Case: multi_types

### gcc-O2, observable work

| variant | checksum | publish instr | setup instr | teardown instr | path instr | calls (direct/indirect) | text | data+bss | retained sub0 (B) | added deps | verdict |
|---|---|---|---|---|---|---|---|---|---|---|---|
| handwritten | ok | 32.0 (+0.0) | 19 (+0) | 16 (+0) | 28 (+0) | 0/0 | 2399 (+0) | 624 (+0) | 0 | - | reference |
| handwritten_erased | ok | 67.0 (+35.0) | 32 (+13) | 16 (+0) | 28 (+0) | 1/2 | 2719 (+320) | 704 (+80) | 0 | - | reference; FAIL: publish instr, setup instr, no extra indirect calls, no extra RAM |
| handwritten_runtime | ok | 36.0 (+4.0) | 25 (+6) | 16 (+0) | 33 (+5) | 0/0 | 2463 (+64) | 656 (+32) | 0 | - | reference; FAIL: publish instr, setup instr, publish path, no extra RAM |
| sub0_b1_mixin (vs handwritten_runtime) | ok | 36.0 (+0.0) | 25 (+0) | 16 (+0) | 33 (+0) | 0/0 | 2463 (+0) | 656 (+0) | 0 | - | PASS |
| sub0_b1_wire (vs handwritten_runtime) | ok | 36.0 (+0.0) | 25 (+0) | 16 (+0) | 33 (+0) | 0/0 | 2463 (+0) | 656 (+0) | 0 | - | PASS |
| sub0_b2_static | ok | 32.0 (+0.0) | 19 (+0) | 16 (+0) | 28 (+0) | 0/0 | 2399 (+0) | 624 (+0) | 0 | - | PASS |
| sub0_b3_sink (vs handwritten_erased) | ok | 67.0 (+0.0) | 32 (+0) | 16 (+0) | 28 (+0) | 1/2 | 2719 (+0) | 704 (+0) | 113 | - | PASS |
| sub0pub_virtual | ok | 114.0 (+82.0) | 65 (+46) | 109 (+93) | 57 (+29) | 1/2 | 5529 (+3130) | 1232 (+608) | 456 | - | FAIL: publish instr, setup instr, teardown instr, publish path, no extra indirect calls, no extra RAM, no Sub0Pub retained |
| sub0pub_virtual_lean | ok | 114.0 (+82.0) | 65 (+46) | 109 (+93) | 57 (+29) | 1/2 | 5529 (+3130) | 1232 (+608) | 456 | - | FAIL: publish instr, setup instr, teardown instr, publish path, no extra indirect calls, no extra RAM, no Sub0Pub retained |

### gcc-O2, removable work

| variant | checksum | publish instr | setup instr | teardown instr | path instr | calls (direct/indirect) | text | data+bss | retained sub0 (B) | added deps | verdict |
|---|---|---|---|---|---|---|---|---|---|---|---|
| handwritten | ok | 9.0 (+0.0) | 19 (+0) | 16 (+0) | 6 (+0) | 0/0 | 2335 (+0) | 624 (+0) | 0 | - | reference |
| handwritten_erased | ok | 37.0 (+28.0) | 32 (+13) | 16 (+0) | 28 (+22) | 1/2 | 2623 (+288) | 704 (+80) | 0 | - | reference; FAIL: publish instr, setup instr, publish path, no extra indirect calls, no extra RAM |
| handwritten_runtime | ok | 10.0 (+1.0) | 25 (+6) | 16 (+0) | 6 (+0) | 0/0 | 2383 (+48) | 656 (+32) | 0 | - | reference; FAIL: setup instr, no extra RAM |
| sub0_b1_mixin (vs handwritten_runtime) | ok | 10.0 (+0.0) | 25 (+0) | 16 (+0) | 6 (+0) | 0/0 | 2383 (+0) | 656 (+0) | 0 | - | PASS |
| sub0_b1_wire (vs handwritten_runtime) | ok | 10.0 (+0.0) | 25 (+0) | 16 (+0) | 6 (+0) | 0/0 | 2383 (+0) | 656 (+0) | 0 | - | PASS |
| sub0_b2_static | ok | 9.0 (+0.0) | 19 (+0) | 16 (+0) | 6 (+0) | 0/0 | 2335 (+0) | 624 (+0) | 0 | - | PASS |
| sub0_b3_sink (vs handwritten_erased) | ok | 37.0 (+0.0) | 32 (+0) | 16 (+0) | 28 (+0) | 1/2 | 2623 (+0) | 704 (+0) | 17 | - | PASS |
| sub0pub_virtual | ok | 83.0 (+74.0) | 65 (+46) | 109 (+93) | 57 (+51) | 1/2 | 5301 (+2966) | 1232 (+608) | 456 | - | FAIL: publish instr, setup instr, teardown instr, publish path, no extra indirect calls, no extra RAM, no Sub0Pub retained |
| sub0pub_virtual_lean | ok | 83.0 (+74.0) | 65 (+46) | 109 (+93) | 57 (+51) | 1/2 | 5301 (+2966) | 1232 (+608) | 456 | - | FAIL: publish instr, setup instr, teardown instr, publish path, no extra indirect calls, no extra RAM, no Sub0Pub retained |

<details><summary>gcc-O2: largest symbols added by handwritten_erased (bytes)</summary>

- 111 `collapse_publish`
- 106 `collapse_setup`
- 65 `(anonymous namespace)::deliverSample(void const*, (anonymous namespace)::Sample const&)`
- 48 `(anonymous namespace)::deliverCommand(void const*, (anonymous namespace)::Command const&)`
- 32 `(anonymous namespace)::sensor`
- 24 `(anonymous namespace)::node`
- 1 `(anonymous namespace)::controller`
- 1 `(anonymous namespace)::actuator`

</details>

<details><summary>gcc-O2: largest symbols added by handwritten_runtime (bytes)</summary>

- 99 `collapse_publish`
- 57 `collapse_setup`
- 24 `(anonymous namespace)::sensor`
- 1 `(anonymous namespace)::controller`
- 1 `(anonymous namespace)::actuator`

</details>

<details><summary>gcc-O2: largest symbols added by sub0_b3_sink (bytes)</summary>

- 65 `sub0::Sink<(anonymous namespace)::Sample>::Sink<sub0::Wiring<(anonymous namespace)::Controller, (anonymous namespace)::Logger, (anonymous namespace)::Actuator>, 0>(sub0::Wiring<(anonymous namespace)::Controller, (anonymous namespace)::Logger, (anonymous namespace)::Actuator>&)::{lambda(void const*, (anonymous namespace)::Sample const&)#1}::_FUN(void const*, (anonymous namespace)::Sample const&)`
- 48 `sub0::Sink<(anonymous namespace)::Command>::Sink<sub0::Wiring<(anonymous namespace)::Controller, (anonymous namespace)::Logger, (anonymous namespace)::Actuator>, 0>(sub0::Wiring<(anonymous namespace)::Controller, (anonymous namespace)::Logger, (anonymous namespace)::Actuator>&)::{lambda(void const*, (anonymous namespace)::Command const&)#1}::_FUN(void const*, (anonymous namespace)::Command const&)`
- 24 `(anonymous namespace)::bus`

</details>

<details><summary>gcc-O2: largest symbols added by sub0pub_virtual (bytes)</summary>

- 587 `collapse_teardown`
- 447 `collapse_setup`
- 201 `collapse_publish`
- 72 `vtable for (anonymous namespace)::Controller`
- 72 `sub0::detail::BrokerImpl<(anonymous namespace)::Sample, sub0::detail::BuiltinT<8u, (sub0::Dispatch)1, (sub0::Context)2, false, sub0::NoLock> >::global_`
- 72 `sub0::detail::BrokerImpl<(anonymous namespace)::Command, sub0::detail::BuiltinT<8u, (sub0::Dispatch)1, (sub0::Context)2, false, sub0::NoLock> >::global_`
- 67 `typeinfo name for sub0::detail::SubscriberInterface<(anonymous namespace)::Command, false>`
- 66 `typeinfo name for sub0::detail::SubscriberInterface<(anonymous namespace)::Sample, false>`

</details>

<details><summary>gcc-O2: largest symbols added by sub0pub_virtual_lean (bytes)</summary>

- 587 `collapse_teardown`
- 447 `collapse_setup`
- 201 `collapse_publish`
- 72 `vtable for (anonymous namespace)::Controller`
- 72 `sub0::detail::BrokerImpl<(anonymous namespace)::Sample, sub0::detail::BuiltinT<8u, (sub0::Dispatch)1, (sub0::Context)2, false, sub0::NoLock> >::global_`
- 72 `sub0::detail::BrokerImpl<(anonymous namespace)::Command, sub0::detail::BuiltinT<8u, (sub0::Dispatch)1, (sub0::Context)2, false, sub0::NoLock> >::global_`
- 67 `typeinfo name for sub0::detail::SubscriberInterface<(anonymous namespace)::Command, false>`
- 66 `typeinfo name for sub0::detail::SubscriberInterface<(anonymous namespace)::Sample, false>`

</details>

### clang-O2, observable work

| variant | checksum | publish instr | setup instr | teardown instr | path instr | calls (direct/indirect) | text | data+bss | retained sub0 (B) | added deps | verdict |
|---|---|---|---|---|---|---|---|---|---|---|---|
| handwritten | ok | 34.0 (+0.0) | 17 (+0) | 14 (+0) | 31 (+0) | 0/0 | 2154 (+0) | 664 (+0) | 0 | - | reference |
| handwritten_erased | ok | 59.0 (+25.0) | 26 (+9) | 14 (+0) | 19 (-12) | 0/2 | 2382 (+228) | 712 (+48) | 0 | - | reference; FAIL: publish instr, setup instr, no extra indirect calls, no extra RAM |
| handwritten_runtime | ok | 34.0 (+0.0) | 17 (+0) | 14 (+0) | 31 (+0) | 0/0 | 2154 (+0) | 664 (+0) | 0 | - | reference; PASS |
| sub0_b1_mixin (vs handwritten_runtime) | ok | 34.0 (+0.0) | 17 (+0) | 14 (+0) | 31 (+0) | 0/0 | 2154 (+0) | 664 (+0) | 0 | - | PASS |
| sub0_b1_wire (vs handwritten_runtime) | ok | 34.0 (+0.0) | 17 (+0) | 14 (+0) | 31 (+0) | 0/0 | 2154 (+0) | 664 (+0) | 0 | - | PASS |
| sub0_b2_static | ok | 34.0 (+0.0) | 17 (+0) | 14 (+0) | 31 (+0) | 0/0 | 2154 (+0) | 664 (+0) | 0 | - | PASS |
| sub0_b3_sink (vs handwritten_erased) | ok | 59.0 (+0.0) | 26 (+0) | 14 (+0) | 57 (+38) | 2/0 | 2388 (+6) | 712 (+0) | 0 | - | FAIL: publish path |
| sub0pub_virtual | ok | 107.0 (+73.0) | 68 (+51) | 112 (+98) | 46 (+15) | 0/2 | 5929 (+3775) | 1248 (+584) | 508 | - | FAIL: publish instr, setup instr, teardown instr, publish path, no extra indirect calls, no extra RAM, no Sub0Pub retained |
| sub0pub_virtual_lean | ok | 107.0 (+73.0) | 68 (+51) | 112 (+98) | 46 (+15) | 0/2 | 5929 (+3775) | 1248 (+584) | 508 | - | FAIL: publish instr, setup instr, teardown instr, publish path, no extra indirect calls, no extra RAM, no Sub0Pub retained |

### clang-O2, removable work

| variant | checksum | publish instr | setup instr | teardown instr | path instr | calls (direct/indirect) | text | data+bss | retained sub0 (B) | added deps | verdict |
|---|---|---|---|---|---|---|---|---|---|---|---|
| handwritten | ok | 9.0 (+0.0) | 17 (+0) | 14 (+0) | 4 (+0) | 0/0 | 2074 (+0) | 664 (+0) | 0 | - | reference |
| handwritten_erased | ok | 18.0 (+9.0) | 26 (+9) | 14 (+0) | 12 (+8) | 0/1 | 2218 (+144) | 712 (+48) | 0 | - | reference; FAIL: publish instr, setup instr, publish path, no extra indirect calls, no extra RAM |
| handwritten_runtime | ok | 9.0 (+0.0) | 17 (+0) | 14 (+0) | 4 (+0) | 0/0 | 2074 (+0) | 664 (+0) | 0 | - | reference; PASS |
| sub0_b1_mixin (vs handwritten_runtime) | ok | 9.0 (+0.0) | 17 (+0) | 14 (+0) | 4 (+0) | 0/0 | 2074 (+0) | 664 (+0) | 0 | - | PASS |
| sub0_b1_wire (vs handwritten_runtime) | ok | 9.0 (+0.0) | 17 (+0) | 14 (+0) | 4 (+0) | 0/0 | 2074 (+0) | 664 (+0) | 0 | - | PASS |
| sub0_b2_static | ok | 9.0 (+0.0) | 17 (+0) | 14 (+0) | 4 (+0) | 0/0 | 2074 (+0) | 664 (+0) | 0 | - | PASS |
| sub0_b3_sink (vs handwritten_erased) | ok | 17.0 (-1.0) | 26 (+0) | 14 (+0) | 14 (+2) | 1/0 | 2224 (+6) | 712 (+0) | 0 | - | PASS |
| sub0pub_virtual | ok | 71.0 (+62.0) | 68 (+51) | 112 (+98) | 46 (+42) | 0/2 | 5842 (+3768) | 1248 (+584) | 508 | - | FAIL: publish instr, setup instr, teardown instr, publish path, no extra indirect calls, no extra RAM, no Sub0Pub retained |
| sub0pub_virtual_lean | ok | 71.0 (+62.0) | 68 (+51) | 112 (+98) | 46 (+42) | 0/2 | 5842 (+3768) | 1248 (+584) | 508 | - | FAIL: publish instr, setup instr, teardown instr, publish path, no extra indirect calls, no extra RAM, no Sub0Pub retained |

<details><summary>clang-O2: largest symbols added by handwritten_erased (bytes)</summary>

- 74 `collapse_setup`
- 56 `(anonymous namespace)::deliverSample(void const*, (anonymous namespace)::Sample const&)`
- 39 `(anonymous namespace)::deliverCommand(void const*, (anonymous namespace)::Command const&)`
- 24 `(anonymous namespace)::node`
- 8 `_ZN12_GLOBAL__N_16sensorE.2`
- 8 `_ZN12_GLOBAL__N_16sensorE.0`
- 4 `(anonymous namespace)::logger`
- 1 `(anonymous namespace)::controller`

</details>

<details><summary>clang-O2: largest symbols added by sub0_b3_sink (bytes)</summary>

- 56 `_ZZN4sub04SinkIN12_GLOBAL__N_16SampleEEC1INS_6WiringIJNS1_10ControllerENS1_6LoggerENS1_8ActuatorEEEETnNSt9enable_ifIXntsr3stdE9is_same_vINSt9remove_cvIT_E4typeES3_EEiE4typeELi0EEERSC_ENUlPKvRKS2_E_8__invokeESJ_SL_`
- 39 `_ZZN4sub04SinkIN12_GLOBAL__N_17CommandEEC1INS_6WiringIJNS1_10ControllerENS1_6LoggerENS1_8ActuatorEEEETnNSt9enable_ifIXntsr3stdE9is_same_vINSt9remove_cvIT_E4typeES3_EEiE4typeELi0EEERSC_ENUlPKvRKS2_E_8__invokeESJ_SL_`
- 24 `(anonymous namespace)::bus`

</details>

<details><summary>clang-O2: largest symbols added by sub0pub_virtual (bytes)</summary>

- 1464 `collapse_teardown`
- 289 `collapse_setup`
- 151 `collapse_publish`
- 72 `vtable for (anonymous namespace)::Controller`
- 72 `sub0::detail::BrokerImpl<(anonymous namespace)::Sample, sub0::detail::BuiltinT<8u, (sub0::Dispatch)1, (sub0::Context)2, false, sub0::NoLock> >::global_`
- 72 `sub0::detail::BrokerImpl<(anonymous namespace)::Command, sub0::detail::BuiltinT<8u, (sub0::Dispatch)1, (sub0::Context)2, false, sub0::NoLock> >::global_`
- 66 `typeinfo name for sub0::detail::SubscriberInterface<(anonymous namespace)::Command, false>`
- 65 `typeinfo name for sub0::detail::SubscriberInterface<(anonymous namespace)::Sample, false>`

</details>

<details><summary>clang-O2: largest symbols added by sub0pub_virtual_lean (bytes)</summary>

- 1464 `collapse_teardown`
- 289 `collapse_setup`
- 151 `collapse_publish`
- 72 `vtable for (anonymous namespace)::Controller`
- 72 `sub0::detail::BrokerImpl<(anonymous namespace)::Sample, sub0::detail::BuiltinT<8u, (sub0::Dispatch)1, (sub0::Context)2, false, sub0::NoLock> >::global_`
- 72 `sub0::detail::BrokerImpl<(anonymous namespace)::Command, sub0::detail::BuiltinT<8u, (sub0::Dispatch)1, (sub0::Context)2, false, sub0::NoLock> >::global_`
- 66 `typeinfo name for sub0::detail::SubscriberInterface<(anonymous namespace)::Command, false>`
- 65 `typeinfo name for sub0::detail::SubscriberInterface<(anonymous namespace)::Sample, false>`

</details>

### cm33-gcc-Os, observable work

| variant | checksum | publish instr | setup instr | teardown instr | path instr | calls (direct/indirect) | text | data+bss | retained sub0 (B) | added deps | verdict |
|---|---|---|---|---|---|---|---|---|---|---|---|
| handwritten | - | - | - | - | 30 (+0) | 4/0 | 1168 (+0) | 512 (+0) | 0 | - | reference |
| handwritten_erased | - | - | - | - | 23 (-7) | 0/2 | 1268 (+100) | 548 (+36) | 0 | - | reference; FAIL: no extra indirect calls, no extra RAM |
| handwritten_runtime | - | - | - | - | 32 (+2) | 4/0 | 1196 (+28) | 532 (+20) | 0 | - | reference; FAIL: no extra RAM |
| sub0_b1_mixin (vs handwritten_runtime) | - | - | - | - | 32 (+0) | 4/0 | 1196 (+0) | 532 (+0) | 0 | - | PASS |
| sub0_b1_wire (vs handwritten_runtime) | - | - | - | - | 32 (+0) | 4/0 | 1196 (+0) | 532 (+0) | 0 | - | PASS |
| sub0_b2_static | - | - | - | - | 30 (+0) | 4/0 | 1168 (+0) | 512 (+0) | 0 | - | PASS |
| sub0_b3_sink (vs handwritten_erased) | - | - | - | - | 23 (+0) | 0/2 | 1268 (+0) | 548 (+0) | 56 | - | PASS |
| sub0pub_virtual | - | - | - | - | 40 (+10) | 0/2 | 1920 (+752) | 616 (+104) | 252 | - | FAIL: publish path, no extra indirect calls, no extra RAM, no Sub0Pub retained |
| sub0pub_virtual_lean | - | - | - | - | 40 (+10) | 0/2 | 1920 (+752) | 616 (+104) | 252 | - | FAIL: publish path, no extra indirect calls, no extra RAM, no Sub0Pub retained |

### cm33-gcc-Os, removable work

| variant | checksum | publish instr | setup instr | teardown instr | path instr | calls (direct/indirect) | text | data+bss | retained sub0 (B) | added deps | verdict |
|---|---|---|---|---|---|---|---|---|---|---|---|
| handwritten | - | - | - | - | 12 (+0) | 0/0 | 1116 (+0) | 512 (+0) | 0 | - | reference |
| handwritten_erased | - | - | - | - | 23 (+11) | 0/2 | 1204 (+88) | 548 (+36) | 0 | - | reference; FAIL: publish path, no extra indirect calls, no extra RAM |
| handwritten_runtime | - | - | - | - | 12 (+0) | 0/0 | 1140 (+24) | 532 (+20) | 0 | - | reference; FAIL: no extra RAM |
| sub0_b1_mixin (vs handwritten_runtime) | - | - | - | - | 12 (+0) | 0/0 | 1140 (+0) | 532 (+0) | 0 | - | PASS |
| sub0_b1_wire (vs handwritten_runtime) | - | - | - | - | 12 (+0) | 0/0 | 1140 (+0) | 532 (+0) | 0 | - | PASS |
| sub0_b2_static | - | - | - | - | 12 (+0) | 0/0 | 1116 (+0) | 512 (+0) | 0 | - | PASS |
| sub0_b3_sink (vs handwritten_erased) | - | - | - | - | 23 (+0) | 0/2 | 1204 (+0) | 548 (+0) | 12 | - | PASS |
| sub0pub_virtual | - | - | - | - | 40 (+28) | 0/2 | 1848 (+732) | 616 (+104) | 252 | - | FAIL: publish path, no extra indirect calls, no extra RAM, no Sub0Pub retained |
| sub0pub_virtual_lean | - | - | - | - | 40 (+28) | 0/2 | 1848 (+732) | 616 (+104) | 252 | - | FAIL: publish path, no extra indirect calls, no extra RAM, no Sub0Pub retained |

<details><summary>cm33-gcc-Os: largest symbols added by handwritten_erased (bytes)</summary>

- 64 `collapse_setup`
- 34 `(anonymous namespace)::deliverSample(void const*, (anonymous namespace)::Sample const&)`
- 22 `(anonymous namespace)::deliverCommand(void const*, (anonymous namespace)::Command const&)`
- 16 `(anonymous namespace)::sensor`
- 12 `(anonymous namespace)::node`
- 1 `(anonymous namespace)::controller`
- 1 `(anonymous namespace)::actuator`

</details>

<details><summary>cm33-gcc-Os: largest symbols added by handwritten_runtime (bytes)</summary>

- 64 `collapse_publish`
- 36 `collapse_setup`
- 12 `(anonymous namespace)::sensor`
- 1 `(anonymous namespace)::controller`
- 1 `(anonymous namespace)::actuator`

</details>

<details><summary>cm33-gcc-Os: largest symbols added by sub0_b3_sink (bytes)</summary>

- 34 `sub0::Sink<(anonymous namespace)::Sample>::Sink<sub0::Wiring<(anonymous namespace)::Controller, (anonymous namespace)::Logger, (anonymous namespace)::Actuator>, 0>(sub0::Wiring<(anonymous namespace)::Controller, (anonymous namespace)::Logger, (anonymous namespace)::Actuator>&)::{lambda(void const*, (anonymous namespace)::Sample const&)#1}::_FUN(void const*, (anonymous namespace)::Sample const&)`
- 22 `sub0::Sink<(anonymous namespace)::Command>::Sink<sub0::Wiring<(anonymous namespace)::Controller, (anonymous namespace)::Logger, (anonymous namespace)::Actuator>, 0>(sub0::Wiring<(anonymous namespace)::Controller, (anonymous namespace)::Logger, (anonymous namespace)::Actuator>&)::{lambda(void const*, (anonymous namespace)::Command const&)#1}::_FUN(void const*, (anonymous namespace)::Command const&)`
- 12 `(anonymous namespace)::bus`

</details>

<details><summary>cm33-gcc-Os: largest symbols added by sub0pub_virtual (bytes)</summary>

- 254 `memmove`
- 92 `collapse_setup`
- 92 `collapse_publish`
- 76 `collapse_teardown`
- 56 `sub0::detail::BrokerImpl<(anonymous namespace)::Sample, sub0::detail::BuiltinT<8ul, (sub0::Dispatch)1, (sub0::Context)2, false, sub0::NoLock> >::disconnect(sub0::Subscribe<(anonymous namespace)::Sample>*) [clone .isra.0]`
- 56 `sub0::detail::BrokerImpl<(anonymous namespace)::Command, sub0::detail::BuiltinT<8ul, (sub0::Dispatch)1, (sub0::Context)2, false, sub0::NoLock> >::disconnect(sub0::Subscribe<(anonymous namespace)::Command>*) [clone .isra.0]`
- 36 `vtable for (anonymous namespace)::Controller`
- 36 `sub0::detail::BrokerImpl<(anonymous namespace)::Sample, sub0::detail::BuiltinT<8ul, (sub0::Dispatch)1, (sub0::Context)2, false, sub0::NoLock> >::global_`

</details>

<details><summary>cm33-gcc-Os: largest symbols added by sub0pub_virtual_lean (bytes)</summary>

- 254 `memmove`
- 92 `collapse_setup`
- 92 `collapse_publish`
- 76 `collapse_teardown`
- 56 `sub0::detail::BrokerImpl<(anonymous namespace)::Sample, sub0::detail::BuiltinT<8ul, (sub0::Dispatch)1, (sub0::Context)2, false, sub0::NoLock> >::disconnect(sub0::Subscribe<(anonymous namespace)::Sample>*) [clone .isra.0]`
- 56 `sub0::detail::BrokerImpl<(anonymous namespace)::Command, sub0::detail::BuiltinT<8ul, (sub0::Dispatch)1, (sub0::Context)2, false, sub0::NoLock> >::disconnect(sub0::Subscribe<(anonymous namespace)::Command>*) [clone .isra.0]`
- 36 `vtable for (anonymous namespace)::Controller`
- 36 `sub0::detail::BrokerImpl<(anonymous namespace)::Sample, sub0::detail::BuiltinT<8ul, (sub0::Dispatch)1, (sub0::Context)2, false, sub0::NoLock> >::global_`

</details>

## Case: nested_publish

### gcc-O2, observable work

| variant | checksum | publish instr | setup instr | teardown instr | path instr | calls (direct/indirect) | text | data+bss | retained sub0 (B) | added deps | verdict |
|---|---|---|---|---|---|---|---|---|---|---|---|
| handwritten | ok | 56.0 (+0.0) | 18 (+0) | 16 (+0) | 85 (+0) | 4/0 | 2683 (+0) | 616 (+0) | 0 | - | reference |
| handwritten_runtime | ok | 56.0 (+0.0) | 26 (+8) | 16 (+0) | 85 (+0) | 4/0 | 2731 (+48) | 656 (+40) | 0 | - | reference; FAIL: setup instr, no extra RAM |
| sub0_b1_wire (vs handwritten_runtime) | ok | 54.5 (-1.5) | 26 (+0) | 16 (+0) | 246 (+161) | 4/0 | 3267 (+536) | 656 (+0) | 269 | - | FAIL: publish path, no Sub0Pub retained |
| sub0_b2_static | ok | 54.5 (-1.5) | 18 (+0) | 16 (+0) | 246 (+161) | 4/0 | 3219 (+536) | 616 (+0) | 269 | - | FAIL: publish path, no Sub0Pub retained |
| sub0pub_virtual | ok | 137.5 (+81.5) | 53 (+35) | 80 (+64) | 37 (-48) | 1/1 | 5425 (+2742) | 1208 (+592) | 573 | - | FAIL: publish instr, setup instr, teardown instr, no extra indirect calls, no extra RAM, no Sub0Pub retained |
| sub0pub_virtual_lean | ok | 137.5 (+81.5) | 53 (+35) | 80 (+64) | 37 (-48) | 1/1 | 5425 (+2742) | 1208 (+592) | 573 | - | FAIL: publish instr, setup instr, teardown instr, no extra indirect calls, no extra RAM, no Sub0Pub retained |

### gcc-O2, removable work

| variant | checksum | publish instr | setup instr | teardown instr | path instr | calls (direct/indirect) | text | data+bss | retained sub0 (B) | added deps | verdict |
|---|---|---|---|---|---|---|---|---|---|---|---|
| handwritten | ok | 8.0 (+0.0) | 18 (+0) | 16 (+0) | 4 (+0) | 0/0 | 2319 (+0) | 616 (+0) | 0 | - | reference |
| handwritten_runtime | ok | 8.0 (+0.0) | 26 (+8) | 16 (+0) | 4 (+0) | 0/0 | 2367 (+48) | 656 (+40) | 0 | - | reference; FAIL: setup instr, no extra RAM |
| sub0_b1_wire (vs handwritten_runtime) | ok | 8.0 (+0.0) | 26 (+0) | 16 (+0) | 4 (+0) | 0/0 | 2367 (+0) | 656 (+0) | 0 | - | PASS |
| sub0_b2_static | ok | 8.0 (+0.0) | 18 (+0) | 16 (+0) | 4 (+0) | 0/0 | 2319 (+0) | 616 (+0) | 0 | - | PASS |
| sub0pub_virtual | ok | 92.5 (+84.5) | 53 (+35) | 80 (+64) | 37 (+33) | 1/1 | 5225 (+2906) | 1208 (+592) | 573 | - | FAIL: publish instr, setup instr, teardown instr, publish path, no extra indirect calls, no extra RAM, no Sub0Pub retained |
| sub0pub_virtual_lean | ok | 92.5 (+84.5) | 53 (+35) | 80 (+64) | 37 (+33) | 1/1 | 5225 (+2906) | 1208 (+592) | 573 | - | FAIL: publish instr, setup instr, teardown instr, publish path, no extra indirect calls, no extra RAM, no Sub0Pub retained |

<details><summary>gcc-O2: largest symbols added by handwritten_runtime (bytes)</summary>

- 147 `(anonymous namespace)::Node::publish((anonymous namespace)::Sample const&) const [clone .isra.0]`
- 61 `collapse_setup`
- 24 `(anonymous namespace)::node`
- 8 `(anonymous namespace)::relay`
- 1 `(anonymous namespace)::tail`
- 1 `(anonymous namespace)::actuator`

</details>

<details><summary>gcc-O2: largest symbols added by sub0_b1_wire (bytes)</summary>

- 516 `collapse_publish`
- 269 `void sub0::Wiring<(anonymous namespace)::Relay, (anonymous namespace)::Actuator, (anonymous namespace)::Tail>::publish<(anonymous namespace)::Sample>((anonymous namespace)::Sample const&) const [clone .isra.0]`
- 24 `(anonymous namespace)::bus`

</details>

<details><summary>gcc-O2: largest symbols added by sub0_b2_static (bytes)</summary>

- 516 `collapse_publish`
- 269 `void sub0::StaticWiring<&(anonymous namespace)::relay, &(anonymous namespace)::actuator, &(anonymous namespace)::tail>::publish<(anonymous namespace)::Sample>((anonymous namespace)::Sample const&)`

</details>

<details><summary>gcc-O2: largest symbols added by sub0pub_virtual (bytes)</summary>

- 427 `collapse_teardown`
- 356 `collapse_setup`
- 213 `(anonymous namespace)::Relay::receive((anonymous namespace)::Sample const&)`
- 72 `typeinfo for (anonymous namespace)::Relay`
- 72 `sub0::detail::BrokerImpl<(anonymous namespace)::Sample, sub0::detail::BuiltinT<8u, (sub0::Dispatch)1, (sub0::Context)2, false, sub0::NoLock> >::global_`
- 72 `sub0::detail::BrokerImpl<(anonymous namespace)::Command, sub0::detail::BuiltinT<8u, (sub0::Dispatch)1, (sub0::Context)2, false, sub0::NoLock> >::global_`
- 67 `typeinfo name for sub0::detail::SubscriberInterface<(anonymous namespace)::Command, false>`
- 66 `typeinfo name for sub0::detail::SubscriberInterface<(anonymous namespace)::Sample, false>`

</details>

<details><summary>gcc-O2: largest symbols added by sub0pub_virtual_lean (bytes)</summary>

- 427 `collapse_teardown`
- 356 `collapse_setup`
- 213 `(anonymous namespace)::Relay::receive((anonymous namespace)::Sample const&)`
- 72 `typeinfo for (anonymous namespace)::Relay`
- 72 `sub0::detail::BrokerImpl<(anonymous namespace)::Sample, sub0::detail::BuiltinT<8u, (sub0::Dispatch)1, (sub0::Context)2, false, sub0::NoLock> >::global_`
- 72 `sub0::detail::BrokerImpl<(anonymous namespace)::Command, sub0::detail::BuiltinT<8u, (sub0::Dispatch)1, (sub0::Context)2, false, sub0::NoLock> >::global_`
- 67 `typeinfo name for sub0::detail::SubscriberInterface<(anonymous namespace)::Command, false>`
- 66 `typeinfo name for sub0::detail::SubscriberInterface<(anonymous namespace)::Sample, false>`

</details>

### clang-O2, observable work

| variant | checksum | publish instr | setup instr | teardown instr | path instr | calls (direct/indirect) | text | data+bss | retained sub0 (B) | added deps | verdict |
|---|---|---|---|---|---|---|---|---|---|---|---|
| handwritten | ok | 61.5 (+0.0) | 16 (+0) | 14 (+0) | 48 (+0) | 2/0 | 2254 (+0) | 656 (+0) | 0 | - | reference |
| handwritten_runtime | ok | 65.5 (+4.0) | 24 (+8) | 14 (+0) | 51 (+3) | 2/0 | 2318 (+64) | 696 (+40) | 0 | - | reference; FAIL: publish instr, setup instr, publish path, no extra RAM |
| sub0_b1_wire (vs handwritten_runtime) | ok | 65.5 (+0.0) | 24 (+0) | 14 (+0) | 51 (+0) | 2/0 | 2318 (+0) | 696 (+0) | 108 | - | PASS |
| sub0_b2_static | ok | 65.5 (+4.0) | 16 (+0) | 14 (+0) | 55 (+7) | 2/0 | 2292 (+38) | 656 (+0) | 0 | - | FAIL: publish instr, publish path |
| sub0pub_virtual | ok | 136.5 (+75.0) | 53 (+37) | 84 (+70) | 28 (-20) | 0/1 | 5554 (+3300) | 1240 (+584) | 623 | - | FAIL: publish instr, setup instr, teardown instr, no extra indirect calls, no extra RAM, no Sub0Pub retained |
| sub0pub_virtual_lean | ok | 136.5 (+75.0) | 53 (+37) | 84 (+70) | 28 (-20) | 0/1 | 5554 (+3300) | 1240 (+584) | 623 | - | FAIL: publish instr, setup instr, teardown instr, no extra indirect calls, no extra RAM, no Sub0Pub retained |

### clang-O2, removable work

| variant | checksum | publish instr | setup instr | teardown instr | path instr | calls (direct/indirect) | text | data+bss | retained sub0 (B) | added deps | verdict |
|---|---|---|---|---|---|---|---|---|---|---|---|
| handwritten | ok | 8.0 (+0.0) | 16 (+0) | 14 (+0) | 4 (+0) | 0/0 | 2074 (+0) | 656 (+0) | 0 | - | reference |
| handwritten_runtime | ok | 8.0 (+0.0) | 24 (+8) | 14 (+0) | 4 (+0) | 0/0 | 2122 (+48) | 696 (+40) | 0 | - | reference; FAIL: setup instr, no extra RAM |
| sub0_b1_wire (vs handwritten_runtime) | ok | 8.0 (+0.0) | 24 (+0) | 14 (+0) | 4 (+0) | 0/0 | 2122 (+0) | 696 (+0) | 0 | - | PASS |
| sub0_b2_static | ok | 8.0 (+0.0) | 16 (+0) | 14 (+0) | 4 (+0) | 0/0 | 2074 (+0) | 656 (+0) | 0 | - | PASS |
| sub0pub_virtual | ok | 102.0 (+94.0) | 53 (+37) | 84 (+70) | 28 (+24) | 0/1 | 5497 (+3423) | 1240 (+584) | 623 | - | FAIL: publish instr, setup instr, teardown instr, publish path, no extra indirect calls, no extra RAM, no Sub0Pub retained |
| sub0pub_virtual_lean | ok | 102.0 (+94.0) | 53 (+37) | 84 (+70) | 28 (+24) | 0/1 | 5497 (+3423) | 1240 (+584) | 623 | - | FAIL: publish instr, setup instr, teardown instr, publish path, no extra indirect calls, no extra RAM, no Sub0Pub retained |

<details><summary>clang-O2: largest symbols added by handwritten_runtime (bytes)</summary>

- 107 `(anonymous namespace)::Node::publish((anonymous namespace)::Sample const&) const`
- 57 `collapse_setup`
- 33 `collapse_publish`
- 24 `(anonymous namespace)::node`
- 8 `(anonymous namespace)::relay`
- 1 `(anonymous namespace)::tail`
- 1 `(anonymous namespace)::actuator`

</details>

<details><summary>clang-O2: largest symbols added by sub0_b1_wire (bytes)</summary>

- 108 `void sub0::Wiring<(anonymous namespace)::Relay, (anonymous namespace)::Actuator, (anonymous namespace)::Tail>::publish<(anonymous namespace)::Sample>((anonymous namespace)::Sample const&) const`
- 24 `(anonymous namespace)::bus`

</details>

<details><summary>clang-O2: largest symbols added by sub0_b2_static (bytes)</summary>

- 95 `(anonymous namespace)::Relay::receive((anonymous namespace)::Sample const&)`
- 59 `collapse_publish`

</details>

<details><summary>clang-O2: largest symbols added by sub0pub_virtual (bytes)</summary>

- 1078 `collapse_teardown`
- 214 `collapse_setup`
- 167 `(anonymous namespace)::Relay::receive((anonymous namespace)::Sample const&)`
- 85 `collapse_publish`
- 72 `typeinfo for (anonymous namespace)::Relay`
- 72 `sub0::detail::BrokerImpl<(anonymous namespace)::Sample, sub0::detail::BuiltinT<8u, (sub0::Dispatch)1, (sub0::Context)2, false, sub0::NoLock> >::global_`
- 72 `sub0::detail::BrokerImpl<(anonymous namespace)::Command, sub0::detail::BuiltinT<8u, (sub0::Dispatch)1, (sub0::Context)2, false, sub0::NoLock> >::global_`
- 66 `typeinfo name for sub0::detail::SubscriberInterface<(anonymous namespace)::Command, false>`

</details>

<details><summary>clang-O2: largest symbols added by sub0pub_virtual_lean (bytes)</summary>

- 1078 `collapse_teardown`
- 214 `collapse_setup`
- 167 `(anonymous namespace)::Relay::receive((anonymous namespace)::Sample const&)`
- 85 `collapse_publish`
- 72 `typeinfo for (anonymous namespace)::Relay`
- 72 `sub0::detail::BrokerImpl<(anonymous namespace)::Sample, sub0::detail::BuiltinT<8u, (sub0::Dispatch)1, (sub0::Context)2, false, sub0::NoLock> >::global_`
- 72 `sub0::detail::BrokerImpl<(anonymous namespace)::Command, sub0::detail::BuiltinT<8u, (sub0::Dispatch)1, (sub0::Context)2, false, sub0::NoLock> >::global_`
- 66 `typeinfo name for sub0::detail::SubscriberInterface<(anonymous namespace)::Command, false>`

</details>

### cm33-gcc-Os, observable work

| variant | checksum | publish instr | setup instr | teardown instr | path instr | calls (direct/indirect) | text | data+bss | retained sub0 (B) | added deps | verdict |
|---|---|---|---|---|---|---|---|---|---|---|---|
| handwritten | - | - | - | - | 40 (+0) | 2/0 | 1180 (+0) | 508 (+0) | 0 | - | reference |
| handwritten_runtime | - | - | - | - | 40 (+0) | 2/0 | 1212 (+32) | 528 (+20) | 0 | - | reference; FAIL: no extra RAM |
| sub0_b1_wire (vs handwritten_runtime) | - | - | - | - | 41 (+1) | 6/0 | 1216 (+4) | 528 (+0) | 0 | - | PASS |
| sub0_b2_static | - | - | - | - | 40 (+0) | 2/0 | 1180 (+0) | 508 (+0) | 72 | - | PASS |
| sub0pub_virtual | - | - | - | - | 23 (-17) | 0/1 | 1868 (+688) | 604 (+96) | 164 | - | FAIL: no extra indirect calls, no extra RAM, no Sub0Pub retained |
| sub0pub_virtual_lean | - | - | - | - | 23 (-17) | 0/1 | 1868 (+688) | 604 (+96) | 164 | - | FAIL: no extra indirect calls, no extra RAM, no Sub0Pub retained |

### cm33-gcc-Os, removable work

| variant | checksum | publish instr | setup instr | teardown instr | path instr | calls (direct/indirect) | text | data+bss | retained sub0 (B) | added deps | verdict |
|---|---|---|---|---|---|---|---|---|---|---|---|
| handwritten | - | - | - | - | 14 (+0) | 2/0 | 1112 (+0) | 508 (+0) | 0 | - | reference |
| handwritten_runtime | - | - | - | - | 7 (-7) | 0/0 | 1128 (+16) | 528 (+20) | 0 | - | reference; FAIL: no extra RAM |
| sub0_b1_wire (vs handwritten_runtime) | - | - | - | - | 7 (+0) | 0/0 | 1128 (+0) | 528 (+0) | 0 | - | PASS |
| sub0_b2_static | - | - | - | - | 14 (+0) | 2/0 | 1112 (+0) | 508 (+0) | 0 | - | PASS |
| sub0pub_virtual | - | - | - | - | 23 (+9) | 0/1 | 1748 (+636) | 604 (+96) | 164 | - | FAIL: publish path, no extra indirect calls, no extra RAM, no Sub0Pub retained |
| sub0pub_virtual_lean | - | - | - | - | 23 (+9) | 0/1 | 1748 (+636) | 604 (+96) | 164 | - | FAIL: publish path, no extra indirect calls, no extra RAM, no Sub0Pub retained |

<details><summary>cm33-gcc-Os: largest symbols added by handwritten_runtime (bytes)</summary>

- 72 `(anonymous namespace)::Node::publish((anonymous namespace)::Sample const&) const [clone .isra.0]`
- 36 `collapse_setup`
- 12 `(anonymous namespace)::node`
- 4 `(anonymous namespace)::relay`
- 1 `(anonymous namespace)::tail`
- 1 `(anonymous namespace)::actuator`

</details>

<details><summary>cm33-gcc-Os: largest symbols added by sub0_b1_wire (bytes)</summary>

- 48 `(anonymous namespace)::Relay::receive((anonymous namespace)::Sample const&) [clone .isra.0]`
- 36 `collapse_publish`
- 20 `collapse::work(unsigned long)`
- 12 `(anonymous namespace)::bus`

</details>

<details><summary>cm33-gcc-Os: largest symbols added by sub0_b2_static (bytes)</summary>

- 72 `void sub0::StaticWiring<&(anonymous namespace)::relay, &(anonymous namespace)::actuator, &(anonymous namespace)::tail>::publish<(anonymous namespace)::Sample>((anonymous namespace)::Sample const&)`

</details>

<details><summary>cm33-gcc-Os: largest symbols added by sub0pub_virtual (bytes)</summary>

- 254 `memmove`
- 104 `collapse_teardown`
- 104 `(anonymous namespace)::Relay::receive((anonymous namespace)::Sample const&)`
- 92 `collapse_setup`
- 56 `sub0::detail::BrokerImpl<(anonymous namespace)::Sample, sub0::detail::BuiltinT<8ul, (sub0::Dispatch)1, (sub0::Context)2, false, sub0::NoLock> >::disconnect(sub0::Subscribe<(anonymous namespace)::Sample>*) [clone .isra.0]`
- 52 `collapse_publish`
- 36 `sub0::detail::BrokerImpl<(anonymous namespace)::Sample, sub0::detail::BuiltinT<8ul, (sub0::Dispatch)1, (sub0::Context)2, false, sub0::NoLock> >::global_`
- 36 `sub0::detail::BrokerImpl<(anonymous namespace)::Command, sub0::detail::BuiltinT<8ul, (sub0::Dispatch)1, (sub0::Context)2, false, sub0::NoLock> >::global_`

</details>

<details><summary>cm33-gcc-Os: largest symbols added by sub0pub_virtual_lean (bytes)</summary>

- 254 `memmove`
- 104 `collapse_teardown`
- 104 `(anonymous namespace)::Relay::receive((anonymous namespace)::Sample const&)`
- 92 `collapse_setup`
- 56 `sub0::detail::BrokerImpl<(anonymous namespace)::Sample, sub0::detail::BuiltinT<8ul, (sub0::Dispatch)1, (sub0::Context)2, false, sub0::NoLock> >::disconnect(sub0::Subscribe<(anonymous namespace)::Sample>*) [clone .isra.0]`
- 52 `collapse_publish`
- 36 `sub0::detail::BrokerImpl<(anonymous namespace)::Sample, sub0::detail::BuiltinT<8ul, (sub0::Dispatch)1, (sub0::Context)2, false, sub0::NoLock> >::global_`
- 36 `sub0::detail::BrokerImpl<(anonymous namespace)::Command, sub0::detail::BuiltinT<8ul, (sub0::Dispatch)1, (sub0::Context)2, false, sub0::NoLock> >::global_`

</details>

## Case: one_receiver

### gcc-O2, observable work

| variant | checksum | publish instr | setup instr | teardown instr | path instr | calls (direct/indirect) | text | data+bss | retained sub0 (B) | added deps | verdict |
|---|---|---|---|---|---|---|---|---|---|---|---|
| handwritten | ok | 15.0 (+0.0) | 19 (+0) | 16 (+0) | 11 (+0) | 0/0 | 2351 (+0) | 624 (+0) | 0 | - | reference |
| handwritten_erased | ok | 31.0 (+16.0) | 25 (+6) | 16 (+0) | 17 (+6) | 1/1 | 2515 (+164) | 648 (+24) | 0 | - | reference; FAIL: publish instr, setup instr, publish path, no extra indirect calls, no extra RAM |
| handwritten_runtime | ok | 16.0 (+1.0) | 21 (+2) | 16 (+0) | 12 (+1) | 0/0 | 2367 (+16) | 632 (+8) | 0 | - | reference; FAIL: setup instr, no extra RAM |
| sub0_b1_mixin (vs handwritten_runtime) | ok | 16.0 (+0.0) | 21 (+0) | 16 (+0) | 12 (+0) | 0/0 | 2367 (+0) | 632 (+0) | 0 | - | PASS |
| sub0_b1_wire (vs handwritten_runtime) | ok | 16.0 (+0.0) | 21 (+0) | 16 (+0) | 12 (+0) | 0/0 | 2367 (+0) | 632 (+0) | 0 | - | PASS |
| sub0_b2_static | ok | 15.0 (+0.0) | 19 (+0) | 16 (+0) | 11 (+0) | 0/0 | 2351 (+0) | 624 (+0) | 0 | - | PASS |
| sub0_b3_sink (vs handwritten_erased) | ok | 31.0 (+0.0) | 25 (+0) | 16 (+0) | 17 (+0) | 1/1 | 2515 (+0) | 648 (+0) | 34 | - | PASS |
| sub0pub_virtual | ok | 26.0 (+11.0) | 31 (+12) | 36 (+20) | 23 (+12) | 0/0 | 3474 (+1123) | 848 (+224) | 227 | - | FAIL: publish instr, setup instr, teardown instr, publish path, no extra RAM, no Sub0Pub retained |
| sub0pub_virtual_lean | ok | 26.0 (+11.0) | 31 (+12) | 36 (+20) | 23 (+12) | 0/0 | 3474 (+1123) | 848 (+224) | 227 | - | FAIL: publish instr, setup instr, teardown instr, publish path, no extra RAM, no Sub0Pub retained |

### gcc-O2, removable work

| variant | checksum | publish instr | setup instr | teardown instr | path instr | calls (direct/indirect) | text | data+bss | retained sub0 (B) | added deps | verdict |
|---|---|---|---|---|---|---|---|---|---|---|---|
| handwritten | ok | 8.0 (+0.0) | 19 (+0) | 16 (+0) | 4 (+0) | 0/0 | 2319 (+0) | 624 (+0) | 0 | - | reference |
| handwritten_erased | ok | 22.0 (+14.0) | 25 (+6) | 16 (+0) | 17 (+13) | 1/1 | 2483 (+164) | 648 (+24) | 0 | - | reference; FAIL: publish instr, setup instr, publish path, no extra indirect calls, no extra RAM |
| handwritten_runtime | ok | 8.0 (+0.0) | 21 (+2) | 16 (+0) | 4 (+0) | 0/0 | 2335 (+16) | 632 (+8) | 0 | - | reference; FAIL: setup instr, no extra RAM |
| sub0_b1_mixin (vs handwritten_runtime) | ok | 8.0 (+0.0) | 21 (+0) | 16 (+0) | 4 (+0) | 0/0 | 2335 (+0) | 632 (+0) | 0 | - | PASS |
| sub0_b1_wire (vs handwritten_runtime) | ok | 8.0 (+0.0) | 21 (+0) | 16 (+0) | 4 (+0) | 0/0 | 2335 (+0) | 632 (+0) | 0 | - | PASS |
| sub0_b2_static | ok | 8.0 (+0.0) | 19 (+0) | 16 (+0) | 4 (+0) | 0/0 | 2319 (+0) | 624 (+0) | 0 | - | PASS |
| sub0_b3_sink (vs handwritten_erased) | ok | 22.0 (+0.0) | 25 (+0) | 16 (+0) | 17 (+0) | 1/1 | 2483 (+0) | 648 (+0) | 5 | - | PASS |
| sub0pub_virtual | ok | 8.0 (+0.0) | 31 (+12) | 36 (+20) | 4 (+0) | 0/0 | 3334 (+1015) | 848 (+224) | 227 | - | FAIL: setup instr, teardown instr, no extra RAM, no Sub0Pub retained |
| sub0pub_virtual_lean | ok | 8.0 (+0.0) | 31 (+12) | 36 (+20) | 4 (+0) | 0/0 | 3334 (+1015) | 848 (+224) | 227 | - | FAIL: setup instr, teardown instr, no extra RAM, no Sub0Pub retained |

<details><summary>gcc-O2: largest symbols added by handwritten_erased (bytes)</summary>

- 79 `collapse_publish`
- 57 `collapse_setup`
- 34 `(anonymous namespace)::deliverNode(void const*, (anonymous namespace)::Sample const&)`
- 16 `(anonymous namespace)::sensor`
- 8 `(anonymous namespace)::node`

</details>

<details><summary>gcc-O2: largest symbols added by handwritten_runtime (bytes)</summary>

- 43 `collapse_publish`
- 29 `collapse_setup`
- 8 `(anonymous namespace)::sensor`

</details>

<details><summary>gcc-O2: largest symbols added by sub0_b3_sink (bytes)</summary>

- 34 `sub0::Sink<(anonymous namespace)::Sample>::Sink<sub0::Wiring<(anonymous namespace)::Controller>, 0>(sub0::Wiring<(anonymous namespace)::Controller>&)::{lambda(void const*, (anonymous namespace)::Sample const&)#1}::_FUN(void const*, (anonymous namespace)::Sample const&)`
- 8 `(anonymous namespace)::bus`

</details>

<details><summary>gcc-O2: largest symbols added by sub0pub_virtual (bytes)</summary>

- 145 `collapse_teardown`
- 83 `collapse_publish`
- 82 `collapse_setup`
- 72 `sub0::detail::BrokerImpl<(anonymous namespace)::Sample, sub0::detail::BuiltinT<8u, (sub0::Dispatch)1, (sub0::Context)2, false, sub0::NoLock> >::global_`
- 66 `typeinfo name for sub0::detail::SubscriberInterface<(anonymous namespace)::Sample, false>`
- 44 `typeinfo name for sub0::Subscribe<(anonymous namespace)::Sample>`
- 32 `vtable for (anonymous namespace)::Controller`
- 32 `(anonymous namespace)::Controller::receive((anonymous namespace)::Sample const&)`

</details>

<details><summary>gcc-O2: largest symbols added by sub0pub_virtual_lean (bytes)</summary>

- 145 `collapse_teardown`
- 83 `collapse_publish`
- 82 `collapse_setup`
- 72 `sub0::detail::BrokerImpl<(anonymous namespace)::Sample, sub0::detail::BuiltinT<8u, (sub0::Dispatch)1, (sub0::Context)2, false, sub0::NoLock> >::global_`
- 66 `typeinfo name for sub0::detail::SubscriberInterface<(anonymous namespace)::Sample, false>`
- 44 `typeinfo name for sub0::Subscribe<(anonymous namespace)::Sample>`
- 32 `vtable for (anonymous namespace)::Controller`
- 32 `(anonymous namespace)::Controller::receive((anonymous namespace)::Sample const&)`

</details>

### clang-O2, observable work

| variant | checksum | publish instr | setup instr | teardown instr | path instr | calls (direct/indirect) | text | data+bss | retained sub0 (B) | added deps | verdict |
|---|---|---|---|---|---|---|---|---|---|---|---|
| handwritten | ok | 18.0 (+0.0) | 17 (+0) | 14 (+0) | 14 (+0) | 0/0 | 2106 (+0) | 664 (+0) | 0 | - | reference |
| handwritten_erased | ok | 26.0 (+8.0) | 21 (+4) | 14 (+0) | 12 (-2) | 0/1 | 2202 (+96) | 680 (+16) | 0 | - | reference; FAIL: publish instr, setup instr, no extra indirect calls, no extra RAM |
| handwritten_runtime | ok | 17.0 (-1.0) | 19 (+2) | 14 (+0) | 13 (-1) | 0/0 | 2122 (+16) | 672 (+8) | 0 | - | reference; FAIL: setup instr, no extra RAM |
| sub0_b1_mixin (vs handwritten_runtime) | ok | 17.0 (+0.0) | 19 (+0) | 14 (+0) | 13 (+0) | 0/0 | 2122 (+0) | 672 (+0) | 0 | - | PASS |
| sub0_b1_wire (vs handwritten_runtime) | ok | 17.0 (+0.0) | 19 (+0) | 14 (+0) | 13 (+0) | 0/0 | 2122 (+0) | 672 (+0) | 0 | - | PASS |
| sub0_b2_static | ok | 18.0 (+0.0) | 17 (+0) | 14 (+0) | 14 (+0) | 0/0 | 2106 (+0) | 664 (+0) | 0 | - | PASS |
| sub0_b3_sink (vs handwritten_erased) | ok | 25.0 (-1.0) | 21 (+0) | 14 (+0) | 22 (+10) | 1/0 | 2214 (+12) | 680 (+0) | 0 | - | FAIL: publish path |
| sub0pub_virtual | ok | 41.0 (+23.0) | 29 (+12) | 34 (+20) | 28 (+14) | 0/1 | 3378 (+1272) | 872 (+208) | 253 | - | FAIL: publish instr, setup instr, teardown instr, publish path, no extra indirect calls, no extra RAM, no Sub0Pub retained |
| sub0pub_virtual_lean | ok | 41.0 (+23.0) | 29 (+12) | 34 (+20) | 28 (+14) | 0/1 | 3378 (+1272) | 872 (+208) | 253 | - | FAIL: publish instr, setup instr, teardown instr, publish path, no extra indirect calls, no extra RAM, no Sub0Pub retained |

### clang-O2, removable work

| variant | checksum | publish instr | setup instr | teardown instr | path instr | calls (direct/indirect) | text | data+bss | retained sub0 (B) | added deps | verdict |
|---|---|---|---|---|---|---|---|---|---|---|---|
| handwritten | ok | 8.0 (+0.0) | 16 (+0) | 14 (+0) | 4 (+0) | 0/0 | 2074 (+0) | 656 (+0) | 0 | - | reference |
| handwritten_erased | ok | 8.0 (+0.0) | 21 (+5) | 14 (+0) | 4 (+0) | 0/0 | 2106 (+32) | 680 (+24) | 0 | - | reference; FAIL: setup instr, no extra RAM |
| handwritten_runtime | ok | 8.0 (+0.0) | 16 (+0) | 14 (+0) | 4 (+0) | 0/0 | 2074 (+0) | 656 (+0) | 0 | - | reference; PASS |
| sub0_b1_mixin (vs handwritten_runtime) | ok | 8.0 (+0.0) | 16 (+0) | 14 (+0) | 4 (+0) | 0/0 | 2074 (+0) | 656 (+0) | 0 | - | PASS |
| sub0_b1_wire (vs handwritten_runtime) | ok | 8.0 (+0.0) | 16 (+0) | 14 (+0) | 4 (+0) | 0/0 | 2074 (+0) | 656 (+0) | 0 | - | PASS |
| sub0_b2_static | ok | 8.0 (+0.0) | 16 (+0) | 14 (+0) | 4 (+0) | 0/0 | 2074 (+0) | 656 (+0) | 0 | - | PASS |
| sub0_b3_sink (vs handwritten_erased) | ok | 8.0 (+0.0) | 21 (+0) | 14 (+0) | 4 (+0) | 0/0 | 2106 (+0) | 680 (+0) | 0 | - | PASS |
| sub0pub_virtual | ok | 32.0 (+24.0) | 29 (+13) | 34 (+20) | 28 (+24) | 0/1 | 3362 (+1288) | 872 (+216) | 253 | - | FAIL: publish instr, setup instr, teardown instr, publish path, no extra indirect calls, no extra RAM, no Sub0Pub retained |
| sub0pub_virtual_lean | ok | 32.0 (+24.0) | 29 (+13) | 34 (+20) | 28 (+24) | 0/1 | 3362 (+1288) | 872 (+216) | 253 | - | FAIL: publish instr, setup instr, teardown instr, publish path, no extra indirect calls, no extra RAM, no Sub0Pub retained |

<details><summary>clang-O2: largest symbols added by handwritten_erased (bytes)</summary>

- 39 `collapse_setup`
- 29 `(anonymous namespace)::deliverNode(void const*, (anonymous namespace)::Sample const&)`
- 8 `_ZN12_GLOBAL__N_16sensorE.0`
- 8 `(anonymous namespace)::node`
- 4 `(anonymous namespace)::controller`

</details>

<details><summary>clang-O2: largest symbols added by handwritten_runtime (bytes)</summary>

- 25 `collapse_setup`
- 8 `(anonymous namespace)::sensor`
- 4 `(anonymous namespace)::controller`

</details>

<details><summary>clang-O2: largest symbols added by sub0_b3_sink (bytes)</summary>

- 29 `_ZZN4sub04SinkIN12_GLOBAL__N_16SampleEEC1INS_6WiringIJNS1_10ControllerEEEETnNSt9enable_ifIXntsr3stdE9is_same_vINSt9remove_cvIT_E4typeES3_EEiE4typeELi0EEERSA_ENUlPKvRKS2_E_8__invokeESH_SJ_`
- 8 `(anonymous namespace)::bus`

</details>

<details><summary>clang-O2: largest symbols added by sub0pub_virtual (bytes)</summary>

- 341 `collapse_teardown`
- 85 `collapse_publish`
- 79 `collapse_setup`
- 72 `sub0::detail::BrokerImpl<(anonymous namespace)::Sample, sub0::detail::BuiltinT<8u, (sub0::Dispatch)1, (sub0::Context)2, false, sub0::NoLock> >::global_`
- 65 `typeinfo name for sub0::detail::SubscriberInterface<(anonymous namespace)::Sample, false>`
- 43 `typeinfo name for sub0::Subscribe<(anonymous namespace)::Sample>`
- 32 `vtable for sub0::Subscribe<(anonymous namespace)::Sample>`
- 32 `vtable for (anonymous namespace)::Controller`

</details>

<details><summary>clang-O2: largest symbols added by sub0pub_virtual_lean (bytes)</summary>

- 341 `collapse_teardown`
- 85 `collapse_publish`
- 79 `collapse_setup`
- 72 `sub0::detail::BrokerImpl<(anonymous namespace)::Sample, sub0::detail::BuiltinT<8u, (sub0::Dispatch)1, (sub0::Context)2, false, sub0::NoLock> >::global_`
- 65 `typeinfo name for sub0::detail::SubscriberInterface<(anonymous namespace)::Sample, false>`
- 43 `typeinfo name for sub0::Subscribe<(anonymous namespace)::Sample>`
- 32 `vtable for sub0::Subscribe<(anonymous namespace)::Sample>`
- 32 `vtable for (anonymous namespace)::Controller`

</details>

### cm33-gcc-Os, observable work

| variant | checksum | publish instr | setup instr | teardown instr | path instr | calls (direct/indirect) | text | data+bss | retained sub0 (B) | added deps | verdict |
|---|---|---|---|---|---|---|---|---|---|---|---|
| handwritten | - | - | - | - | 15 (+0) | 0/0 | 1128 (+0) | 512 (+0) | 0 | - | reference |
| handwritten_erased | - | - | - | - | 15 (+0) | 0/1 | 1176 (+48) | 524 (+12) | 0 | - | reference; FAIL: no extra indirect calls, no extra RAM |
| handwritten_runtime | - | - | - | - | 17 (+2) | 0/0 | 1140 (+12) | 516 (+4) | 0 | - | reference; FAIL: no extra RAM |
| sub0_b1_mixin (vs handwritten_runtime) | - | - | - | - | 17 (+0) | 0/0 | 1140 (+0) | 516 (+0) | 0 | - | PASS |
| sub0_b1_wire (vs handwritten_runtime) | - | - | - | - | 17 (+0) | 0/0 | 1140 (+0) | 516 (+0) | 0 | - | PASS |
| sub0_b2_static | - | - | - | - | 15 (+0) | 0/0 | 1128 (+0) | 512 (+0) | 0 | - | PASS |
| sub0_b3_sink (vs handwritten_erased) | - | - | - | - | 15 (+0) | 0/1 | 1176 (+0) | 524 (+0) | 28 | - | PASS |
| sub0pub_virtual | - | - | - | - | 31 (+16) | 1/0 | 1544 (+416) | 556 (+44) | 38 | - | FAIL: publish path, no extra RAM, no Sub0Pub retained |
| sub0pub_virtual_lean | - | - | - | - | 31 (+16) | 1/0 | 1544 (+416) | 556 (+44) | 38 | - | FAIL: publish path, no extra RAM, no Sub0Pub retained |

### cm33-gcc-Os, removable work

| variant | checksum | publish instr | setup instr | teardown instr | path instr | calls (direct/indirect) | text | data+bss | retained sub0 (B) | added deps | verdict |
|---|---|---|---|---|---|---|---|---|---|---|---|
| handwritten | - | - | - | - | 7 (+0) | 0/0 | 1104 (+0) | 512 (+0) | 0 | - | reference |
| handwritten_erased | - | - | - | - | 15 (+8) | 0/1 | 1152 (+48) | 524 (+12) | 0 | - | reference; FAIL: publish path, no extra indirect calls, no extra RAM |
| handwritten_runtime | - | - | - | - | 7 (+0) | 0/0 | 1112 (+8) | 516 (+4) | 0 | - | reference; FAIL: no extra RAM |
| sub0_b1_mixin (vs handwritten_runtime) | - | - | - | - | 7 (+0) | 0/0 | 1112 (+0) | 516 (+0) | 0 | - | PASS |
| sub0_b1_wire (vs handwritten_runtime) | - | - | - | - | 7 (+0) | 0/0 | 1112 (+0) | 516 (+0) | 0 | - | PASS |
| sub0_b2_static | - | - | - | - | 7 (+0) | 0/0 | 1104 (+0) | 512 (+0) | 0 | - | PASS |
| sub0_b3_sink (vs handwritten_erased) | - | - | - | - | 15 (+0) | 0/1 | 1152 (+0) | 524 (+0) | 2 | - | PASS |
| sub0pub_virtual | - | - | - | - | 7 (+0) | 0/0 | 1484 (+380) | 556 (+44) | 38 | - | FAIL: no extra RAM, no Sub0Pub retained |
| sub0pub_virtual_lean | - | - | - | - | 7 (+0) | 0/0 | 1484 (+380) | 556 (+44) | 38 | - | FAIL: no extra RAM, no Sub0Pub retained |

<details><summary>cm33-gcc-Os: largest symbols added by handwritten_erased (bytes)</summary>

- 36 `collapse_setup`
- 28 `(anonymous namespace)::deliverNode(void const*, (anonymous namespace)::Sample const&)`
- 8 `(anonymous namespace)::sensor`
- 4 `(anonymous namespace)::node`

</details>

<details><summary>cm33-gcc-Os: largest symbols added by handwritten_runtime (bytes)</summary>

- 44 `collapse_publish`
- 20 `collapse_setup`
- 4 `(anonymous namespace)::sensor`

</details>

<details><summary>cm33-gcc-Os: largest symbols added by sub0_b3_sink (bytes)</summary>

- 28 `sub0::Sink<(anonymous namespace)::Sample>::Sink<sub0::Wiring<(anonymous namespace)::Controller>, 0>(sub0::Wiring<(anonymous namespace)::Controller>&)::{lambda(void const*, (anonymous namespace)::Sample const&)#1}::_FUN(void const*, (anonymous namespace)::Sample const&)`
- 4 `(anonymous namespace)::bus`

</details>

<details><summary>cm33-gcc-Os: largest symbols added by sub0pub_virtual (bytes)</summary>

- 254 `memmove`
- 68 `collapse_teardown`
- 52 `collapse_setup`
- 52 `collapse_publish`
- 36 `sub0::detail::BrokerImpl<(anonymous namespace)::Sample, sub0::detail::BuiltinT<8ul, (sub0::Dispatch)1, (sub0::Context)2, false, sub0::NoLock> >::global_`
- 24 `(anonymous namespace)::Controller::receive((anonymous namespace)::Sample const&)`
- 16 `vtable for (anonymous namespace)::Controller`
- 12 `(anonymous namespace)::controller`

</details>

<details><summary>cm33-gcc-Os: largest symbols added by sub0pub_virtual_lean (bytes)</summary>

- 254 `memmove`
- 68 `collapse_teardown`
- 52 `collapse_setup`
- 52 `collapse_publish`
- 36 `sub0::detail::BrokerImpl<(anonymous namespace)::Sample, sub0::detail::BuiltinT<8ul, (sub0::Dispatch)1, (sub0::Context)2, false, sub0::NoLock> >::global_`
- 24 `(anonymous namespace)::Controller::receive((anonymous namespace)::Sample const&)`
- 16 `vtable for (anonymous namespace)::Controller`
- 12 `(anonymous namespace)::controller`

</details>

## Case: publisher_ergonomics

### gcc-O2, observable work

| variant | checksum | publish instr | setup instr | teardown instr | path instr | calls (direct/indirect) | text | data+bss | retained sub0 (B) | added deps | verdict |
|---|---|---|---|---|---|---|---|---|---|---|---|
| handwritten | ok | 30.0 (+0.0) | 21 (+0) | 16 (+0) | 26 (+0) | 0/0 | 2431 (+0) | 632 (+0) | 0 | - | reference |
| handwritten_erased | ok | 52.0 (+22.0) | 31 (+10) | 16 (+0) | 17 (-9) | 1/1 | 2627 (+196) | 672 (+40) | 0 | - | reference; FAIL: publish instr, setup instr, no extra indirect calls, no extra RAM |
| handwritten_runtime | ok | 38.0 (+8.0) | 27 (+6) | 16 (+0) | 35 (+9) | 0/0 | 2495 (+64) | 656 (+24) | 0 | - | reference; FAIL: publish instr, setup instr, publish path, no extra RAM |
| sub0_b1_mixin (vs handwritten_runtime) | ok | 38.0 (+0.0) | 27 (+0) | 16 (+0) | 35 (+0) | 0/0 | 2495 (+0) | 656 (+0) | 0 | - | PASS |

### gcc-O2, removable work

| variant | checksum | publish instr | setup instr | teardown instr | path instr | calls (direct/indirect) | text | data+bss | retained sub0 (B) | added deps | verdict |
|---|---|---|---|---|---|---|---|---|---|---|---|
| handwritten | ok | 9.0 (+0.0) | 21 (+0) | 16 (+0) | 6 (+0) | 0/0 | 2367 (+0) | 632 (+0) | 0 | - | reference |
| handwritten_erased | ok | 24.0 (+15.0) | 31 (+10) | 16 (+0) | 17 (+11) | 1/1 | 2531 (+164) | 672 (+40) | 0 | - | reference; FAIL: publish instr, setup instr, publish path, no extra indirect calls, no extra RAM |
| handwritten_runtime | ok | 10.0 (+1.0) | 27 (+6) | 16 (+0) | 6 (+0) | 0/0 | 2399 (+32) | 656 (+24) | 0 | - | reference; FAIL: setup instr, no extra RAM |
| sub0_b1_mixin (vs handwritten_runtime) | ok | 10.0 (+0.0) | 27 (+0) | 16 (+0) | 6 (+0) | 0/0 | 2399 (+0) | 656 (+0) | 0 | - | PASS |

<details><summary>gcc-O2: largest symbols added by handwritten_erased (bytes)</summary>

- 105 `collapse_setup`
- 99 `(anonymous namespace)::deliverNode(void const*, (anonymous namespace)::Sample const&)`
- 24 `(anonymous namespace)::node`
- 16 `(anonymous namespace)::sensor`

</details>

<details><summary>gcc-O2: largest symbols added by handwritten_runtime (bytes)</summary>

- 114 `collapse_publish`
- 77 `collapse_setup`
- 24 `(anonymous namespace)::sensor`

</details>

### clang-O2, observable work

| variant | checksum | publish instr | setup instr | teardown instr | path instr | calls (direct/indirect) | text | data+bss | retained sub0 (B) | added deps | verdict |
|---|---|---|---|---|---|---|---|---|---|---|---|
| handwritten | ok | 33.0 (+0.0) | 19 (+0) | 14 (+0) | 29 (+0) | 0/0 | 2170 (+0) | 672 (+0) | 0 | - | reference |
| handwritten_erased | ok | 45.0 (+12.0) | 27 (+8) | 14 (+0) | 12 (-17) | 0/1 | 2314 (+144) | 704 (+32) | 0 | - | reference; FAIL: publish instr, setup instr, no extra indirect calls, no extra RAM |
| handwritten_runtime | ok | 33.0 (+0.0) | 19 (+0) | 14 (+0) | 29 (+0) | 0/0 | 2170 (+0) | 672 (+0) | 0 | - | reference; PASS |
| sub0_b1_mixin (vs handwritten_runtime) | ok | 33.0 (+0.0) | 19 (+0) | 14 (+0) | 29 (+0) | 0/0 | 2170 (+0) | 672 (+0) | 0 | - | PASS |

### clang-O2, removable work

| variant | checksum | publish instr | setup instr | teardown instr | path instr | calls (direct/indirect) | text | data+bss | retained sub0 (B) | added deps | verdict |
|---|---|---|---|---|---|---|---|---|---|---|---|
| handwritten | ok | 9.0 (+0.0) | 17 (+0) | 14 (+0) | 4 (+0) | 0/0 | 2074 (+0) | 664 (+0) | 0 | - | reference |
| handwritten_erased | ok | 18.0 (+9.0) | 27 (+10) | 14 (+0) | 12 (+8) | 0/1 | 2234 (+160) | 704 (+40) | 0 | - | reference; FAIL: publish instr, setup instr, publish path, no extra indirect calls, no extra RAM |
| handwritten_runtime | ok | 9.0 (+0.0) | 19 (+2) | 14 (+0) | 4 (+0) | 0/0 | 2090 (+16) | 672 (+8) | 0 | - | reference; FAIL: setup instr, no extra RAM |
| sub0_b1_mixin (vs handwritten_runtime) | ok | 9.0 (+0.0) | 19 (+0) | 14 (+0) | 4 (+0) | 0/0 | 2090 (+0) | 672 (+0) | 0 | - | PASS |

<details><summary>clang-O2: largest symbols added by handwritten_erased (bytes)</summary>

- 87 `collapse_setup`
- 81 `(anonymous namespace)::deliverNode(void const*, (anonymous namespace)::Sample const&)`
- 24 `(anonymous namespace)::node`
- 8 `_ZN12_GLOBAL__N_16sensorE.0`
- 4 `(anonymous namespace)::logger`
- 4 `(anonymous namespace)::controllerB`
- 4 `(anonymous namespace)::controllerA`

</details>

### cm33-gcc-Os, observable work

| variant | checksum | publish instr | setup instr | teardown instr | path instr | calls (direct/indirect) | text | data+bss | retained sub0 (B) | added deps | verdict |
|---|---|---|---|---|---|---|---|---|---|---|---|
| handwritten | - | - | - | - | 30 (+0) | 0/0 | 1188 (+0) | 520 (+0) | 0 | - | reference |
| handwritten_erased | - | - | - | - | 15 (-15) | 0/1 | 1240 (+52) | 540 (+20) | 0 | - | reference; FAIL: no extra indirect calls, no extra RAM |
| handwritten_runtime | - | - | - | - | 32 (+2) | 0/0 | 1200 (+12) | 532 (+12) | 0 | - | reference; FAIL: no extra RAM |
| sub0_b1_mixin (vs handwritten_runtime) | - | - | - | - | 32 (+0) | 0/0 | 1200 (+0) | 532 (+0) | 0 | - | PASS |

### cm33-gcc-Os, removable work

| variant | checksum | publish instr | setup instr | teardown instr | path instr | calls (direct/indirect) | text | data+bss | retained sub0 (B) | added deps | verdict |
|---|---|---|---|---|---|---|---|---|---|---|---|
| handwritten | - | - | - | - | 12 (+0) | 0/0 | 1136 (+0) | 520 (+0) | 0 | - | reference |
| handwritten_erased | - | - | - | - | 15 (+3) | 0/1 | 1184 (+48) | 540 (+20) | 0 | - | reference; FAIL: publish path, no extra indirect calls, no extra RAM |
| handwritten_runtime | - | - | - | - | 12 (+0) | 0/0 | 1148 (+12) | 532 (+12) | 0 | - | reference; FAIL: no extra RAM |
| sub0_b1_mixin (vs handwritten_runtime) | - | - | - | - | 12 (+0) | 0/0 | 1148 (+0) | 532 (+0) | 0 | - | PASS |

<details><summary>cm33-gcc-Os: largest symbols added by handwritten_erased (bytes)</summary>

- 68 `(anonymous namespace)::deliverNode(void const*, (anonymous namespace)::Sample const&)`
- 60 `collapse_setup`
- 12 `(anonymous namespace)::node`
- 8 `(anonymous namespace)::sensor`

</details>

<details><summary>cm33-gcc-Os: largest symbols added by handwritten_runtime (bytes)</summary>

- 44 `collapse_setup`
- 12 `(anonymous namespace)::sensor`

</details>

## Case: static_dynamic_bridge

### gcc-O2, observable work

| variant | checksum | publish instr | setup instr | teardown instr | path instr | calls (direct/indirect) | text | data+bss | retained sub0 (B) | added deps | verdict |
|---|---|---|---|---|---|---|---|---|---|---|---|
| handwritten | ok | 37.0 (+0.0) | 31 (+0) | 32 (+0) | 33 (+0) | 0/0 | 3208 (+0) | 800 (+0) | 0 | - | reference |
| sub0_bridge_broker | ok | 37.0 (+0.0) | 46 (+15) | 61 (+29) | 33 (+0) | 0/0 | 4151 (+943) | 920 (+120) | 187 | - | FAIL: setup instr, teardown instr, no extra RAM, no Sub0Pub retained |
| sub0_bridge_slots | ok | 37.0 (+0.0) | 31 (+0) | 32 (+0) | 33 (+0) | 0/0 | 3256 (+48) | 800 (+0) | 76 | - | FAIL: no Sub0Pub retained |

### gcc-O2, removable work

| variant | checksum | publish instr | setup instr | teardown instr | path instr | calls (direct/indirect) | text | data+bss | retained sub0 (B) | added deps | verdict |
|---|---|---|---|---|---|---|---|---|---|---|---|
| handwritten | ok | 9.0 (+0.0) | 31 (+0) | 32 (+0) | 6 (+0) | 0/0 | 3112 (+0) | 800 (+0) | 0 | - | reference |
| sub0_bridge_broker | ok | 9.0 (+0.0) | 46 (+15) | 61 (+29) | 6 (+0) | 0/0 | 4011 (+899) | 920 (+120) | 187 | - | FAIL: setup instr, teardown instr, no extra RAM, no Sub0Pub retained |
| sub0_bridge_slots | ok | 9.0 (+0.0) | 31 (+0) | 32 (+0) | 6 (+0) | 0/0 | 3160 (+48) | 800 (+0) | 76 | - | FAIL: no Sub0Pub retained |

<details><summary>gcc-O2: largest symbols added by sub0_bridge_broker (bytes)</summary>

- 347 `collapse_teardown`
- 171 `collapse_setup`
- 80 `(anonymous namespace)::domain`
- 66 `typeinfo name for sub0::detail::SubscriberInterface<(anonymous namespace)::Sample, false>`
- 44 `typeinfo name for sub0::Subscribe<(anonymous namespace)::Sample>`
- 32 `vtable for sub0::Subscribe<(anonymous namespace)::Sample>`
- 32 `vtable for (anonymous namespace)::Probe`
- 24 `typeinfo for sub0::Subscribe<(anonymous namespace)::Sample>`

</details>

<details><summary>gcc-O2: largest symbols added by sub0_bridge_slots (bytes)</summary>

- 72 `(anonymous namespace)::port`
- 60 `typeinfo name for sub0::DynamicPort<(anonymous namespace)::Sample, 8u>::Receiver`
- 16 `typeinfo for sub0::DynamicPort<(anonymous namespace)::Sample, 8u>::Receiver`

</details>

### clang-O2, observable work

| variant | checksum | publish instr | setup instr | teardown instr | path instr | calls (direct/indirect) | text | data+bss | retained sub0 (B) | added deps | verdict |
|---|---|---|---|---|---|---|---|---|---|---|---|
| handwritten | ok | 58.0 (+0.0) | 27 (+0) | 30 (+0) | 44 (+0) | 0/1 | 3078 (+0) | 808 (+0) | 0 | - | reference |
| sub0_bridge_broker | ok | 57.0 (-1.0) | 34 (+7) | 50 (+20) | 44 (+0) | 0/1 | 3852 (+774) | 912 (+104) | 181 | - | FAIL: setup instr, teardown instr, no extra RAM, no Sub0Pub retained |
| sub0_bridge_slots | ok | 58.0 (+0.0) | 27 (+0) | 30 (+0) | 44 (+0) | 0/1 | 3107 (+29) | 808 (+0) | 75 | - | FAIL: no Sub0Pub retained |

### clang-O2, removable work

| variant | checksum | publish instr | setup instr | teardown instr | path instr | calls (direct/indirect) | text | data+bss | retained sub0 (B) | added deps | verdict |
|---|---|---|---|---|---|---|---|---|---|---|---|
| handwritten | ok | 34.0 (+0.0) | 27 (+0) | 30 (+0) | 29 (+0) | 0/1 | 3021 (+0) | 808 (+0) | 0 | - | reference |
| sub0_bridge_broker | ok | 33.0 (-1.0) | 34 (+7) | 50 (+20) | 29 (+0) | 0/1 | 3804 (+783) | 912 (+104) | 181 | - | FAIL: setup instr, teardown instr, no extra RAM, no Sub0Pub retained |
| sub0_bridge_slots | ok | 34.0 (+0.0) | 27 (+0) | 30 (+0) | 29 (+0) | 0/1 | 3050 (+29) | 808 (+0) | 75 | - | FAIL: no Sub0Pub retained |

<details><summary>clang-O2: largest symbols added by sub0_bridge_broker (bytes)</summary>

- 627 `collapse_teardown`
- 129 `collapse_setup`
- 80 `(anonymous namespace)::domain`
- 65 `typeinfo name for sub0::detail::SubscriberInterface<(anonymous namespace)::Sample, false>`
- 43 `typeinfo name for sub0::Subscribe<(anonymous namespace)::Sample>`
- 32 `vtable for sub0::Subscribe<(anonymous namespace)::Sample>`
- 32 `vtable for (anonymous namespace)::Probe`
- 24 `typeinfo for sub0::Subscribe<(anonymous namespace)::Sample>`

</details>

<details><summary>clang-O2: largest symbols added by sub0_bridge_slots (bytes)</summary>

- 72 `(anonymous namespace)::port`
- 59 `typeinfo name for sub0::DynamicPort<(anonymous namespace)::Sample, 8u>::Receiver`
- 16 `typeinfo for sub0::DynamicPort<(anonymous namespace)::Sample, 8u>::Receiver`

</details>

### cm33-gcc-Os, observable work

| variant | checksum | publish instr | setup instr | teardown instr | path instr | calls (direct/indirect) | text | data+bss | retained sub0 (B) | added deps | verdict |
|---|---|---|---|---|---|---|---|---|---|---|---|
| handwritten | - | - | - | - | 45 (+0) | 1/0 | 1568 (+0) | 552 (+0) | 0 | - | reference |
| sub0_bridge_broker | - | - | - | - | 48 (+3) | 1/0 | 3212 (+1644) | 668 (+116) | 18 | - | FAIL: publish path, no extra RAM, no Sub0Pub retained |
| sub0_bridge_slots | - | - | - | - | 45 (+0) | 1/0 | 1568 (+0) | 552 (+0) | 0 | - | PASS |

### cm33-gcc-Os, removable work

| variant | checksum | publish instr | setup instr | teardown instr | path instr | calls (direct/indirect) | text | data+bss | retained sub0 (B) | added deps | verdict |
|---|---|---|---|---|---|---|---|---|---|---|---|
| handwritten | - | - | - | - | 12 (+0) | 0/0 | 1488 (+0) | 552 (+0) | 0 | - | reference |
| sub0_bridge_broker | - | - | - | - | 12 (+0) | 0/0 | 3120 (+1632) | 668 (+116) | 18 | - | FAIL: no extra RAM, no Sub0Pub retained |
| sub0_bridge_slots | - | - | - | - | 12 (+0) | 0/0 | 1488 (+0) | 552 (+0) | 0 | - | PASS |

<details><summary>cm33-gcc-Os: largest symbols added by sub0_bridge_broker (bytes)</summary>

- 256 `_malloc_r`
- 236 `memcpy`
- 208 `collapse_teardown`
- 132 `collapse_setup`
- 100 `__sigtramp`
- 96 `collapse_publish`
- 96 `__sigtramp_r`
- 84 `raise`

</details>

<details><summary>cm33-gcc-Os: largest symbols added by sub0_bridge_slots (bytes)</summary>

- 36 `(anonymous namespace)::port`

</details>

## Case: static_dynamic_bridge_churn

### gcc-O2, observable work

| variant | checksum | publish instr | setup instr | teardown instr | path instr | calls (direct/indirect) | text | data+bss | retained sub0 (B) | added deps | verdict |
|---|---|---|---|---|---|---|---|---|---|---|---|
| handwritten | ok | 81.8 (+0.0) | 42 (+0) | 55 (+0) | 100 (+0) | 4/0 | 3690 (+0) | 832 (+0) | 0 | - | reference |
| sub0_bridge_broker | ok | 82.5 (+0.8) | 64 (+22) | 94 (+39) | 139 (+39) | 2/0 | 4845 (+1155) | 952 (+120) | 187 | - | FAIL: setup instr, teardown instr, publish path, no extra RAM, no Sub0Pub retained |
| sub0_bridge_slots | ok | 75.0 (-6.8) | 42 (+0) | 55 (+0) | 115 (+15) | 2/0 | 3758 (+68) | 832 (+0) | 76 | - | FAIL: publish path, no Sub0Pub retained |

### gcc-O2, removable work

| variant | checksum | publish instr | setup instr | teardown instr | path instr | calls (direct/indirect) | text | data+bss | retained sub0 (B) | added deps | verdict |
|---|---|---|---|---|---|---|---|---|---|---|---|
| handwritten | ok | 30.8 (+0.0) | 42 (+0) | 55 (+0) | 57 (+0) | 2/0 | 3486 (+0) | 832 (+0) | 0 | - | reference |
| sub0_bridge_broker | ok | 34.3 (+3.5) | 64 (+22) | 94 (+39) | 71 (+14) | 2/0 | 4545 (+1059) | 952 (+120) | 187 | - | FAIL: publish instr, setup instr, teardown instr, publish path, no extra RAM, no Sub0Pub retained |
| sub0_bridge_slots | ok | 30.8 (+0.0) | 42 (+0) | 55 (+0) | 57 (+0) | 2/0 | 3534 (+48) | 832 (+0) | 76 | - | FAIL: no Sub0Pub retained |

<details><summary>gcc-O2: largest symbols added by sub0_bridge_broker (bytes)</summary>

- 526 `collapse_publish`
- 493 `collapse_teardown`
- 272 `collapse_setup`
- 80 `(anonymous namespace)::domain`
- 66 `typeinfo name for sub0::detail::SubscriberInterface<(anonymous namespace)::Sample, false>`
- 44 `typeinfo name for sub0::Subscribe<(anonymous namespace)::Sample>`
- 32 `vtable for sub0::Subscribe<(anonymous namespace)::Sample>`
- 32 `vtable for (anonymous namespace)::Probe`

</details>

<details><summary>gcc-O2: largest symbols added by sub0_bridge_slots (bytes)</summary>

- 443 `collapse_publish`
- 72 `(anonymous namespace)::port`
- 60 `typeinfo name for sub0::DynamicPort<(anonymous namespace)::Sample, 8u>::Receiver`
- 16 `typeinfo for sub0::DynamicPort<(anonymous namespace)::Sample, 8u>::Receiver`

</details>

### clang-O2, observable work

| variant | checksum | publish instr | setup instr | teardown instr | path instr | calls (direct/indirect) | text | data+bss | retained sub0 (B) | added deps | verdict |
|---|---|---|---|---|---|---|---|---|---|---|---|
| handwritten | ok | 94.0 (+0.0) | 32 (+0) | 53 (+0) | 175 (+0) | 0/2 | 3946 (+0) | 832 (+0) | 0 | - | reference |
| sub0_bridge_broker | ok | 96.5 (+2.5) | 42 (+10) | 79 (+26) | 203 (+28) | 0/2 | 4932 (+986) | 936 (+104) | 181 | - | FAIL: publish instr, setup instr, teardown instr, publish path, no extra RAM, no Sub0Pub retained |
| sub0_bridge_slots | ok | 94.0 (+0.0) | 32 (+0) | 53 (+0) | 175 (+0) | 0/2 | 3975 (+29) | 832 (+0) | 75 | - | FAIL: no Sub0Pub retained |

### clang-O2, removable work

| variant | checksum | publish instr | setup instr | teardown instr | path instr | calls (direct/indirect) | text | data+bss | retained sub0 (B) | added deps | verdict |
|---|---|---|---|---|---|---|---|---|---|---|---|
| handwritten | ok | 58.5 (+0.0) | 32 (+0) | 53 (+0) | 144 (+0) | 0/2 | 3825 (+0) | 832 (+0) | 0 | - | reference |
| sub0_bridge_broker | ok | 61.3 (+2.8) | 42 (+10) | 79 (+26) | 173 (+29) | 0/2 | 4820 (+995) | 936 (+104) | 181 | - | FAIL: publish instr, setup instr, teardown instr, publish path, no extra RAM, no Sub0Pub retained |
| sub0_bridge_slots | ok | 58.5 (+0.0) | 32 (+0) | 53 (+0) | 144 (+0) | 0/2 | 3854 (+29) | 832 (+0) | 75 | - | FAIL: no Sub0Pub retained |

<details><summary>clang-O2: largest symbols added by sub0_bridge_broker (bytes)</summary>

- 1011 `collapse_teardown`
- 771 `collapse_publish`
- 191 `collapse_setup`
- 80 `(anonymous namespace)::domain`
- 65 `typeinfo name for sub0::detail::SubscriberInterface<(anonymous namespace)::Sample, false>`
- 43 `typeinfo name for sub0::Subscribe<(anonymous namespace)::Sample>`
- 32 `vtable for sub0::Subscribe<(anonymous namespace)::Sample>`
- 32 `vtable for (anonymous namespace)::Probe`

</details>

<details><summary>clang-O2: largest symbols added by sub0_bridge_slots (bytes)</summary>

- 72 `(anonymous namespace)::port`
- 59 `typeinfo name for sub0::DynamicPort<(anonymous namespace)::Sample, 8u>::Receiver`
- 16 `typeinfo for sub0::DynamicPort<(anonymous namespace)::Sample, 8u>::Receiver`

</details>

### cm33-gcc-Os, observable work

| variant | checksum | publish instr | setup instr | teardown instr | path instr | calls (direct/indirect) | text | data+bss | retained sub0 (B) | added deps | verdict |
|---|---|---|---|---|---|---|---|---|---|---|---|
| handwritten | - | - | - | - | 187 (+0) | 5/0 | 1688 (+0) | 564 (+0) | 0 | - | reference |
| sub0_bridge_broker | - | - | - | - | 226 (+39) | 6/0 | 3332 (+1644) | 688 (+124) | 166 | - | FAIL: publish path, no extra RAM, no Sub0Pub retained |
| sub0_bridge_slots | - | - | - | - | 187 (+0) | 5/0 | 1688 (+0) | 564 (+0) | 134 | - | PASS |

### cm33-gcc-Os, removable work

| variant | checksum | publish instr | setup instr | teardown instr | path instr | calls (direct/indirect) | text | data+bss | retained sub0 (B) | added deps | verdict |
|---|---|---|---|---|---|---|---|---|---|---|---|
| handwritten | - | - | - | - | 142 (+0) | 2/0 | 1576 (+0) | 564 (+0) | 0 | - | reference |
| sub0_bridge_broker | - | - | - | - | 183 (+41) | 3/0 | 3224 (+1648) | 688 (+124) | 78 | - | FAIL: publish path, no extra RAM, no Sub0Pub retained |
| sub0_bridge_slots | - | - | - | - | 142 (+0) | 2/0 | 1576 (+0) | 564 (+0) | 54 | - | PASS |

<details><summary>cm33-gcc-Os: largest symbols added by sub0_bridge_broker (bytes)</summary>

- 256 `_malloc_r`
- 236 `memcpy`
- 136 `collapse_teardown`
- 108 `collapse_setup`
- 100 `__sigtramp`
- 96 `__sigtramp_r`
- 92 `(anonymous namespace)::Probe::~Probe() [clone .isra.0]`
- 88 `void sub0::StaticWiring<&(anonymous namespace)::controller, &(anonymous namespace)::logger, &(anonymous namespace)::port>::publish<(anonymous namespace)::Sample>((anonymous namespace)::Sample const&)`

</details>

<details><summary>cm33-gcc-Os: largest symbols added by sub0_bridge_slots (bytes)</summary>

- 80 `void sub0::StaticWiring<&(anonymous namespace)::controller, &(anonymous namespace)::logger, &(anonymous namespace)::port>::publish<(anonymous namespace)::Sample>((anonymous namespace)::Sample const&)`
- 54 `sub0::DynamicPort<(anonymous namespace)::Sample, 8ul>::remove(sub0::DynamicPort<(anonymous namespace)::Sample, 8ul>::Receiver*)`
- 36 `(anonymous namespace)::port`

</details>

## Case: static_dynamic_bridge_empty

### gcc-O2, observable work

| variant | checksum | publish instr | setup instr | teardown instr | path instr | calls (direct/indirect) | text | data+bss | retained sub0 (B) | added deps | verdict |
|---|---|---|---|---|---|---|---|---|---|---|---|
| handwritten | ok | 23.0 (+0.0) | 19 (+0) | 16 (+0) | 19 (+0) | 0/0 | 2367 (+0) | 624 (+0) | 0 | - | reference |
| handwritten_registry | ok | 26.0 (+3.0) | 25 (+6) | 16 (+0) | 22 (+3) | 0/0 | 2576 (+209) | 720 (+96) | 0 | pure virtual | reference; FAIL: publish instr, setup instr, publish path, no extra RAM, no extra dependencies |
| sub0_bridge_broker (vs handwritten_registry) | ok | 27.0 (+1.0) | 28 (+3) | 35 (+19) | 24 (+2) | 0/0 | 3036 (+460) | 744 (+24) | 0 | - | FAIL: setup instr, teardown instr, no extra RAM |
| sub0_bridge_slots (vs handwritten_registry) | ok | 26.0 (+0.0) | 25 (+0) | 16 (+0) | 22 (+0) | 0/0 | 2576 (+0) | 720 (+0) | 0 | - | PASS |

### gcc-O2, removable work

| variant | checksum | publish instr | setup instr | teardown instr | path instr | calls (direct/indirect) | text | data+bss | retained sub0 (B) | added deps | verdict |
|---|---|---|---|---|---|---|---|---|---|---|---|
| handwritten | ok | 9.0 (+0.0) | 19 (+0) | 16 (+0) | 6 (+0) | 0/0 | 2335 (+0) | 624 (+0) | 0 | - | reference |
| handwritten_registry | ok | 12.0 (+3.0) | 25 (+6) | 16 (+0) | 9 (+3) | 0/0 | 2544 (+209) | 720 (+96) | 0 | pure virtual | reference; FAIL: publish instr, setup instr, publish path, no extra RAM, no extra dependencies |
| sub0_bridge_broker (vs handwritten_registry) | ok | 13.0 (+1.0) | 28 (+3) | 35 (+19) | 10 (+1) | 0/0 | 2988 (+444) | 744 (+24) | 0 | - | FAIL: setup instr, teardown instr, no extra RAM |
| sub0_bridge_slots (vs handwritten_registry) | ok | 12.0 (+0.0) | 25 (+0) | 16 (+0) | 9 (+0) | 0/0 | 2544 (+0) | 720 (+0) | 0 | - | PASS |

<details><summary>gcc-O2: largest symbols added by handwritten_registry (bytes)</summary>

- 77 `collapse_publish`
- 72 `(anonymous namespace)::registry`
- 58 `collapse_setup`
- 6 `collapse_publish.cold`

</details>

<details><summary>gcc-O2: largest symbols added by sub0_bridge_broker (bytes)</summary>

- 181 `collapse_teardown`
- 81 `collapse_publish`
- 80 `(anonymous namespace)::domain`
- 76 `collapse_setup`
- 8 `(anonymous namespace)::port`
- 5 `collapse_teardown.cold`

</details>

<details><summary>gcc-O2: largest symbols added by sub0_bridge_slots (bytes)</summary>

- 72 `(anonymous namespace)::port`

</details>

### clang-O2, observable work

| variant | checksum | publish instr | setup instr | teardown instr | path instr | calls (direct/indirect) | text | data+bss | retained sub0 (B) | added deps | verdict |
|---|---|---|---|---|---|---|---|---|---|---|---|
| handwritten | ok | 24.0 (+0.0) | 17 (+0) | 14 (+0) | 20 (+0) | 0/0 | 2122 (+0) | 664 (+0) | 0 | - | reference |
| handwritten_registry | ok | 35.0 (+11.0) | 23 (+6) | 14 (+0) | 44 (+24) | 0/1 | 2278 (+156) | 736 (+72) | 0 | - | reference; FAIL: publish instr, setup instr, publish path, no extra indirect calls, no extra RAM |
| sub0_bridge_broker (vs handwritten_registry) | ok | 35.0 (+0.0) | 26 (+3) | 26 (+12) | 44 (+0) | 0/1 | 2600 (+322) | 760 (+24) | 0 | - | FAIL: setup instr, teardown instr, no extra RAM |
| sub0_bridge_slots (vs handwritten_registry) | ok | 35.0 (+0.0) | 23 (+0) | 14 (+0) | 44 (+0) | 0/1 | 2278 (+0) | 736 (+0) | 0 | - | PASS |

### clang-O2, removable work

| variant | checksum | publish instr | setup instr | teardown instr | path instr | calls (direct/indirect) | text | data+bss | retained sub0 (B) | added deps | verdict |
|---|---|---|---|---|---|---|---|---|---|---|---|
| handwritten | ok | 9.0 (+0.0) | 17 (+0) | 14 (+0) | 4 (+0) | 0/0 | 2074 (+0) | 664 (+0) | 0 | - | reference |
| handwritten_registry | ok | 20.0 (+11.0) | 23 (+6) | 14 (+0) | 29 (+25) | 0/1 | 2246 (+172) | 736 (+72) | 0 | - | reference; FAIL: publish instr, setup instr, publish path, no extra indirect calls, no extra RAM |
| sub0_bridge_broker (vs handwritten_registry) | ok | 20.0 (+0.0) | 26 (+3) | 26 (+12) | 29 (+0) | 0/1 | 2568 (+322) | 760 (+24) | 0 | - | FAIL: setup instr, teardown instr, no extra RAM |
| sub0_bridge_slots (vs handwritten_registry) | ok | 20.0 (+0.0) | 23 (+0) | 14 (+0) | 29 (+0) | 0/1 | 2246 (+0) | 736 (+0) | 0 | - | PASS |

<details><summary>clang-O2: largest symbols added by handwritten_registry (bytes)</summary>

- 135 `collapse_publish`
- 72 `(anonymous namespace)::registry`
- 53 `collapse_setup`

</details>

<details><summary>clang-O2: largest symbols added by sub0_bridge_broker (bytes)</summary>

- 227 `collapse_teardown`
- 80 `(anonymous namespace)::domain`
- 70 `collapse_setup`
- 8 `_ZN12_GLOBAL__N_14portE.0`

</details>

<details><summary>clang-O2: largest symbols added by sub0_bridge_slots (bytes)</summary>

- 72 `(anonymous namespace)::port`

</details>

### cm33-gcc-Os, observable work

| variant | checksum | publish instr | setup instr | teardown instr | path instr | calls (direct/indirect) | text | data+bss | retained sub0 (B) | added deps | verdict |
|---|---|---|---|---|---|---|---|---|---|---|---|
| handwritten | - | - | - | - | 22 (+0) | 0/0 | 1144 (+0) | 512 (+0) | 0 | - | reference |
| handwritten_registry | - | - | - | - | 28 (+6) | 0/0 | 1180 (+36) | 548 (+36) | 0 | - | reference; FAIL: publish path, no extra RAM |
| sub0_bridge_broker (vs handwritten_registry) | - | - | - | - | 28 (+0) | 0/0 | 2716 (+1536) | 656 (+108) | 0 | - | FAIL: no extra RAM |
| sub0_bridge_slots (vs handwritten_registry) | - | - | - | - | 28 (+0) | 0/0 | 1180 (+0) | 548 (+0) | 0 | - | PASS |

### cm33-gcc-Os, removable work

| variant | checksum | publish instr | setup instr | teardown instr | path instr | calls (direct/indirect) | text | data+bss | retained sub0 (B) | added deps | verdict |
|---|---|---|---|---|---|---|---|---|---|---|---|
| handwritten | - | - | - | - | 12 (+0) | 0/0 | 1116 (+0) | 512 (+0) | 0 | - | reference |
| handwritten_registry | - | - | - | - | 18 (+6) | 0/0 | 1152 (+36) | 548 (+36) | 0 | - | reference; FAIL: publish path, no extra RAM |
| sub0_bridge_broker (vs handwritten_registry) | - | - | - | - | 18 (+0) | 0/0 | 2688 (+1536) | 656 (+108) | 0 | - | FAIL: no extra RAM |
| sub0_bridge_slots (vs handwritten_registry) | - | - | - | - | 18 (+0) | 0/0 | 1152 (+0) | 548 (+0) | 0 | - | PASS |

<details><summary>cm33-gcc-Os: largest symbols added by handwritten_registry (bytes)</summary>

- 72 `collapse_publish`
- 36 `(anonymous namespace)::registry`
- 32 `collapse_setup`

</details>

<details><summary>cm33-gcc-Os: largest symbols added by sub0_bridge_broker (bytes)</summary>

- 256 `_malloc_r`
- 236 `memcpy`
- 116 `collapse_teardown`
- 100 `__sigtramp`
- 96 `__sigtramp_r`
- 84 `raise`
- 80 `_raise_r`
- 76 `signal`

</details>

<details><summary>cm33-gcc-Os: largest symbols added by sub0_bridge_slots (bytes)</summary>

- 36 `(anonymous namespace)::port`

</details>

## Case: transport_endpoint

### gcc-O2, observable work

| variant | checksum | publish instr | setup instr | teardown instr | path instr | calls (direct/indirect) | text | data+bss | retained sub0 (B) | added deps | verdict |
|---|---|---|---|---|---|---|---|---|---|---|---|
| handwritten | ok | 27.0 (+0.0) | 18 (+0) | 16 (+0) | 23 (+0) | 0/0 | 2383 (+0) | 616 (+0) | 0 | - | reference |
| handwritten_erased | ok | 45.0 (+18.0) | 26 (+8) | 16 (+0) | 26 (+3) | 1/1 | 2563 (+180) | 656 (+40) | 0 | - | reference; FAIL: publish instr, setup instr, publish path, no extra indirect calls, no extra RAM |
| handwritten_runtime | ok | 27.0 (+0.0) | 22 (+4) | 16 (+0) | 23 (+0) | 0/0 | 2415 (+32) | 640 (+24) | 0 | - | reference; FAIL: setup instr, no extra RAM |
| sub0_b1_wire (vs handwritten_runtime) | ok | 27.0 (+0.0) | 22 (+0) | 16 (+0) | 23 (+0) | 0/0 | 2415 (+0) | 640 (+0) | 0 | - | PASS |
| sub0_b1_wire_origin_transport (vs handwritten_runtime) | ok | 27.0 (+0.0) | 22 (+0) | 16 (+0) | 23 (+0) | 0/0 | 2415 (+0) | 640 (+0) | 0 | - | PASS |
| sub0_b2_static | ok | 27.0 (+0.0) | 18 (+0) | 16 (+0) | 23 (+0) | 0/0 | 2383 (+0) | 616 (+0) | 0 | - | PASS |
| sub0_b2_static_origin_transport | ok | 27.0 (+0.0) | 18 (+0) | 16 (+0) | 23 (+0) | 0/0 | 2383 (+0) | 616 (+0) | 0 | - | PASS |
| sub0_b3_sink (vs handwritten_erased) | ok | 45.0 (+0.0) | 26 (+0) | 16 (+0) | 26 (+0) | 1/1 | 2563 (+0) | 656 (+0) | 52 | - | PASS |
| sub0_dynamic_route | ok | 143.0 (+116.0) | 45 (+27) | 80 (+64) | 76 (+53) | 1/2 | 4734 (+2351) | 992 (+376) | 577 | - | FAIL: publish instr, setup instr, teardown instr, publish path, no extra indirect calls, no extra RAM, no Sub0Pub retained |
| sub0_dynamic_route_lean | ok | 143.0 (+116.0) | 45 (+27) | 80 (+64) | 76 (+53) | 1/2 | 4718 (+2335) | 984 (+368) | 577 | - | FAIL: publish instr, setup instr, teardown instr, publish path, no extra indirect calls, no extra RAM, no Sub0Pub retained |

### gcc-O2, removable work

| variant | checksum | publish instr | setup instr | teardown instr | path instr | calls (direct/indirect) | text | data+bss | retained sub0 (B) | added deps | verdict |
|---|---|---|---|---|---|---|---|---|---|---|---|
| handwritten | ok | 14.0 (+0.0) | 18 (+0) | 16 (+0) | 11 (+0) | 0/0 | 2351 (+0) | 616 (+0) | 0 | - | reference |
| handwritten_erased | ok | 28.0 (+14.0) | 26 (+8) | 16 (+0) | 17 (+6) | 1/1 | 2499 (+148) | 656 (+40) | 0 | - | reference; FAIL: publish instr, setup instr, publish path, no extra indirect calls, no extra RAM |
| handwritten_runtime | ok | 14.0 (+0.0) | 22 (+4) | 16 (+0) | 11 (+0) | 0/0 | 2383 (+32) | 640 (+24) | 0 | - | reference; FAIL: setup instr, no extra RAM |
| sub0_b1_wire (vs handwritten_runtime) | ok | 14.0 (+0.0) | 22 (+0) | 16 (+0) | 11 (+0) | 0/0 | 2383 (+0) | 640 (+0) | 0 | - | PASS |
| sub0_b1_wire_origin_transport (vs handwritten_runtime) | ok | 14.0 (+0.0) | 22 (+0) | 16 (+0) | 11 (+0) | 0/0 | 2383 (+0) | 640 (+0) | 0 | - | PASS |
| sub0_b2_static | ok | 14.0 (+0.0) | 18 (+0) | 16 (+0) | 11 (+0) | 0/0 | 2351 (+0) | 616 (+0) | 0 | - | PASS |
| sub0_b2_static_origin_transport | ok | 14.0 (+0.0) | 18 (+0) | 16 (+0) | 11 (+0) | 0/0 | 2351 (+0) | 616 (+0) | 0 | - | PASS |
| sub0_b3_sink (vs handwritten_erased) | ok | 28.0 (+0.0) | 26 (+0) | 16 (+0) | 17 (+0) | 1/1 | 2499 (+0) | 656 (+0) | 26 | - | PASS |
| sub0_dynamic_route | ok | 127.0 (+113.0) | 45 (+27) | 80 (+64) | 76 (+65) | 1/2 | 4674 (+2323) | 992 (+376) | 577 | - | FAIL: publish instr, setup instr, teardown instr, publish path, no extra indirect calls, no extra RAM, no Sub0Pub retained |
| sub0_dynamic_route_lean | ok | 127.0 (+113.0) | 45 (+27) | 80 (+64) | 76 (+65) | 1/2 | 4658 (+2307) | 984 (+368) | 577 | - | FAIL: publish instr, setup instr, teardown instr, publish path, no extra indirect calls, no extra RAM, no Sub0Pub retained |

<details><summary>gcc-O2: largest symbols added by handwritten_erased (bytes)</summary>

- 110 `collapse_publish`
- 61 `collapse_setup`
- 52 `(anonymous namespace)::deliverEgress(void const*, (anonymous namespace)::Sample const&)`
- 16 `(anonymous namespace)::sensor`
- 16 `(anonymous namespace)::node`
- 1 `(anonymous namespace)::radio`
- 1 `(anonymous namespace)::controller`

</details>

<details><summary>gcc-O2: largest symbols added by handwritten_runtime (bytes)</summary>

- 33 `collapse_setup`
- 16 `(anonymous namespace)::node`
- 1 `(anonymous namespace)::radio`
- 1 `(anonymous namespace)::controller`

</details>

<details><summary>gcc-O2: largest symbols added by sub0_b1_wire (bytes)</summary>

- 16 `(anonymous namespace)::bus`

</details>

<details><summary>gcc-O2: largest symbols added by sub0_b1_wire_origin_transport (bytes)</summary>

- 16 `(anonymous namespace)::bus`

</details>

<details><summary>gcc-O2: largest symbols added by sub0_b3_sink (bytes)</summary>

- 52 `sub0::Sink<(anonymous namespace)::Sample>::Sink<sub0::Wiring<(anonymous namespace)::Controller, sub0::Forward<(anonymous namespace)::Radio> >, 0>(sub0::Wiring<(anonymous namespace)::Controller, sub0::Forward<(anonymous namespace)::Radio> >&)::{lambda(void const*, (anonymous namespace)::Sample const&)#1}::_FUN(void const*, (anonymous namespace)::Sample const&)`
- 16 `(anonymous namespace)::bus`

</details>

<details><summary>gcc-O2: largest symbols added by sub0_dynamic_route (bytes)</summary>

- 352 `collapse_publish`
- 326 `collapse_teardown`
- 224 `collapse_setup`
- 102 `sub0::Route<(anonymous namespace)::Sample, (anonymous namespace)::RadioPort>::receive((anonymous namespace)::Sample const&)`
- 97 `void sub0::kit::forgetInOwnDispatches<(anonymous namespace)::Sample>(void const*, sub0::Subscribe<(anonymous namespace)::Sample> const*) [clone .constprop.0]`
- 72 `sub0::detail::BrokerImpl<(anonymous namespace)::Sample, sub0::with<sub0::detail::BuiltinT<8u, (sub0::Dispatch)1, (sub0::Context)2, false, sub0::NoLock>, sub0::ContextWith<(sub0::Context)0> > >::global_`
- 66 `typeinfo name for sub0::detail::SubscriberInterface<(anonymous namespace)::Sample, false>`
- 55 `typeinfo name for sub0::Route<(anonymous namespace)::Sample, (anonymous namespace)::RadioPort>`

</details>

<details><summary>gcc-O2: largest symbols added by sub0_dynamic_route_lean (bytes)</summary>

- 336 `collapse_publish`
- 326 `collapse_teardown`
- 224 `collapse_setup`
- 102 `sub0::Route<(anonymous namespace)::Sample, (anonymous namespace)::RadioPort>::receive((anonymous namespace)::Sample const&)`
- 97 `void sub0::kit::forgetInOwnDispatches<(anonymous namespace)::Sample>(void const*, sub0::Subscribe<(anonymous namespace)::Sample> const*) [clone .constprop.0]`
- 72 `sub0::detail::BrokerImpl<(anonymous namespace)::Sample, sub0::with<sub0::detail::BuiltinT<8u, (sub0::Dispatch)1, (sub0::Context)2, false, sub0::NoLock>, sub0::DispatchWith<(sub0::Dispatch)1>, sub0::ContextWith<(sub0::Context)1>, sub0::NoFilter> >::global_`
- 66 `typeinfo name for sub0::detail::SubscriberInterface<(anonymous namespace)::Sample, false>`
- 55 `typeinfo name for sub0::Route<(anonymous namespace)::Sample, (anonymous namespace)::RadioPort>`

</details>

### clang-O2, observable work

| variant | checksum | publish instr | setup instr | teardown instr | path instr | calls (direct/indirect) | text | data+bss | retained sub0 (B) | added deps | verdict |
|---|---|---|---|---|---|---|---|---|---|---|---|
| handwritten | ok | 29.0 (+0.0) | 16 (+0) | 14 (+0) | 25 (+0) | 0/0 | 2138 (+0) | 656 (+0) | 0 | - | reference |
| handwritten_erased | ok | 43.0 (+14.0) | 22 (+6) | 14 (+0) | 23 (-2) | 0/1 | 2258 (+120) | 688 (+32) | 0 | - | reference; FAIL: publish instr, setup instr, no extra indirect calls, no extra RAM |
| handwritten_runtime | ok | 29.0 (+0.0) | 16 (+0) | 14 (+0) | 25 (+0) | 0/0 | 2138 (+0) | 656 (+0) | 0 | - | reference; PASS |
| sub0_b1_wire (vs handwritten_runtime) | ok | 29.0 (+0.0) | 16 (+0) | 14 (+0) | 25 (+0) | 0/0 | 2138 (+0) | 656 (+0) | 0 | - | PASS |
| sub0_b1_wire_origin_transport (vs handwritten_runtime) | ok | 29.0 (+0.0) | 16 (+0) | 14 (+0) | 25 (+0) | 0/0 | 2138 (+0) | 656 (+0) | 0 | - | PASS |
| sub0_b2_static | ok | 29.0 (+0.0) | 16 (+0) | 14 (+0) | 25 (+0) | 0/0 | 2138 (+0) | 656 (+0) | 0 | - | PASS |
| sub0_b2_static_origin_transport | ok | 29.0 (+0.0) | 16 (+0) | 14 (+0) | 25 (+0) | 0/0 | 2138 (+0) | 656 (+0) | 0 | - | PASS |
| sub0_b3_sink (vs handwritten_erased) | ok | 43.0 (+0.0) | 22 (+0) | 14 (+0) | 40 (+17) | 1/0 | 2271 (+13) | 688 (+0) | 0 | - | FAIL: publish path |
| sub0_dynamic_route | ok | 142.0 (+113.0) | 44 (+28) | 79 (+65) | 74 (+49) | 0/2 | 4822 (+2684) | 976 (+320) | 973 | - | FAIL: publish instr, setup instr, teardown instr, publish path, no extra indirect calls, no extra RAM, no Sub0Pub retained |
| sub0_dynamic_route_lean | ok | 141.0 (+112.0) | 44 (+28) | 79 (+65) | 74 (+49) | 0/2 | 4782 (+2644) | 976 (+320) | 971 | - | FAIL: publish instr, setup instr, teardown instr, publish path, no extra indirect calls, no extra RAM, no Sub0Pub retained |

### clang-O2, removable work

| variant | checksum | publish instr | setup instr | teardown instr | path instr | calls (direct/indirect) | text | data+bss | retained sub0 (B) | added deps | verdict |
|---|---|---|---|---|---|---|---|---|---|---|---|
| handwritten | ok | 14.0 (+0.0) | 16 (+0) | 14 (+0) | 10 (+0) | 0/0 | 2090 (+0) | 656 (+0) | 0 | - | reference |
| handwritten_erased | ok | 22.0 (+8.0) | 22 (+6) | 14 (+0) | 11 (+1) | 0/1 | 2202 (+112) | 688 (+32) | 0 | - | reference; FAIL: publish instr, setup instr, no extra indirect calls, no extra RAM |
| handwritten_runtime | ok | 14.0 (+0.0) | 16 (+0) | 14 (+0) | 10 (+0) | 0/0 | 2090 (+0) | 656 (+0) | 0 | - | reference; PASS |
| sub0_b1_wire (vs handwritten_runtime) | ok | 14.0 (+0.0) | 16 (+0) | 14 (+0) | 10 (+0) | 0/0 | 2090 (+0) | 656 (+0) | 0 | - | PASS |
| sub0_b1_wire_origin_transport (vs handwritten_runtime) | ok | 14.0 (+0.0) | 16 (+0) | 14 (+0) | 10 (+0) | 0/0 | 2090 (+0) | 656 (+0) | 0 | - | PASS |
| sub0_b2_static | ok | 14.0 (+0.0) | 16 (+0) | 14 (+0) | 10 (+0) | 0/0 | 2090 (+0) | 656 (+0) | 0 | - | PASS |
| sub0_b2_static_origin_transport | ok | 14.0 (+0.0) | 16 (+0) | 14 (+0) | 10 (+0) | 0/0 | 2090 (+0) | 656 (+0) | 0 | - | PASS |
| sub0_b3_sink (vs handwritten_erased) | ok | 22.0 (+0.0) | 22 (+0) | 14 (+0) | 19 (+8) | 1/0 | 2206 (+4) | 688 (+0) | 0 | - | FAIL: publish path |
| sub0_dynamic_route | ok | 124.0 (+110.0) | 44 (+28) | 79 (+65) | 74 (+64) | 0/2 | 4806 (+2716) | 976 (+320) | 973 | - | FAIL: publish instr, setup instr, teardown instr, publish path, no extra indirect calls, no extra RAM, no Sub0Pub retained |
| sub0_dynamic_route_lean | ok | 123.0 (+109.0) | 44 (+28) | 79 (+65) | 74 (+64) | 0/2 | 4766 (+2676) | 976 (+320) | 971 | - | FAIL: publish instr, setup instr, teardown instr, publish path, no extra indirect calls, no extra RAM, no Sub0Pub retained |

<details><summary>clang-O2: largest symbols added by handwritten_erased (bytes)</summary>

- 46 `(anonymous namespace)::deliverEgress(void const*, (anonymous namespace)::Sample const&)`
- 43 `collapse_setup`
- 16 `(anonymous namespace)::node`
- 8 `_ZN12_GLOBAL__N_16sensorE.0`
- 1 `(anonymous namespace)::radio`
- 1 `(anonymous namespace)::controller`

</details>

<details><summary>clang-O2: largest symbols added by sub0_b3_sink (bytes)</summary>

- 46 `_ZZN4sub04SinkIN12_GLOBAL__N_16SampleEEC1INS_6WiringIJNS1_10ControllerENS_7ForwardINS1_5RadioEEEEEETnNSt9enable_ifIXntsr3stdE9is_same_vINSt9remove_cvIT_E4typeES3_EEiE4typeELi0EEERSD_ENUlPKvRKS2_E_8__invokeESK_SM_`
- 16 `(anonymous namespace)::bus`

</details>

<details><summary>clang-O2: largest symbols added by sub0_dynamic_route (bytes)</summary>

- 538 `collapse_teardown`
- 515 `sub0::Subscribe<(anonymous namespace)::Sample>::~Subscribe()`
- 316 `collapse_publish`
- 191 `collapse_setup`
- 87 `sub0::Route<(anonymous namespace)::Sample, (anonymous namespace)::RadioPort>::receive((anonymous namespace)::Sample const&)`
- 72 `sub0::detail::BrokerImpl<(anonymous namespace)::Sample, sub0::with<sub0::detail::BuiltinT<8u, (sub0::Dispatch)1, (sub0::Context)2, false, sub0::NoLock>, sub0::ContextWith<(sub0::Context)0> > >::global_`
- 65 `typeinfo name for sub0::detail::SubscriberInterface<(anonymous namespace)::Sample, false>`
- 54 `typeinfo name for sub0::Route<(anonymous namespace)::Sample, (anonymous namespace)::RadioPort>`

</details>

<details><summary>clang-O2: largest symbols added by sub0_dynamic_route_lean (bytes)</summary>

- 538 `collapse_teardown`
- 515 `sub0::Subscribe<(anonymous namespace)::Sample>::~Subscribe()`
- 285 `collapse_publish`
- 191 `collapse_setup`
- 85 `sub0::Route<(anonymous namespace)::Sample, (anonymous namespace)::RadioPort>::receive((anonymous namespace)::Sample const&)`
- 72 `sub0::detail::BrokerImpl<(anonymous namespace)::Sample, sub0::with<sub0::detail::BuiltinT<8u, (sub0::Dispatch)1, (sub0::Context)2, false, sub0::NoLock>, sub0::DispatchWith<(sub0::Dispatch)1>, sub0::ContextWith<(sub0::Context)1>, sub0::NoFilter> >::global_`
- 65 `typeinfo name for sub0::detail::SubscriberInterface<(anonymous namespace)::Sample, false>`
- 54 `typeinfo name for sub0::Route<(anonymous namespace)::Sample, (anonymous namespace)::RadioPort>`

</details>

### cm33-gcc-Os, observable work

| variant | checksum | publish instr | setup instr | teardown instr | path instr | calls (direct/indirect) | text | data+bss | retained sub0 (B) | added deps | verdict |
|---|---|---|---|---|---|---|---|---|---|---|---|
| handwritten | - | - | - | - | 25 (+0) | 0/0 | 1148 (+0) | 508 (+0) | 0 | - | reference |
| handwritten_erased | - | - | - | - | 25 (+0) | 0/1 | 1224 (+76) | 528 (+20) | 0 | - | reference; FAIL: no extra indirect calls, no extra RAM |
| handwritten_runtime | - | - | - | - | 25 (+0) | 0/0 | 1168 (+20) | 520 (+12) | 0 | - | reference; FAIL: no extra RAM |
| sub0_b1_wire (vs handwritten_runtime) | - | - | - | - | 25 (+0) | 0/0 | 1168 (+0) | 520 (+0) | 0 | - | PASS |
| sub0_b1_wire_origin_transport (vs handwritten_runtime) | - | - | - | - | 25 (+0) | 0/0 | 1168 (+0) | 520 (+0) | 0 | - | PASS |
| sub0_b2_static | - | - | - | - | 25 (+0) | 0/0 | 1148 (+0) | 508 (+0) | 0 | - | PASS |
| sub0_b2_static_origin_transport | - | - | - | - | 25 (+0) | 0/0 | 1148 (+0) | 508 (+0) | 0 | - | PASS |
| sub0_b3_sink (vs handwritten_erased) | - | - | - | - | 25 (+0) | 0/1 | 1224 (+0) | 528 (+0) | 44 | - | PASS |
| sub0_dynamic_route | - | - | - | - | 20 (-5) | 0/2 | 1864 (+716) | 832 (+324) | 370 | TLS | FAIL: no extra indirect calls, no extra RAM, no Sub0Pub retained, no extra dependencies |
| sub0_dynamic_route_lean | - | - | - | - | 20 (-5) | 0/2 | 1836 (+688) | 576 (+68) | 350 | - | FAIL: no extra indirect calls, no extra RAM, no Sub0Pub retained |

### cm33-gcc-Os, removable work

| variant | checksum | publish instr | setup instr | teardown instr | path instr | calls (direct/indirect) | text | data+bss | retained sub0 (B) | added deps | verdict |
|---|---|---|---|---|---|---|---|---|---|---|---|
| handwritten | - | - | - | - | 13 (+0) | 0/0 | 1112 (+0) | 508 (+0) | 0 | - | reference |
| handwritten_erased | - | - | - | - | 15 (+2) | 0/1 | 1172 (+60) | 528 (+20) | 0 | - | reference; FAIL: no extra indirect calls, no extra RAM |
| handwritten_runtime | - | - | - | - | 13 (+0) | 0/0 | 1132 (+20) | 520 (+12) | 0 | - | reference; FAIL: no extra RAM |
| sub0_b1_wire (vs handwritten_runtime) | - | - | - | - | 13 (+0) | 0/0 | 1132 (+0) | 520 (+0) | 0 | - | PASS |
| sub0_b1_wire_origin_transport (vs handwritten_runtime) | - | - | - | - | 13 (+0) | 0/0 | 1132 (+0) | 520 (+0) | 0 | - | PASS |
| sub0_b2_static | - | - | - | - | 13 (+0) | 0/0 | 1112 (+0) | 508 (+0) | 0 | - | PASS |
| sub0_b2_static_origin_transport | - | - | - | - | 13 (+0) | 0/0 | 1112 (+0) | 508 (+0) | 0 | - | PASS |
| sub0_b3_sink (vs handwritten_erased) | - | - | - | - | 15 (+0) | 0/1 | 1172 (+0) | 528 (+0) | 20 | - | PASS |
| sub0_dynamic_route | - | - | - | - | 20 (+7) | 0/2 | 1840 (+728) | 832 (+324) | 370 | TLS | FAIL: publish path, no extra indirect calls, no extra RAM, no Sub0Pub retained, no extra dependencies |
| sub0_dynamic_route_lean | - | - | - | - | 20 (+7) | 0/2 | 1812 (+700) | 576 (+68) | 350 | - | FAIL: publish path, no extra indirect calls, no extra RAM, no Sub0Pub retained |

<details><summary>cm33-gcc-Os: largest symbols added by handwritten_erased (bytes)</summary>

- 44 `(anonymous namespace)::deliverEgress(void const*, (anonymous namespace)::Sample const&)`
- 40 `collapse_setup`
- 8 `(anonymous namespace)::sensor`
- 8 `(anonymous namespace)::node`
- 1 `(anonymous namespace)::radio`
- 1 `(anonymous namespace)::controller`

</details>

<details><summary>cm33-gcc-Os: largest symbols added by handwritten_runtime (bytes)</summary>

- 24 `collapse_setup`
- 8 `(anonymous namespace)::node`
- 1 `(anonymous namespace)::radio`
- 1 `(anonymous namespace)::controller`

</details>

<details><summary>cm33-gcc-Os: largest symbols added by sub0_b1_wire (bytes)</summary>

- 8 `(anonymous namespace)::bus`

</details>

<details><summary>cm33-gcc-Os: largest symbols added by sub0_b1_wire_origin_transport (bytes)</summary>

- 8 `(anonymous namespace)::bus`

</details>

<details><summary>cm33-gcc-Os: largest symbols added by sub0_b3_sink (bytes)</summary>

- 44 `sub0::Sink<(anonymous namespace)::Sample>::Sink<sub0::Wiring<(anonymous namespace)::Controller, sub0::Forward<(anonymous namespace)::Radio> >, 0>(sub0::Wiring<(anonymous namespace)::Controller, sub0::Forward<(anonymous namespace)::Radio> >&)::{lambda(void const*, (anonymous namespace)::Sample const&)#1}::_FUN(void const*, (anonymous namespace)::Sample const&)`
- 8 `(anonymous namespace)::bus`

</details>

<details><summary>cm33-gcc-Os: largest symbols added by sub0_dynamic_route (bytes)</summary>

- 256 `tlsBlock`
- 254 `memmove`
- 120 `sub0::Subscribe<(anonymous namespace)::Sample>::disconnect()`
- 96 `sub0::detail::BrokerImpl<(anonymous namespace)::Sample, sub0::with<sub0::detail::BuiltinT<8ul, (sub0::Dispatch)1, (sub0::Context)2, false, sub0::NoLock>, sub0::ContextWith<(sub0::Context)0> > >::publish((anonymous namespace)::Sample const&, void const*, sub0::PublishReport*) const [clone .constprop.0]`
- 68 `collapse_setup`
- 64 `sub0::Route<(anonymous namespace)::Sample, (anonymous namespace)::RadioPort>::receive((anonymous namespace)::Sample const&)`
- 36 `sub0::detail::BrokerImpl<(anonymous namespace)::Sample, sub0::with<sub0::detail::BuiltinT<8ul, (sub0::Dispatch)1, (sub0::Context)2, false, sub0::NoLock>, sub0::ContextWith<(sub0::Context)0> > >::global_`
- 36 `collapse_teardown`

</details>

<details><summary>cm33-gcc-Os: largest symbols added by sub0_dynamic_route_lean (bytes)</summary>

- 254 `memmove`
- 116 `sub0::Subscribe<(anonymous namespace)::Sample>::disconnect()`
- 88 `sub0::detail::BrokerImpl<(anonymous namespace)::Sample, sub0::with<sub0::detail::BuiltinT<8ul, (sub0::Dispatch)1, (sub0::Context)2, false, sub0::NoLock>, sub0::DispatchWith<(sub0::Dispatch)1>, sub0::ContextWith<(sub0::Context)1>, sub0::NoFilter> >::publish((anonymous namespace)::Sample const&, void const*, sub0::PublishReport*) const [clone .constprop.0]`
- 68 `collapse_setup`
- 56 `sub0::Route<(anonymous namespace)::Sample, (anonymous namespace)::RadioPort>::receive((anonymous namespace)::Sample const&)`
- 36 `sub0::detail::BrokerImpl<(anonymous namespace)::Sample, sub0::with<sub0::detail::BuiltinT<8ul, (sub0::Dispatch)1, (sub0::Context)2, false, sub0::NoLock>, sub0::DispatchWith<(sub0::Dispatch)1>, sub0::ContextWith<(sub0::Context)1>, sub0::NoFilter> >::global_`
- 36 `collapse_teardown`
- 32 `sub0::Subscribe<(anonymous namespace)::Sample>::trySubscribe() [clone .isra.0]`

</details>

## Case: transport_two_links

### gcc-O2, observable work

| variant | checksum | publish instr | setup instr | teardown instr | path instr | calls (direct/indirect) | text | data+bss | retained sub0 (B) | added deps | verdict |
|---|---|---|---|---|---|---|---|---|---|---|---|
| handwritten | ok | 40.0 (+0.0) | 20 (+0) | 16 (+0) | 37 (+0) | 0/0 | 2447 (+0) | 624 (+0) | 0 | - | reference |
| handwritten_runtime | ok | 45.0 (+5.0) | 26 (+6) | 16 (+0) | 41 (+4) | 0/0 | 2511 (+64) | 656 (+32) | 0 | - | reference; FAIL: publish instr, setup instr, publish path, no extra RAM |
| sub0_b1_wire (vs handwritten_runtime) | ok | 53.0 (+8.0) | 26 (+0) | 16 (+0) | 56 (+15) | 0/0 | 2559 (+48) | 656 (+0) | 0 | - | FAIL: publish instr, publish path |
| sub0_b1_wire_typed_links (vs handwritten_runtime) | ok | 45.0 (+0.0) | 26 (+0) | 16 (+0) | 41 (+0) | 0/0 | 2511 (+0) | 656 (+0) | 0 | - | PASS |
| sub0_b2_static | ok | 40.0 (+0.0) | 20 (+0) | 16 (+0) | 37 (+0) | 0/0 | 2447 (+0) | 624 (+0) | 0 | - | PASS |

### gcc-O2, removable work

| variant | checksum | publish instr | setup instr | teardown instr | path instr | calls (direct/indirect) | text | data+bss | retained sub0 (B) | added deps | verdict |
|---|---|---|---|---|---|---|---|---|---|---|---|
| handwritten | ok | 27.0 (+0.0) | 20 (+0) | 16 (+0) | 23 (+0) | 0/0 | 2399 (+0) | 624 (+0) | 0 | - | reference |
| handwritten_runtime | ok | 31.0 (+4.0) | 26 (+6) | 16 (+0) | 27 (+4) | 0/0 | 2463 (+64) | 656 (+32) | 0 | - | reference; FAIL: publish instr, setup instr, publish path, no extra RAM |
| sub0_b1_wire (vs handwritten_runtime) | ok | 39.0 (+8.0) | 26 (+0) | 16 (+0) | 42 (+15) | 0/0 | 2511 (+48) | 656 (+0) | 0 | - | FAIL: publish instr, publish path |
| sub0_b1_wire_typed_links (vs handwritten_runtime) | ok | 31.0 (+0.0) | 26 (+0) | 16 (+0) | 27 (+0) | 0/0 | 2463 (+0) | 656 (+0) | 0 | - | PASS |
| sub0_b2_static | ok | 27.0 (+0.0) | 20 (+0) | 16 (+0) | 23 (+0) | 0/0 | 2399 (+0) | 624 (+0) | 0 | - | PASS |

<details><summary>gcc-O2: largest symbols added by handwritten_runtime (bytes)</summary>

- 135 `collapse_publish`
- 67 `collapse_setup`
- 24 `(anonymous namespace)::node`
- 1 `(anonymous namespace)::controller`

</details>

<details><summary>gcc-O2: largest symbols added by sub0_b1_wire (bytes)</summary>

- 186 `collapse_publish`
- 24 `(anonymous namespace)::bus`

</details>

<details><summary>gcc-O2: largest symbols added by sub0_b1_wire_typed_links (bytes)</summary>

- 24 `(anonymous namespace)::bus`

</details>

### clang-O2, observable work

| variant | checksum | publish instr | setup instr | teardown instr | path instr | calls (direct/indirect) | text | data+bss | retained sub0 (B) | added deps | verdict |
|---|---|---|---|---|---|---|---|---|---|---|---|
| handwritten | ok | 40.0 (+0.0) | 18 (+0) | 14 (+0) | 36 (+0) | 0/0 | 2186 (+0) | 664 (+0) | 0 | - | reference |
| handwritten_runtime | ok | 40.0 (+0.0) | 18 (+0) | 14 (+0) | 36 (+0) | 0/0 | 2186 (+0) | 664 (+0) | 0 | - | reference; PASS |
| sub0_b1_wire (vs handwritten_runtime) | ok | 50.0 (+10.0) | 24 (+6) | 14 (+0) | 53 (+17) | 0/0 | 2282 (+96) | 696 (+32) | 0 | - | FAIL: publish instr, setup instr, publish path, no extra RAM |
| sub0_b1_wire_typed_links (vs handwritten_runtime) | ok | 45.0 (+5.0) | 24 (+6) | 14 (+0) | 42 (+6) | 0/0 | 2250 (+64) | 696 (+32) | 0 | - | FAIL: publish instr, setup instr, publish path, no extra RAM |
| sub0_b2_static | ok | 40.0 (+0.0) | 18 (+0) | 14 (+0) | 36 (+0) | 0/0 | 2186 (+0) | 664 (+0) | 0 | - | PASS |

### clang-O2, removable work

| variant | checksum | publish instr | setup instr | teardown instr | path instr | calls (direct/indirect) | text | data+bss | retained sub0 (B) | added deps | verdict |
|---|---|---|---|---|---|---|---|---|---|---|---|
| handwritten | ok | 27.0 (+0.0) | 18 (+0) | 14 (+0) | 24 (+0) | 0/0 | 2138 (+0) | 664 (+0) | 0 | - | reference |
| handwritten_runtime | ok | 27.0 (+0.0) | 18 (+0) | 14 (+0) | 24 (+0) | 0/0 | 2138 (+0) | 664 (+0) | 0 | - | reference; PASS |
| sub0_b1_wire (vs handwritten_runtime) | ok | 36.0 (+9.0) | 24 (+6) | 14 (+0) | 39 (+15) | 0/0 | 2234 (+96) | 696 (+32) | 0 | - | FAIL: publish instr, setup instr, publish path, no extra RAM |
| sub0_b1_wire_typed_links (vs handwritten_runtime) | ok | 31.0 (+4.0) | 24 (+6) | 14 (+0) | 27 (+3) | 0/0 | 2186 (+48) | 696 (+32) | 0 | - | FAIL: publish instr, setup instr, publish path, no extra RAM |
| sub0_b2_static | ok | 27.0 (+0.0) | 18 (+0) | 14 (+0) | 24 (+0) | 0/0 | 2138 (+0) | 664 (+0) | 0 | - | PASS |

<details><summary>clang-O2: largest symbols added by sub0_b1_wire (bytes)</summary>

- 163 `collapse_publish`
- 63 `collapse_setup`
- 24 `(anonymous namespace)::bus`
- 4 `(anonymous namespace)::radioB`
- 4 `(anonymous namespace)::radioA`
- 1 `(anonymous namespace)::controller`

</details>

<details><summary>clang-O2: largest symbols added by sub0_b1_wire_typed_links (bytes)</summary>

- 129 `collapse_publish`
- 63 `collapse_setup`
- 24 `(anonymous namespace)::bus`
- 4 `(anonymous namespace)::radioB`
- 4 `(anonymous namespace)::radioA`
- 1 `(anonymous namespace)::controller`

</details>

### cm33-gcc-Os, observable work

| variant | checksum | publish instr | setup instr | teardown instr | path instr | calls (direct/indirect) | text | data+bss | retained sub0 (B) | added deps | verdict |
|---|---|---|---|---|---|---|---|---|---|---|---|
| handwritten | - | - | - | - | 37 (+0) | 0/0 | 1200 (+0) | 516 (+0) | 0 | - | reference |
| handwritten_runtime | - | - | - | - | 42 (+5) | 0/0 | 1224 (+24) | 532 (+16) | 0 | - | reference; FAIL: publish path, no extra RAM |
| sub0_b1_wire (vs handwritten_runtime) | - | - | - | - | 52 (+10) | 4/0 | 1252 (+28) | 532 (+0) | 0 | - | FAIL: publish path |
| sub0_b1_wire_typed_links (vs handwritten_runtime) | - | - | - | - | 42 (+0) | 3/0 | 1228 (+4) | 532 (+0) | 0 | - | PASS |
| sub0_b2_static | - | - | - | - | 37 (+0) | 0/0 | 1200 (+0) | 516 (+0) | 0 | - | PASS |

### cm33-gcc-Os, removable work

| variant | checksum | publish instr | setup instr | teardown instr | path instr | calls (direct/indirect) | text | data+bss | retained sub0 (B) | added deps | verdict |
|---|---|---|---|---|---|---|---|---|---|---|---|
| handwritten | - | - | - | - | 28 (+0) | 0/0 | 1172 (+0) | 516 (+0) | 0 | - | reference |
| handwritten_runtime | - | - | - | - | 31 (+3) | 0/0 | 1192 (+20) | 532 (+16) | 0 | - | reference; FAIL: publish path, no extra RAM |
| sub0_b1_wire (vs handwritten_runtime) | - | - | - | - | 41 (+10) | 4/0 | 1220 (+28) | 532 (+0) | 0 | - | FAIL: publish path |
| sub0_b1_wire_typed_links (vs handwritten_runtime) | - | - | - | - | 29 (-2) | 3/0 | 1192 (+0) | 532 (+0) | 0 | - | PASS |
| sub0_b2_static | - | - | - | - | 28 (+0) | 0/0 | 1172 (+0) | 516 (+0) | 0 | - | PASS |

<details><summary>cm33-gcc-Os: largest symbols added by handwritten_runtime (bytes)</summary>

- 108 `collapse_publish`
- 40 `collapse_setup`
- 12 `(anonymous namespace)::node`
- 1 `(anonymous namespace)::controller`

</details>

<details><summary>cm33-gcc-Os: largest symbols added by sub0_b1_wire (bytes)</summary>

- 112 `collapse_publish`
- 24 `_ZNK4sub07ForwardIN12_GLOBAL__N_15RadioEE7receiveINS1_6SampleEEEDTcmcldtclL_ZSt7declvalIRS2_EDTcl9__declvalIT_ELi0EEEvEEL_ZNS2_4sendERKS5_Efp_Ecvv_EERKS8_.isra.0`
- 12 `(anonymous namespace)::bus`

</details>

<details><summary>cm33-gcc-Os: largest symbols added by sub0_b1_wire_typed_links (bytes)</summary>

- 24 `_ZNK4sub07ForwardIN12_GLOBAL__N_15RadioEE7receiveINS1_6SampleEEEDTcmcldtclL_ZSt7declvalIRS2_EDTcl9__declvalIT_ELi0EEEvEEL_ZNS2_4sendERKS5_Efp_Ecvv_EERKS8_.isra.0`
- 12 `(anonymous namespace)::bus`

</details>

## Case: two_domains

### gcc-O2, observable work

| variant | checksum | publish instr | setup instr | teardown instr | path instr | calls (direct/indirect) | text | data+bss | retained sub0 (B) | added deps | verdict |
|---|---|---|---|---|---|---|---|---|---|---|---|
| handwritten | ok | 31.0 (+0.0) | 21 (+0) | 16 (+0) | 27 (+0) | 0/0 | 2431 (+0) | 632 (+0) | 0 | - | reference |
| handwritten_erased | ok | 65.0 (+34.0) | 35 (+14) | 16 (+0) | 29 (+2) | 1/2 | 2767 (+336) | 696 (+64) | 0 | - | reference; FAIL: publish instr, setup instr, no extra indirect calls, no extra RAM |
| handwritten_gateway | ok | 37.0 (+6.0) | 27 (+6) | 16 (+0) | 32 (+5) | 0/0 | 2479 (+48) | 656 (+24) | 0 | - | reference; FAIL: publish instr, setup instr, publish path, no extra RAM |
| handwritten_runtime | ok | 38.0 (+7.0) | 27 (+6) | 16 (+0) | 34 (+7) | 0/0 | 2495 (+64) | 664 (+32) | 0 | - | reference; FAIL: publish instr, setup instr, publish path, no extra RAM |
| sub0_b1_mixin (vs handwritten_runtime) | ok | 38.0 (+0.0) | 27 (+0) | 16 (+0) | 34 (+0) | 0/0 | 2495 (+0) | 664 (+0) | 0 | - | PASS |
| sub0_b1_one_publisher (vs handwritten_gateway) | ok | 37.0 (+0.0) | 27 (+0) | 16 (+0) | 32 (+0) | 0/0 | 2479 (+0) | 656 (+0) | 0 | - | PASS |
| sub0_b1_wire (vs handwritten_runtime) | ok | 38.0 (+0.0) | 27 (+0) | 16 (+0) | 34 (+0) | 0/0 | 2495 (+0) | 664 (+0) | 0 | - | PASS |
| sub0_b2_one_publisher | ok | 30.0 (-1.0) | 21 (+0) | 16 (+0) | 26 (-1) | 0/0 | 2431 (+0) | 632 (+0) | 0 | - | PASS |
| sub0_b2_static | ok | 31.0 (+0.0) | 21 (+0) | 16 (+0) | 27 (+0) | 0/0 | 2431 (+0) | 632 (+0) | 0 | - | PASS |
| sub0_b3_sink (vs handwritten_erased) | ok | 65.0 (+0.0) | 35 (+0) | 16 (+0) | 29 (+0) | 1/2 | 2767 (+0) | 696 (+0) | 102 | - | PASS |
| sub0_dynamic_domain | ok | 102.0 (+71.0) | 89 (+68) | 126 (+110) | 59 (+32) | 1/2 | 5258 (+2827) | 1128 (+496) | 187 | - | FAIL: publish instr, setup instr, teardown instr, publish path, no extra indirect calls, no extra RAM, no Sub0Pub retained |

### gcc-O2, removable work

| variant | checksum | publish instr | setup instr | teardown instr | path instr | calls (direct/indirect) | text | data+bss | retained sub0 (B) | added deps | verdict |
|---|---|---|---|---|---|---|---|---|---|---|---|
| handwritten | ok | 10.0 (+0.0) | 21 (+0) | 16 (+0) | 6 (+0) | 0/0 | 2367 (+0) | 632 (+0) | 0 | - | reference |
| handwritten_erased | ok | 38.0 (+28.0) | 35 (+14) | 16 (+0) | 29 (+23) | 1/2 | 2671 (+304) | 696 (+64) | 0 | - | reference; FAIL: publish instr, setup instr, publish path, no extra indirect calls, no extra RAM |
| handwritten_gateway | ok | 10.0 (+0.0) | 27 (+6) | 16 (+0) | 6 (+0) | 0/0 | 2399 (+32) | 656 (+24) | 0 | - | reference; FAIL: setup instr, no extra RAM |
| handwritten_runtime | ok | 11.0 (+1.0) | 27 (+6) | 16 (+0) | 7 (+1) | 0/0 | 2399 (+32) | 664 (+32) | 0 | - | reference; FAIL: setup instr, no extra RAM |
| sub0_b1_mixin (vs handwritten_runtime) | ok | 11.0 (+0.0) | 27 (+0) | 16 (+0) | 7 (+0) | 0/0 | 2399 (+0) | 664 (+0) | 0 | - | PASS |
| sub0_b1_one_publisher (vs handwritten_gateway) | ok | 10.0 (+0.0) | 27 (+0) | 16 (+0) | 6 (+0) | 0/0 | 2399 (+0) | 656 (+0) | 0 | - | PASS |
| sub0_b1_wire (vs handwritten_runtime) | ok | 11.0 (+0.0) | 27 (+0) | 16 (+0) | 7 (+0) | 0/0 | 2399 (+0) | 664 (+0) | 0 | - | PASS |
| sub0_b2_one_publisher | ok | 9.0 (-1.0) | 21 (+0) | 16 (+0) | 6 (+0) | 0/0 | 2367 (+0) | 632 (+0) | 0 | - | PASS |
| sub0_b2_static | ok | 10.0 (+0.0) | 21 (+0) | 16 (+0) | 6 (+0) | 0/0 | 2367 (+0) | 632 (+0) | 0 | - | PASS |
| sub0_b3_sink (vs handwritten_erased) | ok | 38.0 (+0.0) | 35 (+0) | 16 (+0) | 29 (+0) | 1/2 | 2671 (+0) | 696 (+0) | 16 | - | PASS |
| sub0_dynamic_domain | ok | 77.0 (+67.0) | 89 (+68) | 126 (+110) | 59 (+53) | 1/2 | 5166 (+2799) | 1128 (+496) | 187 | - | FAIL: publish instr, setup instr, teardown instr, publish path, no extra indirect calls, no extra RAM, no Sub0Pub retained |

<details><summary>gcc-O2: largest symbols added by handwritten_erased (bytes)</summary>

- 133 `collapse_setup`
- 118 `collapse_publish`
- 68 `(anonymous namespace)::deliverA(void const*, (anonymous namespace)::Sample const&)`
- 34 `(anonymous namespace)::deliverB(void const*, (anonymous namespace)::Sample const&)`
- 16 `(anonymous namespace)::sensorB`
- 16 `(anonymous namespace)::sensorA`
- 16 `(anonymous namespace)::nodeA`
- 8 `(anonymous namespace)::nodeB`

</details>

<details><summary>gcc-O2: largest symbols added by handwritten_gateway (bytes)</summary>

- 112 `collapse_publish`
- 77 `collapse_setup`
- 24 `(anonymous namespace)::gateway`

</details>

<details><summary>gcc-O2: largest symbols added by handwritten_runtime (bytes)</summary>

- 119 `collapse_publish`
- 77 `collapse_setup`
- 16 `(anonymous namespace)::sensorA`
- 8 `(anonymous namespace)::sensorB`

</details>

<details><summary>gcc-O2: largest symbols added by sub0_b3_sink (bytes)</summary>

- 68 `sub0::Sink<(anonymous namespace)::Sample>::Sink<sub0::Wiring<(anonymous namespace)::Controller, (anonymous namespace)::Logger>, 0>(sub0::Wiring<(anonymous namespace)::Controller, (anonymous namespace)::Logger>&)::{lambda(void const*, (anonymous namespace)::Sample const&)#1}::_FUN(void const*, (anonymous namespace)::Sample const&)`
- 34 `sub0::Sink<(anonymous namespace)::Sample>::Sink<sub0::Wiring<(anonymous namespace)::Controller>, 0>(sub0::Wiring<(anonymous namespace)::Controller>&)::{lambda(void const*, (anonymous namespace)::Sample const&)#1}::_FUN(void const*, (anonymous namespace)::Sample const&)`
- 16 `(anonymous namespace)::busA`
- 8 `(anonymous namespace)::busB`

</details>

<details><summary>gcc-O2: largest symbols added by sub0_dynamic_domain (bytes)</summary>

- 797 `collapse_teardown`
- 421 `collapse_setup`
- 207 `collapse_publish`
- 80 `(anonymous namespace)::domainB`
- 80 `(anonymous namespace)::domainA`
- 66 `typeinfo name for sub0::detail::SubscriberInterface<(anonymous namespace)::Sample, false>`
- 44 `typeinfo name for sub0::Subscribe<(anonymous namespace)::Sample>`
- 37 `(anonymous namespace)::Logger::receive((anonymous namespace)::Sample const&)`

</details>

### clang-O2, observable work

| variant | checksum | publish instr | setup instr | teardown instr | path instr | calls (direct/indirect) | text | data+bss | retained sub0 (B) | added deps | verdict |
|---|---|---|---|---|---|---|---|---|---|---|---|
| handwritten | ok | 34.0 (+0.0) | 19 (+0) | 14 (+0) | 31 (+0) | 0/0 | 2186 (+0) | 672 (+0) | 0 | - | reference |
| handwritten_erased | ok | 58.0 (+24.0) | 29 (+10) | 14 (+0) | 22 (-9) | 0/2 | 2406 (+220) | 712 (+40) | 0 | - | reference; FAIL: publish instr, setup instr, no extra indirect calls, no extra RAM |
| handwritten_gateway | ok | 34.0 (+0.0) | 19 (+0) | 14 (+0) | 31 (+0) | 0/0 | 2186 (+0) | 672 (+0) | 0 | - | reference; PASS |
| handwritten_runtime | ok | 34.0 (+0.0) | 21 (+2) | 14 (+0) | 31 (+0) | 0/0 | 2202 (+16) | 680 (+8) | 0 | - | reference; FAIL: setup instr, no extra RAM |
| sub0_b1_mixin (vs handwritten_runtime) | ok | 34.0 (+0.0) | 21 (+0) | 14 (+0) | 31 (+0) | 0/0 | 2202 (+0) | 680 (+0) | 0 | - | PASS |
| sub0_b1_one_publisher (vs handwritten_gateway) | ok | 37.0 (+3.0) | 25 (+6) | 14 (+0) | 34 (+3) | 0/0 | 2234 (+48) | 696 (+24) | 0 | - | FAIL: publish instr, setup instr, publish path, no extra RAM |
| sub0_b1_wire (vs handwritten_runtime) | ok | 34.0 (+0.0) | 21 (+0) | 14 (+0) | 31 (+0) | 0/0 | 2202 (+0) | 680 (+0) | 0 | - | PASS |
| sub0_b2_one_publisher | ok | 34.0 (+0.0) | 19 (+0) | 14 (+0) | 31 (+0) | 0/0 | 2186 (+0) | 672 (+0) | 0 | - | PASS |
| sub0_b2_static | ok | 34.0 (+0.0) | 19 (+0) | 14 (+0) | 31 (+0) | 0/0 | 2186 (+0) | 672 (+0) | 0 | - | PASS |
| sub0_b3_sink (vs handwritten_erased) | ok | 58.0 (+0.0) | 29 (+0) | 14 (+0) | 56 (+34) | 2/0 | 2418 (+12) | 712 (+0) | 0 | - | FAIL: publish path |
| sub0_dynamic_domain | ok | 97.0 (+63.0) | 58 (+39) | 112 (+98) | 53 (+22) | 0/2 | 5320 (+3134) | 1096 (+424) | 181 | - | FAIL: publish instr, setup instr, teardown instr, publish path, no extra indirect calls, no extra RAM, no Sub0Pub retained |

### clang-O2, removable work

| variant | checksum | publish instr | setup instr | teardown instr | path instr | calls (direct/indirect) | text | data+bss | retained sub0 (B) | added deps | verdict |
|---|---|---|---|---|---|---|---|---|---|---|---|
| handwritten | ok | 9.0 (+0.0) | 17 (+0) | 14 (+0) | 6 (+0) | 0/0 | 2090 (+0) | 664 (+0) | 0 | - | reference |
| handwritten_erased | ok | 21.0 (+12.0) | 29 (+12) | 14 (+0) | 14 (+8) | 0/1 | 2258 (+168) | 712 (+48) | 0 | - | reference; FAIL: publish instr, setup instr, publish path, no extra indirect calls, no extra RAM |
| handwritten_gateway | ok | 9.0 (+0.0) | 19 (+2) | 14 (+0) | 6 (+0) | 0/0 | 2106 (+16) | 672 (+8) | 0 | - | reference; FAIL: setup instr, no extra RAM |
| handwritten_runtime | ok | 9.0 (+0.0) | 18 (+1) | 14 (+0) | 6 (+0) | 0/0 | 2106 (+16) | 664 (+0) | 0 | - | reference; FAIL: setup instr |
| sub0_b1_mixin (vs handwritten_runtime) | ok | 9.0 (+0.0) | 18 (+0) | 14 (+0) | 6 (+0) | 0/0 | 2106 (+0) | 664 (+0) | 0 | - | PASS |
| sub0_b1_one_publisher (vs handwritten_gateway) | ok | 9.0 (+0.0) | 19 (+0) | 14 (+0) | 6 (+0) | 0/0 | 2106 (+0) | 672 (+0) | 0 | - | PASS |
| sub0_b1_wire (vs handwritten_runtime) | ok | 9.0 (+0.0) | 18 (+0) | 14 (+0) | 6 (+0) | 0/0 | 2106 (+0) | 664 (+0) | 0 | - | PASS |
| sub0_b2_one_publisher | ok | 9.0 (+0.0) | 17 (+0) | 14 (+0) | 6 (+0) | 0/0 | 2090 (+0) | 664 (+0) | 0 | - | PASS |
| sub0_b2_static | ok | 9.0 (+0.0) | 17 (+0) | 14 (+0) | 6 (+0) | 0/0 | 2090 (+0) | 664 (+0) | 0 | - | PASS |
| sub0_b3_sink (vs handwritten_erased) | ok | 20.0 (-1.0) | 29 (+0) | 14 (+0) | 16 (+2) | 1/0 | 2263 (+5) | 712 (+0) | 0 | - | PASS |
| sub0_dynamic_domain | ok | 69.0 (+60.0) | 58 (+41) | 112 (+98) | 53 (+47) | 0/2 | 5277 (+3187) | 1096 (+432) | 181 | - | FAIL: publish instr, setup instr, teardown instr, publish path, no extra indirect calls, no extra RAM, no Sub0Pub retained |

<details><summary>clang-O2: largest symbols added by handwritten_erased (bytes)</summary>

- 101 `collapse_setup`
- 59 `(anonymous namespace)::deliverA(void const*, (anonymous namespace)::Sample const&)`
- 29 `(anonymous namespace)::deliverB(void const*, (anonymous namespace)::Sample const&)`
- 16 `(anonymous namespace)::nodeA`
- 8 `_ZN12_GLOBAL__N_17sensorBE.0`
- 8 `_ZN12_GLOBAL__N_17sensorAE.0`
- 8 `(anonymous namespace)::nodeB`
- 4 `(anonymous namespace)::loggerA`

</details>

<details><summary>clang-O2: largest symbols added by handwritten_gateway (bytes)</summary>

- 100 `collapse_publish`

</details>

<details><summary>clang-O2: largest symbols added by handwritten_runtime (bytes)</summary>

- 42 `collapse_setup`
- 8 `(anonymous namespace)::sensorB`
- 4 `(anonymous namespace)::controllerB`

</details>

<details><summary>clang-O2: largest symbols added by sub0_b1_one_publisher (bytes)</summary>

- 73 `collapse_setup`
- 24 `(anonymous namespace)::gateway`
- 4 `(anonymous namespace)::loggerA`
- 4 `(anonymous namespace)::controllerB`
- 4 `(anonymous namespace)::controllerA`

</details>

<details><summary>clang-O2: largest symbols added by sub0_b2_one_publisher (bytes)</summary>

- 100 `collapse_publish`

</details>

<details><summary>clang-O2: largest symbols added by sub0_b3_sink (bytes)</summary>

- 59 `_ZZN4sub04SinkIN12_GLOBAL__N_16SampleEEC1INS_6WiringIJNS1_10ControllerENS1_6LoggerEEEETnNSt9enable_ifIXntsr3stdE9is_same_vINSt9remove_cvIT_E4typeES3_EEiE4typeELi0EEERSB_ENUlPKvRKS2_E_8__invokeESI_SK_`
- 29 `_ZZN4sub04SinkIN12_GLOBAL__N_16SampleEEC1INS_6WiringIJNS1_10ControllerEEEETnNSt9enable_ifIXntsr3stdE9is_same_vINSt9remove_cvIT_E4typeES3_EEiE4typeELi0EEERSA_ENUlPKvRKS2_E_8__invokeESH_SJ_`
- 16 `(anonymous namespace)::busA`
- 8 `(anonymous namespace)::busB`

</details>

<details><summary>clang-O2: largest symbols added by sub0_dynamic_domain (bytes)</summary>

- 1603 `collapse_teardown`
- 306 `collapse_setup`
- 185 `collapse_publish`
- 80 `(anonymous namespace)::domainB`
- 80 `(anonymous namespace)::domainA`
- 65 `typeinfo name for sub0::detail::SubscriberInterface<(anonymous namespace)::Sample, false>`
- 43 `typeinfo name for sub0::Subscribe<(anonymous namespace)::Sample>`
- 32 `vtable for sub0::Subscribe<(anonymous namespace)::Sample>`

</details>

### cm33-gcc-Os, observable work

| variant | checksum | publish instr | setup instr | teardown instr | path instr | calls (direct/indirect) | text | data+bss | retained sub0 (B) | added deps | verdict |
|---|---|---|---|---|---|---|---|---|---|---|---|
| handwritten | - | - | - | - | 30 (+0) | 0/0 | 1188 (+0) | 520 (+0) | 0 | - | reference |
| handwritten_erased | - | - | - | - | 27 (-3) | 0/2 | 1300 (+112) | 548 (+28) | 0 | - | reference; FAIL: no extra indirect calls, no extra RAM |
| handwritten_gateway | - | - | - | - | 32 (+2) | 0/0 | 1200 (+12) | 532 (+12) | 0 | - | reference; FAIL: no extra RAM |
| handwritten_runtime | - | - | - | - | 37 (+7) | 0/0 | 1220 (+32) | 532 (+12) | 0 | - | reference; FAIL: publish path, no extra RAM |
| sub0_b1_mixin (vs handwritten_runtime) | - | - | - | - | 37 (+0) | 0/0 | 1220 (+0) | 532 (+0) | 0 | - | PASS |
| sub0_b1_one_publisher (vs handwritten_gateway) | - | - | - | - | 32 (+0) | 0/0 | 1200 (+0) | 532 (+0) | 0 | - | PASS |
| sub0_b1_wire (vs handwritten_runtime) | - | - | - | - | 37 (+0) | 0/0 | 1220 (+0) | 532 (+0) | 0 | - | PASS |
| sub0_b2_one_publisher | - | - | - | - | 30 (+0) | 0/0 | 1188 (+0) | 520 (+0) | 0 | - | PASS |
| sub0_b2_static | - | - | - | - | 30 (+0) | 0/0 | 1188 (+0) | 520 (+0) | 0 | - | PASS |
| sub0_b3_sink (vs handwritten_erased) | - | - | - | - | 27 (+0) | 0/2 | 1300 (+0) | 548 (+0) | 80 | - | PASS |
| sub0_dynamic_domain | - | - | - | - | 39 (+9) | 2/1 | 3420 (+2232) | 748 (+228) | 324 | - | FAIL: publish path, no extra indirect calls, no extra RAM, no Sub0Pub retained |

### cm33-gcc-Os, removable work

| variant | checksum | publish instr | setup instr | teardown instr | path instr | calls (direct/indirect) | text | data+bss | retained sub0 (B) | added deps | verdict |
|---|---|---|---|---|---|---|---|---|---|---|---|
| handwritten | - | - | - | - | 12 (+0) | 0/0 | 1136 (+0) | 520 (+0) | 0 | - | reference |
| handwritten_erased | - | - | - | - | 27 (+15) | 0/2 | 1232 (+96) | 548 (+28) | 0 | - | reference; FAIL: publish path, no extra indirect calls, no extra RAM |
| handwritten_gateway | - | - | - | - | 12 (+0) | 0/0 | 1148 (+12) | 532 (+12) | 0 | - | reference; FAIL: no extra RAM |
| handwritten_runtime | - | - | - | - | 16 (+4) | 0/0 | 1164 (+28) | 532 (+12) | 0 | - | reference; FAIL: publish path, no extra RAM |
| sub0_b1_mixin (vs handwritten_runtime) | - | - | - | - | 16 (+0) | 0/0 | 1164 (+0) | 532 (+0) | 0 | - | PASS |
| sub0_b1_one_publisher (vs handwritten_gateway) | - | - | - | - | 12 (+0) | 0/0 | 1148 (+0) | 532 (+0) | 0 | - | PASS |
| sub0_b1_wire (vs handwritten_runtime) | - | - | - | - | 16 (+0) | 0/0 | 1164 (+0) | 532 (+0) | 0 | - | PASS |
| sub0_b2_one_publisher | - | - | - | - | 12 (+0) | 0/0 | 1136 (+0) | 520 (+0) | 0 | - | PASS |
| sub0_b2_static | - | - | - | - | 12 (+0) | 0/0 | 1136 (+0) | 520 (+0) | 0 | - | PASS |
| sub0_b3_sink (vs handwritten_erased) | - | - | - | - | 27 (+0) | 0/2 | 1232 (+0) | 548 (+0) | 12 | - | PASS |
| sub0_dynamic_domain | - | - | - | - | 39 (+27) | 2/1 | 3376 (+2240) | 748 (+228) | 324 | - | FAIL: publish path, no extra indirect calls, no extra RAM, no Sub0Pub retained |

<details><summary>cm33-gcc-Os: largest symbols added by handwritten_erased (bytes)</summary>

- 84 `collapse_setup`
- 52 `(anonymous namespace)::deliverA(void const*, (anonymous namespace)::Sample const&)`
- 28 `(anonymous namespace)::deliverB(void const*, (anonymous namespace)::Sample const&)`
- 8 `(anonymous namespace)::sensorB`
- 8 `(anonymous namespace)::sensorA`
- 8 `(anonymous namespace)::nodeA`
- 4 `(anonymous namespace)::nodeB`

</details>

<details><summary>cm33-gcc-Os: largest symbols added by handwritten_gateway (bytes)</summary>

- 44 `collapse_setup`
- 12 `(anonymous namespace)::gateway`

</details>

<details><summary>cm33-gcc-Os: largest symbols added by handwritten_runtime (bytes)</summary>

- 92 `collapse_publish`
- 52 `collapse_setup`
- 8 `(anonymous namespace)::sensorA`
- 4 `(anonymous namespace)::sensorB`

</details>

<details><summary>cm33-gcc-Os: largest symbols added by sub0_b3_sink (bytes)</summary>

- 52 `sub0::Sink<(anonymous namespace)::Sample>::Sink<sub0::Wiring<(anonymous namespace)::Controller, (anonymous namespace)::Logger>, 0>(sub0::Wiring<(anonymous namespace)::Controller, (anonymous namespace)::Logger>&)::{lambda(void const*, (anonymous namespace)::Sample const&)#1}::_FUN(void const*, (anonymous namespace)::Sample const&)`
- 28 `sub0::Sink<(anonymous namespace)::Sample>::Sink<sub0::Wiring<(anonymous namespace)::Controller>, 0>(sub0::Wiring<(anonymous namespace)::Controller>&)::{lambda(void const*, (anonymous namespace)::Sample const&)#1}::_FUN(void const*, (anonymous namespace)::Sample const&)`
- 8 `(anonymous namespace)::busA`
- 4 `(anonymous namespace)::busB`

</details>

<details><summary>cm33-gcc-Os: largest symbols added by sub0_dynamic_domain (bytes)</summary>

- 256 `_malloc_r`
- 254 `memmove`
- 236 `memcpy`
- 144 `collapse_setup`
- 100 `__sigtramp`
- 96 `collapse_teardown`
- 96 `__sigtramp_r`
- 84 `sub0::Subscribe<(anonymous namespace)::Sample>::~Subscribe() [clone .isra.0]`

</details>

## Case: zero_receivers

### gcc-O2, observable work

| variant | checksum | publish instr | setup instr | teardown instr | path instr | calls (direct/indirect) | text | data+bss | retained sub0 (B) | added deps | verdict |
|---|---|---|---|---|---|---|---|---|---|---|---|
| handwritten | ok | 8.0 (+0.0) | 18 (+0) | 16 (+0) | 4 (+0) | 0/0 | 2319 (+0) | 616 (+0) | 0 | - | reference |
| handwritten_erased | ok | 22.0 (+14.0) | 22 (+4) | 16 (+0) | 17 (+13) | 1/1 | 2467 (+148) | 640 (+24) | 0 | - | reference; FAIL: publish instr, setup instr, publish path, no extra indirect calls, no extra RAM |
| sub0_b1_mixin | ok | 8.0 (+0.0) | 18 (+0) | 16 (+0) | 4 (+0) | 0/0 | 2319 (+0) | 616 (+0) | 0 | - | PASS |
| sub0_b1_wire | ok | 8.0 (+0.0) | 18 (+0) | 16 (+0) | 4 (+0) | 0/0 | 2319 (+0) | 616 (+0) | 0 | - | PASS |
| sub0_b2_static | ok | 8.0 (+0.0) | 18 (+0) | 16 (+0) | 4 (+0) | 0/0 | 2319 (+0) | 616 (+0) | 0 | - | PASS |
| sub0_b3_sink (vs handwritten_erased) | ok | 22.0 (+0.0) | 22 (+0) | 16 (+0) | 17 (+0) | 1/1 | 2467 (+0) | 640 (+0) | 5 | - | PASS |
| sub0pub_virtual | ok | 8.0 (+0.0) | 18 (+0) | 16 (+0) | 4 (+0) | 0/0 | 2319 (+0) | 616 (+0) | 0 | - | PASS |
| sub0pub_virtual_lean | ok | 8.0 (+0.0) | 18 (+0) | 16 (+0) | 4 (+0) | 0/0 | 2319 (+0) | 616 (+0) | 0 | - | PASS |

### gcc-O2, removable work

| variant | checksum | publish instr | setup instr | teardown instr | path instr | calls (direct/indirect) | text | data+bss | retained sub0 (B) | added deps | verdict |
|---|---|---|---|---|---|---|---|---|---|---|---|
| handwritten | ok | 8.0 (+0.0) | 18 (+0) | 16 (+0) | 4 (+0) | 0/0 | 2319 (+0) | 616 (+0) | 0 | - | reference |
| handwritten_erased | ok | 22.0 (+14.0) | 22 (+4) | 16 (+0) | 17 (+13) | 1/1 | 2467 (+148) | 640 (+24) | 0 | - | reference; FAIL: publish instr, setup instr, publish path, no extra indirect calls, no extra RAM |
| sub0_b1_mixin | ok | 8.0 (+0.0) | 18 (+0) | 16 (+0) | 4 (+0) | 0/0 | 2319 (+0) | 616 (+0) | 0 | - | PASS |
| sub0_b1_wire | ok | 8.0 (+0.0) | 18 (+0) | 16 (+0) | 4 (+0) | 0/0 | 2319 (+0) | 616 (+0) | 0 | - | PASS |
| sub0_b2_static | ok | 8.0 (+0.0) | 18 (+0) | 16 (+0) | 4 (+0) | 0/0 | 2319 (+0) | 616 (+0) | 0 | - | PASS |
| sub0_b3_sink (vs handwritten_erased) | ok | 22.0 (+0.0) | 22 (+0) | 16 (+0) | 17 (+0) | 1/1 | 2467 (+0) | 640 (+0) | 5 | - | PASS |
| sub0pub_virtual | ok | 8.0 (+0.0) | 18 (+0) | 16 (+0) | 4 (+0) | 0/0 | 2319 (+0) | 616 (+0) | 0 | - | PASS |
| sub0pub_virtual_lean | ok | 8.0 (+0.0) | 18 (+0) | 16 (+0) | 4 (+0) | 0/0 | 2319 (+0) | 616 (+0) | 0 | - | PASS |

<details><summary>gcc-O2: largest symbols added by handwritten_erased (bytes)</summary>

- 79 `collapse_publish`
- 33 `collapse_setup`
- 16 `(anonymous namespace)::sensor`
- 5 `(anonymous namespace)::deliverNode(void const*, (anonymous namespace)::Sample const&)`
- 1 `(anonymous namespace)::node`

</details>

<details><summary>gcc-O2: largest symbols added by sub0_b3_sink (bytes)</summary>

- 5 `sub0::Sink<(anonymous namespace)::Sample>::Sink<sub0::Wiring<>, 0>(sub0::Wiring<>&)::{lambda(void const*, (anonymous namespace)::Sample const&)#1}::_FUN(void const*, (anonymous namespace)::Sample const&)`
- 1 `(anonymous namespace)::bus`

</details>

### clang-O2, observable work

| variant | checksum | publish instr | setup instr | teardown instr | path instr | calls (direct/indirect) | text | data+bss | retained sub0 (B) | added deps | verdict |
|---|---|---|---|---|---|---|---|---|---|---|---|
| handwritten | ok | 8.0 (+0.0) | 16 (+0) | 14 (+0) | 4 (+0) | 0/0 | 2074 (+0) | 656 (+0) | 0 | - | reference |
| handwritten_erased | ok | 8.0 (+0.0) | 18 (+2) | 14 (+0) | 4 (+0) | 0/0 | 2074 (+0) | 672 (+16) | 0 | - | reference; FAIL: setup instr, no extra RAM |
| sub0_b1_mixin | ok | 8.0 (+0.0) | 16 (+0) | 14 (+0) | 4 (+0) | 0/0 | 2074 (+0) | 656 (+0) | 0 | - | PASS |
| sub0_b1_wire | ok | 8.0 (+0.0) | 16 (+0) | 14 (+0) | 4 (+0) | 0/0 | 2074 (+0) | 656 (+0) | 0 | - | PASS |
| sub0_b2_static | ok | 8.0 (+0.0) | 16 (+0) | 14 (+0) | 4 (+0) | 0/0 | 2074 (+0) | 656 (+0) | 0 | - | PASS |
| sub0_b3_sink (vs handwritten_erased) | ok | 8.0 (+0.0) | 18 (+0) | 14 (+0) | 4 (+0) | 0/0 | 2074 (+0) | 672 (+0) | 0 | - | PASS |
| sub0pub_virtual | ok | 8.0 (+0.0) | 16 (+0) | 14 (+0) | 4 (+0) | 0/0 | 2074 (+0) | 656 (+0) | 0 | - | PASS |
| sub0pub_virtual_lean | ok | 8.0 (+0.0) | 16 (+0) | 14 (+0) | 4 (+0) | 0/0 | 2074 (+0) | 656 (+0) | 0 | - | PASS |

### clang-O2, removable work

| variant | checksum | publish instr | setup instr | teardown instr | path instr | calls (direct/indirect) | text | data+bss | retained sub0 (B) | added deps | verdict |
|---|---|---|---|---|---|---|---|---|---|---|---|
| handwritten | ok | 8.0 (+0.0) | 16 (+0) | 14 (+0) | 4 (+0) | 0/0 | 2074 (+0) | 656 (+0) | 0 | - | reference |
| handwritten_erased | ok | 8.0 (+0.0) | 18 (+2) | 14 (+0) | 4 (+0) | 0/0 | 2074 (+0) | 672 (+16) | 0 | - | reference; FAIL: setup instr, no extra RAM |
| sub0_b1_mixin | ok | 8.0 (+0.0) | 16 (+0) | 14 (+0) | 4 (+0) | 0/0 | 2074 (+0) | 656 (+0) | 0 | - | PASS |
| sub0_b1_wire | ok | 8.0 (+0.0) | 16 (+0) | 14 (+0) | 4 (+0) | 0/0 | 2074 (+0) | 656 (+0) | 0 | - | PASS |
| sub0_b2_static | ok | 8.0 (+0.0) | 16 (+0) | 14 (+0) | 4 (+0) | 0/0 | 2074 (+0) | 656 (+0) | 0 | - | PASS |
| sub0_b3_sink (vs handwritten_erased) | ok | 8.0 (+0.0) | 18 (+0) | 14 (+0) | 4 (+0) | 0/0 | 2074 (+0) | 672 (+0) | 0 | - | PASS |
| sub0pub_virtual | ok | 8.0 (+0.0) | 16 (+0) | 14 (+0) | 4 (+0) | 0/0 | 2074 (+0) | 656 (+0) | 0 | - | PASS |
| sub0pub_virtual_lean | ok | 8.0 (+0.0) | 16 (+0) | 14 (+0) | 4 (+0) | 0/0 | 2074 (+0) | 656 (+0) | 0 | - | PASS |

<details><summary>clang-O2: largest symbols added by handwritten_erased (bytes)</summary>

- 15 `collapse_setup`
- 8 `_ZN12_GLOBAL__N_16sensorE.0`
- 1 `(anonymous namespace)::node`

</details>

<details><summary>clang-O2: largest symbols added by sub0_b3_sink (bytes)</summary>

- 1 `(anonymous namespace)::bus`

</details>

### cm33-gcc-Os, observable work

| variant | checksum | publish instr | setup instr | teardown instr | path instr | calls (direct/indirect) | text | data+bss | retained sub0 (B) | added deps | verdict |
|---|---|---|---|---|---|---|---|---|---|---|---|
| handwritten | - | - | - | - | 7 (+0) | 0/0 | 1096 (+0) | 508 (+0) | 0 | - | reference |
| handwritten_erased | - | - | - | - | 15 (+8) | 0/1 | 1140 (+44) | 520 (+12) | 0 | - | reference; FAIL: publish path, no extra indirect calls, no extra RAM |
| sub0_b1_mixin | - | - | - | - | 7 (+0) | 0/0 | 1096 (+0) | 508 (+0) | 0 | - | PASS |
| sub0_b1_wire | - | - | - | - | 7 (+0) | 0/0 | 1096 (+0) | 508 (+0) | 0 | - | PASS |
| sub0_b2_static | - | - | - | - | 7 (+0) | 0/0 | 1096 (+0) | 508 (+0) | 0 | - | PASS |
| sub0_b3_sink (vs handwritten_erased) | - | - | - | - | 15 (+0) | 0/1 | 1140 (+0) | 520 (+0) | 2 | - | PASS |
| sub0pub_virtual | - | - | - | - | 7 (+0) | 0/0 | 1096 (+0) | 508 (+0) | 0 | - | PASS |
| sub0pub_virtual_lean | - | - | - | - | 7 (+0) | 0/0 | 1096 (+0) | 508 (+0) | 0 | - | PASS |

### cm33-gcc-Os, removable work

| variant | checksum | publish instr | setup instr | teardown instr | path instr | calls (direct/indirect) | text | data+bss | retained sub0 (B) | added deps | verdict |
|---|---|---|---|---|---|---|---|---|---|---|---|
| handwritten | - | - | - | - | 7 (+0) | 0/0 | 1096 (+0) | 508 (+0) | 0 | - | reference |
| handwritten_erased | - | - | - | - | 15 (+8) | 0/1 | 1140 (+44) | 520 (+12) | 0 | - | reference; FAIL: publish path, no extra indirect calls, no extra RAM |
| sub0_b1_mixin | - | - | - | - | 7 (+0) | 0/0 | 1096 (+0) | 508 (+0) | 0 | - | PASS |
| sub0_b1_wire | - | - | - | - | 7 (+0) | 0/0 | 1096 (+0) | 508 (+0) | 0 | - | PASS |
| sub0_b2_static | - | - | - | - | 7 (+0) | 0/0 | 1096 (+0) | 508 (+0) | 0 | - | PASS |
| sub0_b3_sink (vs handwritten_erased) | - | - | - | - | 15 (+0) | 0/1 | 1140 (+0) | 520 (+0) | 2 | - | PASS |
| sub0pub_virtual | - | - | - | - | 7 (+0) | 0/0 | 1096 (+0) | 508 (+0) | 0 | - | PASS |
| sub0pub_virtual_lean | - | - | - | - | 7 (+0) | 0/0 | 1096 (+0) | 508 (+0) | 0 | - | PASS |

<details><summary>cm33-gcc-Os: largest symbols added by handwritten_erased (bytes)</summary>

- 36 `collapse_publish`
- 24 `collapse_setup`
- 8 `(anonymous namespace)::sensor`
- 2 `(anonymous namespace)::deliverNode(void const*, (anonymous namespace)::Sample const&)`
- 1 `(anonymous namespace)::node`

</details>

<details><summary>cm33-gcc-Os: largest symbols added by sub0_b3_sink (bytes)</summary>

- 2 `sub0::Sink<(anonymous namespace)::Sample>::Sink<sub0::Wiring<>, 0>(sub0::Wiring<>&)::{lambda(void const*, (anonymous namespace)::Sample const&)#1}::_FUN(void const*, (anonymous namespace)::Sample const&)`
- 1 `(anonymous namespace)::bus`

</details>

**Regression gate:** 510 public-API measurements against `tests/collapse/budgets.json`: all within budget

