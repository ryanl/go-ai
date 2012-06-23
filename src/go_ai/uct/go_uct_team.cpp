#include "go_uct_team.hpp"

using namespace std;


void GoUCTTeam::resetToNewState(const GoState& s) {
    for (unsigned int i = 0; i < team_members.size(); i++) {
        team_members[i]->resetToNewState(s);
        //abort();
    }
}

void GoUCTTeam::updateAfterPlay(const GoMove move) {
    for (unsigned int i = 0; i < team_members.size(); i++) {
        team_members[i]->updateAfterPlay(move);
    }
}

#ifdef BOOST_THREAD
class WorkerFunctor {
    unsigned int i;
    GoUCTTeam *parent;
    unsigned int max_sims;

public:
    WorkerFunctor(unsigned int _i, GoUCTTeam *_parent, unsigned int _max_sims) :
        i(_i),
        parent(_parent),
        max_sims(_max_sims)
    {}

    void operator () () {
        GoUCT *ai = parent->team_members[i];
        unsigned int ponders = 0, max_ponders = 0;

        if (max_sims != 0) {
            max_ponders = ((max_sims - 1) / SIMULATIONS_PER_PONDER) + 1;
        }

        for (;;) {
            ai->ponder();

            ponders++;

            if (max_ponders != 0 && ponders >= max_ponders) return;

            {
                boost::mutex::scoped_lock l(parent->m);
                if (parent->please_terminate) return; // terminate thread
            }
        }
    }
};
#endif

GoUCTTeam::GoUCTTeam(const unsigned int num_members, const GoState& s, const GoUCTSettings& _settings) :
    settings(_settings)
{
    for (unsigned int i = 0; i < num_members; i++) {
        team_members.push_back(new GoUCT(s, _settings));
    }
}

GoUCTTeam::~GoUCTTeam() {
    for (unsigned int i = 0; i < team_members.size(); i++) {
        delete team_members[i];
    }
}

void GoUCTTeam::ponderFor(unsigned int sleep_ms, unsigned int max_sims) {
#ifdef USE_BOOST_THREAD
    please_terminate = false;

    std::vector<boost::thread*> threads;
    std::vector<WorkerFunctor*> wfs;

    for (unsigned int i = 0; i < team_members.size(); i++) {
        WorkerFunctor* wf = new WorkerFunctor(i, this, max_sims);
        wfs.push_back(wf);
        threads.push_back(new boost::thread(*wf));
    }

    if (max_sims == 0) {
        boost::this_thread::sleep(boost::posix_time::milliseconds(sleep_ms));

        {
            boost::mutex::scoped_lock l(m);
            please_terminate = true;
        }
    }

    for (unsigned int i = 0; i < team_members.size(); i++) {
        threads[i]->join();
        delete threads[i];
        delete wfs[i];
    }
#else
    if (team_members.size() != 1) {
        std::cout << "Without boost::thread, only 1 thread is supported\n";
        assert(false);
        abort();
    }

    if (max_sims != 0) {
        unsigned int max_ponders = ((max_sims - 1) / SIMULATIONS_PER_PONDER) + 1;

        for (unsigned int ponders = 0; ponders < max_ponders; ponders++) {
            team_members[0]->ponder();
        }
    } else {
        std::cout << "Without boost::thread, only fixed playout count mode is supported\n";
        assert(false);
        abort();
    }
#endif
}
/*
 MoveSelectCriterion {
        SELECT_MAX_TIMES_PLAYED,
        SELECT_MAX_VALUE_ESTIMATE,
        SELECT_MAX_MEAN_WINS,           <-- I feel this is probably not great for small # of sims
                                            and if it is then the getValueUpperBound could be
                                            improved
    };
*/

struct NodeEvaluator {
    virtual float operator () (const std::vector< Tree<UCTNode>::ChildIterator >& nodes) const = 0;
};

