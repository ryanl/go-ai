#include "../../go_mechanics/go_state.hpp"
#include "random/random_permutation.hpp"

class DefaultPolicy_Random {
public:

    DefaultPolicy_Random() {}

    GoMove selectMove(GoState &s, RNG &rng) {
        RandomPermutation rp(rng, BOARDSIZE * BOARDSIZE);
        while (!rp.done()) {
            GoMove m = GoMove(rp.getNext());
            if (s.isValidMove(m) && !s.isSelfEyeFilling(m)) {
                return m;
            }
        }
        return GoMove::pass();
    }

    void completeGame(GoState& s, StaticVector<GoMove, MAX_GAME_LENGTH> &move_seq, RNG &rng) {
        bool game_over = false;

        unsigned int moves = 0;

        // now play a 'random' simulation
        // for now we will just choose a random non-passing non-eyefilling move
        while (!game_over) {
            moves++;

            GoMove move = selectMove(s, rng);
            assert(s.isValidMove(move));

            if (move.isPass() && s.getPreviousMoveWasPass()) {
                game_over = true;
            }

            s.makeMove(move);
            move_seq.push_back(move);
        }
    }
};
