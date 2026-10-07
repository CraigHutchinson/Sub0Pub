/** Sub0Pub: The audit: a record of what a run published and delivered, reported on request and at exit
 * @remark Part of Sub0Pub (https://github.com/BareCpper/Sub0Pub), MIT License: see LICENSE.md.
 *         Included by the umbrella header <sub0pub/sub0pub.hpp>.
 */
#ifndef CROG_SUB0PUB_AUDIT_HPP
#define CROG_SUB0PUB_AUDIT_HPP

#include "sub0pub/config_macros.hpp"
#include <cstdint>

//
// An audit build (SUB0PUB_AUDIT defined true for the whole program) keeps one ledger per message type, fed by the
// runtime broker and by the receivers a type's StaticTo / StaticFirst list names, and reports:
//
//   findings   publications that reached no receiver, and which publisher made them
//              subscriptions refused because the table was full
//              subscribers that never received a message, and message types that were never published
//              subscribers of a statically wired type that its StaticTo list does not name
//   per type   its publishers and their counts; its receivers in the order they joined, with their deliveries and
//              when they joined or left; the runtime table's peak against its capacity; and whether the set of
//              receivers ever changed, which is what makes a brokered type a candidate for StaticTo
//
// The report is written at exit and whenever sub0::auditReport() is called; sub0::auditFindings() gives a test the
// count. Nothing here changes what the program does, and none of it exists in a build without SUB0PUB_AUDIT: both
// functions are then constants and no ledger is instantiated.
//
// What the audit does not see (docs/DESIGN.md, K33): explicit wirings (wire(), StaticWiring); which Domain of a
// Scoped type a message went through (a type's sessions share one ledger); whether filter() or cancel() kept a
// message from a receiver (it counts a subscriber as reached once delivery gets to it); and more than cReceivers
// receivers or cPublishers publisher classes per type. Without RTTI a runtime subscriber is shown by address alone.

#if SUB0PUB_AUDIT

#include <atomic>
#include <cstdio>
#include <cstring>

#if defined(__cpp_rtti) || defined(__GXX_RTTI) || defined(_CPPRTTI)
#include <typeinfo>
#define SUB0PUB_AUDIT_RTTI true
#if __has_include(<cxxabi.h>)
#include <cstdlib>
#include <cxxabi.h>
#define SUB0PUB_AUDIT_DEMANGLE true
#else
#define SUB0PUB_AUDIT_DEMANGLE false
#endif
#else
#define SUB0PUB_AUDIT_RTTI false
#define SUB0PUB_AUDIT_DEMANGLE false
#endif

/** Where a line of the audit's report goes
 * @param line  Null-terminated text, without a line ending
 * Default writes it to stderr. Override to send the report to a log, a test or a debug channel; the audit holds no
 * lock while it runs.
 */
#ifndef SUB0PUB_AUDIT_PRINT
#define SUB0PUB_AUDIT_PRINT(line) do { std::fputs((line), stderr); std::fputc('\n', stderr); } while(false)
#endif

/** Action after the audit has been reported at exit
 * @param findings  The number of findings reported
 * Default does nothing. A test or CI build can fail the run instead, for example
 * `#define SUB0PUB_AUDIT_EXIT(findings) do { if ((findings) != 0U) std::_Exit(1); } while(false)`.
 */
#ifndef SUB0PUB_AUDIT_EXIT
#define SUB0PUB_AUDIT_EXIT(findings) ((void)(findings))
#endif

namespace sub0
{
    namespace detail
    {
    namespace audit
    {
        inline constexpr uint32_t cReceivers = 16U;  ///< receivers remembered per message type
        inline constexpr uint32_t cPublishers = 8U;  ///< publisher classes remembered per message type
        inline constexpr uint32_t cLine = 256U;      ///< longest report line, including its terminator

#if SUB0PUB_AUDIT_RTTI
        using TypeId = const std::type_info*;
#else
        using TypeId = const void*; // always null: receivers are shown by address
#endif

        /// Guards every ledger and the list of them. Held only for the few instructions of one update or one line.
        inline std::atomic_flag gLock;

