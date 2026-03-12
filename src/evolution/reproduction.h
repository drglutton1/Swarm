#pragma once

#include <optional>
#include <string>

#include "crossover.h"
#include "lifecycle.h"
#include "mutation.h"

namespace swarm::evolution {

struct ReproductionPolicy {
    LifecyclePolicy lifecycle {};
    CrossoverPolicy crossover {};
    MutationPolicy mutation {};
};

struct ReproductionResult {
    bool success = false;
    std::string reason;
    std::optional<swarm::core::Swarm> offspring;
};

[[nodiscard]] bool pairing_parity_allowed(const swarm::core::Swarm& first, const swarm::core::Swarm& second);
[[nodiscard]] ReproductionResult reproduce(
    swarm::core::Swarm& first,
    swarm::core::Swarm& second,
    std::size_t state_size,
    std::uint32_t ocean_size,
    swarm::util::Rng& rng,
    const ReproductionPolicy& policy = {});

} // namespace swarm::evolution
