#include "crossover.h"

#include <algorithm>

namespace swarm::evolution {
namespace {

template <typename T>
void maybe_swap(T& first, T& second, swarm::util::Rng& rng) {
    if (rng.uniform_real() < 0.5) {
        std::swap(first, second);
    }
}

} // namespace

std::pair<swarm::core::Genome, swarm::core::Genome> maybe_crossover(
    const swarm::core::Genome& first,
    const swarm::core::Genome& second,
    swarm::util::Rng& rng,
    const CrossoverPolicy& policy) {
    swarm::core::Genome left = first;
    swarm::core::Genome right = second;

    if (rng.uniform_real() >= policy.crossover_probability) {
        return {left, right};
    }

    maybe_swap(left.alpha_bias, right.alpha_bias, rng);
    maybe_swap(left.risk_gene, right.risk_gene, rng);
    maybe_swap(left.confidence_gene, right.confidence_gene, rng);
    maybe_swap(left.hidden_units, right.hidden_units, rng);
    maybe_swap(left.ocean_window_radius, right.ocean_window_radius, rng);
    maybe_swap(left.hidden_indices, right.hidden_indices, rng);
    maybe_swap(left.modulation_indices, right.modulation_indices, rng);

    left.action_count = 3;
    right.action_count = 3;
    return {left, right};
}

} // namespace swarm::evolution