        class Guard
        {
        public:
            Guard() noexcept { while (gLock.test_and_set(std::memory_order_acquire)) {} }
            ~Guard() { gLock.clear(std::memory_order_release); }
            Guard(const Guard&) = delete;
            Guard& operator=(const Guard&) = delete;
        };

        /// The text a compiler gives a function template instantiated for T: it contains T's name
        template<class T>
        const char* typeSignature() noexcept
        {
#if defined(__clang__) || defined(__GNUC__)
            return __PRETTY_FUNCTION__;
#else
            return __FUNCSIG__;
#endif
        }

        /// As typeSignature(), for an object's address: it contains the object's name
        template<auto* Bound>
        const char* boundSignature() noexcept
        {
#if defined(__clang__) || defined(__GNUC__)
            return __PRETTY_FUNCTION__;
#else
            return __FUNCSIG__;
#endif
        }

        /** The dynamic type of an object, or null while it is only its Base (under construction or destruction)
         * @remark The audit also reads this for subscribers it has not named yet when another subscriber joins or
         *         leaves. One that is part-way through its own construction then reads as whatever class its
         *         constructors have reached; its first delivery corrects the name.
         */
        template<class Base>
        TypeId dynamicType(const Base* object) noexcept
        {
#if SUB0PUB_AUDIT_RTTI
            // MSVC evaluates typeid(*p) at compile time, naming the base, when p's class is a template specialisation
            // that has not been instantiated yet at this point; requiring its size first does that
            static_assert(sizeof(Base) > 0);
            const std::type_info& type = typeid(*object);
            return type == typeid(Base) ? nullptr : &type;
#else
            (void)object;
            return nullptr;
#endif
        }

        /** One line of the report. A ledger describes every line it has, in order; the Line keeps the text of the one
         *  that was asked for and counts them all, so the same description serves counting and printing. */
        class Line
        {
        public:
            /** @param buffer  Where the wanted line is written, or null to count only
             *  @param wanted  Index of the line to keep
             */
            Line(char* buffer, uint32_t wanted) noexcept : buffer_(buffer), wanted_(wanted) {}

            /// Start the next line; @return whether it is the one to write
            bool begin() noexcept
            {
                writing_ = buffer_ != nullptr && count_ == wanted_;
                ++count_;
                if (writing_)
                {
                    found_ = true;
                    length_ = 0;
                    buffer_[0] = '\0';
                }
                return writing_;
            }

            uint32_t count() const noexcept { return count_; }
            bool found() const noexcept { return found_; }

            void text(const char* s) noexcept { text(s, static_cast<uint32_t>(std::strlen(s))); }

            void text(const char* s, uint32_t n) noexcept
            {
                if (!writing_)
                    return;
                if (n > cLine - 1U - length_)
                    n = cLine - 1U - length_;
                std::memcpy(buffer_ + length_, s, n);
                length_ += n;
                buffer_[length_] = '\0';
            }

            void number(uint64_t value) noexcept
            {
                char digits[24];
                const int n = std::snprintf(digits, sizeof(digits), "%llu", static_cast<unsigned long long>(value));
                if (n > 0)
                    text(digits, static_cast<uint32_t>(n));
            }

            void address(const void* p) noexcept
            {
                char digits[24];
                const int n = std::snprintf(digits, sizeof(digits), "%p", p);
                if (n > 0)
                    text(digits, static_cast<uint32_t>(n));
            }

            /// "1 publication" / "3 publications"
            void counted(uint64_t value, const char* singular, const char* plural) noexcept
            {
                number(value);
                text(" ");
                text(value == 1U ? singular : plural);
            }

            /// The type named in a typeSignature()
            void type(const char* signature) noexcept
            {
                const char* first = signature;
                const char* last = signature + std::strlen(signature);
                if (const char* gnu = std::strstr(signature, "T = ")) // GCC, Clang: "... [with T = Name]"
                {
                    first = gnu + 4;
                    while (last > first && last[-1] != ']')
                        --last;
                    last = (last > first) ? last - 1 : signature + std::strlen(signature);
                }
                else if (const char* msvc = std::strstr(signature, "typeSignature<")) // MSVC: "...typeSignature<Name>(void)..."
                {
                    first = msvc + 14;
                    const char* end = nullptr;
                    for (const char* c = first; (c = std::strstr(c, ">(")) != nullptr; ++c)
                        end = c;
                    if (end != nullptr)
                        last = end;
                }
                name(first, last);
            }

