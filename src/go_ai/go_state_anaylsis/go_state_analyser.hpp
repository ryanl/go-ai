#ifndef __GO_STATE_ANALYSER_HPP
#define __GO_STATE_ANALYSER_HPP

#include "go_mechanics/go_state.hpp"
#include "random/random_permutation.hpp"
#include "static_vector.hpp"
#include "go_ai/pattern/pattern_matcher.hpp"

/*     GoStateAnalyser should be created for analysis of a single board state and then destroyed
    I've separated it out from GoState as it will change a lot more, while I want GoState to remain stable
    one thing I might do is make it more tied to GoState and keep it for an entire game (for better performance than recalculating after every move)
*/

class GoStateAnalyser {
private:
    GoState &s;
    RNG &rng;
    PatternMatcher &pattern_matcher;

    unsigned int root_to_index[BOARDSIZE * BOARDSIZE];

    StaticVector<unsigned int, BOARDSIZE * BOARDSIZE> player_groups;
    StaticVector<unsigned int, BOARDSIZE * BOARDSIZE> opponent_groups;

    bool matchesAnyPattern(unsigned int x, unsigned int y);

    // 0 -> unknown
    // 1 -> invalid
    // 2 -> valid
    //char valid_move_cache[BOARDSIZE * BOARDSIZE];

    // possibly could modify ADS to make this faster
    void calculateGroups();

    inline bool isValidMove(GoMove m) {
        return s.isValidMove(m);
        /*
        char &cache = valid_move_cache[m.getXY()];
        if (cache) {
            return cache - 1;
        } else {
            bool ret = s.isValidMove(m);
            cache = char(ret) + 1;
            return ret;
        }
        */
    }

public:
    GoStateAnalyser(GoState &_s, RNG &_rng, PatternMatcher& _pattern_matcher) :
        s(_s),
        rng(_rng),
        pattern_matcher(_pattern_matcher)
    {
        /*
        for (unsigned int i = 0; i < BOARDSIZE * BOARDSIZE; i++)  {
            valid_move_cache[i] = 0;
        }
        */

        calculateGroups();
    }


    GoMove largestAvailableCapture() {
        unsigned int max_capture_size = 0;
        GoMove ret = GoMove::none();

        for (unsigned int i = 0; i < opponent_groups.size(); i++) {
            LibertySet liberties = s.groups.tokenForRoot(opponent_groups[i]).expanded_stones & s.board_spaces;

            unsigned int size = s.groups.setMembersOfRoot(opponent_groups[i]).count();
            unsigned int liberty_count = liberties.count();

            assert(liberty_count > 0);

            if (size > max_capture_size && liberty_count == 1) {
                  unsigned int only_liberty_position = liberties.getFirst();
                  if (isValidMove(GoMove(only_liberty_position))) {
                      max_capture_size = size;
                       ret = GoMove(only_liberty_position);
                }
            }
        }

        return ret;
    }

    bool isSelfAtari(GoMove position);

    GoMove anyMoveThatSavesAStoneInAtari() {
        for (unsigned int i = 0; i < player_groups.size(); i++) {
            LibertySet liberties = s.groups.tokenForRoot(player_groups[i]).expanded_stones & s.board_spaces;
            if (liberties.count() == 1) {
                unsigned int only_liberty_position = liberties.getFirst();
                if (isValidMove(GoMove(only_liberty_position)) && !isSelfAtari(GoMove(only_liberty_position))) {
                    return GoMove(only_liberty_position);
                }
            }
        }

        return GoMove::none();
    }

    GoMove biggestAtari() {
        unsigned int max_atari_size = 0;
        GoMove ret = GoMove::none();

        // maybe do some ladder checking here to see if it's worth doing?

        for (unsigned int i = 0; i < opponent_groups.size(); i++) {
            unsigned int num_stones = s.groups.setMembersOfRoot(opponent_groups[i]).count();
            LibertySet liberties = s.groups.tokenForRoot(opponent_groups[i]).expanded_stones & s.board_spaces;

            if (num_stones > max_atari_size && liberties.count() == 2) {
                // check if either of the two liberties are valid, non-self-atari moves
                for (LibertySet::SetBitIterator sbi = liberties.getSetBitIterator(); !sbi.isDone(); ++sbi) {
                    if (isValidMove(GoMove(*sbi)) && !isSelfAtari(GoMove(*sbi))) {
                        max_atari_size = num_stones;
                        ret = GoMove(*sbi);
                        break;
                    }
                }

            }
        }

        return ret;
    }

    /*
    static LibertySet expandByAdjacency(const LibertySet& points) {
        LibertySet ret;

        for (unsigned int i = 0; i < BOARDSIZE * BOARDSIZE; i++) {
            if (points.getBit(i)) {
                StaticVector<GoMove, 4> a = GoMove(i).adjacentPoints_SV();
                for (unsigned int j = 0; j < a.size(); j++) {
                    ret[a[j].getXY()] = true;
                }
            }
        }
        return ret;
    }
    */

    bool passWinsTheGame() {
        // this next bit is specific to the Tromp Taylor 2-passes-and-thats-it rule
        if (s.getPreviousMoveWasPass()) {
            if (s.getWinnerOfGame() == s.getNextToPlay()) {
                return true; // if passing wins you the game, then by all means pass
            }
        }
        return false;
    }

    GoMove selectMoveForSimulation() {
        return selectMoveForSimulation_Mogo<false>();
    }

    StaticVector<unsigned int, 4> playerGroupsAtariedByLastMove();

    StaticVector<GoMove, BOARDSIZE * BOARDSIZE> movesThatSaveAtariedPlayerGroup(unsigned int group_index);

    template <bool verbose>
    GoMove selectMoveForSimulation_Mogo();
};

#endif
