#ifndef __GO_GTP_INTERFACE_HPP
#define __GO_GTP_INTERFACE_HPP

#include <ctime>
#include <string>

#include <sstream>

//#include "sgf/sgf.hpp"

#include "go_ai/go_state_anaylsis/go_state_analyser.hpp"

#include "go_mechanics/go_state.hpp"

#include "generic/gtp_parser.hpp"

#include "go_ai/uct/go_uct_threadinterface.hpp"
#include "go_gtp_utils.hpp"
#include "go_callbacks/callbacks.hpp"

#include "console_arguments.hpp"

class GoGTPInterface {
private:
    const ConsoleArguments &args;

    GoState s;
    GoUCT_ThreadInterface ai_interface;

    GTPCallbackStatic       cb_protocol_version;
    GTPCallbackBoardsize    cb_boardsize;
    GTPCallbackClearBoard   cb_clear_board;
    GTPCallbackShowboard    cb_showboard;
    GTPCallbackKomi         cb_komi;
    GTPCallbackPlay         cb_play;
    GTPCallbackGenmove      cb_genmove;
    GTPCallbackFinalScore   cb_final_score;
    GTPCallbackTimeLeft     cb_time_left;
    GTPCallbackTimeSettings    cb_time_settings;
    GTPCallbackQuit         cb_quit;
    GTPCallbackCputime      cb_cputime;
//    GTPCallbackLoadSGF      cb_loadsgf;

    int black_time_left, white_time_left;

public:
    GoGTPInterface(const ConsoleArguments &_args) :
        args(_args),
        s(GoState::newGame(SUPERKO_POSITIONAL)),
        ai_interface(s, _args),
        cb_protocol_version("2"),
        cb_boardsize(this),
        cb_clear_board(this),
        cb_showboard(this),
        cb_komi(this),
        cb_play(this),
        cb_genmove(this),
        cb_final_score(this),

        cb_time_left(this),
        cb_time_settings(this),
        cb_quit(this),
        cb_cputime(this),
//        cb_loadsgf(this),

        // 5 mins is a sensible default
        black_time_left(300),
        white_time_left(300)
    {

    }

    void registerCallbacksWithParser(GTPParser& p) {
        p.addCommandCallback("boardsize", &cb_boardsize);
        p.addCommandCallback("protocol_version", &cb_protocol_version);
        p.addCommandCallback("clear_board", &cb_clear_board);
        p.addCommandCallback("showboard", &cb_showboard);
        p.addCommandCallback("komi", &cb_komi);
        p.addCommandCallback("play", &cb_play);
        p.addCommandCallback("genmove", &cb_genmove);
        p.addCommandCallback("final_score", &cb_final_score);

        p.addCommandCallback("time_settings", &cb_time_settings);
        p.addCommandCallback("time_left", &cb_time_left);
        p.addCommandCallback("quit", &cb_quit);
        p.addCommandCallback("cputime", &cb_cputime);
//        p.addCommandCallback("loadsgf", &cb_loadsgf);
    }

    // board_size <size>
    GTPResponse boardsize(unsigned int size) {
        if (size != BOARDSIZE) {
            return GTPResponse(GTP_FAILURE, "unacceptable size # " + intToString(BOARDSIZE) + " only");
        } else {
            return GTPResponse(GTP_SUCCESS, "");
        }
    }

    // board_size <size>
    GTPResponse komi(float f) {
        s.setKomi(f);
        ai_interface.resetToNewState(s);

        return GTPResponse(GTP_SUCCESS, "");
    }

    GTPResponse showboard() {
        std::string ret = "Note: showboard is for debugging only\n";

        return GTPResponse(GTP_SUCCESS, boardToString(s));
    }


    // clear_board
    GTPResponse clear_board() {
        float old_komi = s.getKomi();
        s = GoState::newGame(SUPERKO_POSITIONAL);
        s.setKomi(old_komi);
        ai_interface.resetToNewState(s);

        return GTPResponse(GTP_SUCCESS, "");
    }

    // play
    GTPResponse play(int stone_colour, GoMove position) {
        if (stone_colour == s.getNextToPlay()) {
            if (s.isValidMove(position)) {
                s.makeMove(position);
                ai_interface.notifyPlayHasBeenMade(position);

                return GTPResponse(GTP_SUCCESS, "");
            } else {
                return GTPResponse(GTP_FAILURE, "illegal move");
            }
        } else {
            return GTPResponse(GTP_FAILURE, "Consecutive moves of the same color are not supported.");
        }
    }

