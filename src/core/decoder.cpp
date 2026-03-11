#include "decoder.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>

namespace swarm::core {
namespace {

float substrate_signal(const Ocean& ocean, std::uint32_t index, std::uint32_t radius) {
    return 0.7f * ocean.window_mean(index, radius) + 0.3f * ocean.local_gradient(index);
}

float read_or_zero(const std::vector<std::uint32_t>& indices, std::size_t position, const Ocean& ocean, std::uint32_t radius) {
    if (indices.empty()) {
        return 0.0f;
    }
    return substrate_signal(ocean, indices[position % indices.size()], radius);
}

} // namespace

Decision Decoder::decode(const Ocean& ocean, const Genome& genome, const DecoderConfig& config, const PokerStateVector& state) const {
    if (config.state_size == 0 || state.features.size() != config.state_size) {
        throw std::invalid_argument("decoder state size mismatch");
    }
    if (genome.empty()) {
        throw std::invalid_argument("decoder requires non-empty genome");
    }

    const std::size_t hidden_units = std::max<std::size_t>(1, genome.hidden_units == 0 ? config.hidden_units : genome.hidden_units);
    const std::uint32_t radius = std::max<std::uint32_t>(1, genome.ocean_window_radius);
    std::vector<float> hidden(hidden_units, 0.0f);

    for (std::size_t h = 0; h < hidden_units; ++h) {
        float activation = read_or_zero(genome.hidden_indices, h * 2, ocean, radius);
        for (std::size_t i = 0; i < state.features.size(); ++i) {
            const float sensory = read_or_zero(genome.sensory_indices, i, ocean, radius);
            const float hidden_weight = read_or_zero(genome.hidden_indices, h * 2 + 1 + i, ocean, radius);
            activation += state.features[i] * sensory * hidden_weight;
        }
        hidden[h] = tanh_like(activation);
    }

    std::array<float, 3> logits {0.0f, 0.0f, 0.0f};
    for (std::size_t a = 0; a < logits.size(); ++a) {
        float value = read_or_zero(genome.action_indices, a * 2, ocean, radius);
        for (std::size_t h = 0; h < hidden.size(); ++h) {
            value += hidden[h] * read_or_zero(genome.action_indices, a * 2 + 1 + h, ocean, radius);
        }
        logits[a] = value;
    }

    logits[0] -= state.to_call * 0.03f + genome.risk_gene * 0.1f;
    logits[1] += (state.stack > 0.0f ? (state.pot / std::max(state.stack, 1.0f)) : 0.0f) * 0.15f;
    logits[2] += genome.alpha_bias * 0.2f + genome.risk_gene * 0.35f;

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

    const float raise_signal = sigmoid(read_or_zero(genome.modulation_indices, 0, ocean, radius) + hidden.front() * genome.risk_gene + state.pot * 0.01f);
    const float confidence_signal = sigmoid(read_or_zero(genome.modulation_indices, 1, ocean, radius) + hidden.back() * genome.confidence_gene);
    const float stack_cap = std::max(state.stack, 0.0f);
    const float min_raise = std::max(state.min_raise, 0.0f);
    const float raise_room = std::max(0.0f, stack_cap - state.to_call);
    const float raise_span = std::max(state.pot + min_raise, min_raise);
    const float raise_amount = std::clamp(min_raise + raise_signal * raise_span, 0.0f, raise_room);

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