            /// The object named in a boundSignature(): the identifier its address expression ends with
            void object(const char* signature) noexcept
            {
                const char* amp = nullptr;
                for (const char* c = signature; *c != '\0'; ++c)
                    if (*c == '&')
                        amp = c;
                if (amp == nullptr)
                {
                    text("a receiver");
                    return;
                }
                // The address expression ends where the signature's argument list does: "...]" (GCC, Clang) or
                // "...>(void)" (MSVC), possibly inside parentheses
                const char* last = nullptr;
                for (const char* c = amp; *c != '\0'; ++c)
                    if (*c == ']')
                        last = c;
                if (last == nullptr)
                    for (const char* c = amp; (c = std::strstr(c, ">(")) != nullptr; ++c)
                        last = c;
                if (last == nullptr)
                    last = amp + std::strlen(amp);
                while (last > amp + 1 && (last[-1] == ')' || last[-1] == ' '))
                    --last;
                const char* first = last;
                while (first > amp + 1 && isIdentifier(first[-1]))
                    --first;
                text(first, static_cast<uint32_t>(last - first));
            }

            /// The name of a dynamic type; "a subscriber" when it was never learned
            void dynamic(TypeId type) noexcept
            {
                if (type == nullptr)
                {
                    text("a subscriber");
                    return;
                }
#if SUB0PUB_AUDIT_RTTI
#if SUB0PUB_AUDIT_DEMANGLE
                int status = 0;
                char* const readable = abi::__cxa_demangle(type->name(), nullptr, nullptr, &status); // owning: freed below
                if (status == 0 && readable != nullptr)
                    text(readable);
                else
                    text(type->name());
                std::free(readable);
#else
                const char* const raw = type->name();
                name(raw, raw + std::strlen(raw));
#endif
#endif
            }

        private:
            static bool isIdentifier(char c) noexcept
            {
                return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_';
            }

            /// A type name without the class-key MSVC puts in front of it
            void name(const char* first, const char* last) noexcept
            {
                static constexpr const char* cKeys[] = {"struct ", "class ", "union ", "enum "};
                for (const char* key : cKeys)
                {
                    const uint32_t n = static_cast<uint32_t>(std::strlen(key));
                    if (static_cast<uint32_t>(last - first) > n && std::strncmp(first, key, n) == 0)
                    {
                        first += n;
                        break;
                    }
                }
                text(first, static_cast<uint32_t>(last - first));
            }

            char* buffer_;          // non-owning; cLine characters, or null
            uint32_t wanted_;
            uint32_t count_ = 0;
            uint32_t length_ = 0;
            bool writing_ = false;
            bool found_ = false;
        };

        /// One message type's entry in the audit
        struct Record
        {
            void (*findings)(Line& line) noexcept; ///< describes one line per finding
            void (*detail)(Line& line) noexcept;   ///< describes the type's publishers and receivers
            void (*learn)() noexcept;              ///< names the runtime subscribers it holds now; null if it has none
            Record* next;                          // non-owning; records have static storage
        };

        /** The process's audit: every message type that recorded something, reported when the program exits */
        struct Registry
        {
            Record* head = nullptr; // non-owning
            Record* tail = nullptr; // non-owning
            uint32_t unnamed = 0;   ///< registered runtime subscribers whose type has not been read yet

            /// @pre gLock is held
            void add(Record& record) noexcept
            {
                (tail != nullptr ? tail->next : head) = &record;
                tail = &record;
            }

            /** Name every runtime subscriber that is registered now and has not been delivered to yet.
             *  A subscriber's type cannot be read while it is being constructed, which is when it joins, so the
             *  audit reads it at its first delivery, or here: when any subscriber joins or leaves, when anything is
             *  published, and before a report. A subscriber that nothing happens around stays unnamed.
             * @pre gLock is held
             */
            void learn() const noexcept
            {
                if (unnamed == 0U)
                    return;
                for (const Record* record = head; record != nullptr; record = record->next)
                    if (record->learn != nullptr)
                        record->learn();
            }

