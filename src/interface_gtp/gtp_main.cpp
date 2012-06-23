#include "generic/gtp_parser.hpp"
#include "go_gtp_interface.hpp"

#include <string>
#include <algorithm>

#include <boost/assign/std/set.hpp> // for 'operator+=()'

using namespace boost::assign; // bring 'operator+=()' into scope

int main(int argc, char* argv[]) {
    // BUILD_NUM is the build number which increments every time build.py is run

    std::string build_id = "v" BUILD_NUM;

    GTPParser p("RyanBot", build_id);

    // ALLOPTS is supplied by build.py as a compiler option

    std::set<std::string> known_keys;
    known_keys += "ai_type", "rave_weight_initial", "rave_weight_final", "exploration", "memory",
                  "playouts", "num_threads", "no_rave", "no_weighted_rave", "no_patterns",
                  "grandfather_heuristic_weighting", "move_select", "no_summarise", "ai",
                  "rave_check_same", "rave_update_passes", "expansion_threshold",
                  "square_rave_weight", "include_rave_count_for_exploration";

    if (argc == 2 && (std::string(argv[1]) == "--help")) {
        std::cout << "Ryanbot " << build_id << "\n";
        std::cout << "Options: " << ALLOPTS << "\n";

        std::cout << "Command-line options accepted (prefix with -):";
        bool first = true;
        for (std::set<std::string>::const_iterator it = known_keys.begin(); it != known_keys.end(); ++it) {
            if (first) {
                first = false;
            } else {
                std::cout << ",";
            }
            std::cout << " " << *it;
        }
        std::cout << "\n";

        return 0;
    }


    ConsoleArguments args;
    args.parse(argc, argv);

    std::set<std::string> keys = args.getKeys();
    std::set<std::string> bad_keys;

    set_difference(keys.begin(), keys.end(), known_keys.begin(), known_keys.end(), std::inserter(bad_keys, bad_keys.end()));

    if (bad_keys.size() > 0) {
        for (std::set<std::string>::iterator it = bad_keys.begin(); it != bad_keys.end(); ++it) {
            std::cout << "Unknown argument: " << *it << "\n";
        }


        return 1;
    }

    GoGTPInterface gogtp(args);
    gogtp.registerCallbacksWithParser(p);

    p.run(std::cin, std::cout);
}
