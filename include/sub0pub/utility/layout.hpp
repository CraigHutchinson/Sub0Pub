/** Sub0Pub: Aggregate arity detection and struct layout fingerprinting: memberCount, TypeFingerprint, makeLayout()
 * @remark Part of Sub0Pub (https://github.com/BareCpper/Sub0Pub), MIT License: see LICENSE.md.
 *         Included by the umbrella header <sub0pub/sub0pub.hpp>.
 */
#ifndef CROG_SUB0PUB_UTILITY_LAYOUT_HPP
#define CROG_SUB0PUB_UTILITY_LAYOUT_HPP

#include "sub0pub/config_macros.hpp"
#include "sub0pub/utility/hash.hpp"
#include <cstddef>
#include <cstdint>
#include <type_traits>
#include <utility>

namespace sub0
{
    namespace utility
    {
        /** Aggregate arity detection via structured bindings / aggregate init
         * @remark Detects the number of members in an aggregate type at compile time.
         *         Used to create a cheap struct-layout fingerprint for IPC verification.
         *         Only works for aggregate types (no user-declared constructors, no virtual functions).
         * @note   Technique from Boost.PFR / Antony Polukhin
         */
        namespace arity {
            // A type that is implicitly convertible to anything
            struct ubiq { template<typename T> operator T() const; };

            // Test whether T can be aggregate-initialized with N arguments
            template<typename T, typename Seq, typename = void>
            struct is_aggregate_constructible : std::false_type {};

            template<typename T, std::size_t... Is>
            struct is_aggregate_constructible<T, std::index_sequence<Is...>,
                std::void_t<decltype(T{ (void(Is), ubiq{})... })>>
                : std::true_type {};

            // Class-type variant: each ubiq is wrapped in its own braces so it initializes exactly one
            // direct member. A bare ubiq is brace-elided into array members (float[4] counting as 4),
            // over-counting the arity that structured bindings (layout::Decompose) see.
            template<typename T, typename Seq, typename = void>
            struct is_aggregate_constructible_braced : std::false_type {};

            template<typename T, std::size_t... Is>
            struct is_aggregate_constructible_braced<T, std::index_sequence<Is...>,
                std::void_t<decltype(T{ { (void(Is), ubiq{}) }... })>>
                : std::true_type {};

            template<typename T, std::size_t N>
            constexpr bool can_construct = std::conditional_t<std::is_class_v<T>,
                is_aggregate_constructible_braced<T, std::make_index_sequence<N>>,
                is_aggregate_constructible<T, std::make_index_sequence<N>>>::value;

            // Binary search for the maximum N where T{ubiq, ubiq, ..., ubiq} compiles
            template<typename T, std::size_t Lo, std::size_t Hi, typename = void>
            struct detect_impl {
                static constexpr std::size_t value = Lo;
            };

            template<typename T, std::size_t Lo, std::size_t Hi>
            struct detect_impl<T, Lo, Hi, std::enable_if_t<(Lo < Hi)>> {
                static constexpr std::size_t Mid = Lo + (Hi - Lo + 1) / 2;
                // Select the type before requesting its value: a conditional expression would
                // instantiate both subtrees and turn this binary search into a full traversal.
                static constexpr std::size_t value = std::conditional_t<can_construct<T, Mid>,
                    detect_impl<T, Mid, Hi>, detect_impl<T, Lo, Mid - 1>>::value;
            };

            /// Upper bound capped at 32 to prevent MSVC template depth/heap exhaustion
            /// on large types (arrays, nested structs). 32 direct members covers
            /// virtually all IPC message types.
            static constexpr std::size_t MaxArity = 32;

            template<typename T>
            struct detect : detect_impl<T, 0, MaxArity> {};
        } // namespace arity

        /** Compile-time count of aggregate members in T
         * @tparam T  Aggregate type to count members of
         * @return Number of direct data members (0 for non-aggregate types)
         * @note Only valid for aggregate types (POD structs, C-style structs)
         */
        template<typename T>
        constexpr std::size_t memberCount = arity::detect<T>::value;

        /** Layout fingerprint combining sizeof, alignof, member count, and element info
         * @remark A cheap compile-time check for struct compatibility across IPC.
         *         Recursive: arrays include the element fingerprint, so changes to
         *         a struct used inside an array are always detected.
         */
        struct TypeFingerprint
        {
            uint32_t size;          ///< sizeof(T)
            uint32_t alignment;     ///< alignof(T)
            uint32_t arity;         ///< number of aggregate members (or 1 for scalars)
            uint32_t extent;        ///< array element count (0 for non-arrays)
            uint32_t elementHash;   ///< recursive fingerprint hash of element type (0 for non-arrays)

