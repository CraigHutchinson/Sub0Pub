/** Sub0Pub: Types<...>, a list of message types, and general-purpose helper types shared across the library
 * @remark Part of Sub0Pub (https://github.com/CraigHutchinson/Sub0Pub), MIT License: see LICENSE.md.
 *         Included by the umbrella header <sub0pub/sub0pub.hpp>.
 */
#ifndef CROG_SUB0PUB_TYPES_HPP
#define CROG_SUB0PUB_TYPES_HPP

#include "sub0pub/config_macros.hpp"
#include <cstddef>
#include <type_traits>

namespace sub0
{
    /** A list of message types, kept under one name, for the forms that take several of them
     *
     *     using StationMessages = sub0::Types<Reading, Fault>;
     *     class Logger : public sub0::SubscribeAll<StationMessages> { ... };
     *
     * SubscribeAll, ForwardSubscribeAll and ForwardPublishAll expand a Types argument in place, so a list can be
     * named once and used, or combined with more types or lists, wherever the set is needed. It is not a message
     * type itself, and it is the only list: a std::tuple argument is one message type, like any other.
     */
    template<class... Ts>
    struct Types
    {
        static constexpr std::size_t size = sizeof...(Ts);
    };

    namespace detail
    {
        struct Empty {};

        template<class T> struct IsTypes : std::false_type {};
        template<class... Ts> struct IsTypes<Types<Ts...>> : std::true_type {};

        /// Whether any of Ts is a Types list
        template<class... Ts>
        inline constexpr bool cAnyTypes = (IsTypes<Ts>::value || ...);

        template<class T, class List> struct Prepend;
        template<class T, class... Ts> struct Prepend<T, Types<Ts...>> { using type = Types<T, Ts...>; };

        /// The message types among Ts, with every Types list expanded in place (nested lists too)
        template<class... Ts> struct Flatten { using type = Types<>; };
        template<class T, class... Rest> struct Flatten<T, Rest...>
        { using type = typename Prepend<T, typename Flatten<Rest...>::type>::type; };
        template<class... Inner, class... Rest> struct Flatten<Types<Inner...>, Rest...>
        { using type = typename Flatten<Inner..., Rest...>::type; };

        /// Template<Prefix..., Ds...> for Types<Ds...>
        template<template<class...> class Template, class List, class... Prefix> struct Apply;
        template<template<class...> class Template, class... Ds, class... Prefix> struct Apply<Template, Types<Ds...>, Prefix...>
        { using type = Template<Prefix..., Ds...>; };
    } // END: detail
} // END: sub0

#endif // CROG_SUB0PUB_TYPES_HPP
