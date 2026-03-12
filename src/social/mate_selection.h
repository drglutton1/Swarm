#pragma once

#include <optional>
#include <string>
#include <vector>

#include "face.h"
#include "info_exchange.h"

namespace swarm::social {

struct SocialKnowledge {
    PublicFace public_face {};
    std::optional<InterpretedInfo> bankroll;
    std::optional<InterpretedInfo> reproductive_readiness;
    std::optional<InterpretedInfo> honesty;
};

struct MateSelectionResult {
    const swarm::core::Swarm* partner = nullptr;
    double score = 0.0;
    std::string reason;
};

[[nodiscard]] MateSelectionResult select_partner(
    const swarm::core::Swarm& seeker,
    const std::vector<const swarm::core::Swarm*>& candidates,
    const std::vector<SocialKnowledge>& knowledge,
    const swarm::evolution::LifecyclePolicy& lifecycle = {});

} // namespace swarm::social
