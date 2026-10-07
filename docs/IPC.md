# IPC and transports

Sub0Pub provides synchronous message forwarding and stream serialization. It does not provide a network stack,
operating-system pipe, queue, scheduler, retries, or delivery guarantees. The application supplies and owns the
transport.

## Transport bridges

- `Forward<Transport>` and `StaticForward<&transport>` attach a transport endpoint to an explicit wiring
  ([usage](USAGE.md#explicit-wiring-when-the-type-cannot-decide)).
- `Route<T, Transport>` attaches a transport to the runtime broker and can report the transport's `SendResult` in a
  `PublishReport`.
- `publishFrom(transport, message)` and the corresponding route injection path provide split-horizon behavior: a
  message received from an endpoint is not immediately sent back to that endpoint. This does not prevent arbitrary
  network cycles.
- `Forward` does not collect send results. A route's reported acceptance is not proof of remote delivery.

See [link forwarding](../examples/link_forwarding.cpp), [static link forwarding](../examples/static_link_forwarding.cpp),
and [route reports](../examples/route_reports.cpp).

## Stream serialization

`StreamSerializer` observes selected published message types and writes them to an application-supplied
`OStream`. `StreamDeserializer` reads framed messages from an `IStream` and republishes them locally. The
[IPC pipe example](../examples/ipc_pipe/main.cpp) demonstrates a complete in-memory round trip.

The default binary format transfers each message's in-memory representation. Peers must agree on byte order, type
identity, and layout. This is not a portable schema format: it does not convert endianness or make arbitrary C++
objects safe to serialize. The application must validate compatibility and provide any conversions required by its
deployment.

`makeLayout<T>()` reports a type's size, alignment, arity, and available member-layout details. It helps compare
builds, but does not itself guarantee compatibility. See [layout checking](../examples/layout_check/main.cpp) and
[design limitations](DESIGN.md#known-limitations).

For API design and comparison notes around serialization and bridges, see
[the integration guide](INTEGRATION.md) and [comparisons](COMPARISONS.md).
