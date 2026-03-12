#include "transfer.h"

#include <stdexcept>

namespace swarm::economy {

void TransferService::validate(const swarm::core::Swarm& from, const swarm::core::Swarm& to, std::int64_t amount) {
    if (&from == &to) {
        throw std::invalid_argument("cannot transfer chips to the same swarm");
    }
    if (amount <= 0) {
        throw std::invalid_argument("transfer amount must be positive");
    }
    if (from.bankroll() < amount) {
        throw std::invalid_argument("transfer exceeds source bankroll");
    }
    if (!from.alive()) {
        throw std::invalid_argument("source swarm is not alive");
    }
    if (!to.alive()) {
        throw std::invalid_argument("destination swarm is not alive");
    }
}

TransferResult TransferService::execute(swarm::core::Swarm& from, swarm::core::Swarm& to, std::int64_t amount, const ChipManager& manager) {
    validate(from, to, amount);
    if (!manager.invariants_hold()) {
        throw std::logic_error("cannot transfer while chip invariants are broken");
    }

    from.remove_bankroll(amount);
    to.add_bankroll(amount);

    return TransferResult{amount, from.bankroll(), to.bankroll()};
}

} // namespace swarm::economy
