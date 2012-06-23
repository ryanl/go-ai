#include "go_uct.hpp"

#include <iostream>
#include <sstream>
#include <fstream>

using namespace std;

GoUCT::GoUCT(const GoState &_s, const GoUCTSettings _settings) :
    settings(_settings),
    force_cull(false),
    tree(getMaxNodes()),

    initial_state(_s),
    times_played_originally(0)
{}


void GoUCT::updateAfterPlay(GoMove move) {
    initial_state.makeMove(move);

    if (settings.reuse_tree) {
        // look for the move as a child of the root
        Node *root = tree.getRoot();

        for (Tree_t::ChildIterator it = tree.childBegin(root); !it.done(); ++it) {
            if (it->val.move_that_got_to_here == move) {

                // std::cerr << "Reusing " << it->val.times_played << " of " << root->val.times_played << " simulations ("
                //           << ((100.0f * it->val.times_played) / root->val.times_played) << "%)\n";

                times_played_originally = it->val.times_played;

                tree.reRoot(&*it);

                tree.getRoot()->val.is_win_for = 0; // so that children will be created even if we know it's a win

                assert(tree.getRoot()->val.move_that_got_to_here == move);

                // set force cull so that on the next ponder we will get
                // rid of nodes no longer in the current subtree
                force_cull = true;

                return;
            }
        }

        // std::cerr << "Note: updateAfterPlay reached end of tree - no search data can be reused\n";
    }

    tree.eraseAllButRoot();
    Node *root = tree.getRoot();
    root->val = UCTNode(); // reset root to an empty node
}

void GoUCT::createChildrenForNode(GoState &s, Node* node) {
    assert(tree.getNumChildren(node) == 0);

    StaticVector< pair<GoMove, GoMoveInfo>, 1 + (BOARDSIZE * BOARDSIZE) > valid_moves;
    s.queryValidMoves_SV_byref(valid_moves);

    for (unsigned int i = 0; i < valid_moves.size(); i++) {
        GoMove move = valid_moves[i].first;

        // <-- if I was to do any hard pruning, here would be a good place to do it

        UCTNode uct_data;
        uct_data.move_that_got_to_here = move;
        //uct_data.move_info = valid_moves[i].second;

        if (s.getPreviousMoveWasPass() && move.isPass()) {
            // game over
            uct_data.is_win_for = (s.getWinnerOfGame() == s.getNextToPlay()) ? 1 : -1;
        } else {
            uct_data.is_win_for = 0;
        }

        Node* new_node = tree.addChild(node);
        new_node->val = uct_data;
    }
}

GoUCT::Node* GoUCT::selectMoveSequenceByUCT(GoState *s, StaticVector<GoMove, MAX_GAME_LENGTH> *move_seq) {
    assert(move_seq->size() == 0); // precondition

    Node *node = tree.getRoot();
    node->val.times_played++;

    // any state has a valid move: pass, so no children means leaf of exploration tree (or two passes in a row i.e. game end)
    while (tree.getNumChildren(node) > 0 && node->val.is_win_for == 0) {
        Node* next_node = descendByUCB(node);

        GoMove move = next_node->val.move_that_got_to_here;
        move_seq->push_back(move);
        s->makeMove(move);

        node = next_node;
        node->val.times_played++;
    }

    if (node->val.is_win_for == 0) { // if the node is not the end of a game
        if (node->val.times_played >= settings.expansion_threshold) {
            // add children to leaf
            createChildrenForNode(*s, node);

            node = descendByUCB(node);
            node->val.times_played++;

            GoMove move = node->val.move_that_got_to_here;
            move_seq->push_back(move);
            s->makeMove(move);
        }
    }

    return node;
}


