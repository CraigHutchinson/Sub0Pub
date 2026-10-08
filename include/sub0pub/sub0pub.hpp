/** Sub0Pub core header-only library
 * @remark C++ Type-based Subscriber-Publisher messaging model for embedded, desktop, games, and distributed systems.
 *
 *  This file is part of Sub0Pub. Original project source available at https://github.com/Crog/Sub0Pub/blob/master/sub0pub.hpp
 *
 *  MIT License
 *
 * Copyright (c) 2018 Craig Hutchinson <craig-sub0pub@crog.uk>
 *
 *  Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files
 *  (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge,
 *  publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do
 *  so, subject to the following conditions:
 *
 *  The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
 *
 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 *  MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE
 *  FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 *  WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */
#ifndef CROG_SUB0PUB_HPP
#define CROG_SUB0PUB_HPP

/** Sub0Pub: the umbrella header. Includes every part of the library; `#include <sub0pub/sub0pub.hpp>` is all most
 *  applications need. A translation unit that uses only one part may include that part's entry header instead:
 *
 *   sub0pub/config_macros.hpp  SUB0PUB_* configuration macros: every default, defined once
 *   sub0pub/types.hpp          general-purpose helper types
 *   sub0pub/utility/           hashing and type identity, layout fingerprinting, streams
 *   sub0pub/audit.hpp          the audit (SUB0PUB_AUDIT): what a run published and delivered, and its findings
 *   sub0pub/config.hpp         per-Data policy (capacity, dispatch, context, lock, filter, storage) and its resolution
 *   sub0pub/broker.hpp         the runtime broker: Subscribe, Publish, SubscribeAll, Domain, Route, publish(), cancel()
 *   sub0pub/wiring.hpp         explicit wiring: wire(), StaticWiring, Sink, Forward
 *   sub0pub/wiring/broker_port.hpp   BrokerPort: an explicit wiring to the runtime broker
 *   sub0pub/wiring/static_topology.hpp  StaticTo / StaticFirst: a Data type's configuration wired to fixed receivers
 *   sub0pub/ipc.hpp            IPC serialisation: StreamSerializer, StreamDeserializer, DefaultSerialisation
 *   sub0pub/ipc/forward.hpp    ForwardSubscribe/ForwardPublish: the runtime broker to IPC
 *
 *  Each area directory (utility/, broker/, wiring/, ipc/) holds one header per responsibility. SUB0PUB_* macros are
 *  read when sub0pub/config_macros.hpp is first included, so define them before the first Sub0Pub include (or on
 *  the compiler command line) whichever header that is.
 */

#include "sub0pub/config_macros.hpp"
#include "sub0pub/types.hpp"
#include "sub0pub/utility/hash.hpp"
#include "sub0pub/utility/layout.hpp"
#include "sub0pub/utility/streams.hpp"
#include "sub0pub/utility/type_info.hpp"
#include "sub0pub/audit.hpp"
#include "sub0pub/config.hpp"
#include "sub0pub/broker.hpp"
#include "sub0pub/wiring.hpp"
#include "sub0pub/wiring/broker_port.hpp"
#include "sub0pub/wiring/static_topology.hpp"
#include "sub0pub/ipc.hpp"
#include "sub0pub/ipc/forward.hpp"

#endif // CROG_SUB0PUB_HPP
