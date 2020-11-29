#pragma once

#include <random>
#include <unordered_map>

#include "State.h"

namespace mf83 {

class TranspoTable
{
public:
    TranspoTable()
    {
        std::uniform_int_distribution<size_t> dist{};
        std::random_device rd{};
        std::default_random_engine gen{rd()};

        for (auto& card_pom_hash : m_stack_hashes)
        {
            for (auto& card_kind_hash : card_pom_hash)
            {
                card_kind_hash = dist(gen);
            }
        }

        for (auto& card_pom_hash : m_pile_hashes)
        {
            for (auto& card_kind_hash : card_pom_hash)
            {
                card_kind_hash = dist(gen);
            }
        }
    }

    void insert(const State& state, size_t best_move, unsigned int best_score)
    {
        auto key = getKey(state);
        m_map.emplace(key,
                      std::make_pair(best_move, best_score - state.getScore()));
    }

    std::pair<size_t, unsigned int> getVal(const State& state) const
    {
        auto key = getKey(state);

        const auto search_ret = m_map.find(key);
        if (search_ret == m_map.end())
        {
            return {std::numeric_limits<size_t>::max(), 0};
        }

        return search_ret->second;
    }

    void clear() { m_map.clear(); }

private:
    uint64_t getKey(const State& state) const
    {
        uint64_t key = 0;

        size_t card_idx = 0;
        for (const auto& card : state.getStack())
        {
            key ^= m_stack_hashes[card_idx++][getCardKindIdx(card)];
        }

        const auto& piles = state.getPiles();
        for (size_t pile_idx = 0; pile_idx < 4; pile_idx++)
        {
            const auto& pile = piles[pile_idx];

            card_idx = 0;
            for (const auto& card : pile)
            {
                key ^= m_pile_hashes[pile_idx * 13 + card_idx++]
                                    [getCardKindIdx(card)];
            }
        }

        return key;
    }

    static size_t getCardKindIdx(const Card& card)
    {
        switch (card.kind())
        {
            case '1':
                return 1;
            case '2':
                return 2;
            case '3':
                return 3;
            case '4':
                return 4;
            case '5':
                return 5;
            case '6':
                return 6;
            case '7':
                return 7;
            case '8':
                return 8;
            case '9':
                return 9;
            case 'T':
                return 10;
            case 'J':
                return 11;
            case 'Q':
                return 12;
            case 'K':
                return 13;
            default:
                std::abort();
        }
    }

    // State key -> score
    std::unordered_map<uint64_t, std::pair<size_t, unsigned int>> m_map{};
    std::array<std::array<uint64_t, 14>, 31> m_stack_hashes{};
    std::array<std::array<uint64_t, 14>, 52> m_pile_hashes{};
};

} // namepsace mf83