            ~Registry();
        };

        /** The process's audit.
         *  A function-local static on purpose: it is created by the first recorded event, which may be the constructor
         *  of a subscriber with static storage, and is therefore destroyed after that subscriber, so the report at
         *  exit sees every subscriber leave. It is write-only diagnostic state: no behaviour depends on it.
         */
        inline Registry& registry() noexcept
        {
            static Registry instance;
            return instance;
        }

        /// The record after `current` (the first when null), or null at the end
        inline Record* following(Record* current) noexcept
        {
            Guard guard;
            return current != nullptr ? current->next : registry().head;
        }

        /// @return The findings recorded so far, across every message type
        inline uint32_t findings() noexcept
        {
            Guard guard;
            registry().learn();
            uint32_t count = 0;
            for (const Record* record = registry().head; record != nullptr; record = record->next)
            {
                Line line(nullptr, 0U);
                record->findings(line);
                count += line.count();
            }
            return count;
        }

        /// Print one section of every record, a line at a time; no lock is held while a line is printed
        inline void print(void (*Record::*section)(Line&) noexcept) noexcept
        {
            char buffer[cLine];
            for (Record* record = following(nullptr); record != nullptr; record = following(record))
                for (uint32_t index = 0;; ++index)
                {
                    Line line(buffer, index);
                    {
                        Guard guard;
                        (record->*section)(line);
                    }
                    if (!line.found())
                        break;
                    SUB0PUB_AUDIT_PRINT(buffer);
                }
        }

        /// Print the whole report; @return the number of findings
        inline uint32_t report() noexcept
        {
            const uint32_t count = findings();
            char buffer[cLine];
            Line heading(buffer, 0U);
            if (heading.begin())
            {
                heading.text("sub0pub audit: ");
                heading.counted(count, "finding", "findings");
            }
            SUB0PUB_AUDIT_PRINT(buffer);
            print(&Record::findings);
            print(&Record::detail);
            return count;
        }

        inline Registry::~Registry()
        {
            if (head == nullptr)
                return;
            const uint32_t count = report();
            SUB0PUB_AUDIT_EXIT(count);
        }

        /** Everything the audit knows about one message type. It does not depend on the type's configuration, so
         *  every translation unit feeds the same ledger. */
        template<class Data>
        class Ledger
        {
        public:
            /** A runtime subscriber registered
             * @param subscriber  Its Subscribe<Data> base; identifies it in left() and delivered()
             * @param capacity    The capacity of the type's subscription table
             * @param learn       Names the subscribers this ledger holds (see Registry::learn())
             */
            static void joined(const void* subscriber, uint32_t capacity, void (*learn)() noexcept) noexcept
            {
                Guard guard;
                link();
                registry().learn(); // before this one is added: it is still under construction
                record_.learn = learn;
                capacity_ = capacity;
                if (++present_ > peak_)
                    peak_ = present_;
                if (Receiver* const receiver = add(subscriber))
                {
                    receiver->runtime = true;
                    receiver->joinedAt = publications_;
#if SUB0PUB_AUDIT_RTTI
                    ++registry().unnamed;
#endif
                }
            }

            /** A subscriber stopped receiving, or is being destroyed
             * @param wasSubscribed  Whether it was registered until now
             */
            static void left(const void* subscriber, bool wasSubscribed) noexcept
            {
                Guard guard;
                if (wasSubscribed && present_ != 0U)
                    --present_;
                if (Receiver* const receiver = find(subscriber))
                {
                    named(*receiver, receiver->type); // it will not be named now: stop looking for it
                    receiver->present = false;
                    receiver->leftAt = publications_;
                }
                if (linked_)
                    registry().learn(); // after this one left: it may already be partly destroyed
            }

            /// A subscription was refused because the table was full
            static void refused(uint32_t capacity) noexcept
            {
                Guard guard;
                link();
                capacity_ = capacity;
                ++refused_;
            }

