#pragma once

#include <cstddef>

#include "decoder.h"
#include "genome.h"
#include "ocean.h"

namespace swarm::core {

class Agent {
public:
    Agent() = default;
    Agent(Genome genome, DecoderConfig config);

    [[nodiscard]] const Genome& genome() const noexcept;
    [[nodiscard]] const DecoderConfig& config() const noexcept;

    [[nodiscard]] Decision decide(const Ocean& ocean, const PokerStateVector& state) const;

private:
    Genome genome_;
    DecoderConfig config_;
    Decoder decoder_;
};

} // namespace swarm::core
