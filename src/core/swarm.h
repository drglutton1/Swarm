#pragma once

#include <cstddef>
#include <cstdint>

#include "chromosome.h"
#include "../util/rng.h"

namespace swarm::core {

enum class GovernanceMode {
    alpha_led,
    democratic,
};

class Swarm {
public:
    static constexpr std::int64_t starting_bankroll = 5000;

    Swarm() = default;
    Swarm(Chromosome first_chromosome, Chromosome second_chromosome, std::int64_t bankroll = starting_bankroll);

    static Swarm random(std::uint32_t ocean_size, std::size_t state_size, swarm::util::Rng& rng);

    [[nodiscard]] GovernanceMode governance_mode() const noexcept;
    [[nodiscard]] Decision decide(const Ocean& ocean, const PokerStateVector& state, std::size_t hand_number = 0) const;

    [[nodiscard]] const Chromosome& first_chromosome() const noexcept;
    [[nodiscard]] const Chromosome& second_chromosome() const noexcept;
    [[nodiscard]] std::size_t total_agents() const noexcept;
    [[nodiscard]] std::int64_t bankroll() const noexcept;
    [[nodiscard]] bool alive() const noexcept;
    [[nodiscard]] std::uint64_t hands_played() const noexcept;
    [[nodiscard]] std::uint64_t last_reproduction_hand() const noexcept;
    [[nodiscard]] std::uint32_t offspring_count() const noexcept;

    void add_bankroll(std::int64_t amount);
    void remove_bankroll(std::int64_t amount);
    void record_hands(std::uint64_t count) noexcept;
    void set_hands_played(std::uint64_t count) noexcept;
    void note_reproduction() noexcept;
    void mark_dead() noexcept;

private:
    Chromosome first_chromosome_;
    Chromosome second_chromosome_;
    std::int64_t bankroll_ = 0;
    bool alive_ = true;
    std::uint64_t hands_played_ = 0;
    std::uint64_t last_reproduction_hand_ = 0;
    std::uint32_t offspring_count_ = 0;
};

} // namespace swarm::core
