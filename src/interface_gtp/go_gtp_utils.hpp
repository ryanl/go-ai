#ifndef __Go_GTP_UTILS_HPP
#define __Go_GTP_UTILS_HPP

#include "generic/gtp_parser_callbacks.hpp"
#include "go_mechanics/go_state.hpp"

inline char colToLetter(unsigned int y) {
    assert(y < BOARDSIZE);

    char ret = 'A' + y;
    if (ret >= 'I') ret++; // 'I' is omitted - this is a really silly requirement of the protocol
    return ret;
}


inline std::string boardToString(GoState s) {
    std::string ret;

    if (BOARDSIZE >= 10) {
        ret.append("\n   ");
    } else {
        ret.append("\n  ");
    }

    for (unsigned int col = 0; col < BOARDSIZE; col++) {
        ret.push_back(colToLetter(col));
        ret.push_back(' ');
    }
    ret.push_back('\n');


    for (unsigned int row = BOARDSIZE - 1; row < BOARDSIZE; row--) {
        std::string row_str = intToString(row + 1);
        if (BOARDSIZE >= 10) {
            assert(BOARDSIZE < 100);
            if (row_str.size() == 1) ret.push_back(' ');
        }
        ret.append(row_str);

        ret.push_back(' ');

        for (unsigned int col = 0; col < BOARDSIZE; col++) {
            switch (s.get(col, row)) {
                case BLACK:
                    ret.push_back('X');
                    break;

                case EMPTY:
                    ret.push_back('_');
                    break;

                case WHITE:
                    ret.push_back('O');
                    break;

                default:
                    assert(0);
            }
            ret.push_back(' ');
        }

        ret.append("\n");
    }

    if (s.getNextToPlay() == WHITE) {
        ret.append("WHITE to play");
    } else {
        ret.append("BLACK to play");
    }

    return ret;
}

inline int stringToBlackWhite(std::string s) {
    for (unsigned int i = 0; i < s.length(); i++) {
        s[i] = tolower(s[i]);
    }

    if (s == "w")     return WHITE;
    if (s == "white") return WHITE;
    if (s == "b")     return BLACK;
    if (s == "black") return BLACK;

    return EMPTY;
}

inline void makeLowercase(std::string &s) {
    for (unsigned int i = 0; i < s.length(); i++) {
        s[i] = tolower(s[i]);
    }
}

inline GoMove stringToMove(std::string s) {
    makeLowercase(s);

    if (s == "pass") {
        return GoMove::pass();
    } else if (s == "resign") {
        return GoMove::resign();
    }

    if (s.size() < 2) {
        return GoMove::none();
    }

    char letter_part = s[0];

    if (letter_part == 'i') {
        return GoMove::none(); // 'I' is omitted - this is a really silly requirement of the protocol
    }

    unsigned int col;

    if (letter_part < 'i') {
        col = letter_part - 'a';
    } else {
        col = (letter_part - 'a') - 1;
    }

    std::string number_part = s.substr(1);
    if (!isInt(number_part)) {
        return GoMove::none();
    }
    unsigned int row = stringToInt(number_part) - 1;

    if (row > BOARDSIZE || col > BOARDSIZE) {
        return GoMove::none();
    } else {
        return GoMove::move(col, row);
    }
}

inline std::string moveToString(GoMove move) {
    if (move.isPass()) {
        return "pass";
    } else if (move.isResign()) {
        return "resign";
    } else if (move.isNone()) {
        return "none";
    } else {
        std::string ret = "";
        ret.push_back(colToLetter(move.getX()));
        ret.append(intToString(1 + move.getY()));


        return ret;
    }
}

inline bool isBlackOrWhite(const std::string& s) {
    int colour = stringToBlackWhite(s);
    return colour != EMPTY;
}

#endif
