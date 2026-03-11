#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

#include "genome.h"
#include "ocean.h"

namespace swarm::core {

enum class ActionType : std::uint8_t {
    fold = 0,
    check_call = 1,
    raise = 2,
};

struct PokerStateVector {
    std::vector<float> features;
    float to_call {0.0f};
    float min_raise {0.0f};
    float pot {0.0f};
    float stack {0.0f};
};

struct Decision {
    std::array<float, 3> action_scores {0.0f, 0.0f, 0.0f};
    ActionType action {ActionType::check_call};
    float raise_amount {0.0f};
    float confidence {0.0f};
};

struct DecoderConfig {
    std::size_t state_size {0};
    std::uint32_t hidden_units {4};
    std::uint32_t action_count {3};
};

class Decoder {
public:
    [[nodiscard]] Decision decode(const Ocean& ocean, const Genome& genome, const DecoderConfig& config, const PokerStateVector& state) const;

private:
    [[nodiscard]] static float tanh_like(float x) noexcept;
    [[nodiscard]] static float sigmoid(float x) noexcept;
};

} // namespace swarm::core
