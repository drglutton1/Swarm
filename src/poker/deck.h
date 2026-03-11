#pragma once

#include <stdexcept>
#include <vector>

#include "card.h"
#include "../util/rng.h"

namespace swarm::poker {

class Deck {
public:
    Deck() { reset(); }

    void reset() {
        cards_.clear();
        cards_.reserve(52);
        for (int suit = 0; suit < 4; ++suit) {
            for (int rank = 2; rank <= 14; ++rank) {
                cards_.push_back(Card{static_cast<Rank>(rank), static_cast<Suit>(suit)});
            }
        }
        index_ = 0;
    }

    void shuffle(swarm::util::Rng& rng) {
        rng.shuffle(cards_.begin(), cards_.end());
        index_ = 0;
    }

    [[nodiscard]] std::size_t remaining() const noexcept { return cards_.size() - index_; }

    Card draw() {
        if (index_ >= cards_.size()) {
            throw std::runtime_error("deck exhausted");
        }
        return cards_[index_++];
    }

private:
    std::vector<Card> cards_;
    std::size_t index_ {0};
};

} // namespace swarm::poker
