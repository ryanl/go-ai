#include "../go_gtp_interface.hpp"

/* boardsize */

GTPResponse GTPCallbackBoardsize::callback(const std::vector<std::string>& args) {
    if (args.size() != 1 || !isInt(args[0])) {
        return GTPResponse(GTP_FAILURE, "syntax error # board_size takes one integer argument");
    } else {
        int i = stringToInt(args[0].c_str());
        return parent->boardsize(i);
    }
}

/* clear_board */

GTPResponse GTPCallbackClearBoard::callback(const std::vector<std::string>& args) {
    if (args.size() != 0) {
        return GTPResponse(GTP_FAILURE, "syntax error # clear_board takes no arguments");
    } else {
        return parent->clear_board();
    }
}

/* showboard */

GTPResponse GTPCallbackShowboard::callback(const std::vector<std::string>& args) {
    if (args.size() != 0) {
        return GTPResponse(GTP_FAILURE, "syntax error # showboard takes no arguments");
    } else {
        return parent->showboard();
    }
}

/* komi */

GTPResponse GTPCallbackKomi::callback(const std::vector<std::string>& args) {
    if (args.size() != 1 || !isFloat(args[0])) {
        return GTPResponse(GTP_FAILURE, "invalid syntax # komi takes one float argument");
    } else {
        double f = stringToFloat(args[0].c_str());
        return parent->komi(f);
    }
}

/* play */

GTPResponse GTPCallbackPlay::callback(const std::vector<std::string>& args) {
    if (args.size() != 2) {
        return GTPResponse(GTP_FAILURE, "invalid syntax # play takes colour and position as arguments");
    } else {
        int colour = stringToBlackWhite(args[0]);

        if (colour == EMPTY) {
            return GTPResponse(GTP_FAILURE, "invalid syntax # colour argument should be 'black' or 'white' (or 'b' or 'w')");
        }

        GoMove move = stringToMove(args[1]);
        if (move.isNone()) {
            return GTPResponse(GTP_FAILURE, "invalid syntax # bad position format");
        }

        return parent->play(colour,  move);
    }
}

/* genmove */

GTPResponse GTPCallbackGenmove::callback(const std::vector<std::string>& args) {
    if (args.size() == 1 || args.size() == 2) {
        int colour = stringToBlackWhite(args[0]);
        if (colour == EMPTY) {
            return GTPResponse(GTP_FAILURE, "invalid syntax # colour argument should be 'black' or 'white' (or 'b' or 'w')");
        } else {
            if (args.size() == 2) {
                if (args[1] == "verbose") {
                    return parent->genmove(colour, true);
                } else {
                    return GTPResponse(GTP_FAILURE, "invalid syntax # genmove takes a colour as its argument");
                }
            } else {
                return parent->genmove(colour);
            }
        }
    } else {
        return GTPResponse(GTP_FAILURE, "invalid syntax # genmove takes a colour as its argument");
    }
}

/* final_score */

GTPResponse GTPCallbackFinalScore::callback(const std::vector<std::string>& args) {
    if (args.size() != 0) {
        return GTPResponse(GTP_FAILURE, "invalid syntax # final_score takes no arguments");
    } else {
        return parent->final_score();
    }
}

/* time_left */

GTPResponse GTPCallbackTimeLeft::callback(const std::vector<std::string>& args) {
    if (args.size() != 3) {
        return GTPResponse(GTP_FAILURE, "invalid syntax # time_left takes three arguments");
    } else {
        int colour = stringToBlackWhite(args[0]);
        if (colour == EMPTY) {
            return GTPResponse(GTP_FAILURE, "invalid syntax # colour argument should be 'black' or 'white' (or 'b' or 'w')");
        }
        if (!isInt(args[1]) || !isInt(args[2])) {
            return GTPResponse(GTP_FAILURE, "invalid syntax # second and third arguments should be integers");
        }

        int time = stringToInt(args[1]);
        int stones = stringToInt(args[2]);

        return parent->time_left(colour, time, stones);
    }
}

/* time_settings */

GTPResponse GTPCallbackTimeSettings::callback(const std::vector<std::string>& args) {
    if (args.size() != 3 || !isInt(args[0]) || !isInt(args[1]) || !isInt(args[2])) {
        return GTPResponse(GTP_FAILURE, "invalid syntax # time_settings takes three  integer arguments");
    } else {
        int main_time       = stringToInt(args[0]);
        int byo_yomi_time   = stringToInt(args[1]);
        int byo_yomi_stones = stringToInt(args[2]);

        return parent->time_settings(main_time, byo_yomi_time, byo_yomi_stones);
    }
}

/* quit */

GTPResponse GTPCallbackQuit::callback(const std::vector<std::string>& args) {
    if (args.size() != 0) {
        return GTPResponse(GTP_FAILURE, "invalid syntax # quit takes no arguments");
    } else {
        return parent->quit();
    }
}


/* cputime */
// the only program I know of that uses this is gogui-regress
GTPResponse GTPCallbackCputime::callback(const std::vector<std::string>& args) {
    if (args.size() != 0) {
        return GTPResponse(GTP_FAILURE, "invalid syntax # quit takes no arguments");
    } else {
        return parent->cputime();
    }
}

/* loadsgf */
/*
GTPResponse GTPCallbackLoadSGF::callback(const std::vector<std::string>& args) {
    if (args.size() != 2 || !isInt(args[1])) {
        return GTPResponse(GTP_FAILURE, "invalid syntax # loadsgf takes two arguments: file name and move number");
    } else {
        return parent->loadsgf(args[0], stringToInt(args[1]));
    }
}
*/