struct NodeEvaluator_MaxTimesPlayed : public NodeEvaluator {
    float operator () (const std::vector<Tree<UCTNode>::ChildIterator>& nodes) const {
        unsigned int total_times_played = 0;
        for (unsigned int i = 0; i < nodes.size(); i++) {
            if (nodes[i]->val.is_win_for == -1) {
                return -1.0f;
            } else if (nodes[i]->val.is_win_for == 1)  {
                return 9999999.0f;
            } else {
                total_times_played += nodes[i]->val.times_played;
           }
        }

        return total_times_played;
    }
};

struct NodeEvaluator_MaxValueEstimate : public NodeEvaluator {
    const GoUCT* helper;
    NodeEvaluator_MaxValueEstimate(const GoUCT *_helper) :
        helper(_helper)
    {}

    float operator () (const std::vector<Tree<UCTNode>::ChildIterator>& nodes) const {
        Tree<UCTNode>::Node fake_node;

        fake_node.val.move_that_got_to_here = nodes[0]->val.move_that_got_to_here;
        fake_node.val.is_win_for = 0;
        fake_node.val.wins = 0;
        fake_node.val.times_played = 0;
        fake_node.val.rave_wins = 0;
        fake_node.val.rave_times_played = 0;

        for (unsigned int i = 0; i < nodes.size(); i++) {
            if (nodes[i]->val.is_win_for == -1) {
                return -1.0f;
            } else if (nodes[i]->val.is_win_for == 1)  {
                return 9999999.0f;
            } else {
                fake_node.val.wins += nodes[i]->val.wins;
                fake_node.val.times_played += nodes[i]->val.times_played;
                fake_node.val.rave_wins += nodes[i]->val.rave_wins;
                fake_node.val.rave_times_played += nodes[i]->val.rave_times_played;
            }
        }

        std::cerr << " (RAVE " << fake_node.val.rave_wins << " of "
                  << fake_node.val.rave_times_played << " = "
                  << (fake_node.val.rave_wins / fake_node.val.rave_times_played) << ") ";
        return helper->getValueUpperBound(&fake_node, 0.0f, 0.0f, 0.0f, false);
    }
};

struct NodeEvaluator_MaxMeanWins : public NodeEvaluator {
    float operator () (const std::vector<Tree<UCTNode>::ChildIterator>& nodes) const {
        unsigned int total_wins = 0, total_times_played = 0; // +1 prevents divide by zero
        for (unsigned int i = 0; i < nodes.size(); i++) {
            if (nodes[i]->val.is_win_for == -1) {
                return -1.0f;
            } else if (nodes[i]->val.is_win_for == 1)  {
                return 999999.0f;
            } else {
                total_wins += nodes[i]->val.wins;
                total_times_played += nodes[i]->val.times_played;
            }
        }

        return total_wins / float(total_times_played + 0.0001f);
    }
};



GoMove GoUCTTeam::selectMove() {
    unsigned int total_playouts = 0;
    std::cerr << "Komi: " << team_members[0]->initial_state.getKomi() << "\n";

    std::cerr << "Playouts: ";
    for (unsigned int i = 0; i < team_members.size(); i++) {
        if (settings.summarise_tree_structure) team_members[i]->summariseTreeStructure();

        if (i > 0) cerr << ", ";

        unsigned int x = team_members[i]->tree.getRoot()->val.times_played;
        cerr << " [" << i << "] = " << x;
        total_playouts += x;
    }

    std::cerr << " -> " << total_playouts << " total playouts\n";

    NodeEvaluator_MaxTimesPlayed   ne_mtp;
    NodeEvaluator_MaxValueEstimate ne_mve(team_members[0]);
    NodeEvaluator_MaxMeanWins      ne_mmw;

    NodeEvaluator *nes[3] = { &ne_mtp, &ne_mve, &ne_mmw };

    GoMove best_move = GoMove::pass();

    float max_f[3] = {-99999.0f, -99999.0f, -99999.0f};

    std::vector<Tree<UCTNode>::ChildIterator> its;
    for (unsigned int i = 0; i < team_members.size(); i++) {
         its.push_back(team_members[i]->tree.childBegin(team_members[i]->tree.getRoot()));
    }

    do {
        std::cerr << moveToString(its[0]->val.move_that_got_to_here) << " = ";
        float f[3];
        for (unsigned int i = 0; i < 3; i++) {
            f[i] = (*nes[i])(its);
            if (i > 0) {
                 std::cerr << ", ";
            }
            std::cerr << f[i];
        }
        std::cerr << "\n";

        GoMove move = its[0]->val.move_that_got_to_here;
        if (f[settings.move_select_criterion] >= max_f[settings.move_select_criterion]) {
            for (unsigned int i = 0; i < 3; i++) max_f[i] = f[i];
            best_move = move;
        }

        for (unsigned int i = 0; i < its.size(); i++) {
            if (its[i]->val.move_that_got_to_here != move) {
                assert(false); abort();
            }
            ++its[i];
            if (its[i].done() != its[0].done()) {
                assert(false); abort();
            }
        }
    } while (!its[0].done());

    std::cerr << "Best move valuation (times played, value, mean): ";
    for (unsigned int i = 0; i < 3; i++) {
        if (i) {
            std::cerr << ", ";
        }
        std::cerr << max_f[i];
    }
    std::cerr << " ~ " << moveToString(best_move) << "\n";

    if (settings.resign_if_appropriate && max_f[2] <= 0.01) { // resign if less than 1% chance
        return GoMove::resign();
    } else {
        return best_move;
    }
}

