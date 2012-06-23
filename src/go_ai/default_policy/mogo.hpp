#include "go_ai/go_state_anaylsis/go_state_analyser.hpp"

class DefaultPolicy_Mogo {
private:
    PatternMatcher pattern_matcher;

public:
    DefaultPolicy_Mogo() {}

    GoMove selectMove(GoState &s, RNG &rng) {
        GoStateAnalyser gsa(s, rng, pattern_matcher);
        GoMove move = gsa.selectMoveForSimulation();
        return move;
    }

    void completeGame(GoState& s, StaticVector<GoMove, MAX_GAME_LENGTH>& move_seq, RNG &rng) {
        bool game_over = false;

        unsigned int moves = 0;

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
