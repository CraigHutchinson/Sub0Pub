/** Sub0Pub: Per-Data configuration: policy vocabulary, the project header include point, resolution (config_t)
 * @remark Part of Sub0Pub (https://github.com/BareCpper/Sub0Pub), MIT License: see LICENSE.md.
 *         Included by the umbrella header <sub0pub/sub0pub.hpp>.
 */
#ifndef CROG_SUB0PUB_CONFIG_HPP
#define CROG_SUB0PUB_CONFIG_HPP

#include "sub0pub/config_macros.hpp"
#include <cstdint>
#include <type_traits>

#if SUB0PUB_THREAD_SAFE
#include <mutex>
#endif

namespace sub0
{
    /** Dispatch policy: how publish() walks the subscription table */
    enum class Dispatch : uint8_t
    {
        Snapshot,       ///< Copy the table before dispatch: re-entrant publish/subscribe/unsubscribe are safe
        Direct,         ///< Iterate the live table: fastest; re-entrant use of the same type's table is not supported
        DirectChecked   ///< Direct, and report re-entrant use through SUB0PUB_REENTRANT_VIOLATION (needs a context)
    };

    /** Publish context policy: per-dispatch state for cancel(), nesting, routes and same-thread disconnect */
    enum class Context : uint8_t
    {
        ThreadLocal,    ///< thread_local context: cancel() and nested publish on any thread
        Static,         ///< plain static context: as ThreadLocal for single-threaded images, without TLS
        None            ///< no context: no cancel(), no routes; the cheapest dispatch
    };

    /** Storage policy: where a type's subscription table lives */
    enum class Storage : uint8_t
    {
        Global,         ///< one table per Data type
        Scoped          ///< tables live in Domain<Data> instances passed at construction (independent sessions)
    };

    /** Lock policy for single-threaded use: an empty base, costs nothing */
    struct NoLock
    {
        void lock() noexcept {}
        void unlock() noexcept {}
    };

#if SUB0PUB_THREAD_SAFE
    /** Lock policy selected by SUB0PUB_THREAD_SAFE */
    struct StdMutexLock
    {
        void lock() noexcept { m.lock(); }
        void unlock() noexcept { m.unlock(); }
        std::mutex m;
    };
#endif

    namespace detail
    {
        template<class Data, class Config> class BrokerImpl; ///< the library broker (default implementation)

        /// Builtin configuration, named by the macro values it derives from: a translation unit that sets
        /// different SUB0PUB_* values for its own (TU-local) Data types gets a different type, not a second
        /// definition of the same one
        template<uint32_t Capacity, Dispatch D, Context C, bool Filter, class LockT>
        struct BuiltinT
        {
            /// Broker implementation (see Implementation<> and the broker concept on detail::BrokerImpl)
            template<class Data, class Config> using broker = BrokerImpl<Data, Config>;
            static constexpr uint32_t capacity = Capacity;
            static constexpr Dispatch dispatch = D;
            static constexpr Context context = C;
            static constexpr Storage storage = Storage::Global;
            static constexpr bool filter = Filter;
            using Lock = LockT;
        };
    }

    /** Builtin defaults: the configuration the SUB0PUB_* macros describe
     * Without any macro: Direct dispatch (DirectChecked in debug builds), no publish context, no filter(), no lock.
     */
    using Builtin = detail::BuiltinT<SUB0PUB_MAX_SUBSCRIPTIONS,
        (SUB0PUB_REENTRANT_SAFE || SUB0PUB_THREAD_SAFE) ? Dispatch::Snapshot
            : (SUB0PUB_REENTRANT_CHECK ? Dispatch::DirectChecked : Dispatch::Direct),
        (SUB0PUB_REENTRANT_SAFE || SUB0PUB_THREAD_SAFE || SUB0PUB_CANCEL) ? Context::ThreadLocal : Context::None,
        SUB0PUB_FILTER,
#if SUB0PUB_THREAD_SAFE
        StdMutexLock
#else
        NoLock
#endif
        >;

