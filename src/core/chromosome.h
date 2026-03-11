#pragma once

#include <cstdint>
#include <vector>

#include "agent.h"

namespace swarm::core {

class Chromosome {
public:
    Chromosome() = default;
    explicit Chromosome(std::vector<Agent> agents, Genome blueprint = {});

    static Chromosome spawn_from_blueprint(const Genome& blueprint, std::size_t agent_count, const DecoderConfig& config, std::uint32_t ocean_size);

    [[nodiscard]] const std::vector<Agent>& agents() const noexcept;
    [[nodiscard]] const Genome& blueprint() const noexcept;
    [[nodiscard]] bool empty() const noexcept;
    [[nodiscard]] std::size_t size() const noexcept;

    [[nodiscard]] Decision decide(const Ocean& ocean, const PokerStateVector& state) const;

private:
    std::vector<Agent> agents_;
    Genome blueprint_;
};

} // namespace swarm::core
