#pragma once

#include <cstdint>

#include "../core/swarm.h"
#include "../evolution/lifecycle.h"
#include "../util/rng.h"

namespace swarm::social {

enum class PrivateInfoField {
    bankroll,
    reproductive_readiness,
    honesty_gene,
    skepticism_gene,
    confidence_gene,
};

struct InfoRequest {
    std::uint64_t requester_id = 0;
    std::uint64_t responder_id = 0;
    PrivateInfoField field = PrivateInfoField::bankroll;
};

struct InfoResponse {
    InfoRequest request {};
    bool refused = false;
    bool truthful = false;
    double reported_value = 0.0;
    double actual_value = 0.0;
};

struct InterpretedInfo {
    InfoResponse response {};
    double believed_value = 0.0;
    double trust_weight = 0.0;
};

[[nodiscard]] double private_info_value(const swarm::core::Swarm& swarm, PrivateInfoField field, const swarm::evolution::LifecyclePolicy& lifecycle = {});
[[nodiscard]] InfoResponse request_private_info(
    const swarm::core::Swarm& requester,
    const swarm::core::Swarm& responder,
    PrivateInfoField field,
    swarm::util::Rng& rng,
    const swarm::evolution::LifecyclePolicy& lifecycle = {});
[[nodiscard]] InterpretedInfo interpret_response(const swarm::core::Swarm& receiver, const InfoResponse& response);

} // namespace swarm::social
