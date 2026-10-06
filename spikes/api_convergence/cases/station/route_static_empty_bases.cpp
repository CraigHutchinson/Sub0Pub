/** Case station. Candidate "typed route", StaticTo, with the participants that have two Sub0Pub bases marked
 *  __declspec(empty_bases) on the MSVC ABI. That ABI otherwise gives a class with two empty bases two bytes and
 *  zeroes them at construction; this variant shows the marked form is hand-written code there too. */
#if defined(_MSC_VER)
#define ROUTE_LAYOUT __declspec(empty_bases)
#endif
#define ROUTE_TOPOLOGY ROUTE_STATIC
#include "route_app.hpp"
