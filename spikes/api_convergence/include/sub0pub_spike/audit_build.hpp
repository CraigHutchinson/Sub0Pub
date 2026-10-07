#pragma once

/** @file audit_build.hpp
 *  Turns a build into an audit build (see audit.hpp). Name it as the project header from the build system:
 *
 *      -DSUB0PUB_CONFIG_HEADER="sub0pub_spike/audit_build.hpp"
 *
 *  It makes the audit's broker the default for every brokered message type and switches on the recording in
 *  publish(), so no source changes. sub0pub/config.hpp includes it part-way through, which is why it only names
 *  the broker: the definition arrives with sub0pub_spike/topology.hpp.
 *
 *  A project that already has a SUB0PUB_CONFIG_HEADER of its own would include this file from it and derive its
 *  defaults from AuditDefaults; the spike does not exercise that.
 */

#define SUB0PUB_SPIKE_AUDIT true

namespace sub0::spike
{
    namespace detail
    {
        template<class Data, class Config> class AuditBroker;
    }

    /** The project default of an audit build: the library's defaults, delivered by the audit's broker. */
    struct AuditDefaults : sub0::with<sub0::Builtin, sub0::Implementation<detail::AuditBroker>>
    {
    };
} // namespace sub0::spike

#define SUB0PUB_DEFAULT_CONFIG sub0::spike::AuditDefaults
