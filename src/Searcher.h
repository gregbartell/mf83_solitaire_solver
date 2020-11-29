#pragma once

#include "State.h"
#include "TranspoTable.h"

#include <utility>
#include <limits>

namespace mf83 {

class Searcher
{
public:
    Searcher(State state) : m_state{std::move(state)} {}

    // First = pile index to choose
    // Second = resultant score
    std::pair<std::uint_fast8_t, std::uint_fast8_t> getBestMove()
    {
        if (auto ret = m_transpo_table.getVal(m_state); ret.second != 0)
        {
            return {ret.first, ret.second + m_state.getScore()};
        }

        auto legal_moves = m_state.getLegalMoves();
        if (legal_moves.empty())
        {
            return {std::numeric_limits<size_t>::max(), m_state.getScore()};
        }

        size_t best_move        = std::numeric_limits<size_t>::max();
        unsigned int best_score = 0;
        for (const auto& move : legal_moves)
        {
            m_state.makeMove(move);

            auto move_result = getBestMove();
            if (move_result.second > best_score)
            {
                best_move  = move;
                best_score = move_result.second;
            }

            m_state.undoMove();
        }

        m_transpo_table.insert(m_state, best_move, best_score);

        return {best_move, best_score};
    }

    void makeMove(size_t pile_idx) { m_state.makeMove(pile_idx); }

    auto getLegalMoves() const { return m_state.getLegalMoves(); }
    const auto& getStack() const { return m_state.getStack(); }
    auto getScore() const { return m_state.getScore(); }

private:
    State m_state;
    TranspoTable m_transpo_table{};
};

} // namespace mf83