            bool operator==(const TypeFingerprint& rhs) const
            {
                return size == rhs.size && alignment == rhs.alignment
                    && arity == rhs.arity && extent == rhs.extent
                    && elementHash == rhs.elementHash;
            }

            bool operator!=(const TypeFingerprint& rhs) const
            { return !(*this == rhs); }
        };

        /** Hash a TypeFingerprint into a single uint32_t for embedding in parent fingerprints
         */
        constexpr uint32_t hashFingerprint(const TypeFingerprint& fp)
        {
            uint32_t h = 5381U;
            h = ((h << 5) + h) + fp.size;
            h = ((h << 5) + h) + fp.alignment;
            h = ((h << 5) + h) + fp.arity;
            h = ((h << 5) + h) + fp.extent;
            h = ((h << 5) + h) + fp.elementHash;
            return h;
        }

        /** Create a TypeFingerprint for any type at compile time
         * @remark Recursive: for array types T[N], the fingerprint includes
         *         the element type's fingerprint hash so that changes to nested
         *         structs are always detected through any depth of array nesting.
         */
        template<typename T>
        [[nodiscard]] constexpr TypeFingerprint makeFingerprint()
        {
            if constexpr (std::is_array_v<T>)
            {
                using Elem = std::remove_extent_t<T>;
                constexpr auto elemFp = makeFingerprint<Elem>();
                return { static_cast<uint32_t>(sizeof(T)),
                         static_cast<uint32_t>(alignof(T)),
                         static_cast<uint32_t>(elemFp.arity), //, Arity is from the Elem for arrays
                         static_cast<uint32_t>(std::extent_v<T>),
                         hashFingerprint(elemFp) };
            }
            else
            {
                return { static_cast<uint32_t>(sizeof(T)),
                         static_cast<uint32_t>(alignof(T)),
                         static_cast<uint32_t>(memberCount<T>),
                         0U, 0U };
            }
        }

        /** Per-member layout entry: offset, size, and recursive element hash
         */
        struct MemberEntry
        {
            uint32_t offset;      ///< byte offset from struct base
            uint32_t size;        ///< sizeof this member
            uint32_t elementHash; ///< recursive fingerprint hash (non-zero for arrays and structs with members)
        };

        /** Compute a hash over an array of MemberEntry for wire comparison
         * @remark Uses djb2 over offset, size, and elementHash of each member
         *         to produce a single uint32_t capturing the exact recursive
         *         byte layout of a struct.
         */
        constexpr uint32_t hashMemberLayout(const MemberEntry* entries, std::size_t count)
        {
            uint32_t h = 5381U;
            for (std::size_t i = 0; i < count; ++i)
            {
                h = ((h << 5) + h) + entries[i].offset;
                h = ((h << 5) + h) + entries[i].size;
                h = ((h << 5) + h) + entries[i].elementHash;
            }
            return h;
        }


        /** Extended layout fingerprint with per-member offset, size, and recursive element verification
         */
        struct TypeLayout
        {
            TypeFingerprint fingerprint; ///< sizeof + alignof + arity + array info
            uint32_t layoutHash;         ///< hash of per-member {offset, size, elementHash} triples

            bool operator==(const TypeLayout& rhs) const
            { return fingerprint == rhs.fingerprint && layoutHash == rhs.layoutHash; }

            bool operator!=(const TypeLayout& rhs) const
            { return !(*this == rhs); }
        };

        /** Create a MemberEntry from a structured binding reference
         * @remark Computes offset via pointer arithmetic from the struct base.
         *         Recursively fingerprints array and aggregate member types.
         */
        template<typename MemberT, typename BaseT>
        MemberEntry entryFrom(const BaseT& base, const MemberT& member)
        {
            using Raw = std::remove_cv_t<std::remove_reference_t<MemberT>>;
            const auto offset = static_cast<uint32_t>(
                reinterpret_cast<const char*>(&member) - reinterpret_cast<const char*>(&base));
            const auto size = static_cast<uint32_t>(sizeof(MemberT));
            uint32_t elemHash = 0;
            if constexpr (std::is_array_v<Raw> || (std::is_class_v<Raw> && memberCount<Raw> > 0))
                elemHash = hashFingerprint(makeFingerprint<Raw>());
            return { offset, size, elemHash };
        }

