#pragma once

#include <cstdint>
#include <stdexcept>

#include "../core/swarm.h"

namespace swarm::economy {

class ChipManager {
public:
    [[nodiscard]] std::int64_t chips_injected() const noexcept;
    [[nodiscard]] std::int64_t chips_burned() const noexcept;
    [[nodiscard]] std::int64_t chips_in_play() const noexcept;

    void inject(swarm::core::Swarm& swarm, std::int64_t amount);
    void burn(swarm::core::Swarm& swarm, std::int64_t amount);

    [[nodiscard]] bool invariants_hold() const noexcept;
    void require_invariants() const;

private:
    std::int64_t chips_injected_ = 0;
    std::int64_t chips_burned_ = 0;
    std::int64_t chips_in_play_ = 0;
};

} // namespace swarm::economy
