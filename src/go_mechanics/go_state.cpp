#include "go_state.hpp"
#include <queue>

using namespace std;

/* constructor - creates a new game (but use ::newGame instead)  */
GoState::GoState(TypeOfSuperko _superko) :
superko(_superko),
komi(6.5),
first_move(true),
previous_move(GoMove::none()),
next_to_play(BLACK),
board_spaces(),
hasher(superko),
history(BOARDSIZE * BOARDSIZE * 3) // sensible size for the hashset
{
    for (unsigned int i = 0; i < BOARDSIZE * BOARDSIZE; i++) {
        board_contents[i] = EMPTY;
        board_spaces.setBit(i);
    }

    switch (superko) {

    case SUPERKO_POSITIONAL:
    case SUPERKO_SITUATIONAL:
    case SUPERKO_NATURAL_SITUATIONAL:
        // okay
        break;

    default:
        // should always be one of the three above
        assert(false); abort();

    }
}

GoState GoState::newGame(TypeOfSuperko superko) {
    GoState ret(superko);
    return ret;
}

// TODO: introduce circumference lists for eye spaces and
// do fast eye detection?

struct SpaceInfo {
    bool touches_black, touches_white;

    void operator |= (const SpaceInfo& other) {
        touches_black = touches_black || other.touches_black;
        touches_white = touches_white || other.touches_white;
    }
};

int GoState::getWinnerOfGame() {
    ScoredGame sg = scoreGame();

    if (sg.black_score > sg.white_score)        return BLACK;
    else if (sg.white_score > sg.black_score)   return WHITE;
    else                                        return EMPTY;
}

ScoredGame GoState::scoreGame() {
    ScoredGame ret;
    SpaceInfo initial_spaceinfo = { false, false };
    ADSFast<SpaceInfo, BOARDSIZE * BOARDSIZE> spaces;

    for (unsigned int x = 0; x < BOARDSIZE; x++) {
        for (unsigned int y = 0; y < BOARDSIZE; y++) {
            //ret.owned_by[x][y] = EMPTY;

            const unsigned int xy = GoMove::xyToMoveId(x, y);

            if (get(x, y) == EMPTY) {
                spaces.createSingleton(xy);
                spaces.tokenForRoot(xy) = initial_spaceinfo;

                bool touches_black = false, touches_white = false;

                if (x > 0) {
                    int g = get(x - 1, y);
                    switch (g) {
                        case EMPTY: spaces.join(xy, GoMove::xyToMoveId(x - 1, y)); break;
                        case BLACK: touches_black = true; break;
                        case WHITE: touches_white = true; break;
                    }
                }
                if (y > 0) {
                    int g = get(x, y - 1);
                    switch (g) {
                        case EMPTY: spaces.join(xy, GoMove::xyToMoveId(x, y - 1)); break;
                        case BLACK: touches_black = true; break;
                        case WHITE: touches_white = true; break;
                    }
                }
                if (x < BOARDSIZE - 1) {
                    int g = get(x + 1, y);
                    switch (g) {
                        case BLACK: touches_black = true; break;
                        case WHITE: touches_white = true; break;
                    }
                }
                if (y < BOARDSIZE - 1) {
                    int g = get(x, y + 1);
                    switch (g) {
                        case BLACK: touches_black = true; break;
                        case WHITE: touches_white = true; break;
                    }
                }
                SpaceInfo &si = spaces.tokenForRoot(spaces.find(xy));
                si.touches_black |= touches_black;
                si.touches_white |= touches_white;
            }
        }
    }

    unsigned int black_score = 0, white_score = 0; // white_prisoners, white_score = black_prisoners; NO PRISONERS IN CHINESE RULES

    for (unsigned int x = 0; x < BOARDSIZE; x++) {
        for (unsigned int y = 0; y < BOARDSIZE; y++) {
            int colour = get(x, y);

            if (colour == EMPTY) {
                const SpaceInfo &si = spaces.tokenForRoot(spaces.find(GoMove::xyToMoveId(x, y)));

                if (si.touches_black && !si.touches_white) {
                    black_score++;
                    //ret.owned_by[x][y] = BLACK;

                } else if (si.touches_white && !si.touches_black) {
                    white_score++;
                    //ret.owned_by[x][y] = WHITE;

                } // otherwise neutral

            } else if (colour == BLACK) {
                black_score++; // we assume all groups are alive
                //ret.owned_by[x][y] = BLACK;

            } else if (colour == WHITE) {
                white_score++;
                //ret.owned_by[x][y] = WHITE;
            } else abort();
        }
    }

    ret.black_score = black_score;
    ret.white_score = white_score + getKomi();

    return ret;
}

