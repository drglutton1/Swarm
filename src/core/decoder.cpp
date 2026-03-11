#include "decoder.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>

namespace swarm::core {
namespace {

float consume_parameter(const Ocean& ocean, const Genome& genome, std::size_t& cursor) {
    if (genome.parameter_indices.empty()) {
        return 0.0f;
    }
    const std::uint32_t ocean_index = genome.parameter_indices[cursor % genome.parameter_indices.size()];
    ++cursor;
    return ocean.get(ocean_index);
}

} // namespace

Decision Decoder::decode(const Ocean& ocean, const Genome& genome, const DecoderConfig& config, const PokerStateVector& state) const {
    if (config.state_size == 0 || state.features.size() != config.state_size) {
        throw std::invalid_argument("decoder state size mismatch");
    }

    const std::size_t hidden_units = std::max<std::size_t>(1, genome.hidden_units == 0 ? config.hidden_units : genome.hidden_units);
    std::vector<float> hidden(hidden_units, 0.0f);
    std::size_t cursor = 0;

    for (std::size_t h = 0; h < hidden_units; ++h) {
        float activation = consume_parameter(ocean, genome, cursor);
        for (std::size_t i = 0; i < state.features.size(); ++i) {
            activation += state.features[i] * consume_parameter(ocean, genome, cursor);
        }
        hidden[h] = tanh_like(activation);
    }

    std::array<float, 3> logits {0.0f, 0.0f, 0.0f};
    for (std::size_t a = 0; a < logits.size(); ++a) {
        float value = consume_parameter(ocean, genome, cursor);
        for (float h : hidden) {
            value += h * consume_parameter(ocean, genome, cursor);
        }
        logits[a] = value;
    }

    float max_logit = *std::max_element(logits.begin(), logits.end());
    std::array<float, 3> scores {0.0f, 0.0f, 0.0f};
    float total = 0.0f;
    for (std::size_t i = 0; i < logits.size(); ++i) {
        scores[i] = std::exp(logits[i] - max_logit);
        total += scores[i];
    }
    if (total <= std::numeric_limits<float>::epsilon()) {
        scores = {0.0f, 1.0f, 0.0f};
        total = 1.0f;
    }
    for (float& score : scores) {
        score /= total;
    }

    std::size_t best_index = 0;
    for (std::size_t i = 1; i < scores.size(); ++i) {
        if (scores[i] > scores[best_index]) {
            best_index = i;
        }
    }

    const float raise_signal = sigmoid(consume_parameter(ocean, genome, cursor) + hidden.front() * genome.aggressiveness_gene);
    const float confidence_signal = sigmoid(consume_parameter(ocean, genome, cursor) + hidden.back() * genome.confidence_gene);
    const float stack_cap = std::max(state.stack, 0.0f);
    const float min_raise = std::max(state.min_raise, 0.0f);
    const float raise_room = std::max(0.0f, stack_cap - state.to_call);
    const float raise_amount = std::clamp(min_raise + raise_signal * (state.pot + min_raise), 0.0f, raise_room);

    Decision decision;
    decision.action_scores = scores;
    decision.action = static_cast<ActionType>(best_index);
    decision.raise_amount = decision.action == ActionType::raise ? raise_amount : 0.0f;
    decision.confidence = std::clamp((scores[best_index] + confidence_signal) * 0.5f, 0.0f, 1.0f);
    return decision;
}

float Decoder::tanh_like(float x) noexcept {
    return std::tanh(x);
}

float Decoder::sigmoid(float x) noexcept {
    return 1.0f / (1.0f + std::exp(-x));
}

} // namespace swarm::core
