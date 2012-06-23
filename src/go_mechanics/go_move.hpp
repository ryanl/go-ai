#ifndef __GO_MOVE_HPP
#define __GO_MOVE_HPP

#include "go_definitions.hpp"
#include "static_vector.hpp"

/*!
@class GoMove
@brief Represents a move in the game of go.
@author Ryan Lothian
*/
class GoMove {
private:
    static const int XYPASS   = -1;
    static const int XYNONE   = -2;
    static const int XYRESIGN = -3;

    int xy;

public:
    static const int Y_INCREASE_TO_XY = BOARDSIZE;
    static const int X_INCREASE_TO_XY = 1;

    /*!
        Returns true if the move is PASS.
    */
    inline bool isPass() const {
        return xy == XYPASS;
    }

    /*!
        Returns true if the move is not PASS, NONE or RESIGN.
    */

    inline bool isNormal() const {
        return xy >= 0;
    }

    /*!
        Returns true if the move is NONE.
        NONE may be used e.g. as the value of "previous move"
        at the beginning of the game.
    */
    inline bool isNone() const {
        return xy == XYNONE;
    }

    /*!
        Returns true if the move is RESIGN.
    */
    inline bool isResign() const {
        return xy == XYRESIGN;
    }

    /*!
        Returns a numerical representation of the move.
    */
    inline int getXY() const {
        return xy;
    }

    /*!
        Returns the x co-ordinate of the move.
        Do not call this for NONE, PASS or RESIGN moves.
    */
    inline unsigned int getX() const {
        assert(xy >= 0);
        return (unsigned int)xy % BOARDSIZE;
    }

    /*!
        Returns the y co-ordinate of the move.
        Do not call this for NONE, PASS or RESIGN moves.
    */

    inline unsigned int getY() const {
           assert(xy >= 0);
        return (unsigned int)xy / BOARDSIZE;
    }

    /*!
        Creates a PASS move.
    */
    static GoMove pass() {
        return GoMove(XYPASS);
    }

    /*!
        Creates a NONE move.
    */
    static GoMove none() {
        return GoMove(XYNONE);
    }

    /*!
        Creates a RESIGN move.
    */
    static GoMove resign() {
        return GoMove(XYRESIGN);
    }

    /*!
        Gives a numerical representation of a move (x,y).
    */
    static int xyToMoveId(unsigned int x,unsigned  int y) {
        assert(x < BOARDSIZE && y < BOARDSIZE);

        return     x + (y * BOARDSIZE);
    }

    /*!
        Creates a move (x,y).
    */
    static GoMove move(unsigned int x, unsigned int y) {
        return GoMove(xyToMoveId(x, y));
    }

    /*!
        Default constructor creates a NONE move.
        If you actually want a NONE move, I recommend using GoMove::none()
        to make it clear what you are doing - a default constructor is provided
        because it makes some things like arrays easier.
    */
    GoMove() :
        xy(XYNONE)
    {}

    /*!
        Creates a move from a numerical move representation.
    */
    GoMove(int _xy) {
        xy = _xy;
    }

    /*!
        Comparison operator (based on the move's numerical representation)
        so that GoMoves can be used in std::set and std::map efficiently.
    */
    bool operator < (const GoMove &other) const {
        return xy < other.xy;
    }

    /*!
        Returns true if both moves are the same.
    */
    bool operator == (const GoMove &other) const {
        return xy == other.xy;
    }

    /*!
        Returns false if both moves are the same.
    */
    bool operator != (const GoMove &other) const {
        return xy != other.xy;
    }

    inline StaticVector<GoMove, 4> adjacentPoints_SV() const {
        unsigned int y = getY(), x = xy - (y * BOARDSIZE);
        assert(x == getX());

        StaticVector<GoMove, 4> ret;
        if (x > 0)             ret.push_back(GoMove(xy - GoMove::X_INCREASE_TO_XY));
        if (x < BOARDSIZE - 1) ret.push_back(GoMove(xy + GoMove::X_INCREASE_TO_XY));
        if (y > 0)             ret.push_back(GoMove(xy - GoMove::Y_INCREASE_TO_XY));
        if (y < BOARDSIZE - 1) ret.push_back(GoMove(xy + GoMove::Y_INCREASE_TO_XY));

        return ret;
    }

};

#endif
