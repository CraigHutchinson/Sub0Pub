# VTune A/B: case `station`, build `msvc-O2`

`Intel(R) VTune(TM) Profiler 2026.4.0 (build 632893) Command Line Tool`, hotspots collection, sampling mode `sw` (`tools/vtune_ab.py`). Each variant publishes in a loop for about 3 s. `Sub0Pub share` is the part of the executable's CPU time that VTune attributes to functions of namespace `sub0`; application code inlined into such a function counts towards it. A collapsed delivery has no such function left to sample.

| variant | ns / publication | CPU time in image (s) | Sub0Pub share | where the time is |
|---|---:|---:|---:|---|
| handwritten | 2.93 | 3.19 | 0% | `Logger::receive` 36%; `collapse::work` 34%; `collapse_publish` 30% |
| today_static | 2.80 | 2.84 | 0% | `collapse::work` 37%; `Logger::receive` 36%; `collapse_publish` 15%; `Sensor<sub0::StaticWiring<&controller,&A0x13ad3cb1::logger,&A0x13ad3cb` 12% |
| route_static | 2.79 | 3.12 | 0% | `collapse::work` 42%; `Logger::receive` 31%; `collapse_publish` 15%; `Sensor::send` 12% |
| today_dynamic | 8.34 | 2.87 | 6% | `collapse::work` 46%; `Logger::receive` 24%; `Controller::receive` 10%; `Actuator::receive` 5% |
| route_dynamic | 7.77 | 3.13 | 5% | `collapse::work` 48%; `Logger::receive` 23%; `Controller::receive` 10%; `Actuator::receive` 7% |
| bus_dynamic | 7.81 | 3.11 | 30% | `collapse::work` 48%; `Logger::receive` 15%; `sub0::spike::detail::Forwarding<sub0::spike::Subscription<Logger,A0x16` 11%; `sub0::spike::detail::Forwarding<sub0::spike::Subscription<Controller,A` 9% |