/*
GoMove GoUCT::selectMove(bool verbose) {
    if (verbose) {
        // output the entire tree as an HTML page
        //descendByUCB1Tuned(&this->nodes[0], true);
        ofstream out("tree.html");
        out << "<html>\n<body>\n";
        outputTreeVerbose(&this->nodes[0], initial_state, 0, out);
        out << "</body>\n</html>\n";
    }

    cerr << "Note: total simulations for this move (reuse from previous moves): " << nodes[0].times_played << " (" << times_played_originally << ")\n";

    std::vector< GoUCT_TreeNode* > principle_variation = getPrincipleVariation(6);

    cerr << "Principle variation: ";
    outputMovesFromNodes(principle_variation, cerr);
    cerr << "\n";


    if (principle_variation.size() == 0) {
        cerr << "Didn't examine any moves! Must pass.";
        assert(false);
        abort();
        return GoMove::pass();
    } else {
        float est_win_chance;
        GoUCT_TreeNode *node = principle_variation[0];

        if (node->is_win_for == -1) {
            est_win_chance = 0.0f;
            cerr << "Perfect play for opponent found: 0% chance of winning\n";
        } else if (node->is_win_for == 1)  {
            est_win_chance = 1.0f;
            cerr << "Perfect play found: 100% chance of winning\n";
        } else {
            est_win_chance = float(node->wins) / node->times_played;
            cerr << "Estimated chance of winning: " << (100.0f * est_win_chance) << "%\n";
        }

        if (settings.resign_if_appropriate) {
            if (est_win_chance < 0.01f) {
                cerr << "Less than 1% chance, so I am resigning\n";

                ofstream out("tree.html");
                out << "<html>\n<body>\n";
                outputTreeVerbose(&this->nodes[0], initial_state, 0, out);
                out << "</body>\n</html>\n";

                return GoMove::resign();
            }
        }

        return principle_variation[0]->move_that_got_to_here;
    }
}

// TODO: write to file
// It's important for debugging to be able to trace the effects of each function and check it
// does what it's supposed to! The easiest way is to write the tree to an image with GraphViz
// though it's best to do this with a small board size, e.g. 5x5



std::vector< GoUCT_TreeNode* > GoUCT::getPrincipleVariation(unsigned int moves, NodeEvaluator* ne) {
    GoUCT_TreeNode *node = &nodes[0];
    std::vector< GoUCT_TreeNode* > ret;

    for (unsigned int m = 0; m < moves && node->num_children > 0; m++) {
        unsigned int max_index = -1; // placeholder
        float max_estimate = -100000.0f;
        for (unsigned int i = 0; i < node->num_children; i++) {

            GoUCT_TreeNode *child = &nodes[node->child_start_index + i];

            float f = getValueUpperBound(node, child, 0.0f, false);
            if (f > max_estimate) {
                max_estimate = f;
                max_index = node->child_start_index + i;
            }
        }
        node = &nodes[max_index];
        ret.push_back(node);
    }

    return ret;
}

void GoUCT::outputTreeVerbose(const GoUCT_TreeNode* tn, const GoState &s, unsigned int depth, std::ostream &f) {
    if (depth > 4) return;

    string table[BOARDSIZE][BOARDSIZE];
    string pass = "";

    GoState s_copy = s;

    for (unsigned int y = BOARDSIZE -1; y < BOARDSIZE; y--) {
        for (unsigned int x = 0; x < BOARDSIZE; x++) {
            int colour = s_copy.get(x, y);
            switch (colour) {
                case EMPTY:
                    table[x][y] = "";
                    break;

                case WHITE:
                    table[x][y] = "<td style='width: 32px; border: 1px solid black; background-color: white; text-align: center;'>O</td>";
                    break;

                case BLACK:
                    table[x][y] = "<td style='width: 32px; border: 1px solid black; background-color: white; text-align: center;'>#</td>";
                    break;

                default:
                    abort();
            }
        }
    }

    for (unsigned int i = 0; i < tn->num_children; i++) {
        const GoUCT_TreeNode* child = &nodes[tn->child_start_index + i];

        ostringstream oss;

        if (child->times_played == 0) {
            oss << "<td style='width: 32px; border: 1px solid black; background-color: white; text-align: center;'>";
            oss << "<a href='#" << child << "' style='text-decoration: none;' title='" << child->times_played << "'>";
            oss << "-";
        }  else {
            float win_rate = child->wins / float(child->times_played);
            oss << "<td style='width: 32px; border: 1px solid black; background-color: rgb(" << int(256 * sqrt(1.0 - win_rate)) << ","
                << int(255 * sqrt(win_rate)) << "," << int(255 * (win_rate - (win_rate * win_rate))) << "); text-align: center;'>";
               oss << "<a href='#" << child << "' style='text-decoration: none;' title='" << child->times_played << "'>";

            oss << ((100 * child->wins) / child->times_played) << "%</a>";
        }
        oss << "</td>";

        if (child->move_that_got_to_here.isPass()) {
            pass = oss.str();
        } else {
            table[child->move_that_got_to_here.getX()][child->move_that_got_to_here.getY()] = oss.str();
        }
    }

    f << "<p><a name='" << tn << "'></a></p>\n";
    f << "<p>" << (s.getNextToPlay() == BLACK ? "BLACK" : "WHITE") << " to play</p>\n";
    f << "<table style='border-collapse: collapse; border: 2px solid black;'>";
    for (unsigned int y = 0; y < BOARDSIZE; y++) {
        f << "<tr style='height: 32px;'>";
        for (unsigned int x = 0; x < BOARDSIZE; x++) {
            f << table[x][y];
        }
        f << "</tr>\n";
    }
    f << "</table>";

    for (unsigned int i = 0; i < tn->num_children; i++) {
        const GoUCT_TreeNode* child = &nodes[tn->child_start_index + i];
        if (child->times_played > 10) {
            GoState s2 = s;
            s2.makeMove(child->move_that_got_to_here);
            outputTreeVerbose(child, s2, depth + 1, f);
        }
    }
}


void GoUCT::outputMovesFromNodes(const std::vector< GoUCT_TreeNode* >& v, std::ostream &o) {
    for (unsigned int i = 0; i < v.size(); i++) {
        GoUCT_TreeNode* node = v[i];
        o << moveToString(node->move_that_got_to_here) << " (";
        o << (getValueUpperBound(&nodes[node->parent_index], node, 0.0f, false)) << " for " << node->times_played << ") ";
#ifdef OPT_RAVE
        float rave_value = node->rave_wins / float(node->rave_times_played);
        o << "[" << rave_value << " for " << node->rave_times_played << "]";
#endif
        o << ", ";
    }
}




*/
