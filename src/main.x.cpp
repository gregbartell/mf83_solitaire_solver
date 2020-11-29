#include "TranspoTable.h"
#include "Searcher.h"

#include <array>
#include <deque>
#include <iostream>

int main()
{
    using namespace mf83;

    std::array<std::deque<Card>, 4> piles{};
    for (size_t i = 0; i < 52; i++)
    {
        unsigned int val = 0;
        std::cin >> val;

        if (val < 10) { piles[i / 13].emplace_front(val + '0'); }
        else
        {
            switch (val)
            {
                case 0:  // A sentinel value meaning the card isn't there
                    continue;
                case 10:
                    piles[i / 13].emplace_front('T');
                    break;
                case 11:
                    piles[i / 13].emplace_front('J');
                    break;
                case 12:
                    piles[i / 13].emplace_front('Q');
                    break;
                case 13:
                    piles[i / 13].emplace_front('K');
                    break;
                default:
                    std::abort();
            }
        }
    }

    Searcher searcher{piles};
    while (!searcher.getLegalMoves().empty())
    {
        const auto& [best_move, best_score] = searcher.getBestMove();
        searcher.makeMove(best_move);

        std::cout << best_move << std::endl;
        if (searcher.getStack().empty() && !searcher.getLegalMoves().empty())
        {
            std::cout << "-" << std::endl;
        }
    }

    return 0;
}