        /** Automatic layout decomposition via structured bindings (Boost.PFR-style)
         * @remark On GCC/Clang: uses class template partial specialization with
         *         structured bindings to decompose aggregates into per-member
         *         offset + size + recursive element hash.
         * @remark On MSVC: structured bindings in template specializations trigger
         *         eager parsing bugs (C3448). Falls back to TypeFingerprint only
         *         (sizeof+alignof+arity) without per-member offset detail.
         *         Full per-member support on MSVC awaits C++26 reflection.
         * @note   Supports up to 32 direct members (arity::MaxArity).
         */
        namespace layout {
            template<typename T, std::size_t N>
            struct Decompose { static uint32_t hash(T&) { return 0; } };

#if !defined(_MSC_VER)
            #define SUB0_E_(v, m) entryFrom(v, m)

            #define SUB0_LAYOUT_CASE(N, ...) \
                template<typename T> struct Decompose<T, N> { static uint32_t hash(T& v) { \
                    auto& [__VA_ARGS__] = v; \
                    MemberEntry e[] = {

            #define SUB0_LAYOUT_END(N) \
                    }; return hashMemberLayout(e, N); } };

            SUB0_LAYOUT_CASE(1,  m0) SUB0_E_(v,m0) SUB0_LAYOUT_END(1)
            SUB0_LAYOUT_CASE(2,  m0,m1) SUB0_E_(v,m0),SUB0_E_(v,m1) SUB0_LAYOUT_END(2)
            SUB0_LAYOUT_CASE(3,  m0,m1,m2) SUB0_E_(v,m0),SUB0_E_(v,m1),SUB0_E_(v,m2) SUB0_LAYOUT_END(3)
            SUB0_LAYOUT_CASE(4,  m0,m1,m2,m3) SUB0_E_(v,m0),SUB0_E_(v,m1),SUB0_E_(v,m2),SUB0_E_(v,m3) SUB0_LAYOUT_END(4)
            SUB0_LAYOUT_CASE(5,  m0,m1,m2,m3,m4) SUB0_E_(v,m0),SUB0_E_(v,m1),SUB0_E_(v,m2),SUB0_E_(v,m3),SUB0_E_(v,m4) SUB0_LAYOUT_END(5)
            SUB0_LAYOUT_CASE(6,  m0,m1,m2,m3,m4,m5) SUB0_E_(v,m0),SUB0_E_(v,m1),SUB0_E_(v,m2),SUB0_E_(v,m3),SUB0_E_(v,m4),SUB0_E_(v,m5) SUB0_LAYOUT_END(6)
            SUB0_LAYOUT_CASE(7,  m0,m1,m2,m3,m4,m5,m6) SUB0_E_(v,m0),SUB0_E_(v,m1),SUB0_E_(v,m2),SUB0_E_(v,m3),SUB0_E_(v,m4),SUB0_E_(v,m5),SUB0_E_(v,m6) SUB0_LAYOUT_END(7)
            SUB0_LAYOUT_CASE(8,  m0,m1,m2,m3,m4,m5,m6,m7) SUB0_E_(v,m0),SUB0_E_(v,m1),SUB0_E_(v,m2),SUB0_E_(v,m3),SUB0_E_(v,m4),SUB0_E_(v,m5),SUB0_E_(v,m6),SUB0_E_(v,m7) SUB0_LAYOUT_END(8)
            SUB0_LAYOUT_CASE(9,  m0,m1,m2,m3,m4,m5,m6,m7,m8) SUB0_E_(v,m0),SUB0_E_(v,m1),SUB0_E_(v,m2),SUB0_E_(v,m3),SUB0_E_(v,m4),SUB0_E_(v,m5),SUB0_E_(v,m6),SUB0_E_(v,m7),SUB0_E_(v,m8) SUB0_LAYOUT_END(9)
            SUB0_LAYOUT_CASE(10, m0,m1,m2,m3,m4,m5,m6,m7,m8,m9) SUB0_E_(v,m0),SUB0_E_(v,m1),SUB0_E_(v,m2),SUB0_E_(v,m3),SUB0_E_(v,m4),SUB0_E_(v,m5),SUB0_E_(v,m6),SUB0_E_(v,m7),SUB0_E_(v,m8),SUB0_E_(v,m9) SUB0_LAYOUT_END(10)
            SUB0_LAYOUT_CASE(11, m0,m1,m2,m3,m4,m5,m6,m7,m8,m9,m10) SUB0_E_(v,m0),SUB0_E_(v,m1),SUB0_E_(v,m2),SUB0_E_(v,m3),SUB0_E_(v,m4),SUB0_E_(v,m5),SUB0_E_(v,m6),SUB0_E_(v,m7),SUB0_E_(v,m8),SUB0_E_(v,m9),SUB0_E_(v,m10) SUB0_LAYOUT_END(11)
            SUB0_LAYOUT_CASE(12, m0,m1,m2,m3,m4,m5,m6,m7,m8,m9,m10,m11) SUB0_E_(v,m0),SUB0_E_(v,m1),SUB0_E_(v,m2),SUB0_E_(v,m3),SUB0_E_(v,m4),SUB0_E_(v,m5),SUB0_E_(v,m6),SUB0_E_(v,m7),SUB0_E_(v,m8),SUB0_E_(v,m9),SUB0_E_(v,m10),SUB0_E_(v,m11) SUB0_LAYOUT_END(12)
            SUB0_LAYOUT_CASE(13, m0,m1,m2,m3,m4,m5,m6,m7,m8,m9,m10,m11,m12) SUB0_E_(v,m0),SUB0_E_(v,m1),SUB0_E_(v,m2),SUB0_E_(v,m3),SUB0_E_(v,m4),SUB0_E_(v,m5),SUB0_E_(v,m6),SUB0_E_(v,m7),SUB0_E_(v,m8),SUB0_E_(v,m9),SUB0_E_(v,m10),SUB0_E_(v,m11),SUB0_E_(v,m12) SUB0_LAYOUT_END(13)
            SUB0_LAYOUT_CASE(14, m0,m1,m2,m3,m4,m5,m6,m7,m8,m9,m10,m11,m12,m13) SUB0_E_(v,m0),SUB0_E_(v,m1),SUB0_E_(v,m2),SUB0_E_(v,m3),SUB0_E_(v,m4),SUB0_E_(v,m5),SUB0_E_(v,m6),SUB0_E_(v,m7),SUB0_E_(v,m8),SUB0_E_(v,m9),SUB0_E_(v,m10),SUB0_E_(v,m11),SUB0_E_(v,m12),SUB0_E_(v,m13) SUB0_LAYOUT_END(14)
            SUB0_LAYOUT_CASE(15, m0,m1,m2,m3,m4,m5,m6,m7,m8,m9,m10,m11,m12,m13,m14) SUB0_E_(v,m0),SUB0_E_(v,m1),SUB0_E_(v,m2),SUB0_E_(v,m3),SUB0_E_(v,m4),SUB0_E_(v,m5),SUB0_E_(v,m6),SUB0_E_(v,m7),SUB0_E_(v,m8),SUB0_E_(v,m9),SUB0_E_(v,m10),SUB0_E_(v,m11),SUB0_E_(v,m12),SUB0_E_(v,m13),SUB0_E_(v,m14) SUB0_LAYOUT_END(15)
            SUB0_LAYOUT_CASE(16, m0,m1,m2,m3,m4,m5,m6,m7,m8,m9,m10,m11,m12,m13,m14,m15) SUB0_E_(v,m0),SUB0_E_(v,m1),SUB0_E_(v,m2),SUB0_E_(v,m3),SUB0_E_(v,m4),SUB0_E_(v,m5),SUB0_E_(v,m6),SUB0_E_(v,m7),SUB0_E_(v,m8),SUB0_E_(v,m9),SUB0_E_(v,m10),SUB0_E_(v,m11),SUB0_E_(v,m12),SUB0_E_(v,m13),SUB0_E_(v,m14),SUB0_E_(v,m15) SUB0_LAYOUT_END(16)
            SUB0_LAYOUT_CASE(17, m0,m1,m2,m3,m4,m5,m6,m7,m8,m9,m10,m11,m12,m13,m14,m15,m16) SUB0_E_(v,m0),SUB0_E_(v,m1),SUB0_E_(v,m2),SUB0_E_(v,m3),SUB0_E_(v,m4),SUB0_E_(v,m5),SUB0_E_(v,m6),SUB0_E_(v,m7),SUB0_E_(v,m8),SUB0_E_(v,m9),SUB0_E_(v,m10),SUB0_E_(v,m11),SUB0_E_(v,m12),SUB0_E_(v,m13),SUB0_E_(v,m14),SUB0_E_(v,m15),SUB0_E_(v,m16) SUB0_LAYOUT_END(17)
            SUB0_LAYOUT_CASE(18, m0,m1,m2,m3,m4,m5,m6,m7,m8,m9,m10,m11,m12,m13,m14,m15,m16,m17) SUB0_E_(v,m0),SUB0_E_(v,m1),SUB0_E_(v,m2),SUB0_E_(v,m3),SUB0_E_(v,m4),SUB0_E_(v,m5),SUB0_E_(v,m6),SUB0_E_(v,m7),SUB0_E_(v,m8),SUB0_E_(v,m9),SUB0_E_(v,m10),SUB0_E_(v,m11),SUB0_E_(v,m12),SUB0_E_(v,m13),SUB0_E_(v,m14),SUB0_E_(v,m15),SUB0_E_(v,m16),SUB0_E_(v,m17) SUB0_LAYOUT_END(18)
            SUB0_LAYOUT_CASE(19, m0,m1,m2,m3,m4,m5,m6,m7,m8,m9,m10,m11,m12,m13,m14,m15,m16,m17,m18) SUB0_E_(v,m0),SUB0_E_(v,m1),SUB0_E_(v,m2),SUB0_E_(v,m3),SUB0_E_(v,m4),SUB0_E_(v,m5),SUB0_E_(v,m6),SUB0_E_(v,m7),SUB0_E_(v,m8),SUB0_E_(v,m9),SUB0_E_(v,m10),SUB0_E_(v,m11),SUB0_E_(v,m12),SUB0_E_(v,m13),SUB0_E_(v,m14),SUB0_E_(v,m15),SUB0_E_(v,m16),SUB0_E_(v,m17),SUB0_E_(v,m18) SUB0_LAYOUT_END(19)
            SUB0_LAYOUT_CASE(20, m0,m1,m2,m3,m4,m5,m6,m7,m8,m9,m10,m11,m12,m13,m14,m15,m16,m17,m18,m19) SUB0_E_(v,m0),SUB0_E_(v,m1),SUB0_E_(v,m2),SUB0_E_(v,m3),SUB0_E_(v,m4),SUB0_E_(v,m5),SUB0_E_(v,m6),SUB0_E_(v,m7),SUB0_E_(v,m8),SUB0_E_(v,m9),SUB0_E_(v,m10),SUB0_E_(v,m11),SUB0_E_(v,m12),SUB0_E_(v,m13),SUB0_E_(v,m14),SUB0_E_(v,m15),SUB0_E_(v,m16),SUB0_E_(v,m17),SUB0_E_(v,m18),SUB0_E_(v,m19) SUB0_LAYOUT_END(20)
            SUB0_LAYOUT_CASE(21, m0,m1,m2,m3,m4,m5,m6,m7,m8,m9,m10,m11,m12,m13,m14,m15,m16,m17,m18,m19,m20) SUB0_E_(v,m0),SUB0_E_(v,m1),SUB0_E_(v,m2),SUB0_E_(v,m3),SUB0_E_(v,m4),SUB0_E_(v,m5),SUB0_E_(v,m6),SUB0_E_(v,m7),SUB0_E_(v,m8),SUB0_E_(v,m9),SUB0_E_(v,m10),SUB0_E_(v,m11),SUB0_E_(v,m12),SUB0_E_(v,m13),SUB0_E_(v,m14),SUB0_E_(v,m15),SUB0_E_(v,m16),SUB0_E_(v,m17),SUB0_E_(v,m18),SUB0_E_(v,m19),SUB0_E_(v,m20) SUB0_LAYOUT_END(21)
            SUB0_LAYOUT_CASE(22, m0,m1,m2,m3,m4,m5,m6,m7,m8,m9,m10,m11,m12,m13,m14,m15,m16,m17,m18,m19,m20,m21) SUB0_E_(v,m0),SUB0_E_(v,m1),SUB0_E_(v,m2),SUB0_E_(v,m3),SUB0_E_(v,m4),SUB0_E_(v,m5),SUB0_E_(v,m6),SUB0_E_(v,m7),SUB0_E_(v,m8),SUB0_E_(v,m9),SUB0_E_(v,m10),SUB0_E_(v,m11),SUB0_E_(v,m12),SUB0_E_(v,m13),SUB0_E_(v,m14),SUB0_E_(v,m15),SUB0_E_(v,m16),SUB0_E_(v,m17),SUB0_E_(v,m18),SUB0_E_(v,m19),SUB0_E_(v,m20),SUB0_E_(v,m21) SUB0_LAYOUT_END(22)
            SUB0_LAYOUT_CASE(23, m0,m1,m2,m3,m4,m5,m6,m7,m8,m9,m10,m11,m12,m13,m14,m15,m16,m17,m18,m19,m20,m21,m22) SUB0_E_(v,m0),SUB0_E_(v,m1),SUB0_E_(v,m2),SUB0_E_(v,m3),SUB0_E_(v,m4),SUB0_E_(v,m5),SUB0_E_(v,m6),SUB0_E_(v,m7),SUB0_E_(v,m8),SUB0_E_(v,m9),SUB0_E_(v,m10),SUB0_E_(v,m11),SUB0_E_(v,m12),SUB0_E_(v,m13),SUB0_E_(v,m14),SUB0_E_(v,m15),SUB0_E_(v,m16),SUB0_E_(v,m17),SUB0_E_(v,m18),SUB0_E_(v,m19),SUB0_E_(v,m20),SUB0_E_(v,m21),SUB0_E_(v,m22) SUB0_LAYOUT_END(23)
            SUB0_LAYOUT_CASE(24, m0,m1,m2,m3,m4,m5,m6,m7,m8,m9,m10,m11,m12,m13,m14,m15,m16,m17,m18,m19,m20,m21,m22,m23) SUB0_E_(v,m0),SUB0_E_(v,m1),SUB0_E_(v,m2),SUB0_E_(v,m3),SUB0_E_(v,m4),SUB0_E_(v,m5),SUB0_E_(v,m6),SUB0_E_(v,m7),SUB0_E_(v,m8),SUB0_E_(v,m9),SUB0_E_(v,m10),SUB0_E_(v,m11),SUB0_E_(v,m12),SUB0_E_(v,m13),SUB0_E_(v,m14),SUB0_E_(v,m15),SUB0_E_(v,m16),SUB0_E_(v,m17),SUB0_E_(v,m18),SUB0_E_(v,m19),SUB0_E_(v,m20),SUB0_E_(v,m21),SUB0_E_(v,m22),SUB0_E_(v,m23) SUB0_LAYOUT_END(24)
            SUB0_LAYOUT_CASE(25, m0,m1,m2,m3,m4,m5,m6,m7,m8,m9,m10,m11,m12,m13,m14,m15,m16,m17,m18,m19,m20,m21,m22,m23,m24) SUB0_E_(v,m0),SUB0_E_(v,m1),SUB0_E_(v,m2),SUB0_E_(v,m3),SUB0_E_(v,m4),SUB0_E_(v,m5),SUB0_E_(v,m6),SUB0_E_(v,m7),SUB0_E_(v,m8),SUB0_E_(v,m9),SUB0_E_(v,m10),SUB0_E_(v,m11),SUB0_E_(v,m12),SUB0_E_(v,m13),SUB0_E_(v,m14),SUB0_E_(v,m15),SUB0_E_(v,m16),SUB0_E_(v,m17),SUB0_E_(v,m18),SUB0_E_(v,m19),SUB0_E_(v,m20),SUB0_E_(v,m21),SUB0_E_(v,m22),SUB0_E_(v,m23),SUB0_E_(v,m24) SUB0_LAYOUT_END(25)
            SUB0_LAYOUT_CASE(26, m0,m1,m2,m3,m4,m5,m6,m7,m8,m9,m10,m11,m12,m13,m14,m15,m16,m17,m18,m19,m20,m21,m22,m23,m24,m25) SUB0_E_(v,m0),SUB0_E_(v,m1),SUB0_E_(v,m2),SUB0_E_(v,m3),SUB0_E_(v,m4),SUB0_E_(v,m5),SUB0_E_(v,m6),SUB0_E_(v,m7),SUB0_E_(v,m8),SUB0_E_(v,m9),SUB0_E_(v,m10),SUB0_E_(v,m11),SUB0_E_(v,m12),SUB0_E_(v,m13),SUB0_E_(v,m14),SUB0_E_(v,m15),SUB0_E_(v,m16),SUB0_E_(v,m17),SUB0_E_(v,m18),SUB0_E_(v,m19),SUB0_E_(v,m20),SUB0_E_(v,m21),SUB0_E_(v,m22),SUB0_E_(v,m23),SUB0_E_(v,m24),SUB0_E_(v,m25) SUB0_LAYOUT_END(26)
            SUB0_LAYOUT_CASE(27, m0,m1,m2,m3,m4,m5,m6,m7,m8,m9,m10,m11,m12,m13,m14,m15,m16,m17,m18,m19,m20,m21,m22,m23,m24,m25,m26) SUB0_E_(v,m0),SUB0_E_(v,m1),SUB0_E_(v,m2),SUB0_E_(v,m3),SUB0_E_(v,m4),SUB0_E_(v,m5),SUB0_E_(v,m6),SUB0_E_(v,m7),SUB0_E_(v,m8),SUB0_E_(v,m9),SUB0_E_(v,m10),SUB0_E_(v,m11),SUB0_E_(v,m12),SUB0_E_(v,m13),SUB0_E_(v,m14),SUB0_E_(v,m15),SUB0_E_(v,m16),SUB0_E_(v,m17),SUB0_E_(v,m18),SUB0_E_(v,m19),SUB0_E_(v,m20),SUB0_E_(v,m21),SUB0_E_(v,m22),SUB0_E_(v,m23),SUB0_E_(v,m24),SUB0_E_(v,m25),SUB0_E_(v,m26) SUB0_LAYOUT_END(27)
            SUB0_LAYOUT_CASE(28, m0,m1,m2,m3,m4,m5,m6,m7,m8,m9,m10,m11,m12,m13,m14,m15,m16,m17,m18,m19,m20,m21,m22,m23,m24,m25,m26,m27) SUB0_E_(v,m0),SUB0_E_(v,m1),SUB0_E_(v,m2),SUB0_E_(v,m3),SUB0_E_(v,m4),SUB0_E_(v,m5),SUB0_E_(v,m6),SUB0_E_(v,m7),SUB0_E_(v,m8),SUB0_E_(v,m9),SUB0_E_(v,m10),SUB0_E_(v,m11),SUB0_E_(v,m12),SUB0_E_(v,m13),SUB0_E_(v,m14),SUB0_E_(v,m15),SUB0_E_(v,m16),SUB0_E_(v,m17),SUB0_E_(v,m18),SUB0_E_(v,m19),SUB0_E_(v,m20),SUB0_E_(v,m21),SUB0_E_(v,m22),SUB0_E_(v,m23),SUB0_E_(v,m24),SUB0_E_(v,m25),SUB0_E_(v,m26),SUB0_E_(v,m27) SUB0_LAYOUT_END(28)
            SUB0_LAYOUT_CASE(29, m0,m1,m2,m3,m4,m5,m6,m7,m8,m9,m10,m11,m12,m13,m14,m15,m16,m17,m18,m19,m20,m21,m22,m23,m24,m25,m26,m27,m28) SUB0_E_(v,m0),SUB0_E_(v,m1),SUB0_E_(v,m2),SUB0_E_(v,m3),SUB0_E_(v,m4),SUB0_E_(v,m5),SUB0_E_(v,m6),SUB0_E_(v,m7),SUB0_E_(v,m8),SUB0_E_(v,m9),SUB0_E_(v,m10),SUB0_E_(v,m11),SUB0_E_(v,m12),SUB0_E_(v,m13),SUB0_E_(v,m14),SUB0_E_(v,m15),SUB0_E_(v,m16),SUB0_E_(v,m17),SUB0_E_(v,m18),SUB0_E_(v,m19),SUB0_E_(v,m20),SUB0_E_(v,m21),SUB0_E_(v,m22),SUB0_E_(v,m23),SUB0_E_(v,m24),SUB0_E_(v,m25),SUB0_E_(v,m26),SUB0_E_(v,m27),SUB0_E_(v,m28) SUB0_LAYOUT_END(29)
            SUB0_LAYOUT_CASE(30, m0,m1,m2,m3,m4,m5,m6,m7,m8,m9,m10,m11,m12,m13,m14,m15,m16,m17,m18,m19,m20,m21,m22,m23,m24,m25,m26,m27,m28,m29) SUB0_E_(v,m0),SUB0_E_(v,m1),SUB0_E_(v,m2),SUB0_E_(v,m3),SUB0_E_(v,m4),SUB0_E_(v,m5),SUB0_E_(v,m6),SUB0_E_(v,m7),SUB0_E_(v,m8),SUB0_E_(v,m9),SUB0_E_(v,m10),SUB0_E_(v,m11),SUB0_E_(v,m12),SUB0_E_(v,m13),SUB0_E_(v,m14),SUB0_E_(v,m15),SUB0_E_(v,m16),SUB0_E_(v,m17),SUB0_E_(v,m18),SUB0_E_(v,m19),SUB0_E_(v,m20),SUB0_E_(v,m21),SUB0_E_(v,m22),SUB0_E_(v,m23),SUB0_E_(v,m24),SUB0_E_(v,m25),SUB0_E_(v,m26),SUB0_E_(v,m27),SUB0_E_(v,m28),SUB0_E_(v,m29) SUB0_LAYOUT_END(30)
            SUB0_LAYOUT_CASE(31, m0,m1,m2,m3,m4,m5,m6,m7,m8,m9,m10,m11,m12,m13,m14,m15,m16,m17,m18,m19,m20,m21,m22,m23,m24,m25,m26,m27,m28,m29,m30) SUB0_E_(v,m0),SUB0_E_(v,m1),SUB0_E_(v,m2),SUB0_E_(v,m3),SUB0_E_(v,m4),SUB0_E_(v,m5),SUB0_E_(v,m6),SUB0_E_(v,m7),SUB0_E_(v,m8),SUB0_E_(v,m9),SUB0_E_(v,m10),SUB0_E_(v,m11),SUB0_E_(v,m12),SUB0_E_(v,m13),SUB0_E_(v,m14),SUB0_E_(v,m15),SUB0_E_(v,m16),SUB0_E_(v,m17),SUB0_E_(v,m18),SUB0_E_(v,m19),SUB0_E_(v,m20),SUB0_E_(v,m21),SUB0_E_(v,m22),SUB0_E_(v,m23),SUB0_E_(v,m24),SUB0_E_(v,m25),SUB0_E_(v,m26),SUB0_E_(v,m27),SUB0_E_(v,m28),SUB0_E_(v,m29),SUB0_E_(v,m30) SUB0_LAYOUT_END(31)
            SUB0_LAYOUT_CASE(32, m0,m1,m2,m3,m4,m5,m6,m7,m8,m9,m10,m11,m12,m13,m14,m15,m16,m17,m18,m19,m20,m21,m22,m23,m24,m25,m26,m27,m28,m29,m30,m31) SUB0_E_(v,m0),SUB0_E_(v,m1),SUB0_E_(v,m2),SUB0_E_(v,m3),SUB0_E_(v,m4),SUB0_E_(v,m5),SUB0_E_(v,m6),SUB0_E_(v,m7),SUB0_E_(v,m8),SUB0_E_(v,m9),SUB0_E_(v,m10),SUB0_E_(v,m11),SUB0_E_(v,m12),SUB0_E_(v,m13),SUB0_E_(v,m14),SUB0_E_(v,m15),SUB0_E_(v,m16),SUB0_E_(v,m17),SUB0_E_(v,m18),SUB0_E_(v,m19),SUB0_E_(v,m20),SUB0_E_(v,m21),SUB0_E_(v,m22),SUB0_E_(v,m23),SUB0_E_(v,m24),SUB0_E_(v,m25),SUB0_E_(v,m26),SUB0_E_(v,m27),SUB0_E_(v,m28),SUB0_E_(v,m29),SUB0_E_(v,m30),SUB0_E_(v,m31) SUB0_LAYOUT_END(32)

            #undef SUB0_LAYOUT_CASE
            #undef SUB0_LAYOUT_END
            #undef SUB0_E_
#endif // !_MSC_VER
        } // namespace layout

        /** Create a TypeLayout automatically for any aggregate type
         * @remark Fully automatic — no macro or member list needed.
         *         Uses structured bindings (Boost.PFR-style) to decompose the
         *         struct and compute per-member offset + size + recursive element hash.
         * @note   Supports up to 32 direct members (arity::MaxArity).
         * @tparam T  Aggregate type to fingerprint
         */
        template<typename T>
        [[nodiscard]] TypeLayout makeLayout()
        {
            constexpr auto N = memberCount<T>;
            auto fp = makeFingerprint<T>();
            T val{};
            return { fp, layout::Decompose<T, N>::hash(val) };
        }
    } // END: utility
} // END: sub0

#endif // CROG_SUB0PUB_UTILITY_LAYOUT_HPP
