#include "../go_ai/uct/go_uct.hpp"
#include <ctime>
#include <iostream>

#include <valgrind/callgrind.h>

using namespace std;
int main(int argc, char *argv[]) {
    GoState s = GoState::newGame(SUPERKO_POSITIONAL);

    if (BOARDSIZE == 5) {
        s.setKomi(2.5);
    }

    GoUCTSettings settings;

    unsigned int ponders = 200;

    if (argc == 2) {
        ponders = atoi(argv[1]);
    } else if (argc != 1) {
        std::cout << "Usage: " << argv[0] << " [ponders]\n";
    }

    settings.use_patterns = true;
    settings.max_mem_mb = ponders * 1.5;
    if (settings.max_mem_mb > 600) settings.max_mem_mb = 600;

    GoUCT ai(s, settings);

    unsigned int start = clock();
    cerr << "Start: " << float(start) / CLOCKS_PER_SEC << "\n";

    CALLGRIND_START_INSTRUMENTATION;

    for (unsigned int i = 0; i < ponders; i++) {
        ai.ponder();
    }
    CALLGRIND_STOP_INSTRUMENTATION;
    CALLGRIND_DUMP_STATS;

    float cpu_time_used = float(clock() - start) / CLOCKS_PER_SEC;
    cout << cpu_time_used << "\n";

    float simulations_per_second = ponders * SIMULATIONS_PER_PONDER  / cpu_time_used;
    cerr << simulations_per_second << " sims per second\n";
}