/* very simple check */
bool GoState::isSelfEyeFilling(GoMove move) {
    StaticVector<GoMove, 4> adjacent_points = move.adjacentPoints_SV();
    LibertySet single_liberty_at_move;
    single_liberty_at_move.setBit(move.getXY());

    for (unsigned int i = 0; i < adjacent_points.size(); i++) {
        GoMove a = adjacent_points[i];

        if (get(a) != getNextToPlay()) {
            // one of the neighbours is empty or unfriendly, so the move is not eye-filling
            return false;
        } else {
            unsigned int f = groups.find(a.getXY());
            LibertySet liberties = groups.setMembersOfRoot(f) & board_spaces;

            if (liberties == single_liberty_at_move) {
                // one of the friendly neighbours is in atari
                return false;
            }
        }
    }

    return true;
}

/*
This is perhaps the most performance critical piece of code in the program.
It will be called tens of millions of times per game but checking whether
a move is valid is not entirely trivial */

template <bool make_move>
GoMoveInfo GoState::makeOrCheckValidityOfMove(const GoMove move) {
    /* pass is always a valid move */
    if (__builtin_expect(move.isPass(), 0)) {
        if (make_move) {
            previous_move = move;

            // change the turn
            next_to_play = opponentOf(next_to_play);

            // update the hash for superko
            current_hash ^= hasher.turnChanged();

            // in natural situational superko you don't record
            // game states following passes
            if (superko != SUPERKO_NATURAL_SITUATIONAL) {
                history.insert(current_hash);
            }
        }

        GoMoveInfo ret = { true, false };
        return ret; // move is valid
    }

    /* we don't deal with GoMove::resign or GoMove::none in this method */
    assert(move.isNormal());

    /* check whether there is anything already at this point on the board */
    int current_contents = get(move);
    if (current_contents != EMPTY) {
        /* you can only play at unfilled points */
        GoMoveInfo ret = { false, false };
        return ret;
    }

    int opponent = opponentOf(next_to_play);

    /* co-ordinates of points touching move position horizontally or vertically */
    StaticVector<GoMove, 4> adjacent_points = move.adjacentPoints_SV();

    /* hash is used for superko detection */
    Zobhash hash_after_move = current_hash;
    hash_after_move ^= hasher.turnChanged();

    /* update hash for the stone being played */
    hash_after_move ^= hasher.stoneAdded(move, next_to_play);

    /* this is used to check for capture - a group will be captured if
       it has exactly one liberty and that liberty is at the position of the
       move being played */
    LibertySet single_liberty_at_position_played;
    single_liberty_at_position_played.setBit(move.getXY());

    bool move_legal_except_superko = false;

    bool any_captures = false, captures[4];
    unsigned int finds[4];

    for (unsigned int i = 0; i < 4; i++) {
        finds[i] = (unsigned int)-1;
        captures[i] = false;
    }

    for (unsigned int i = 0; i < adjacent_points.size(); i++) {
        /* value will be WHITE, BLACK or EMPTY */
        int adjacent_point_contents = get(adjacent_points[i]);

        if (adjacent_point_contents == EMPTY) {
            /* the newly placed stone will have a liberty */
            move_legal_except_superko = true;
        } else {
            unsigned int find_adj_point = groups.find(adjacent_points[i].getXY());

            bool already_seen = false;

            for (unsigned int j = 0; j < i; j++) {
                if (finds[j] == find_adj_point) {
                    already_seen = true;
                }
            }

            // this already seen check is to prevent groups being captured twice and messing up the zobrist hashing
            if (!already_seen) {
                finds[i] = find_adj_point;

                /* retrieve details of the adjacent group, in particular its liberties */
                LibertySet adj_grp_liberties = groups.tokenForRoot(find_adj_point).expanded_stones & board_spaces;

                if (adjacent_point_contents == opponent) {
                    if (adj_grp_liberties == single_liberty_at_position_played) {
                        captures[i] = true;
                        any_captures = true;

                        // if we can capture then the move isn't suicide so apart from checking the superko rule it's valid
                        move_legal_except_superko = true;
                        hash_after_move ^= groups.tokenForRoot(find_adj_point).group_hash;
                    }
                } else {
                    /* if adjacent group is friendly and has liberties other than the point of play, then it isn't suicide */
                    move_legal_except_superko = move_legal_except_superko || (adj_grp_liberties != single_liberty_at_position_played);
                }
            }
        }
    }

    if (move_legal_except_superko) {
        /* check to see if superko rule prohibits move */

        bool is_repeated_position_for_superko = history.find(hash_after_move) != history.end();
        if (__builtin_expect(is_repeated_position_for_superko, 0)) {
            GoMoveInfo ret = { false, false };
            return ret;
        }
    } else {
        GoMoveInfo ret = { false, false };
        return ret;
    }

    if (make_move) {
        // the move is valid - this part of the code modifies the state to the state of the board after the move is played

        // place the stone
        groups.createSingleton(move.getXY());
        board_contents[move.getXY()] = next_to_play;
        board_spaces.unsetBit(move.getXY());

        LibertySet expansion_of_new_stone = single_liberty_at_position_played;

        for (unsigned int i = 0; i < adjacent_points.size(); i++) {
            expansion_of_new_stone.setBit(adjacent_points[i].getXY());
        }

        // enclosed in a private scope as the reference becomes invalid when this->groups is modified
        {
            GoGroupInfo& info_for_new_stone = groups.tokenForRoot(move.getXY());

            info_for_new_stone.group_hash = hasher.stoneAdded(move, next_to_play);
            info_for_new_stone.expanded_stones = expansion_of_new_stone;
        }

        // apply the effects to adjacent stones
        for (unsigned int i = 0; i < adjacent_points.size(); i++) {
            unsigned int finds_i = finds[i];
            if (finds_i == (unsigned int)-1) continue;

            int adjacent_point_contents = get(finds[i]);
            /* value will be WHITE, BLACK or EMPTY */

            if (adjacent_point_contents == opponent) {
                if (captures[i]) {
                    LibertySet points_cleared = groups.setMembersOfRoot(finds_i);
                    board_spaces |= points_cleared;

                    typedef ImprovedBitset<BOARDSIZE * BOARDSIZE>::SetBitIterator it_t;
                    for (it_t it = points_cleared.getSetBitIterator(); !it.isDone(); ++it) {
                        board_contents[*it] = EMPTY;
                    }
                    groups.disperse(finds_i);
                }
            } else {
                assert(!captures[i]);
                groups.join(adjacent_points[i].getXY(), move.getXY());
            }
        }

        // change the turn
        next_to_play = opponentOf(next_to_play);
        previous_move = move;

        // update the hash for superko
        current_hash = hash_after_move;
        history.insert(current_hash);
    }

    GoMoveInfo ret = { true, any_captures };
    return ret;
}

