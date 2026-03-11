#pragma once

#include <cstddef>

#include "chromosome.h"

namespace swarm::core {

enum class GovernanceMode {
    alpha_led,
    democratic,
};

class Swarm {
public:
    Swarm() = default;
    Swarm(Chromosome odd_chromosome, Chromosome even_chromosome);

    [[nodiscard]] GovernanceMode governance_for_turn(std::size_t hand_number) const noexcept;
    [[nodiscard]] Decision decide(const Ocean& ocean, const PokerStateVector& state, std::size_t hand_number) const;

    [[nodiscard]] const Chromosome& odd_chromosome() const noexcept;
    [[nodiscard]] const Chromosome& even_chromosome() const noexcept;

private:
    Chromosome odd_chromosome_;
    Chromosome even_chromosome_;
};

} // namespace swarm::core