    // Options: each applies itself on top of a base configuration -----------------------------------------

    /// Fixed subscription table size
    template<uint32_t N> struct Capacity
    { template<class B> struct apply : B { static constexpr uint32_t capacity = N; }; };

    template<Dispatch D> struct DispatchWith
    { template<class B> struct apply : B { static constexpr Dispatch dispatch = D; }; };

    /// Snapshot dispatch: subscribe/unsubscribe a type from inside its own receive(). A snapshot needs a publish
    /// context, so this also selects ThreadLocalContext when the base has none (StaticContext after it selects that
    /// instead; NoContext after it is a compile error).
    struct Snapshot
    {
        template<class B> struct apply : B
        {
            static constexpr Dispatch dispatch = Dispatch::Snapshot;
            static constexpr Context context = B::context == Context::None ? Context::ThreadLocal : B::context;
        };
    };
    using Direct = DispatchWith<Dispatch::Direct>;
    using DirectChecked = DispatchWith<Dispatch::DirectChecked>;

    template<Context C> struct ContextWith
    { template<class B> struct apply : B { static constexpr Context context = C; }; };
    using ThreadLocalContext = ContextWith<Context::ThreadLocal>;
    using StaticContext = ContextWith<Context::Static>;
    using NoContext = ContextWith<Context::None>;

    /// Any type with lock()/unlock() (and optionally a static yield()) makes the type safe for concurrent use.
    /// Concurrent use requires Snapshot dispatch and ThreadLocalContext, so this selects them too (a later option
    /// that contradicts them is a compile error).
    template<class L> struct LockWith
    {
        template<class B> struct apply : B
        {
            using Lock = L;
            static constexpr Dispatch dispatch = Dispatch::Snapshot;
            static constexpr Context context = Context::ThreadLocal;
        };
    };

    /// Subscribe<Data>::filter(): subscribers may skip a message (a virtual call per subscriber per publish)
    struct Filter
    { template<class B> struct apply : B { static constexpr bool filter = true; }; };

    /// No filter(): no per-subscriber filter call; a subscriber declaring filter() is then a compile error
    struct NoFilter
    { template<class B> struct apply : B { static constexpr bool filter = false; }; };

    /// Tables live in Domain<Data> instances passed to Subscribe/Publish at construction
    struct Scoped
    { template<class B> struct apply : B { static constexpr Storage storage = Storage::Scoped; }; };

    /// Replace the broker implementation for a Data type with an application-defined one (Global storage)
    template<template<class, class> class BrokerTemplate> struct Implementation
    { template<class B> struct apply : B { template<class Data, class Config> using broker = BrokerTemplate<Data, Config>; }; };

    namespace detail
    {
        template<class Base, class... Opts> struct fold { using type = Base; };
        template<class Base, class O, class... Rest>
        struct fold<Base, O, Rest...> : fold<typename O::template apply<Base>, Rest...> {};
    }

    /// Base configuration with options applied left to right (later options win)
    template<class Base, class... Opts>
    struct with : detail::fold<Base, Opts...>::type {};
} // END: sub0

/** Project default configuration (resolution step 2)
 * Name a header with SUB0PUB_CONFIG_HEADER from the build system, so every translation unit agrees. It may
 * define e.g. `struct ProjectDefaults : sub0::with<sub0::Builtin, sub0::NoFilter> {};` and
 * `#define SUB0PUB_DEFAULT_CONFIG ProjectDefaults`.
 */
#if defined(SUB0PUB_CONFIG_HEADER)
#include SUB0PUB_CONFIG_HEADER
#endif

namespace sub0
{
#if defined(SUB0PUB_DEFAULT_CONFIG)
    using GlobalDefault = SUB0PUB_DEFAULT_CONFIG;
#else
    using GlobalDefault = Builtin;
#endif

