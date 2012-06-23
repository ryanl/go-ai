#include "go_state_analyser.hpp"

using namespace std;

void GoStateAnalyser::calculateGroups() {
    for (unsigned int i = 0; i < s.groups.countRoots(); i++) {
        unsigned int r = s.groups.getRootListElement(i);

           int p = s.get(GoMove(r));

        if (p == s.getNextToPlay()) {
            root_to_index[r] = player_groups.size();
            player_groups.push_back(r);
        } else {
            assert(p == opponentOf(s.getNextToPlay()));
            root_to_index[r] = opponent_groups.size();
            opponent_groups.push_back(r);
        }
    }
}

bool GoStateAnalyser::isSelfAtari(GoMove position) {
    LibertySet single_liberty_at_position;
    single_liberty_at_position.setBit(position.getXY());

    for (unsigned int i = 0; i < opponent_groups.size(); i++) {
        // check if the move captures stones - captures are not counted as self-ataris
        LibertySet liberties = s.groups.tokenForRoot(opponent_groups[i]).expanded_stones & s.board_spaces;
        if (liberties == single_liberty_at_position) return false;
    }

    // ---

    StaticVector<GoMove, 4> neighbours = position.adjacentPoints_SV();
    LibertySet new_liberties;

    for (unsigned int i = 0; i < neighbours.size(); i++) {
        int g = s.get(neighbours[i]);
        unsigned int neighbour_xy = neighbours[i].getXY();

        if (g == s.getNextToPlay()) {
            unsigned int find_neighbour = s.groups.find(neighbour_xy);
            assert(find_neighbour < BOARDSIZE * BOARDSIZE);

            new_liberties |= s.groups.tokenForRoot(find_neighbour).expanded_stones;
        } else if (g == EMPTY) {
            new_liberties.setBit(neighbour_xy);
            assert(s.board_spaces.getBit(neighbour_xy));
        }
    }

    new_liberties &= s.board_spaces;
    new_liberties.unsetBit(position.getXY());

    assert(new_liberties.count() > 0);

    /*if (new_liberties.count() == 0) {
        std::cerr << s.toHumanReadableString() << "\n" << position.getXY() << "\n";
        assert(false);
        abort();
    }*/

    return new_liberties.count() == 1;
}

StaticVector<unsigned int, 4> GoStateAnalyser::playerGroupsAtariedByLastMove() {
    StaticVector<unsigned int, 4> ret;

    GoMove prev = s.getPreviousMove();

    if (!prev.isPass()) {
        StaticVector<GoMove, 4> adjacent_points = prev.adjacentPoints_SV();

        for (unsigned int i = 0; i < adjacent_points.size(); i++) {
            GoMove m = adjacent_points[i];
            if (s.get(m.getXY()) == s.getNextToPlay()) {
                unsigned int j = s.groups.find(m.getXY());

                LibertySet liberties = s.groups.tokenForRoot(j).expanded_stones & s.board_spaces;
                if (liberties.count() == 1) {
                    unsigned int gi = root_to_index[j];
                    bool found = false;
                    for (unsigned int k = 0; k < ret.size(); k++) {
                        if (ret[k] == gi) found = true;
                    }
                    if (!found) {
                        ret.push_back(gi);
                    }
                }
            }
        }
    }

    return ret;
}

StaticVector<GoMove, BOARDSIZE * BOARDSIZE> GoStateAnalyser::movesThatSaveAtariedPlayerGroup(unsigned int group_index) {
    StaticVector<GoMove, BOARDSIZE * BOARDSIZE> ret;

    LibertySet neighbours = s.groups.tokenForRoot(player_groups[group_index]).expanded_stones;
    LibertySet liberties = neighbours & s.board_spaces;
    assert(liberties.count() == 1);

    // look for captures
    for (unsigned int i = 0; i < opponent_groups.size(); i++) {
        GoGroupInfo &o = s.groups.tokenForRoot(opponent_groups[i]);

        if ((s.groups.setMembersOfRoot(opponent_groups[i]) & neighbours).any()) {
            LibertySet op_liberties = o.expanded_stones & s.board_spaces;
            if (op_liberties.count() == 1) {
                unsigned int ol = op_liberties.getFirst();

                if (isValidMove(GoMove(ol))) {
                    ret.push_back(GoMove(ol));
                }
            }
        }
    }

    unsigned int gl = liberties.getFirst();

    // look for extensions
    if (isValidMove(GoMove(gl)) && !isSelfAtari(GoMove(gl))) {
        ret.push_back(GoMove(gl));
    }
    return ret;
}

