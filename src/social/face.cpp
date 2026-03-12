#include "face.h"

namespace swarm::social {

PublicFace make_public_face(const swarm::core::Swarm& swarm) {
    return PublicFace{
        swarm.id(),
        swarm.hands_played(),
        swarm.total_agents(),
        swarm.offspring_count(),
        swarm.average_risk_gene(),
        swarm.average_confidence_gene(),
        swarm.average_honesty_gene()};
}

} // namespace swarm::social
