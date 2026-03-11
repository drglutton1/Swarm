#pragma once

#include <cstddef>

#include "chromosome.h"
#include "../util/rng.h"

namespace swarm::core {

enum class GovernanceMode {
    alpha_led,
    democratic,
};

class Swarm {
public:
    Swarm() = default;
    Swarm(Chromosome first_chromosome, Chromosome second_chromosome);

    static Swarm random(std::uint32_t ocean_size, std::size_t state_size, swarm::util::Rng& rng);

    [[nodiscard]] GovernanceMode governance_mode() const noexcept;
    [[nodiscard]] Decision decide(const Ocean& ocean, const PokerStateVector& state, std::size_t hand_number = 0) const;

    [[nodiscard]] const Chromosome& first_chromosome() const noexcept;
    [[nodiscard]] const Chromosome& second_chromosome() const noexcept;
    [[nodiscard]] std::size_t total_agents() const noexcept;

private:
    Chromosome first_chromosome_;
    Chromosome second_chromosome_;
};

} // namespace swarm::core