    // genmove
    GTPResponse genmove(int stone_colour, bool verbose = false) {

        if (stone_colour == s.getNextToPlay()) {
             time_t start = time(NULL);

            GoMove move = GoMove::none();

            std::string ai_type = args.get("ai", "");

            int time_left = (stone_colour == BLACK) ? black_time_left : white_time_left;

            if (ai_type == "simulate") {
                RNG rng;
                PatternMatcher p;
                GoStateAnalyser gsa(s, rng, p);
                move = gsa.selectMoveForSimulation_Mogo<true>();
            } else {
                move = ai_interface.selectMove(time_left, verbose);
                std::cerr << "Performing update (tree cull after move play)\n";
                ai_interface.notifyPlayHasBeenMade(move);
            }
            if (!move.isResign()) {
                s.makeMove(move);
            }

            time_t end = time(NULL);
            time_left -= (end - start); // estimate time used in case we don't have time_left
            if (time_left < 0) {
                time_left = 0;
            }

            std::cerr << "Spent approx " << (end - start) << " secs thinking\n";
            (stone_colour == BLACK ? black_time_left : white_time_left) = time_left;

            std::cerr << boardToString(s) << "\n";

            return GTPResponse(GTP_SUCCESS, moveToString(move));

        } else {
            return GTPResponse(GTP_FAILURE, "Consecutive moves of the same color are not supported.");
        }
    }


    // time_settings
    GTPResponse time_settings(int main_time, int byo_yomi_time, int byo_yomi_stones) {
        // TODO: implement this prooperly

        white_time_left = main_time;
        black_time_left = main_time;

        return GTPResponse(GTP_SUCCESS, "");
    }


    // time_left
    GTPResponse time_left(int colour, int time, int stones) {
        // TODO: implement stones (byo-yomi) correctly

        if (colour == WHITE) {
            white_time_left = time;
        } else if (colour == BLACK) {
            black_time_left = time;
        }

        return GTPResponse(GTP_SUCCESS, "");
    }

    // cputime
    // used by gogui-regress
    GTPResponse cputime() {
        float cpu_time_used = float(clock()) / CLOCKS_PER_SEC;

        return GTPResponse(GTP_SUCCESS, toString(cpu_time_used));
    }

    // loadsgf -- disabled, needs a library
/*
    GTPResponse loadsgf(std::string filename, int moves_to_use) {
        SGFReader reader;
        std::vector<GoMove> moves = reader.loadSGF(filename);

        if (moves_to_use > moves.size()) {
            return GTPResponse(GTP_FAILURE, "SGF file contains only " + intToString(moves.size()) + " moves");
        } else {
            std::cerr << "SGF file contains " << intToString(moves.size()) << " moves\n";

            for (unsigned int i = 0; i < moves_to_use; i++) {
                s.makeMove(moves[i]);
                ai_interface.notifyPlayHasBeenMade(moves[i]);
            }
        }
        return GTPResponse(GTP_SUCCESS, "");
    }
*/

    // final_score
    GTPResponse final_score() {
        ScoredGame sg = s.scoreGame();

        std::ostringstream oss;

        if (sg.black_score > sg.white_score) {
             oss << "B+" << (sg.black_score - sg.white_score);
        } else if (sg.white_score > sg.black_score) {
             oss << "W+" << (sg.white_score - sg.black_score);
        } else {
            oss << "0"; // draw is represented by a zero
        }
        /*
        for (unsigned int y = BOARDSIZE - 1; y < BOARDSIZE; y--) {
            oss << "\n# ";
            for (unsigned int x = 0; x < BOARDSIZE; x++) {
                if (sg.owned_by[x][y] == WHITE) oss << "W ";
                else if (sg.owned_by[x][y] == BLACK) oss << "B ";
                else oss << "_ ";
            }
        }
        */

        return GTPResponse(GTP_SUCCESS, oss.str());
    }

    GTPResponse quit() {
        exit(0);
    }
};

/*
    Required commands:

    "protocol_version",
    "name",
    "version",
    "known_command",
    "list_commands",
    "boardsize",
    "quit",
    "clear_board",
    "komi",
    "play"
*/

#endif