void GoUCT::playOneSequence() {
    StaticVector<GoMove, MAX_GAME_LENGTH> move_seq;

    Node *root = tree.getRoot();
    if (root->val.is_win_for != 0) {
        return; // perfect play has been found
    }

    //unsigned int max_tree_depth = (BOARDSIZE * BOARDSIZE * 2) + 10; // guess
    //StaticVector<GoUCT_TreeNode*, max_tree_depth> node_seq;

    GoState s = this->initial_state; // copy go state

    Node *leaf = selectMoveSequenceByUCT(&s, &move_seq);

    unsigned int num_moves_in_tree = move_seq.size();
    assert(num_moves_in_tree != 0 || tree.getRoot()->val.times_played < settings.expansion_threshold);

    if (leaf->val.is_win_for == 0) { // game not over
        if (settings.use_patterns) {
            default_policy_mogo.completeGame(s, move_seq, rng); // roughly and quickly simulate the rest of the game
        } else {
            default_policy_random.completeGame(s, move_seq, rng); // roughly and quickly simulate the rest of the game
        }
        /*
        for (unsigned int i = 0; i < move_seq.size(); i++) {
            std::cerr << moveToString(move_seq[i]) << " ";
        }
        std::cerr << "\n";
        */
        assert(move_seq.size() == 1 || move_seq[move_seq.size() - 2] == GoMove::pass());
        assert(move_seq[move_seq.size() - 1] == GoMove::pass());
    }

    unsigned int final_player = opponentOf(s.getNextToPlay());
    unsigned int winner = s.getWinnerOfGame();

    updateWins(leaf, final_player, winner, move_seq, num_moves_in_tree);

    // SLOW, DEBUGGING CODE
    /*
    std::string id = intToString(tree.getRoot()->val.times_played);
    while (id.length() < 4) id = "0" + id;
    ofstream out( ("graphviz" + id + ".txt").c_str() );

    debugGenerateGraphVizFile(out, move_seq, num_moves_in_tree, winner, final_player);
    */
}

// RAVE version based on Fuego's

