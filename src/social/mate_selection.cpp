#include "mate_selection.h"

#include <algorithm>

#include "../evolution/reproduction.h"

namespace swarm::social {
namespace {

double find_believed_value(const std::optional<InterpretedInfo>& info, double fallback) {
    return info.has_value() && !info->response.refused ? info->believed_value : fallback;
}

double candidate_score(const swarm::core::Swarm& candidate, const SocialKnowledge& knowledge, const swarm::evolution::LifecyclePolicy& lifecycle) {
    const double public_size_score = 1.0 - (static_cast<double>(knowledge.public_face.size) / 18.0);
    const double offspring_penalty = std::min(1.0, static_cast<double>(knowledge.public_face.offspring_count) / static_cast<double>(lifecycle.max_offspring));
    const double believed_bankroll = find_believed_value(knowledge.bankroll, static_cast<double>(candidate.bankroll()));
    const double bankroll_score = std::clamp(believed_bankroll / static_cast<double>(swarm::core::Swarm::starting_bankroll * 2), 0.0, 1.5);
    const double readiness_score = find_believed_value(knowledge.reproductive_readiness, swarm::evolution::can_reproduce(candidate, lifecycle) ? 1.0 : 0.0);
    const double honesty_score = find_believed_value(knowledge.honesty, knowledge.public_face.self_reported_honesty);

    return bankroll_score * 1.2 + readiness_score * 2.0 + honesty_score * 0.7 + public_size_score * 0.4 - offspring_penalty * 0.8;
}

} // namespace

MateSelectionResult select_partner(
    const swarm::core::Swarm& seeker,
    const std::vector<const swarm::core::Swarm*>& candidates,
    const std::vector<SocialKnowledge>& knowledge,
    const swarm::evolution::LifecyclePolicy& lifecycle) {
    MateSelectionResult best;
    best.reason = "no eligible partner";

    for (const auto* candidate : candidates) {
        if (candidate == nullptr || candidate->id() == seeker.id()) {
            continue;
        }
        if (!swarm::evolution::pairing_parity_allowed(seeker, *candidate)) {
            continue;
        }
        if (!swarm::evolution::can_reproduce(*candidate, lifecycle)) {
            continue;
        }

        auto it = std::find_if(knowledge.begin(), knowledge.end(), [candidate](const SocialKnowledge& entry) {
            return entry.public_face.swarm_id == candidate->id();
        });
        const SocialKnowledge fallback{make_public_face(*candidate), std::nullopt, std::nullopt, std::nullopt};
        const SocialKnowledge& known = it != knowledge.end() ? *it : fallback;
        const double score = candidate_score(*candidate, known, lifecycle);
        if (best.partner == nullptr || score > best.score) {
            best.partner = candidate;
            best.score = score;
            best.reason = "best eligible parity-compatible partner";
        }
    }

    return best;
}

} // namespace swarm::social
