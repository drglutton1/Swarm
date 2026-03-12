#pragma once

#include <cstdint>
#include <vector>

#include "chip_manager.h"

namespace swarm::economy {

struct InheritanceDistribution {
    std::int64_t starting_bankroll = 0;
    std::int64_t burned = 0;
    std::int64_t distributed = 0;
    std::vector<std::int64_t> payouts;
};

class InheritancePolicy {
public:
    virtual ~InheritancePolicy() = default;
    [[nodiscard]] virtual std::int64_t burn_amount(std::int64_t bankroll, std::size_t living_offspring_count) const = 0;
};

class DefaultInheritancePolicy final : public InheritancePolicy {
public:
    [[nodiscard]] std::int64_t burn_amount(std::int64_t bankroll, std::size_t living_offspring_count) const override;
};

class InheritanceService {
public:
    static InheritanceDistribution distribute_on_death(
        swarm::core::Swarm& deceased,
        const std::vector<swarm::core::Swarm*>& offspring,
        ChipManager& manager,
        const InheritancePolicy& policy = default_policy());

    [[nodiscard]] static const InheritancePolicy& default_policy();
};

} // namespace swarm::economy
