#ifndef __GO_DEFINITIONS_HPP
#define __GO_DEFINITIONS_HPP

#include <vector>

enum TypeOfSuperko {
    SUPERKO_POSITIONAL,
    SUPERKO_SITUATIONAL,
    SUPERKO_NATURAL_SITUATIONAL
};


const int EMPTY =  0;
const int WHITE = -1;
const int BLACK =  1;

/*!
    Returns BLACK if parameter is WHITE, and vice-versa.
*/
inline int opponentOf(int turn) {
    return (BLACK + WHITE) - turn;
}

/* BOARDSIZE is fixed - you need to recompile to use a
   different one. This is done to reduce memory usage
   and increase performance.

   Making BOARDSIZE a template parameter causes more problems than advantages (I tried it).
*/
const unsigned int BOARDSIZE = OPT_BOARDSIZE; // this is a compile option

const unsigned int MAX_GAME_LENGTH = BOARDSIZE * BOARDSIZE * 4; // guessed

#endif