vector<GoMove> GoState::validMoves() {
    vector<GoMove> moves;
    moves.reserve(1 + (BOARDSIZE * BOARDSIZE));

    for (unsigned int y = 0; y < BOARDSIZE; y++) {
        for (unsigned int x = 0; x < BOARDSIZE; x++) {
            GoMove thismove = GoMove::move(x, y);
            if (isValidMove(thismove)) moves.push_back(thismove);
        }
    }

    /* pass is always a valid move */
    GoMove pass = GoMove::pass();
    moves.push_back(pass);

    return moves;
}

void GoState::queryValidMoves_SV_byref(StaticVector< pair<GoMove, GoMoveInfo>, 1 + (BOARDSIZE * BOARDSIZE) >& ret) {
    ret.clear();

    for (unsigned int y = 0; y < BOARDSIZE; y++) {
        for (unsigned int x = 0; x < BOARDSIZE; x++) {
            GoMove this_move = GoMove::move(x, y);
            GoMoveInfo move_info = queryMove(this_move);
            if (move_info.valid) {
                ret.push_back(make_pair(this_move, move_info));
            }
        }
    }

    /* pass is always a valid move */
    pair<GoMove, GoMoveInfo> pass(GoMove::pass(), queryMove(GoMove::pass()));
    ret.push_back(pass);
}

