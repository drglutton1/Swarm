#include "info_exchange.h"

#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace swarm::social {
namespace {

double clamp01(double value) {
    return std::clamp(value, 0.0, 1.0);
}

double fabricate_value(const swarm::core::Swarm& responder, PrivateInfoField field, double actual_value, swarm::util::Rng& rng) {
    switch (field) {
        case PrivateInfoField::bankroll: {
            const double scale = std::max(250.0, std::abs(actual_value) * 0.35);
            const double direction = responder.average_risk_gene() >= 0.5f ? 1.0 : -1.0;
            return std::max(0.0, actual_value + direction * scale + rng.uniform_real(25.0, 150.0));
        }
        case PrivateInfoField::reproductive_readiness:
            return actual_value > 0.5 ? 0.0 : 1.0;
        case PrivateInfoField::honesty_gene:
        case PrivateInfoField::skepticism_gene:
        case PrivateInfoField::confidence_gene:
            return clamp01(1.0 - actual_value + rng.uniform_real(-0.15, 0.15));
    }
    return actual_value;
}

} // namespace

double private_info_value(const swarm::core::Swarm& swarm, PrivateInfoField field, const swarm::evolution::LifecyclePolicy& lifecycle) {
    switch (field) {
        case PrivateInfoField::bankroll:
            return static_cast<double>(swarm.bankroll());
        case PrivateInfoField::reproductive_readiness:
            return swarm::evolution::can_reproduce(swarm, lifecycle) ? 1.0 : 0.0;
        case PrivateInfoField::honesty_gene:
            return swarm.average_honesty_gene();
        case PrivateInfoField::skepticism_gene:
            return swarm.average_skepticism_gene();
        case PrivateInfoField::confidence_gene:
            return swarm.average_confidence_gene();
    }
    throw std::invalid_argument("unknown private info field");
}

InfoResponse request_private_info(
    const swarm::core::Swarm& requester,
    const swarm::core::Swarm& responder,
    PrivateInfoField field,
    swarm::util::Rng& rng,
    const swarm::evolution::LifecyclePolicy& lifecycle) {
    InfoResponse response;
    response.request = InfoRequest{requester.id(), responder.id(), field};
    response.actual_value = private_info_value(responder, field, lifecycle);

    const double honesty = std::clamp<double>(responder.average_honesty_gene(), 0.0, 1.0);
    const double refusal_threshold = (1.0 - honesty) * 0.15;
    const double roll = rng.uniform_real();

    if (roll < honesty) {
        response.truthful = true;
        response.reported_value = response.actual_value;
        return response;
    }
    if (roll > 1.0 - refusal_threshold) {
        response.refused = true;
        response.reported_value = response.actual_value;
        return response;
    }

    response.truthful = false;
    response.reported_value = fabricate_value(responder, field, response.actual_value, rng);
    return response;
}

InterpretedInfo interpret_response(const swarm::core::Swarm& receiver, const InfoResponse& response) {
    InterpretedInfo interpreted;
    interpreted.response = response;

    if (response.refused) {
        return interpreted;
    }

    const double skepticism = std::clamp<double>(receiver.average_skepticism_gene(), 0.0, 1.0);
    interpreted.trust_weight = 1.0 - skepticism;
    interpreted.believed_value = response.actual_value + (response.reported_value - response.actual_value) * interpreted.trust_weight;
    return interpreted;
}

} // namespace swarm::social