    /** Per-Data configuration: the project default with options applied. The usual spelling at a Data site:
     *  `struct Imu { ...; using sub0_config = sub0::config<sub0::Capacity<2>, sub0::NoFilter>; };`
     */
    /// An alias, not a class: `config<Opts...>` names a different type wherever the base (the project default or the
    /// macro-derived Builtin) differs, so translation units with different SUB0PUB_* values never share its definition
    template<class... Opts>
    using config = with<GlobalDefault, Opts...>;

    /** Traits hook (resolution step 1c) for types that cannot carry a member alias or ADL declaration
     * @see SUB0PUB_CONFIGURE
     */
    template<class Data> struct configure {};

    namespace detail
    {
        /// Poison pill: ordinary lookup only ever finds this deleted template, so sub0_config is an ADL-only
        /// customisation point. A user's non-template sub0_config(T*) wins overload resolution against it.
        template<class T> void sub0_config(T*) = delete;

        /// Overload-based detection (portable to MSVC, which mishandles ADL inside void_t partial specialisations)
        template<class T> auto adl_probe(int) -> decltype(sub0_config(static_cast<T*>(nullptr)))*;
        template<class T> void adl_probe(...);

        template<class T, class = void> struct member_config { static constexpr bool found = false; };
        template<class T> struct member_config<T, std::void_t<typename T::sub0_config>>
        { static constexpr bool found = true; using type = typename T::sub0_config; };

        template<class T>
        struct adl_config
        {
            using probed = decltype(adl_probe<T>(0));
            static constexpr bool found = !std::is_void_v<probed>;
            using type = std::remove_pointer_t<probed>;
        };

        template<class T, class = void> struct traits_config { static constexpr bool found = false; };
        template<class T> struct traits_config<T, std::void_t<typename configure<T>::type>>
        { static constexpr bool found = true; using type = typename configure<T>::type; };

        template<class Data>
        struct resolve
        {
            static_assert(!std::is_reference_v<Data> && !std::is_const_v<Data>, "sub0pub: Data must be an unqualified object type");
            static constexpr int count = int(member_config<Data>::found) + int(adl_config<Data>::found) + int(traits_config<Data>::found);
            static_assert(count <= 1, "sub0pub: a Data type must be configured in exactly one place "
                                      "(member sub0_config, ADL sub0_config(Data*), or sub0::configure<Data>)");
            using type = std::conditional_t<member_config<Data>::found, member_config<Data>,
                         std::conditional_t<adl_config<Data>::found, adl_config<Data>,
                         std::conditional_t<traits_config<Data>::found, traits_config<Data>,
                         std::enable_if<true, GlobalDefault>>>>;
        };
    }

    /** The configuration every use of Data resolves to
     * Resolution: (1) exactly one of a member alias `Data::sub0_config`, an ADL declaration `sub0_config(Data*)`
     * or `sub0::configure<Data>` (SUB0PUB_CONFIGURE); else (2) SUB0PUB_DEFAULT_CONFIG; else (3) Builtin.
     * @warning Every translation unit must resolve the same configuration for a Data type: Subscribe<Data> depends on
     *          it, so resolving differently is an ODR violation. Member and ADL configuration are part of the type's
     *          definition and agree by construction; traits and the project header must be visible everywhere.
     */
    template<class Data>
    using config_t = typename detail::resolve<Data>::type::type;

    template<class Data> class Subscribe;
    template<class Data> class Publish;
    template<class Data> class Domain;
} // END: sub0

/** Configure a Data type you cannot modify (resolution step 1c). Use at global namespace scope, next to the type:
 *  `SUB0PUB_CONFIGURE(int, sub0::Capacity<32>);`
 */
#define SUB0PUB_CONFIGURE(Type, ...) \
    template<> struct sub0::configure<Type> { using type = sub0::config<__VA_ARGS__>; }

#endif // CROG_SUB0PUB_CONFIG_HPP
