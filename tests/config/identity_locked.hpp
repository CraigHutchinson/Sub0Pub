/** A project default with a lock, for identity_b.cpp alone: its translation unit names this header as its
 *  SUB0PUB_CONFIG_HEADER, so `config<Opts...>` there starts from a different base than in identity_a.cpp. */
#pragma once

#include <mutex>

struct IdentityLocked : sub0::with<sub0::Builtin, sub0::LockWith<std::mutex>> {};
#define SUB0PUB_DEFAULT_CONFIG IdentityLocked
