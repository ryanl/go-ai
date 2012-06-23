/* Zobrist hashing is used for superko detection */


#include "random/rng.hpp"
#include "static_vector.hpp"
#include "improved_bitset.hpp"


#include "assert.h"

#include <bitset>
#include <set>
#include <iostream>
#include <string>

const unsigned int ZOBRIST_HASH_SIZE = 64;

typedef ImprovedBitset<ZOBRIST_HASH_SIZE> Zobhash;

template <unsigned int BOARDSIZE>
class GoZobHasher {
private:
    RNG rng;

    static Zobhash randomZobhash(RNG &rng) {
        Zobhash ret;
        for (unsigned int i = 0; i < ZOBRIST_HASH_SIZE; i++) {
            ret.setBit(i, rng.getBool());
        }
        return ret;
    }


    static Zobhash generateHashForWhiteNextToPlay(TypeOfSuperko superko, RNG &rng) {
        switch (superko) {

        case SUPERKO_POSITIONAL:
            return Zobhash(); // zero

        case SUPERKO_SITUATIONAL:
        case SUPERKO_NATURAL_SITUATIONAL:
            return randomZobhash(rng); // random value

        default:
            std::cerr << "Unknown superko type " << superko << std::endl;
            assert(false);
            abort();
        }
    }

    /* These are constant but const makes initialisation difficult */
    Zobhash stoneHashBlack[BOARDSIZE * BOARDSIZE], stoneHashWhite[BOARDSIZE * BOARDSIZE];
    Zobhash whiteNextToPlay;

public:
    GoZobHasher(TypeOfSuperko superko) :
        rng(),
        whiteNextToPlay(generateHashForWhiteNextToPlay(superko, rng))
    {
        for (unsigned int i = 0;  i < BOARDSIZE * BOARDSIZE; i++) {
            stoneHashBlack[i] =randomZobhash(rng);
            stoneHashWhite[i] = randomZobhash(rng);
        }
        whiteNextToPlay = generateHashForWhiteNextToPlay(superko, rng);
    }

    Zobhash turnChanged() const {
        return whiteNextToPlay;
    }

    Zobhash stoneRemoved(GoMove position, int colour) const {
        return stoneAdded(position, colour);
    }


    Zobhash stoneAdded(GoMove position, int colour) const {
        switch (colour) {
        case WHITE:
            return stoneHashWhite[position.getXY()];

        case BLACK:
            return stoneHashBlack[position.getXY()];

        default:
            assert(false);
            abort();
        }
    }
};