template <bool verbose>
GoMove GoStateAnalyser::selectMoveForSimulation_Mogo() {
    // if pass wins the game, then pass
    if (passWinsTheGame()) {
        if (verbose) std::cerr << "selectMoveForSimulation_Mogo: pass rule\n";
        return GoMove::pass();
    }

    GoMove prev = s.getPreviousMove();

    if (!prev.isPass() && !prev.isNone()) {
        // look for groups ataried by the last move
        StaticVector<unsigned int, 4> groups_ataried = playerGroupsAtariedByLastMove();

        // if any moves exist that save those groups (by capture or extension) choose a random one of those
        // (though if they save more than 1 group then they have higher probability)
        bool seen[BOARDSIZE * BOARDSIZE];
        for (unsigned int i = 0; i < BOARDSIZE * BOARDSIZE; i++) {
            seen[i] = false;
        }

        StaticVector<GoMove, BOARDSIZE * BOARDSIZE > saving_moves;
        for (unsigned int i = 0; i < groups_ataried.size(); i++) {
            StaticVector<GoMove, BOARDSIZE * BOARDSIZE> saving_moves_for_group = movesThatSaveAtariedPlayerGroup(groups_ataried[i]);
            for (unsigned int j = 0; j < saving_moves_for_group.size(); j++) {
                if (!seen[saving_moves_for_group[j].getXY()]) {
                    seen[saving_moves_for_group[j].getXY()] = true;
                    saving_moves.push_back(saving_moves_for_group[j]);
                }
            }
        }

        if (saving_moves.size() > 0) {
            //cerr << "Selecting saving move\n";
            if (verbose) std::cerr << "selectMoveForSimulation_Mogo: saving move\n";
            return saving_moves[rng.getIntBetween(0, saving_moves.size() - 1)];
        }

        // look for moves in the 8 points close to the previous move that match patterns (see MoGo)
        int min_x = prev.getX() - 1, max_x = prev.getX() + 1;
        int min_y = prev.getY() - 1, max_y = prev.getY() + 1;
        if (min_x < 0) min_x = 0;
        if (max_x > int(BOARDSIZE - 1)) max_x = BOARDSIZE - 1;
        if (min_y < 0) min_y = 0;
        if (max_y > int(BOARDSIZE - 1)) max_y = BOARDSIZE - 1;

        StaticVector<GoMove, 8> interesting_local_moves;
        for (int x = min_x; x <= max_x; x++) {
            for (int y = min_y; y <= max_y; y++) {
                if (s.get(x, y) == EMPTY &&
                    matchesAnyPattern(x, y) &&
                    isValidMove(GoMove::move(x, y)) &&
                    !isSelfAtari(GoMove::move(x, y)))
                {
                    interesting_local_moves.push_back(GoMove::move(x, y));
                }
            }
        }

        if (interesting_local_moves.size() > 0) {
            //cerr << "Selecting interesting move\n";
            if (verbose) std::cerr << "selectMoveForSimulation_Mogo: local pattern move\n";
            return interesting_local_moves[rng.getIntBetween(0, interesting_local_moves.size() - 1)];
        }
    }

    // look for captures
    GoMove biggest_capture = largestAvailableCapture();
    if (!biggest_capture.isNone()) {
           if (verbose) std::cerr << "selectMoveForSimulation_Mogo: largest available capture\n";
        return biggest_capture;
    }

    // TODO: this needs some thought
    bool allow_self_ataris = true; //rng.getBool();

    // otherwise randomly play anywhere
    RandomPermutation rp(rng, BOARDSIZE * BOARDSIZE);
    while (!rp.done()) {
        GoMove move = GoMove(rp.getNext());
        if (isValidMove(move) &&
            !s.isSelfEyeFilling(move)) {

            if (allow_self_ataris || !isSelfAtari(move)) {
                   if (verbose) std::cerr << "selectMoveForSimulation_Mogo: random play\n";
                   return move;
               }
        }
    }

    if (verbose) std::cerr << "selectMoveForSimulation_Mogo: pass as last resort\n";
    return GoMove::pass(); // no valid non-eye-filling non-self-atari moves
}

bool GoStateAnalyser::matchesAnyPattern(unsigned int x, unsigned int y) {
    uint32_t v = pattern_matcher.convertToInteger(x, y, s);
    return pattern_matcher.checkForPatternMatch(v);
}


template GoMove GoStateAnalyser::selectMoveForSimulation_Mogo<true>();
template GoMove GoStateAnalyser::selectMoveForSimulation_Mogo<false>();
