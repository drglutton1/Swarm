#pragma once

#include <cstdint>

#include "chip_manager.h"

namespace swarm::economy {

struct TransferResult {
    std::int64_t amount = 0;
    std::int64_t from_bankroll = 0;
    std::int64_t to_bankroll = 0;
};

class TransferService {
public:
    static void validate(const swarm::core::Swarm& from, const swarm::core::Swarm& to, std::int64_t amount);
    static TransferResult execute(swarm::core::Swarm& from, swarm::core::Swarm& to, std::int64_t amount, const ChipManager& manager);
};

} // namespace swarm::economy
