#include "../../console_arguments.hpp"

struct GoUCTSettings {
    enum MoveSelectCriterion {
        SELECT_MAX_TIMES_PLAYED = 0,
        SELECT_MAX_VALUE_ESTIMATE = 1,  // probably good
        SELECT_MAX_MEAN_WINS = 2
    };

    /*! If set, GoUCT will use the UCB1 Tuned bandit selection formula (with its variance estimate
        component) rather than the standard UCB1 formula. This has no effect if
        exploration_constant is set to zero.
    */

    bool use_ucb1_tuned;

    /* Size of the tree per thread in megabytes. */
    size_t max_mem_mb;

    /* If set, RAVE will be used to estimate move values */
    bool use_rave;

    /* Used by RAVE to determine what weighting to give to RAVE values in comparison to the normal wins / times_played value */
    float rave_weight_initial, rave_weight_final;

    /* Resigns if the estimate win 'chance' is very small */
    bool resign_if_appropriate;

    /* Reuses the part of the tree that is still valid */
    bool reuse_tree;

    /* Reuses the part of the tree that is still valid */
    bool weighted_rave;

    float exploration_constant;

    bool include_rave_count_for_exploration;

    bool use_patterns;

    unsigned int num_threads;

    unsigned int fixed_num_playouts;
    std::string opening_book;

    MoveSelectCriterion move_select_criterion;

    float grandfather_heuristic_weighting;

    bool summarise_tree_structure;

    unsigned int expansion_threshold; // create node children after this many plays, min value 1. A value > 1 reduces memory usage and improves speed a little but slows tree growth.

    bool rave_update_passes;

    bool rave_check_same;

    bool square_rave_weight;

    /*! rave_param1 and rave_param2 are calculated from rave_weight_initial and rave_weight_final */
    float rave_param1, rave_param2;

    void updateCachedParamValues() {
        rave_param1 = 1.0f / rave_weight_initial;
        rave_param2 = 1.0f / rave_weight_final;
    }

    GoUCTSettings() :
        use_ucb1_tuned(false),
        max_mem_mb(350),
        use_rave(true),
        rave_weight_initial(1.0f),
        rave_weight_final(5000.0f),
        resign_if_appropriate(true),
        reuse_tree(false),
        weighted_rave(true),
        exploration_constant(0.1f),
        include_rave_count_for_exploration(false),
        use_patterns(true),
        num_threads(1),
        fixed_num_playouts(0),
        opening_book(""),
        move_select_criterion(SELECT_MAX_TIMES_PLAYED),
        grandfather_heuristic_weighting(4.0f),
        summarise_tree_structure(false), // debugging info
        expansion_threshold(2),
        rave_update_passes(false),
        rave_check_same(false)
        //square_rave_weight(false)
    {
        updateCachedParamValues();
    }

    static GoUCTSettings parseConsoleArgs(const ConsoleArguments &args) {
        GoUCTSettings s;

        if (args.has("rave_weight_initial")) {
            s.rave_weight_initial = atof(args.get("rave_weight_initial")->c_str());
        }

        if (args.has("rave_weight_final")) {
            s.rave_weight_final = atof(args.get("rave_weight_final")->c_str());
        }

        if (args.has("exploration")) {
            s.exploration_constant = atof(args.get("exploration")->c_str());
        }

        if (args.has("memory")) {
            s.max_mem_mb = atoi(args.get("memory")->c_str());
        }

        if (args.has("include_rave_count_for_exploration")) {
            s.include_rave_count_for_exploration = true;
        }


        if (args.has("playouts")) {
            s.fixed_num_playouts = atoi(args.get("playouts")->c_str());
        }


        if (args.has("num_threads")) {
            s.num_threads = atoi(args.get("num_threads")->c_str());
        }

        if (args.has("no_rave")) {
            s.use_rave = false;
        }

        if (args.has("no_weighted_rave")) {
            s.weighted_rave = false;
        }

        if (args.has("no_patterns")) {
            s.use_patterns = false;
        }

        if (args.has("grandfather_heuristic_weighting")) {
            s.grandfather_heuristic_weighting = atof(args.get("grandfather_heuristic_weighting")->c_str());
        }

        if (args.has("opening_book")) {
            s.opening_book = *args.get("opening_book");
        }

        if (args.has("move_select")) {
            std::string ms = *args.get("move_select");
            if (ms == "times_played")        s.move_select_criterion = SELECT_MAX_TIMES_PLAYED;
            else if (ms == "value_estimate") s.move_select_criterion = SELECT_MAX_VALUE_ESTIMATE;
            else if (ms == "mean_wins")      s.move_select_criterion = SELECT_MAX_MEAN_WINS;
            else {
                std::cout << "Valid move_select options: times_played, value_estimate, mean_wins\n";
                abort();
            }
        }

        if (args.has("no_summarise")) {
            s.summarise_tree_structure = false;
        }

        if (args.has("expansion_threshold")) {
            s.expansion_threshold = atof(args.get("expansion_threshold")->c_str());
        }

        if (args.has("rave_update_passes")) {
            s.rave_update_passes = atoi(args.get("rave_update_passes")->c_str());
        }

        if (args.has("rave_check_same")) {
            s.rave_check_same = atoi(args.get("rave_check_same")->c_str());
        }
        /*
        if (args.has("square_rave_weight")) {
            s.square_rave_weight = atoi(args.get("square_rave_weight")->c_str());
        }
        */

        s.updateCachedParamValues();

        return s;
    }
};