/*
set<GoMove> getConnectedComponent(int board[BOARDSIZE][BOARDSIZE], GoMove start) {
    queue<GoMove> checking_queue;
    set<GoMove> ret;

    bool seen[BOARDSIZE][BOARDSIZE];

    int colour = board[start.getX()][start.getY()];

    seen[start.getX()][start.getY()] = true;

    checking_queue.push(start);
    while (!checking_queue.empty()) {
        GoMove c = checking_queue.front();
        ret.insert(c);

        checking_queue.pop();

        vector<GoMove> adjacent_points = GoState::adjacentPoints(c);
        for (unsigned int i = 0; i < adjacent_points.size(); i++) {
            GoMove a = adjacent_points[i];
            if (board[a.getX()][a.getY()] == colour && !seen[a.getX()][a.getY()]) {
                checking_queue.push(a);
                seen[a.getX()][a.getY()] = true;
            }
        }
    }
    return ret;
}

LibertySet getLiberties(int board[BOARDSIZE][BOARDSIZE], set<GoMove> component) {
    LibertySet ret;
    for (set<GoMove>::iterator it = component.begin(); it != component.end(); ++it) {
        StaticVector<GoMove, 4>  adjacent_points = GoState::adjacentPoints_SV(*it);
        for (unsigned int i = 0; i < adjacent_points.size(); i++) {
            GoMove a = adjacent_points[i];
            if (board[a.getX()][a.getY()] == EMPTY) ret[a.getXY()] = true;
        }
    }
    return ret;
}
*/

void GoState::debugging_checkSelfConsistency() const {
/*
    GoState s_copy = *this;

    int board[BOARDSIZE][BOARDSIZE] = {0};

    for (unsigned int x = 0; x < BOARDSIZE; x++) {
        for (unsigned int y = 0; y < BOARDSIZE; y++) {
            board[x][y] = s_copy.get(x, y);
        }
    }

    set<GoMove> assigned_points;
    vector< set<GoMove> > calculated_groups;

    // work out groups
    for (unsigned int i = 0; i < BOARDSIZE * BOARDSIZE; i++) {
        GoMove m(i);

        if (board[m.getX()][m.getY()] != EMPTY && assigned_points.find(GoMove(i)) == assigned_points.end()) {
            set<GoMove> group = getConnectedComponent(board, GoMove(i));
            for (set<GoMove>::iterator it = group.begin(); it != group.end(); ++it) {
                assigned_points.insert(*it);
            }
            calculated_groups.push_back(group);

            // check getNextGroupMember
            set<GoMove> points_given;
            s_copy = *this;
            for (unsigned int j = s_copy.groups.find(m.getXY()); j != groups.NONE; j = s_copy.groups.nextGroupMember(j)) {
                points_given.insert(GoMove(j));
            }
            assert(points_given == group);
        }
    }




    // work out liberties
    for (unsigned int i = 0; i < calculated_groups.size(); i++) {
        bitset<BOARDSIZE * BOARDSIZE> liberties = getLiberties(board, calculated_groups[i]);

        s_copy = *this;

        GoGroupInfo token = s_copy.groups.tokenForSet(calculated_groups[i].begin()->getXY());
        if (token.liberties != liberties) {
            cerr << "Liberties of group at " << calculated_groups[i].begin()->getX() << "," << calculated_groups[i].begin()->getY() << " should be\n" <<
                    liberties << "\n" <<
                    "but were in fact\n" <<
                    token.liberties << "\n";
            assert(false);
        }

        assert(token.number_of_stones == calculated_groups[i].size());
    }
*/
}

template GoMoveInfo GoState::makeOrCheckValidityOfMove<true>(GoMove move);
template GoMoveInfo GoState::makeOrCheckValidityOfMove<false>(GoMove move);

