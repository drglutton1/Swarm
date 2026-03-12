#pragma once

#include <cstdint>

#include "../core/chromosome.h"
#include "../core/genome.h"
#include "../util/rng.h"

namespace swarm::evolution {

struct MutationPolicy {
    double point_mutation_rate = 0.05;
    double meta_mutation_rate = 0.03;
    double structural_mutation_rate = 0.02;
};

void mutate_genome(swarm::core::Genome& genome, std::uint32_t ocean_size, swarm::util::Rng& rng, const MutationPolicy& policy = {});
void mutate_chromosome(swarm::core::Chromosome& chromosome, std::size_t state_size, std::uint32_t ocean_size, swarm::util::Rng& rng, const MutationPolicy& policy = {});

} // namespace swarm::evolution
