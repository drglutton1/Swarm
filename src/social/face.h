#pragma once

#include <cstdint>

#include "../core/swarm.h"

namespace swarm::social {

struct PublicFace {
    std::uint64_t swarm_id = 0;
    std::uint64_t hands_played = 0;
    std::size_t size = 0;
    std::uint32_t offspring_count = 0;
    float self_reported_risk = 0.0f;
    float self_reported_confidence = 0.0f;
    float self_reported_honesty = 0.0f;
};

[[nodiscard]] PublicFace make_public_face(const swarm::core::Swarm& swarm);

} // namespace swarm::social
