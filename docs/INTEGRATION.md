# Integration with Sub0Pipeline

Sub0Pub and [Sub0Pipeline](https://github.com/CraigHutchinson/Sub0Pipeline) share the C++23 baseline, so a consumer
can use both without a language-mode mismatch. Neither library depends on the other, and no adapter exists yet: this
document records the integration boundary and the open ownership decision, so an adapter can be designed against them.
It was reviewed against Sub0Pipeline `f6f54c623908649e8daac3613545062cf08b3822`
([public interfaces](https://github.com/CraigHutchinson/Sub0Pipeline/blob/f6f54c623908649e8daac3613545062cf08b3822/include/sub0pipeline/sub0pipeline.hpp),
[implementation](https://github.com/CraigHutchinson/Sub0Pipeline/blob/f6f54c623908649e8daac3613545062cf08b3822/src/sub0pipeline.cpp)).

## Boundary

Start with typed lifecycle output: an `IObserver` implementation publishes Pipeline events to a supplied wiring or
`Sink<T>`. Running workflows from messages needs a distinct, bounded admission and ownership contract: an ordinary
publication returns no admission result and does not transfer payload ownership. Constraints at the reviewed revision:

- `trigger()` dispatches one on-demand job, not its successor DAG; duplicate active submissions can return `kBusy`.
- On-demand execution emits start and finish events but not the DAG failure-detail hook.
- Observer callbacks may overlap, and names are borrowed views rather than stable per-invocation IDs.
- Terminal status can precede safe reclamation of timed-out work; completion and orphan joining must stay explicit.
- Sub0Pub cancellation stops a publication; Pipeline cancellation requests that work stop. They are different contracts.
- `IExecutor::dispatch` must eventually invoke completion. A publication with optional or multiple recipients cannot
  serve as a general executor without a separate admission and exactly-once completion protocol.

## Where an adapter should live

| Home | Advantages | Costs, and when it fits |
|---|---|---|
| Sub0Pub extension | Discoverable to messaging users; next to the wiring | Scheduler lifecycle maintenance in the messaging project; the optional dependency must be isolated |
| Sub0Pipeline extension | Observer semantics and tests nearby; no new release process | Adapter releases follow Pipeline; Sub0Pub links to it |
| Dedicated integration package | Neutral ownership, independent bridge releases | A third package and a compatibility matrix; worth it when the bridge is substantial |
| Umbrella project | One entry point, tested version combinations, adapters and examples | Broader support responsibility; justified by a reusable toolkit, not a few forwarding functions |

For a narrow lifecycle adapter, Sub0Pipeline is the preferred first home. If the scope becomes a reusable composition
toolkit, an umbrella-owned component is a credible alternative; its dependencies point at both libraries and neither
core depends on it. Whichever home is chosen, keep one canonical bridge and one contract suite, with optional
component targets, pinned compatibility tests and installed-package checks. Do not impose a global bus, executor,
allocator or automatic dependency download.

## Acceptance gates for an adapter

Settle the owner and the first use case first. Then validate payload lifetime, concurrent observation, overload
reporting, wake-up races and teardown with controlled executors, and compare against equal-work hand-written glue with
the same ownership and queue policy ([EVIDENCE.md](EVIDENCE.md)). Bounded ingress must not be described as heap-free or
ISR-safe execution: those guarantees need separate scheduler and platform work.
