/* This file uses lots of obscure boost libraries */

#include <iostream>
#include <string>

#ifdef HAS_BOOST_MATH
#include <boost/math/special_functions/beta.hpp> // for ibeta
#endif

#include <pstreams/pstream.h> // for process control functions
//#include <boost/thread.hpp>

#include "random/rng.hpp"
#include "go_mechanics/go_state.hpp"
#include "interface_gtp/generic/gtp_parser.hpp"
#include "interface_gtp/generic/external_gtp_process.hpp"
#include "interface_gtp/go_gtp_utils.hpp"
#include <unistd.h> // for sleep

using namespace std;

inline bool isEven(int i) {
    return (i & 1) == 0;
}


class WorkerGroup;

struct CallableGamePlayer {
    bool play;

    unsigned int thread_id;
    vector<std::string> progs;
    vector<unsigned int> prog_ids;

    unsigned int time;
    WorkerGroup *wg;

    void operator () ();

    // with a as black, b as white
    int playGame(ExternalGTPProcess& child_a, ExternalGTPProcess& child_b, unsigned int time, std::vector<GoMove>& moves) {
        child_a.sendGTPCommand("boardsize 9");
        child_b.sendGTPCommand("boardsize 9");

        child_a.sendGTPCommand("clear_board");
        child_b.sendGTPCommand("clear_board");

        child_a.sendGTPCommand("komi 6.5");
        child_b.sendGTPCommand("komi 6.5");

        child_a.sendGTPCommand("time_left b " + intToString(time) + " 0");
        child_b.sendGTPCommand("time_left w " + intToString(time) + " 0");

        GoState s = GoState::newGame(SUPERKO_POSITIONAL);

        for (unsigned int n = 0;; n++) {
            cerr << "Move " << n << "\n";
            GTPResponse resp;

            if (isEven(n)) {
                resp = child_a.sendGTPQuery("genmove b");
            } else {
                resp = child_b.sendGTPQuery("genmove w");
            }

            GoMove move = stringToMove(resp.getMsg());

            if (move.isNone() || move.isResign() || !s.isValidMove(move)) {
                cerr << "Player tried to play " << resp.getMsg() << " which was not a valid move\n";
                return isEven(n) ? 1 : 0;

            } else {
                moves.push_back(move);

                if (move.isPass() && s.getPreviousMoveWasPass())
                    break;

                s.makeMove(move);

                if (isEven(n)) {
                    child_b.sendGTPCommand("play b " + moveToString(move));
                } else {
                    child_a.sendGTPCommand("play w " + moveToString(move));
                }
            }
        }

        int winner_colour = s.getWinnerOfGame();
        assert(winner_colour == BLACK || winner_colour == WHITE);

        return winner_colour == BLACK ? 0 : 1;
    }
};

class WorkerGroup {
private:
    //boost::mutex m;

    vector<CallableGamePlayer> callables;
    //boost::thread_group tg;
    unsigned int threads;

    /*! do not call this from another thread while WorkerGroup.wait() is in progress */
    void addThread() {

        CallableGamePlayer c;
        c.play = true;
        c.time = time;
        c.progs.resize(2);
        c.prog_ids.resize(2);
        c.wg = this;
        c.thread_id = threads; // tg.size(); - old version of boost don't have this so I don't use it here
        threads++;

        gameCompleteOrInit(c, -1, 0, 0, true);
        callables.push_back(c);

        //tg.create_thread(callables[callables.size() - 1]);
    }

public:


    void outputStats() {
        for (unsigned int j = 0; j < wins.size(); j++) {
            if (j) cout << ", ";
            cout << "progs[" << j << "] = " << wins[j] << "/" << plays[j] << " = " << (wins[j] * 100.0f / plays[j]) << "%";
        }
        cout << "\n";

#ifdef HAS_BOOST_MATH
        if (wins.size() == 2) {
            float chance_0_stronger_than_1 = boost::math::ibeta(1.0f + wins[1], 1.0f + wins[0], 0.5f);
            cout << "Chance progs[0] is stronger than progs[1] = " << (chance_0_stronger_than_1 * 100.0f) << "%\n";
        }
#endif
    }

    RNG rng;
    vector<string> progs;
    vector<unsigned int> wins;
    vector<unsigned int> plays;

    unsigned int games_so_far;
    unsigned int games;
    unsigned int time;
    bool batch_mode;

    WorkerGroup() :
        progs(0),
        wins(0),
        plays(0),
        games_so_far(0),
        games(0),
        time(0),
        threads(0)
    {}

    void addProgram(std::string program_command_line) {
        if (games_so_far != 0) abort();

        progs.push_back(program_command_line);
        wins.push_back(0);
        plays.push_back(0);
    }

