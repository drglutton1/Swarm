#include "inheritance.h"

#include <stdexcept>

namespace swarm::economy {
namespace {

std::vector<swarm::core::Swarm*> living_offspring_only(const std::vector<swarm::core::Swarm*>& offspring) {
    std::vector<swarm::core::Swarm*> living;
    living.reserve(offspring.size());
    for (swarm::core::Swarm* child : offspring) {
        if (child != nullptr && child->alive()) {
            living.push_back(child);
        }
    }
    return living;
}

} // namespace

std::int64_t DefaultInheritancePolicy::burn_amount(std::int64_t bankroll, std::size_t living_offspring_count) const {
    if (bankroll < 0) {
        throw std::invalid_argument("bankroll cannot be negative");
    }
    if (living_offspring_count == 0) {
        return bankroll;
    }
    return bankroll / 2;
}

InheritanceDistribution InheritanceService::distribute_on_death(
    swarm::core::Swarm& deceased,
    const std::vector<swarm::core::Swarm*>& offspring,
    ChipManager& manager,
    const InheritancePolicy& policy) {
    if (!deceased.alive()) {
        throw std::invalid_argument("deceased swarm already marked dead");
    }

    InheritanceDistribution distribution;
    distribution.starting_bankroll = deceased.bankroll();

    auto living = living_offspring_only(offspring);
    distribution.payouts.assign(living.size(), 0);

    distribution.burned = policy.burn_amount(distribution.starting_bankroll, living.size());
    if (distribution.burned < 0 || distribution.burned > distribution.starting_bankroll) {
        throw std::logic_error("inheritance policy returned invalid burn amount");
    }

    const std::int64_t distributable = distribution.starting_bankroll - distribution.burned;
    if (distribution.burned > 0) {
        manager.burn(deceased, distribution.burned);
    }

    if (!living.empty() && distributable > 0) {
        const std::int64_t base_share = distributable / static_cast<std::int64_t>(living.size());
        const std::int64_t remainder = distributable % static_cast<std::int64_t>(living.size());
        for (std::size_t i = 0; i < living.size(); ++i) {
            const std::int64_t payout = base_share + (static_cast<std::int64_t>(i) < remainder ? 1 : 0);
            living[i]->add_bankroll(payout);
            distribution.payouts[i] = payout;
            distribution.distributed += payout;
        }
    }

    if (distribution.distributed > 0) {
        deceased.remove_bankroll(distribution.distributed);
    }

    deceased.mark_dead();
    manager.require_invariants();
    return distribution;
}

const InheritancePolicy& InheritanceService::default_policy() {
    static const DefaultInheritancePolicy policy;
    return policy;
}

} // namespace swarm::economy
