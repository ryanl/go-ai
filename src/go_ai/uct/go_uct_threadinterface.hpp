#include "go_uct.hpp"
#include "../../console_arguments.hpp"

#include <sys/time.h>

class GoUCT_ThreadInterface {
    private:
        GoState s;
        GoUCTSettings settings;
        GoUCTTeam uct_team;
        const ConsoleArguments &args;

        unsigned long long time_micros() {
            struct timeval tv;
            gettimeofday(&tv, NULL);
            return (tv.tv_sec * 1000000LL) + (unsigned long long) tv.tv_usec;
        }

    public:
        GoUCT_ThreadInterface(const GoState &_s, const ConsoleArguments &_args) :
            s(_s),
            settings(GoUCTSettings::parseConsoleArgs(_args)),
            uct_team(settings.num_threads, _s, settings),
            args(_args)
        {}

        void notifyPlayHasBeenMade(GoMove move) {
            if (move.isResign()) {
                return;
            } else {
                s.makeMove(move);
                uct_team.updateAfterPlay(move);
            }
        }

        unsigned int countEmptyPositions() {
            unsigned int ret = 0;

            for (unsigned int x = 0; x < BOARDSIZE; x++) {
                for (unsigned int y = 0; y < BOARDSIZE; y++) {
                    ret += (s.get(x, y) == EMPTY) ? 1 : 0;
                }
            }

            return ret;
        }

        GoMove selectMove(unsigned int time, bool verbose = false) {
            unsigned int empties = countEmptyPositions();

            float time_allocated = time - 5.0f;

            time_allocated /= sqrtf(empties + 15.0f); // made up formula

            if (time_allocated > (time - 5.0f) / 3.0f) {
                time_allocated = (time - 5.0f) / 3.0f;
            }
            if (time_allocated < 0.1) {
                time_allocated = 0.1f;
            }

            if (settings.fixed_num_playouts == 0) {
                //unsigned long long end = time_micros() + (unsigned long long)(time_allocated * 1000000.0f);
                std::cerr << "Waiting " << int(time_allocated * 1000.0f) << " ms (threadless) = " << int(100.0f * time_allocated / time) << "% of " << int(time * 1000.0f) << "ms left, with " << empties << " empties\n";

                uct_team.ponderFor(int(time_allocated * 1000.0f));

                std::cerr << "Wait over\n";
            } else {
                std::cerr << "Performing approx. " << settings.fixed_num_playouts << " playouts per thread\n";

                uct_team.ponderFor(0, settings.fixed_num_playouts);
            }

            GoMove ret = uct_team.selectMove();

            return ret;
        }

        void resetToNewState(const GoState &s_new) {
            s = s_new;
            uct_team.resetToNewState(s_new);
        }
};

