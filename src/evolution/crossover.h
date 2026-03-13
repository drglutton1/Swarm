#pragma once

#include <utility>

#include "../core/genome.h"
#include "../util/rng.h"

namespace swarm::evolution {

struct CrossoverPolicy {
    double crossover_probability = 0.01;
};

[[nodiscard]] std::pair<swarm::core::Genome, swarm::core::Genome> maybe_crossover(
    const swarm::core::Genome& first,
    const swarm::core::Genome& second,
    swarm::util::Rng& rng,
    const CrossoverPolicy& policy = {});

} // namespace swarm::evolution
