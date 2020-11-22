#include <algorithm>
#include <array>
#include <cassert>
#include <cstdint>
#include <deque>
#include <iostream>
#include <random>
#include <unordered_map>
#include <vector>

class Card
{
public:
    // Takes char of number or T/J/Q/K/A
    Card(char kind) : m_kind{kind}
    {
        assert(m_kind == 'A' || (m_kind > '0' && m_kind <= '9') ||
               m_kind == 'T' || m_kind == 'J' || m_kind == 'Q' ||
               m_kind == 'K');

        // Store aces as 1, since they are always low
        if (m_kind == 'A') { m_kind = '1'; }
    }

    bool operator==(const Card& other) const { return m_kind == other.m_kind; }
    bool operator!=(const Card& other) const { return !(*this == other); }

    unsigned int value() const
    {
        if (m_kind > '0' && m_kind <= '9') { return m_kind - '0'; }

        assert(m_kind == 'T' || m_kind == 'J' || m_kind == 'Q' ||
               m_kind == 'K');
        return 10;
    }

    auto kind() const { return m_kind; }

private:
    char m_kind;
};

class State
{
public:
    State(std::array<std::deque<Card>, 4> piles,
          std::vector<Card> stack = {},
          unsigned int score      = 0) :
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

    unsigned int getStackVal() const
    {
        unsigned int ret = 0;
        for (const auto& card : getStack()) { ret += card.value(); }

        return ret;
    }

    std::vector<size_t> getLegalMoves() const
    {
        std::vector<size_t> ret{};

        auto stack_val = getStackVal();

        for (size_t pile_idx = 0; pile_idx < m_piles.size(); pile_idx++)
        {
            const auto& pile = m_piles[pile_idx];
            if (!pile.empty() && (stack_val + pile.front().value()) <= 31)
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
        assert(getStackVal() + m_piles[pile_idx].front().value() <= 31);

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
        if (stack.size() == 1 && card.kind() == 'J') { score += 2; }

        // Stack total is exactly 15 - 2 points
        // Stack total is exactly 31 - 2 points (and clear stack)
        if (auto stack_val = getStackVal(); stack_val == 15 || stack_val == 31)
        {
            score += 2;
        }

        // Doubles/Triples/Quads - 2/6/12 points
        size_t set_size = 0;
        for (auto rit = stack.rbegin(); rit != stack.rend(); rit++)
        {
            if (rit->kind() != card.kind()) { break; }

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
                if (rit->value() < 10) { run_set.emplace_back(rit->value()); }
                else
                {
                    switch (rit->kind())
                    {
                        case 'T':
                            run_set.emplace_back(10);
                            break;
                        case 'J':
                            run_set.emplace_back(11);
                            break;
                        case 'Q':
                            run_set.emplace_back(12);
                            break;
                        case 'K':
                            run_set.emplace_back(13);
                            break;
                        default:
                            std::abort();
                    }
                }
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
    std::deque<unsigned int> m_scores{};
    // Cards moved from a pile to a stack, and the pile idx
    std::deque<std::pair<Card, size_t>> m_cards{};
    std::deque<std::vector<Card>> m_stacks{};

    std::array<std::deque<Card>, 4> m_piles{};
};

class TranspositionTable
{
public:
    TranspositionTable()
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
        while (card_idx < 13) { key ^= m_stack_hashes[card_idx++][0]; }

        const auto& piles = state.getPiles();
        for (size_t pile_idx = 0; pile_idx < 4; pile_idx++)
        {
            const auto& pile = piles[pile_idx];

            card_idx = 0;
            for (const auto& card : pile)
            {
                size_t card_kind_idx = getCardKindIdx(card);

                key ^= m_pile_hashes[pile_idx * 13 + card_idx++][card_kind_idx];
            }

            while (card_idx < 13)
            {
                key ^= m_pile_hashes[pile_idx * 13 + card_idx++][0];
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

class Searcher
{
public:
    Searcher(State state) : m_state{std::move(state)} {}

    // First = pile index to choose
    // Second = resultant score
    std::pair<size_t, unsigned int> getBestMove()
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

private:
    State m_state;
    TranspositionTable m_transpo_table{};
};

int main()
{
    // This is where you'd put in your game state, with the first card in each
    // pile being on top The stack, if you want to start from something other
    // than the initial position, is the other way around, with the top card
    // being at the end of the deque. I know that doesn't make a whole lot of
    // sense, but this isn't a super serious project
    /* State game_state{std::array<std::deque<Card>, 4>{std::deque<Card> */
    /*     {'A', '2', '3', '4', '5', '6', '7', '8', '9', 'T', 'J', 'Q', 'K'}, */
    /*     {'A', '2', '3', '4', '5', '6', '7', '8', '9', 'T', 'J', 'Q', 'K'}, */
    /*     {'A', '2', '3', '4', '5', '6', '7', '8', '9', 'T', 'J', 'Q', 'K'}, */
    /*     {'A', '2', '3', '4', '5', '6', '7', '8', '9', 'T', 'J', 'Q', 'K'}}};
     */
    State game_state{
        std::array<std::deque<Card>, 4>{
            std::deque<Card>{'J',
                             '6',
                             '9',
                             'T',
                             '2',
                             '3',
                             'Q',
                             '8',
                             '6',
                             'Q',
                             'J',
                             '5',
                             '2'},
            {'7', 'A', 'K', 'T', 'Q', '7', '4', '6', '4', '9', '8', '5', '5'},
            {'3', '8', 'A', 'Q', 'J', '3', 'K', '2', 'T', '2', '7', '8', 'K'},
            {'K', '5', '3', 'J', '9', 'A', '7', '4', 'A', '6', '4', 'T', '9'}},
    };

    while (!game_state.getLegalMoves().empty())
    {
        Searcher searcher{game_state};

        const auto& [best_move, best_score] = searcher.getBestMove();
        game_state.makeMove(best_move);

        std::cout << "Best move: " << best_move
                  << ", resultant score: " << game_state.getScore()
                  << std::endl;
        if (game_state.getStack().empty())
        {
            std::cout << "--- New stack ---" << std::endl;
        }
    }

    return 0;
}
