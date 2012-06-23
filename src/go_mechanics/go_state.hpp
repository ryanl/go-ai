#ifndef __GO_STATE_HPP
#define __GO_STATE_HPP

#include <bitset>
#include "assert.h"
#include "../static_vector.hpp"

#include "../augmented_disjoint_set/ads_faster.hpp"

#include "go_definitions.hpp"
#include "go_move.hpp"
#include "random/rng.hpp"
#include "zobrist_hashing.hpp"

#include <boost/unordered/unordered_set.hpp>
#define hashset_t boost::unordered_set

#ifdef OPT_USE_IMPROVED_BITSET
#include "../improved_bitset.hpp"
typedef ImprovedBitset<BOARDSIZE * BOARDSIZE> LibertySet;
#else
typedef std::bitset<BOARDSIZE * BOARDSIZE> LibertySet;
#endif

/*
@class GoGroupInfo
@brief Used in an augmented disjoint set structure to represent a connected group of one player's
       stones. The |= operator joins two groups.
*/
struct GoGroupInfo {

    /*!
        Contribution to the zobrist hash of the board made by these stones. Allows the effect on the
        board hash made by removing the stones to be calculated very quickly (for superko checks).
    */
    Zobhash group_hash;

    /*!
        The set of points contained within or adjcaent to the stones.
        This is not simply liberties union stones as it also contains points filled by opponent
        stones.
    */
    LibertySet expanded_stones;

    /*!
        Updates group after merging with another group.
    */
    void operator |= (const GoGroupInfo &other) {
        group_hash ^= other.group_hash;
        expanded_stones |= other.expanded_stones;
    }
};

struct ScoredGame {
    //int owned_by[BOARDSIZE][BOARDSIZE];
    float black_score;
    float white_score;
};


struct GoMoveInfo {
    bool valid;
    bool captures;
//    bool self_atari;

/*
    void write(std::ostream& out) {
        out << int(valid) << " ";
        out << int(captures) << " ";
    }

    void read(std::istream& in) {
        int tmp;
        in >> tmp; valid = tmp;
        in >> tmp; captures = tmp;
    }
*/
};

class GoState {
    friend class GoStateAnalyser;

private:
    RNG rng;

    int board_contents[BOARDSIZE * BOARDSIZE];

    /* positional, situational or natural situational */
    TypeOfSuperko superko;

    // no prisoners in chinese rules
    //unsigned int black_prisoners, white_prisoners; // black prisoners are dead BLACK stones

    float komi;

    bool first_move;
    GoMove previous_move;

    /* BLACK or WHITE */
    int next_to_play;

    /* set of empty intersections */
    LibertySet board_spaces;

    /* representation of the board design to be fast on operations
       (e.g. steps used to check move validity) */
    ADSFast<GoGroupInfo, BOARDSIZE * BOARDSIZE> groups;


    /* used to apply the superko rule */
    GoZobHasher<BOARDSIZE> hasher;

    hashset_t< Zobhash, ImprovedBitsetHasher<ZOBRIST_HASH_SIZE> > history;
    Zobhash current_hash;

    GoState(TypeOfSuperko superko);

public:
    void debugging_checkSelfConsistency() const;
    static GoState newGame(TypeOfSuperko superko);

    // generate random bits strings for zobrist hashing
    static void initialize();

    // we assume that play continues until all dead groups are removed
    // we score by the Tromp Taylor rules
    int getWinnerOfGame();

    ScoredGame scoreGame();

    std::string toHumanReadableString() {
        std::string ret = "";
        if (getNextToPlay() == BLACK) {
            ret.append("BLACK to play\n");
        } else {
            ret.append("WHITE to play\n");
        }

        for (unsigned int x = 0; x < BOARDSIZE; x++) {
            for (unsigned int y = 0; y < BOARDSIZE; y++) {
                switch (get(x, y)) {
                    case WHITE:
                        ret.push_back('O');
                        break;
                    case BLACK:
                        ret.push_back('#');
                        break;
                    case EMPTY:
                        ret.push_back('_');
                        break;
                    default:
                        assert(false);
                }
                ret.push_back(' ');
            }
            ret.push_back('\n');
        }

        /*
        ret.append("CURRENT: ");
        ret.append(current_hash.toString());
        ret.push_back('\n');
        for (std::set<Zobhash>::iterator it = history.begin(); it != history.end(); ++it) {
            ret.append("HISTORY: ");
               ret.append(current_hash.toString());
            ret.push_back('\n');
        }
        */
        return ret;
    }

    inline bool isValidMove(GoMove move) {
        return makeOrCheckValidityOfMove<false>(move).valid;
    }

    GoMoveInfo queryMove(GoMove move) {
        return makeOrCheckValidityOfMove<false>(move);
    }

    bool isSelfEyeFilling(GoMove move);

    template <bool makeMove>
    GoMoveInfo makeOrCheckValidityOfMove(GoMove move);

    void placeStone(GoMove move, int colour);

    std::vector<GoMove> validMoves();

    void queryValidMoves_SV_byref(StaticVector< std::pair<GoMove, GoMoveInfo>, 1 + (BOARDSIZE * BOARDSIZE) >& ret);

    int get(const GoMove position) const {
        unsigned int xy = position.getXY();
        assert(xy < BOARDSIZE * BOARDSIZE);

        return board_contents[xy];
    }

    int get(unsigned int x, unsigned int y) const {
        return get(GoMove::move(x, y));
    }

    float getKomi() const { return komi; }
    void setKomi(const float _komi) { komi = _komi; }

    int getNextToPlay() const { return next_to_play;  }

    char getPreviousMoveWasPass() const { return previous_move.isPass(); }

    GoMove getPreviousMove() const { return previous_move;  }

    void makeMove(GoMove move) {
        GoMoveInfo gmi = makeOrCheckValidityOfMove<true>(move);

                (void)(gmi); // prevent unused warning when asserts turned off

                // not combined with the above statement as we want to be able disable asserts without making makeMove do nothing
        assert(gmi.valid);
    }
};

#endif
