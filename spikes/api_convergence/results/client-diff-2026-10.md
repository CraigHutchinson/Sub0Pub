# Client-code cost of switching delivery mode

Lines of code that differ between a candidate's `dynamic` build and each other build of the same station (`tools/client_diff.py`). Shared lines are the sources every build of that candidate compiles unchanged. A participant edit means the publisher or receiver classes themselves had to change.

| candidate | switch | lines removed | lines added | files touched | participant lines edited | shared lines |
|---|---|---:|---:|---|---:|---:|
| bus_parameter | dynamic -> static | 7 | 5 | compose.hpp | 0 | 46 |
| bus_parameter | dynamic -> wired | 6 | 3 | compose.hpp | 0 | 46 |
| port_member | dynamic -> static | 5 | 1 | compose.hpp | 0 | 48 |
| status_quo | dynamic -> static | 12 | 17 | compose.hpp, participants.hpp | 20 | 7 |
| typed_route | dynamic -> bridged | 0 | 4 | station_types.hpp | 0 | 52 |
| typed_route | dynamic -> foreign | 9 | 9 | station_types.hpp | 0 | 52 |
| typed_route | dynamic -> hot_path | 0 | 7 | station_types.hpp | 0 | 52 |
| typed_route | dynamic -> report | 1 | 3 | station_types.hpp | 0 | 52 |
| typed_route | dynamic -> static | 0 | 8 | station_types.hpp | 0 | 52 |
