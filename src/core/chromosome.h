#pragma once

#include <vector>

#include "agent.h"

namespace swarm::core {

class Chromosome {
public:
    Chromosome() = default;
    explicit Chromosome(std::vector<Agent> agents);

    [[nodiscard]] const std::vector<Agent>& agents() const noexcept;
    [[nodiscard]] bool empty() const noexcept;

    [[nodiscard]] Decision decide(const Ocean& ocean, const PokerStateVector& state) const;

private:
    std::vector<Agent> agents_;
};

} // namespace swarm::core