            /// Delivery reached a runtime subscriber; `type` is its dynamic type, when that could be read
            static void delivered(const void* subscriber, TypeId type) noexcept
            {
                Guard guard;
                if (Receiver* const receiver = find(subscriber))
                {
                    if (type != nullptr)
                        named(*receiver, type);
                    ++receiver->deliveries;
                }
            }

            /// Delivery reached a receiver that the type's StaticTo / StaticFirst list names
            static void bound(const void* receiver, const char* typeSignature, const char* objectSignature) noexcept
            {
                Guard guard;
                link();
                Receiver* entry = find(receiver);
                if (entry == nullptr)
                    entry = add(receiver);
                if (entry == nullptr)
                    return;
                entry->signature = typeSignature;
                entry->object = objectSignature;
                ++entry->deliveries;
            }

            /// A subscriber of a statically wired type was constructed that the type's StaticTo list does not name
            static void unlisted(const void* subscriber) noexcept
            {
                Guard guard;
                link();
                if (Receiver* const receiver = add(subscriber))
                {
                    receiver->unlisted = true;
                    receiver->joinedAt = publications_;
                }
            }

            /** One publication
             * @param publisher       typeSignature() of the publishing class, or null when it is not known
             * @param boundReceivers  The receivers the type's StaticTo / StaticFirst list delivers to
             * @param unheardAllowed  Whether the type is configured to allow a publication nobody receives
             */
            static void published(const char* publisher, uint32_t boundReceivers, bool unheardAllowed) noexcept
            {
                Guard guard;
                link();
                registry().learn();
                unheardAllowed_ = unheardAllowed_ || unheardAllowed;
                const bool unheard = (boundReceivers + present_) == 0U;
                uint32_t i = 0;
                while (i < publisherCount_ && !same(publishers_[i].signature, publisher))
                    ++i;
                if (i == publisherCount_ && publisherCount_ < cPublishers)
                    publishers_[publisherCount_++] = Publisher{publisher, 0U, 0U};
                if (i < publisherCount_)
                {
                    ++publishers_[i].publications;
                    publishers_[i].unheard += unheard ? 1U : 0U;
                }
                ++publications_;
                unheard_ += unheard ? 1U : 0U;
            }

            /** Name the registered runtime subscribers that have no type yet
             * @param typeOf  Reads a subscriber's dynamic type from its Subscribe<Data> base; null if not readable yet
             * @pre gLock is held
             */
            static void learn(TypeId (*typeOf)(const void* subscriber) noexcept) noexcept
            {
                for (uint32_t i = 0; i < count_; ++i)
                    if (waiting(receivers_[i]))
                        if (const TypeId type = typeOf(receivers_[i].address))
                            named(receivers_[i], type);
            }

        private:
            struct Receiver
            {
                const void* address;   // non-owning; compared and printed. Dereferenced only by learn(), while present
                TypeId type;           // a runtime subscriber's dynamic type, once it could be read
                const char* signature; // a listed receiver's typeSignature(), else null
                const char* object;    // a listed receiver's boundSignature(), else null
                uint64_t deliveries;
                uint64_t joinedAt;     // publications of the type before it joined
                uint64_t leftAt;       // publications of the type before it left
                bool present;
                bool runtime;          // registered with the runtime broker
                bool unlisted;         // a subscriber of a wired type that its StaticTo list does not name
            };

            struct Publisher
            {
                const char* signature; // typeSignature() of the publishing class; null when not known
                uint64_t publications;
                uint64_t unheard;
            };

            /// Whether the audit is still looking for this subscriber's type
            static bool waiting(const Receiver& receiver) noexcept
            {
                return receiver.runtime && receiver.present && receiver.type == nullptr;
            }

            /// Give a subscriber its type, or (null, as it leaves) give up on it
            static void named(Receiver& receiver, TypeId type) noexcept
            {
#if SUB0PUB_AUDIT_RTTI
                if (waiting(receiver) && registry().unnamed != 0U)
                    --registry().unnamed;
#endif
                receiver.type = type;
            }

            static bool same(const char* a, const char* b) noexcept
            {
                return a == b || (a != nullptr && b != nullptr && std::strcmp(a, b) == 0);
            }