void GoUCT::updateWins(Node *leaf, int final_player, int winner,
                       const StaticVector<GoMove, MAX_GAME_LENGTH>& move_seq, unsigned int num_moves_in_tree)
{
    unsigned int move_last_seen_at[(BOARDSIZE * BOARDSIZE) + 1][2];

    for (unsigned int i = 0; i < (BOARDSIZE * BOARDSIZE) + 1; i++) {
        move_last_seen_at[i][0] = (unsigned int)-1;
        move_last_seen_at[i][1] = (unsigned int)-1;
    }
    // move_last_seen_at is used for weighting the RAVE updates

    // we process the moves in the simulation
    // since there is no tree for these, there is no rave updating
    // we just need to update rave_rewards

    assert(num_moves_in_tree > 0 || tree.getRoot()->val.times_played < settings.expansion_threshold);

    int i = move_seq.size() - 1;
    unsigned int reward = (final_player == winner) ? 1 : 0;

    for (; i >= (int)num_moves_in_tree; i--) {
        //cerr << i << ": " << moveToString(move_seq[i]) << "\n";
        assert(i >= 0 && i < (int)move_seq.size());

        move_last_seen_at[move_seq[i].getXY() + 1][reward] = i;
        reward = 1 - reward;
    }

#ifndef NDEBUG
    unsigned int iterations = 0;
#endif

    // some of the stuff below is designed to reduce cache misses

    Node *node = leaf;
    do {
#ifndef NDEBUG
        iterations++;
        if (iterations > 40000) {
            cout << "INFINITE LOOP DETECTED\n";
            assert(false);
            abort();
        }
#endif

        if (reward) {
            // enclosing this in an if-statement rather than doing += reward may reduce memory accesses
            node->val.wins++;
        }

        // update children's RAVE values

        for (Tree_t::ChildIterator it = tree.childBegin(node); !it.done(); ++it) {
            Node* child = &*it;
            GoMove move = child->val.move_that_got_to_here;

            if (move.isPass() && !settings.rave_update_passes) {
                continue;
            }

            unsigned int rave_index     = move_last_seen_at[move.getXY() + 1][1 - reward];

            // if rave_check same we care whether the opponent has made the same move more recently
            unsigned int rave_index_opp = settings.rave_check_same ? move_last_seen_at[move.getXY() + 1][reward] : (unsigned int)-1;

            // if move has been played later in the tree by this player and not after the same move was played by her opponent
#ifndef NDEBUG
            if (rave_index != (unsigned int)-1 && rave_index <= rave_index_opp) {
                assert(rave_index != rave_index_opp);
#else
            if (rave_index < rave_index_opp) {
#endif
                float weight;
                if (settings.weighted_rave) {
                    weight = 2.0f - (float(rave_index) - i) / ( move_seq.size() - i );
/*
                    if (settings.square_rave_weight) {
                        weight *= weight;
                    }
*/
                    if (!(weight >= 0.0f && weight <= 2.0f)) {
                        std::cerr << "weight: "     << weight     << "\n";
                        std::cerr << "rave_index: " << rave_index << "\n";
                        std::cerr << "move_seq.size(): "   << move_seq.size()   << "\n";
                        std::cerr << "i: "   << i << "\n";

                        assert(false);
                        abort();
                    }
                } else {
                    weight = 1.0f;
                }

                //#pragma omp atomic
                child->val.rave_times_played += weight;
                //node->val.rave_children_times_played += weight;

                // save a memory access if we would be adding 0
                if (reward != 1) {
                    //#pragma omp atomic
                    child->val.rave_wins += weight * (1 - reward);
                }

                //cerr << "child " << moveToString(child->move_that_got_to_here) << " rave wins += " << weight * (1 - reward) << "\n";
            }
        }

        if (i >= 0) { // if not root
            assert(node->val.move_that_got_to_here == move_seq[i]);
            move_last_seen_at[move_seq[i].getXY() + 1][reward] = i;
        }

        node = tree.getParent(node);
        reward = 1 - reward;
        i--;
    } while (i != -2);
}

float GoUCT::raveCountToRaveWeight(float rave_times_played) const {
    return rave_times_played / (settings.rave_param1 + (settings.rave_param2 * rave_times_played));
}

// Credit where credit is due - after writing my own version
// I had trouble working out how RAVE was supposed to fit in.
// so this function is based heavily on Fuego's one

float GoUCT::getValueUpperBound(const Node *child, const float log_n, const float grandfather_mean,
                                const float grandfather_weighting,    const bool add_uct_term) const
{
    if (child->val.is_win_for == 1) {
        return 9999.0f; // this value is large enough to force this move to be chosen
    } else if (child->val.is_win_for == -1) {
        return -1.0f; // this should make this move a least favoured move
    }

    float value = 0.0f, weight_sum = 0.0f;

    if (child->val.times_played > 0) {
        value += float(child->val.wins);
        weight_sum += float(child->val.times_played);
    }

    value += grandfather_weighting * grandfather_mean;
    weight_sum += grandfather_weighting;

    if (settings.use_rave && child->val.rave_times_played > 0.0f) {
        // NOTE: consult the UCT_RLGO_MoGo or Fuego papers for how to do this correctly

        float rave_value = child->val.rave_wins / float(child->val.rave_times_played);
        assert(rave_value >= 0.0f && rave_value <= 1.0f);

        // http://webdocs.cs.ualberta.ca/~games/go/fuego/fuego-doc/smartgame-doc/sguctsearchweights.html
        // says what values these should take - I need to look into it more closely

        // also look at the Nakade heuristic

        float rave_weight = raveCountToRaveWeight(child->val.rave_times_played);

        value += rave_value * rave_weight;
        weight_sum += rave_weight;
    }

    if (weight_sum < epsilon) {
        return 1.0f; // first play urgency
    } else {
        value /= weight_sum;
    }

    if (add_uct_term) {
        float times_played = settings.include_rave_count_for_exploration ? weight_sum + 1.0f : child->val.times_played + 1.0f;

        if (settings.use_ucb1_tuned) {
            // UCB1_Tuned
            float v = (value - (value * value)) + sqrtf(2.0f * log_n / times_played);
            if (v > 0.25f) {
                v = 0.25f;
             }
            float uct_term = settings.exploration_constant * sqrtf(log_n * v / times_played);
            value += uct_term;
        } else {
            // UCB1
            float uct_term = settings.exploration_constant * sqrtf(log_n / times_played);
            value += uct_term;
        }
    }

    return value;
}

/* UCB1_Tuned */
GoUCT::Node* GoUCT::descendByUCB(Node* node) {
    float max_f = -1000000.0f; // effectively -infinity

    Node* grandfather    = tree.getParent(tree.getParent(node));
    bool  has_grandfather = (settings.grandfather_heuristic_weighting > epsilon) && !tree.isRoot(tree.getParent(node));
    Tree_t::ChildIterator it = tree.childBegin(grandfather);
    Node* max_node = NULL;
    bool add_uct_term = settings.exploration_constant > epsilon;
    float log_n = 0.0f;

    if (add_uct_term) {
        float n = node->val.times_played + 1.0f;

        if (settings.include_rave_count_for_exploration) {
            for (Tree_t::ChildIterator child = tree.childBegin(node); !child.done(); ++child) {
                n += raveCountToRaveWeight(child->val.rave_times_played);
            }
        }
        log_n = logf(n); // this is only used when adding the UCT exploration term
    }

    for (Tree_t::ChildIterator child = tree.childBegin(node); !child.done(); ++child) {
        float grandfather_mean = 1.0f, grandfather_weight = 0.0;
        if (has_grandfather) {
            // std::cerr << moveToString(it->val.move_that_got_to_here) << " vs. " << moveToString(child->val.move_that_got_to_here) << "\n";
            while (!it.done() && it->val.move_that_got_to_here < child->val.move_that_got_to_here) {
                ++it;
            }

            if (it->val.move_that_got_to_here == child->val.move_that_got_to_here) {
                grandfather_mean = (it->val.wins + 1.0f) / (it->val.times_played + 1.0f);
                grandfather_weight = settings.grandfather_heuristic_weighting;
                assert(0.0f <= grandfather_mean && grandfather_mean <= 1.0f);
                //std::cerr << "f";
                ++it;
            } else {
               // std::cerr << "n";
            }
        }
        float f = getValueUpperBound(&*child, log_n, grandfather_mean, grandfather_weight, add_uct_term);
        f += epsilon * (rng.getInt() & 0xFF); // small randomness to discourage bias towards lower index moves

        if (f > max_f) {
            max_f = f;
            max_node = &*child;
        }
    }


    // we can bubble perfect play information up the tree
    if (max_f == -1.0f) {
        // every win is a loss given perfect play, so parent is a win (for opponent)
        node->val.is_win_for = 1;
    } else if (max_f == 9999.0f) {
        // at least one move is a win given perfect play, so parent is a loss (for opponent)
        node->val.is_win_for = -1;
    }

    assert(max_node != NULL);
    return max_node;
}

void GoUCT::ponder() {
    for (unsigned int i = 0; i < SIMULATIONS_PER_PONDER; i++) {
        cullIfNeeded();
        playOneSequence();
    }
}

void GoUCT::cullIfNeeded() {
    unsigned int threshold_visits = 0;
    unsigned int min = BOARDSIZE * BOARDSIZE + 1; // enough for one node to be expanded
    bool cull = force_cull;

    if ((tree.getUnusedCapacity() < min)) {
        cull = true;
        threshold_visits = 5;  // TODO - add a dynamic threshold based the values of times_visited, maybe choose the median
    }

    if (cull) {
        force_cull = false;

        // we want to get rid of at least 50% of existing nodes so we don't have to cull again for ages
        unsigned int threshold_nodes = tree.getMaxNodes() / 2;

        do {
            cerr << "Performing (slow) cull [" << threshold_visits << "]\n";

            Node* root = tree.getRoot();

            struct NodeConditionalVisitThreshold : public Tree_t::NodeConditional {
                const unsigned int threshold_visits;

                NodeConditionalVisitThreshold(unsigned int _threshold_visits) : threshold_visits(_threshold_visits) {}

                bool operator () (const Node* node) const {
                    return (node->val.times_played >= threshold_visits); // && (node->val.is_win_for == 0);
                }
            } nc(threshold_visits);

            tree.recursivelyMarkIf(root, nc);

            tree.eraseChildrenOfUnmarkedNodes();

            threshold_visits *= 2;
        } while (tree.getUnusedCapacity() <= threshold_nodes);
        cerr << "Cull complete, tree now contains " << (tree.getMaxNodes() - tree.getUnusedCapacity()) << " of a maximum " << tree.getMaxNodes() << "\n";
    }
}

void GoUCT::summariseTreeStructure() {
    stack< pair<Node*, unsigned int> > st;
    st.push(make_pair(tree.getRoot(), 0));

    unsigned long long total_depth = 0, nodes_examined = 0, max_depth = 0;
    unsigned long long total_weighted_depth = 0, total_weight = 0;

    while (!st.empty()) {
        Node* cur = st.top().first;
        unsigned int &child = st.top().second;

        if (child != tree.getNumChildren(cur)) {
            unsigned int old_child = child;
            child++;
            st.push(make_pair(tree.getChild(cur, old_child), 0));
        } else {
            total_depth += st.size();

            unsigned int weight = cur->val.times_played;

            for (unsigned int i = 0; i < tree.getNumChildren(cur); i++) {
                weight -= tree.getChild(cur, i)->val.times_played;
            }
            total_weighted_depth += weight * st.size();
            total_weight += weight;


            if (st.size() > max_depth) {
                max_depth = st.size();
            }
            nodes_examined++;
            st.pop();
        }
    }

    std::cerr << "Nodes: " << nodes_examined << "\n"
              << "Average node depth: " << (float(total_depth) / float(nodes_examined)) << "\n"
              << "Average weighted node depth: " << (float(total_weighted_depth) / float(total_weight)) << "\n"
              << "Max depth: " << max_depth << "\n\n";
}

void GoUCT::recursiveOutputAsGraphViz(std::ostream &f, const Node* node, const set<const Node*> node_set, int node_player) const {

    f << "node" << int(node - tree.getRoot()) <<  "[shape=record, height=1.2, width=0.5, ";
    if (node_player == BLACK) {
        f << "bgcolor=black, ";
    }
    f << "label=\"{";

    // RAVE
    if (node->val.rave_times_played > 0.0f) {
        int rave_percentage = 100.0f * node->val.rave_wins / node->val.rave_times_played;
        f << rave_percentage << "% (" << int(node->val.rave_times_played * 10.0f) << ")" << "|";
    } else {
        f << " - |";
    }

    if (node->val.times_played > 0.0f) {
        int percentage = (100.0f * node->val.wins) / node->val.times_played;
        f << percentage << "% (" << int(node->val.times_played * 10.0f) << ")" << "|";
    } else {
        f << " - |";
    }

    if (node->val.is_win_for == 0) {
        f << " - ";
    } else if (node->val.is_win_for == -1) {
        f << " loss ";
    } else if (node->val.is_win_for == 1) {
        f << " win ";
    } else {
        assert(false);
    }

    f << "}\"];";

    for (unsigned int i = 0; i < tree.getNumChildren(node); i++) {
        const Node* child = tree.getChild(node, i);
        recursiveOutputAsGraphViz(f, tree.getChild(node, i), node_set,  opponentOf(node_player));
        f << "edge [label=" << moveToString(child->val.move_that_got_to_here);

        if (node_set.find(child) != node_set.end()) {
            f << ", color=red";
        } else {
            f << ", color=black";
        }

        f << "];\n";

        f << "node" << int(node - tree.getRoot()) << " -> " << "node" << int(child - tree.getRoot()) << "\n;";
    }
}

void GoUCT::debugGenerateGraphVizFile(std::ostream &f, const StaticVector<GoMove, BOARDSIZE * BOARDSIZE * 4>& move_seq, unsigned int num_moves_in_tree, int winner, int final_player) const {

    set<const Node*> node_set;
    const Node* cur = tree.getRoot();
    node_set.insert(cur);

    for (unsigned int i = 0; i < num_moves_in_tree; i++) {
        for (unsigned int j = 0; j < tree.getNumChildren(cur); j++) {
            const Node* child = tree.getChild(cur, j);
            if (child->val.move_that_got_to_here == move_seq[i]) {
                cur = child;
                node_set.insert(cur);
                break;
            }
        }
        assert (node_set.size() == i + 2);
    }

    f << "digraph g {\n";
    recursiveOutputAsGraphViz(f, tree.getRoot(), node_set, opponentOf(initial_state.getNextToPlay()));

    f << "nodefinal [shape=record, height=0.5, label=\"";

    for (unsigned int i = num_moves_in_tree; i < move_seq.size(); i++) {
        f << moveToString(move_seq[i]);
        f << " | ";
    }
    if (winner == BLACK) f << " winner: BLACK | ";
    if (winner == WHITE) f << " winner: WHITE | ";
    if (final_player == BLACK) f << " final player: BLACK | ";
    if (final_player == WHITE) f << " final player: WHITE | ";
    f << " moves in tree: " << num_moves_in_tree;

    f << "\"];";
    f << "edge [color=red, label=\"\"];\n";
    //f << "node" << leaf_index << " -> " << "nodefinal;\n";
    f << "}\n";
}

