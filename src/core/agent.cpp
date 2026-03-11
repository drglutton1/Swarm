#include "agent.h"

namespace swarm::core {

Agent::Agent(Genome genome, DecoderConfig config) : genome_(std::move(genome)), config_(config) {
    if (config_.hidden_units == 0) {
        config_.hidden_units = genome_.hidden_units;
    }
    if (config_.action_count == 0) {
        config_.action_count = genome_.action_count;
    }
}

const Genome& Agent::genome() const noexcept {
    return genome_;
}

const DecoderConfig& Agent::config() const noexcept {
    return config_;
}

Decision Agent::decide(const Ocean& ocean, const PokerStateVector& state) const {
    return decoder_.decode(ocean, genome_, config_, state);
}

} // namespace swarm::core