            static void link() noexcept
            {
                if (!linked_)
                {
                    linked_ = true;
                    registry().add(record_);
                }
            }

            /// The entry of a receiver that is here now
            static Receiver* find(const void* address) noexcept
            {
                for (uint32_t i = count_; i-- != 0U;)
                    if (receivers_[i].address == address && receivers_[i].present)
                        return &receivers_[i];
                return nullptr;
            }

            static Receiver* add(const void* address) noexcept
            {
                if (count_ >= cReceivers)
                {
                    truncated_ = true;
                    return nullptr;
                }
                receivers_[count_] = Receiver{address, nullptr, nullptr, nullptr, 0U, 0U, 0U, true, false, false};
                return &receivers_[count_++];
            }

            static bool leftEarly(const Receiver& receiver) noexcept
            {
                return !receiver.present && receiver.leftAt != publications_;
            }

            /// Whether every receiver was there for every publication
            static bool fixedSet() noexcept
            {
                for (uint32_t i = 0; i < count_; ++i)
                    if (receivers_[i].joinedAt != 0U || leftEarly(receivers_[i]) || receivers_[i].unlisted)
                        return false;
                return !truncated_;
            }

            static void describe(Line& line, const Receiver& receiver) noexcept
            {
                if (receiver.object != nullptr)
                {
                    line.object(receiver.object);
                    line.text(" (");
                    line.type(receiver.signature);
                    line.text(")");
                }
                else
                    line.dynamic(receiver.type);
                line.text(" at ");
                line.address(receiver.address);
            }

            static void heading(Line& line) noexcept
            {
                line.text("  ");
                line.type(typeSignature<Data>());
                line.text(": ");
            }

            static void findings(Line& line) noexcept
            {
                if (unheard_ != 0U && !unheardAllowed_ && line.begin())
                {
                    heading(line);
                    line.number(unheard_);
                    line.text(" of ");
                    line.counted(publications_, "publication", "publications");
                    line.text(" reached no receiver");
                    for (uint32_t i = 0; i < publisherCount_; ++i)
                        if (publishers_[i].unheard != 0U)
                        {
                            line.text("; ");
                            line.number(publishers_[i].unheard);
                            line.text(" by ");
                            publisher(line, publishers_[i]);
                        }
                }
                if (refused_ != 0U && line.begin())
                {
                    heading(line);
                    line.counted(refused_, "subscription", "subscriptions");
                    line.text(" refused: its table of ");
                    line.number(capacity_);
                    line.text(" was full");
                }
                for (uint32_t i = 0; i < count_; ++i)
                {
                    const Receiver& receiver = receivers_[i];
                    const bool neverReceived = receiver.deliveries == 0U && publications_ != 0U;
                    if ((!receiver.unlisted && !neverReceived) || !line.begin())
                        continue;
                    heading(line);
                    describe(line, receiver);
                    if (receiver.unlisted)
                        line.text(" subscribes to it but is not in its StaticTo list, so it is never called");
                    else if (receiver.joinedAt == publications_)
                        line.text(" never received it: it subscribed after the last publication");
                    else if (!receiver.present && receiver.leftAt == 0U)
                        line.text(" never received it: it left before the first publication");
                    else if (!receiver.present && receiver.leftAt == receiver.joinedAt)
                    {
                        line.text(" never received it: it was subscribed only between publications ");
                        line.number(receiver.joinedAt);
                        line.text(" and ");
                        line.number(receiver.joinedAt + 1U);
                    }
                    else
                        line.text(" never received it, although it was subscribed while it was published");
                }
                if (publications_ == 0U && count_ != 0U && line.begin())
                {
                    heading(line);
                    line.text("never published, although ");
                    line.counted(count_, "receiver", "receivers");
                    line.text(" subscribed to it");
                }
            }

            static void publisher(Line& line, const Publisher& entry) noexcept
            {
                if (entry.signature != nullptr)
                    line.type(entry.signature);
                else
                    line.text("a publisher the audit could not name");
            }

