#pragma once

#include <algorithm>
#include <vector>
#include <array>
#include <deque>
#include <cassert>
#include <cstdint>
#include <type_traits>

namespace mf83 {

enum class Card
{
    ACE = 1,
    TWO = 2,
    THREE = 3,
    FOUR = 4,
    FIVE = 5,
    SIX = 6,
    SEVEN = 7,
    EIGHT = 8,
    NINE = 9,
    TEN = 10,
    JACK = 11,
    QUEEN = 12,
    KING = 13,
};

template <typename E>
std::underlying_type_t<E> enumCast(const E& e)
{
    return static_cast<std::underlying_type_t<E>>(e);
}

inline Card getCardFromInt(int card)
{
    assert(card >= enumCast(Card::ACE) && card <= enumCast(Card::KING));
    return static_cast<Card>(card);
}

inline std::uint_fast8_t getVal(Card card)
{
    using enum Card; // TODO gregory this is C++20, might not work here idk yet
    switch (card)
    {
    case ACE: return 1;
    case TWO: return 2;
    case THREE: return 3;
    case FOUR: return 4;
    case FIVE: return 5;
    case SIX: return 6;
    case SEVEN: return 7;
    case EIGHT: return 8;
    case NINE: return 9;
    case TEN:
    case JACK:
    case QUEEN:
    case KING: return 10;
    default: std::abort();
    }
}

class State
{
public:
    State(std::array<std::deque<Card>, 4> piles,
          std::vector<Card> stack = {},
          std::uint_fast8_t score      = 0) :
        m_scores{score},
        m_stacks{std::move(stack)},
        m_piles{std::move(piles)}
    {
    }

    auto getScore() const
    {
        assert(m_scores.size() > 0);
        return m_scores.back();
    }

    const auto& getStack() const
    {
        assert(m_stacks.size() > 0);
        return m_stacks.back();
    }
    const auto& getPiles() const { return m_piles; }

    unsigned int getStackSum() const
    {
        unsigned int ret = 0;
        for (const auto& card : getStack()) { ret += getVal(card); }

        return ret;
    }

    // TODO gregory benchmark switching this to bitset
    std::vector<std::uint_fast8_t> getLegalMoves() const
    {
        std::vector<std::uint_fast8_t> ret{};

        auto stack_val = getStackSum();

        for (size_t pile_idx = 0; pile_idx < m_piles.size(); pile_idx++)
        {
            const auto& pile = m_piles[pile_idx];
            if (!pile.empty() && (stack_val + getVal(pile.front())) <= 31)
            {
                ret.emplace_back(pile_idx);
            }
        }

        return ret;
    }

    void makeMove(size_t pile_idx)
    {
        assert(pile_idx < m_piles.size());
        assert(!m_piles[pile_idx].empty());
        assert(getStackSum() + m_piles[pile_idx].front().value() <= 31);

        m_cards.emplace_back(m_piles[pile_idx].front(), pile_idx);
        const auto& card = m_cards.back().first;
        m_piles[pile_idx].pop_front();

        // Copy the previous stack, adding our card
        m_stacks.emplace_back(m_stacks.back());
        m_stacks.back().emplace_back(card);
        auto& stack = m_stacks.back();

        // Copy the previous score; we'll increment it next
        m_scores.emplace_back(m_scores.back());
        auto& score = m_scores.back();

        // Scoring conditions:
        // First card is a Jack - 2 points
        if (stack.size() == 1 && card == Card::JACK) { score += 2; }

        // Stack total is exactly 15 - 2 points
        // Stack total is exactly 31 - 2 points (and clear stack)
        if (auto stack_sum = getStackSum(); stack_sum == 15 || stack_sum == 31)
        {
            score += 2;
        }

        // Doubles/Triples/Quads - 2/6/12 points
        size_t set_size = 0;
        for (auto rit = stack.rbegin(); rit != stack.rend(); rit++)
        {
            if (*rit != card) { break; }

            set_size++;
        }

        switch (set_size)
        {
            case 1:
                break;
            case 2:
                score += 2;
                break;
            case 3:
                score += 6;
                break;
            case 4:
                score += 12;
                break;
            default:
                std::abort();
        }

        // Runs of numbers, in any order - points equal to run length
        for (size_t run_size = std::min(size_t{7}, stack.size()); run_size >= 3;
             run_size--)
        {
            std::vector<unsigned int> run_set{};
            for (auto rit = stack.rbegin();
                 static_cast<size_t>(rit - stack.rbegin()) < run_size;
                 rit++)
            {
                run_set.emplace_back(enumCast(*rit));
            }

            std::sort(run_set.begin(), run_set.end());

            bool is_run       = true;
            unsigned int prev = run_set.front();
            for (size_t i = 1; i < run_set.size(); i++)
            {
                if (run_set[i] != prev + 1)
                {
                    is_run = false;
                    break;
                }

                prev = run_set[i];
            }

            if (is_run)
            {
                score += run_size;
                break;
            }
        }

        if (getLegalMoves().empty()) { stack.clear(); }
    }

    void undoMove()
    {
        // Can't undo before the initial state
        assert(m_scores.size() > 1);
        // Should have had the same number of moves made
        assert(m_scores.size() == m_stacks.size());
        // We'll have one more score than card because the initial state has no
        // card moved yet
        assert(m_scores.size() == m_cards.size() + 1);

        // Remove any points added by the last move
        m_scores.pop_back();
        // Put the last moved card back
        const auto& [card, pile_idx] = m_cards.back();
        m_piles[pile_idx].emplace_front(card);
        m_cards.pop_back();
        // Reset the stack to its previous state
        m_stacks.pop_back();
    }

private:
    // Store these as queues of every game state, with more recent states at the
    // back
    std::deque<std::uint_fast8_t> m_scores{};
    // TODO gregory I feel like at least one of these can be removed
    // Cards moved from a pile to a stack, and the index of the pile it came from
    std::deque<std::pair<Card, std::uint_fast8_t>> m_cards{};
    std::deque<std::vector<Card>> m_stacks{};

    std::array<std::deque<Card>, 4> m_piles{};
};

} // namespace mf83