    void gameCompleteOrInit(CallableGamePlayer &caller, unsigned int winner_colour, unsigned int winner_id, unsigned int loser_id, bool init) {
       // std::cout << "Attempting to lock\n";
        {
            //boost::mutex::scoped_lock l(m);
            //std::cout << "Locked\n";

            if (!init) {
                wins[winner_id]++;
                plays[winner_id]++;
                plays[loser_id]++;

                   if (!batch_mode) {
                       std::string winner_colour_name = (int)winner_colour == BLACK ? "BLACK" : "WHITE";
                    cout << "progs[" << winner_id << "] = " << progs[winner_id] << " (" << winner_colour_name << ") won against progs[" << loser_id << "] = " << progs[loser_id] << "\n";
                    outputStats();
                }
            }

            if (games_so_far == games && games > 0) {
                caller.play = false;
                return;
            } else {
                std::cerr << "Beginning game " << games_so_far << "\n";

                // choose a pair of players
                unsigned int least_losses = games_so_far + 1, least_losses_n = 0;
                for (unsigned int n = 0; n < progs.size(); n++) {
                    unsigned int losses = plays[n] - wins[n];
                    if (losses < least_losses) {
                        least_losses = losses;
                        least_losses_n = n;
                    }
                }

                least_losses = games_so_far + 1;
                unsigned int least_losses_m = 0;

                for (unsigned int m = 0; m < progs.size(); m++) {
                    unsigned int losses = plays[m] - wins[m];
                    if (m != least_losses_n && losses < least_losses) {
                        least_losses = losses;
                        least_losses_m = m;
                    }
                }

                // randomly decide who will play BLACk
                if (rng.getBool()) swap(least_losses_n, least_losses_m);

                caller.progs[0] = progs[least_losses_n];
                caller.prog_ids[0] = least_losses_n;

                caller.progs[1] = progs[least_losses_m];
                caller.prog_ids[1] = least_losses_m;

                games_so_far++;
            }
        }
        //std::cout << "Unlocked\n";
    }

    void run(unsigned int threads) {
        if (threads != 1) abort();
        addThread();
        callables[0]();
        /* for (unsigned int i = 0; i < threads; i++) {
            addThread();
        }
        tg.join_all(); */
    }
};

void CallableGamePlayer::operator () () {
    unsigned int k = 0;

    while (play) {
        k++;
        std::cerr << "Iteration " << k << " of thread " << thread_id << "\n";
        std::cerr << "Playing " << progs[0] << " against " << progs[1] << "\n";

        ExternalGTPProcess child_a(progs[0]);
        ExternalGTPProcess child_b(progs[1]);

        std::vector<GoMove> moves;

        unsigned int winner_colour = playGame(child_a, child_b, time, moves);
        unsigned int winner_id = prog_ids[winner_colour];
        unsigned int loser_id = prog_ids[1 - winner_colour];
        wg->gameCompleteOrInit(*this, winner_colour, winner_id, loser_id, false);

        /*
        if (child_a_has_printsgf) {
            child_a.sendGTPCommand("printsgf " + intToString(k) + ".sgf");
        } else if (child_b_has_printsgf) {
            child_b.sendGTPCommand("printsgf " + intToString(k) + ".sgf");
        }
        */

        child_a.sendGTPQuery("quit");
        child_b.sendGTPQuery("quit");
    }


    std::cerr << "Thread " << thread_id << "terminated.\n";
}


int main(int argc, char* argv[]) {
    WorkerGroup wg;

    wg.batch_mode = false;

    if (argc != 4) {
        if (argc == 5 && std::string(argv[4])== "--batch") {
            wg.batch_mode = true;
        } else {
            cout << "Usage: " << argv[0] << " <games per unordered pair> <time> <parallelgames> [--batch]\n";
            cout << "       (provide a list of programs on stdin)\n";
            return 1;
        }
    }

    std::string line;
    while (std::getline(std::cin, line))
    {
        wg.addProgram(line);
    }

    unsigned int n_choose_2 = (wg.progs.size() * (wg.progs.size() - 1)) / 2;

    wg.games = stringToInt(argv[1]) * n_choose_2;
    wg.time = stringToInt(argv[2]);

    unsigned int parallel_games = stringToInt(argv[3]);

    if (parallel_games != 1) {
        std::cout << "pstreams is not thread safe so I am only allowing one thread now.";
        return 1;
    }
    if (wg.games == 0 && wg.batch_mode) {
        std::cout << "You can't have infinite games and batch_mode\n";
        return 1;
    }

    if (!wg.batch_mode) {
        for (unsigned int i = 0; i < wg.progs.size(); ++i) {
            cout << "progs[" << i << "]: " << wg.progs[i] << "\n";
        }
        cout << "games: " << wg.games;

        if (wg.games == 0) {
            wg.batch_mode || cout << " (i.e. unlimited)";
        }
        cout << ", parallelism: " << parallel_games << ", time: " << wg.time << " seconds per player per game\n";
    }

    wg.run(parallel_games);

    if (wg.batch_mode) {
        for (unsigned int i = 0; i < wg.progs.size(); i++) {
            cout << wg.wins[i] << "\n";
        }
    } else {

        cout << "\n\nFinal scores\n";
        for (unsigned int i = 0; i < wg.progs.size(); i++) {
            cout << wg.progs[i] << ": " << wg.wins[i] << " / " << wg.plays[i] << "\n";
        }

    }
}