            static void detail(Line& line) noexcept
            {
                if (line.begin())
                {
                    heading(line);
                    line.counted(publications_, "publication", "publications");
                    if (unheard_ != 0U && unheardAllowed_)
                    {
                        line.text(" (");
                        line.number(unheard_);
                        line.text(" reached nobody, which its configuration allows)");
                    }
                    if (capacity_ != 0U)
                    {
                        line.text("; runtime table peaked at ");
                        line.number(peak_);
                        line.text(" of ");
                        line.number(capacity_);
                    }
                }
                for (uint32_t i = 0; i < publisherCount_; ++i)
                    if (line.begin())
                    {
                        line.text("    published by ");
                        publisher(line, publishers_[i]);
                        line.text(": ");
                        line.number(publishers_[i].publications);
                    }
                for (uint32_t i = 0; i < count_; ++i)
                {
                    const Receiver& receiver = receivers_[i];
                    if (!line.begin())
                        continue;
                    line.text("    ");
                    line.number(i + 1U);
                    line.text(". ");
                    describe(line, receiver);
                    line.text(": ");
                    line.counted(receiver.deliveries, "delivery", "deliveries");
                    if (receiver.unlisted)
                        line.text(", not listed");
                    else if (receiver.object != nullptr)
                        line.text(", wired");
                    if (receiver.joinedAt != 0U && !receiver.unlisted)
                    {
                        line.text(", joined after publication ");
                        line.number(receiver.joinedAt);
                    }
                    if (leftEarly(receiver))
                    {
                        line.text(", left after publication ");
                        line.number(receiver.leftAt);
                    }
                }
                if (truncated_ && line.begin())
                {
                    line.text("    more than ");
                    line.number(cReceivers);
                    line.text(" receivers: the rest were not recorded");
                }
                // No advice for a type that turned a subscriber away: its receivers are not what the program meant
                if (count_ != 0U && publications_ != 0U && capacity_ != 0U && refused_ == 0U && line.begin())
                    line.text(fixedSet()
                        ? "    its receivers never changed: a candidate for sub0::StaticTo, in this order, if they have static storage"
                        : "    receivers joined or left while it was being published: keep it brokered, or sub0::StaticFirst for those that stayed");
            }

            inline static Receiver receivers_[cReceivers] = {};
            inline static Publisher publishers_[cPublishers] = {};
            inline static uint32_t count_ = 0;
            inline static uint32_t publisherCount_ = 0;
            inline static uint64_t publications_ = 0;
            inline static uint64_t unheard_ = 0;
            inline static uint32_t present_ = 0;   // runtime subscribers registered now, recorded or not
            inline static uint32_t peak_ = 0;
            inline static uint32_t capacity_ = 0;
            inline static uint32_t refused_ = 0;
            inline static bool unheardAllowed_ = false;
            inline static bool truncated_ = false;
            inline static bool linked_ = false;
            inline static Record record_{&Ledger::findings, &Ledger::detail, nullptr, nullptr};
        };
    } // END: audit
    } // END: detail

    /** The number of findings the audit has recorded so far
     * @return Findings across every message type; subscribers that still exist are judged as they stand, so call it
     *         after the work under test. Always 0 in a build without SUB0PUB_AUDIT.
     */
    [[nodiscard]] inline uint32_t auditFindings() noexcept { return detail::audit::findings(); }

    /** Write the audit's report now, a line at a time through SUB0PUB_AUDIT_PRINT
     * @return The number of findings reported. Writes nothing and returns 0 in a build without SUB0PUB_AUDIT.
     * @remark The same report is written when the program exits.
     */
    inline uint32_t auditReport() noexcept { return detail::audit::report(); }
} // END: sub0

#else // SUB0PUB_AUDIT

namespace sub0
{
    /// @return 0: this build records nothing (see SUB0PUB_AUDIT)
    [[nodiscard]] constexpr uint32_t auditFindings() noexcept { return 0U; }

    /// @return 0, and writes nothing: this build records nothing (see SUB0PUB_AUDIT)
    constexpr uint32_t auditReport() noexcept { return 0U; }
} // END: sub0

#endif // SUB0PUB_AUDIT

#endif // CROG_SUB0PUB_AUDIT_HPP
