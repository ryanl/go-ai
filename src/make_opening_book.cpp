#include <ctime>
#include <iostream>
#include <fstream>

#include "go_ai/uct/go_uct.hpp"

using namespace std;

void cullToSize(Tree<UCTNode> &tree, unsigned int max_nodes) {
    typedef Tree<UCTNode>::Node Node;

    unsigned int threshold_visits = 5;
    unsigned int threshold_nodes = tree.getMaxNodes() - max_nodes;

    while (tree.getUnusedCapacity() <= threshold_nodes) {
        // cerr << "Performing (slow) cull [" << threshold_visits << "]\n";

        Node* root = tree.getRoot();

        struct NodeConditionalVisitThreshold : public Tree<UCTNode>::NodeConditional {
            const unsigned int threshold_visits;

            NodeConditionalVisitThreshold(unsigned int _threshold_visits) : threshold_visits(_threshold_visits) {}

            bool operator () (const Node* node) const {
                return (node->val.times_played >= threshold_visits); // && (node->val.is_win_for == 0);
            }
        } nc(threshold_visits);

        tree.recursivelyMarkIf(root, nc);

        tree.eraseChildrenOfUnmarkedNodes();

        threshold_visits = ((threshold_visits * 3)/ 2) + 1;
    } while (tree.getUnusedCapacity() <= threshold_nodes);
}

int main(int argc, char *argv[]) {

    if (argc != 3) {
        std::cout << "Usage: " << argv[0] << " <opening book file to write> <simulations>\n";
        exit(1);
    }

    std::cout << "I will write an opening book to '" << argv[1] << "'\n";

    ofstream out(argv[1]);
    unsigned int simulations = atoi(argv[2]);

    GoState s = GoState::newGame(SUPERKO_POSITIONAL);
    GoUCTSettings settings;

    settings.max_mem_mb = 2000; // tune this for your machine's RAM
    settings.exploration_constant = 0.25;

    std::cout << "Creating a GoUCT object\n";
    GoUCT ai(s, settings);

    clock_t start = clock();
    // we'll do 10 million simulations rather than the usual 100k or so
    unsigned long ponders = simulations / SIMULATIONS_PER_PONDER;

    for (unsigned long i = 0; i < ponders; i++) {
        std::cout << "Ponder " << i << " of " << ponders << "\n";
        ai.ponder();
    }
    float cpu_time_used = float(clock() - start) / CLOCKS_PER_SEC;
    cout << "cpu time used: " << cpu_time_used << "\n";

    float simulations_per_second = ponders * SIMULATIONS_PER_PONDER  / cpu_time_used;
    cerr << simulations_per_second << " sims per second\n";

    cout << "Culling...\n";

    // now we will cull off enough nodes for the tree to fit in 300MB
    unsigned int cull_nodes_mb = 290;
    unsigned int cull_nodes = (cull_nodes_mb * 1024L * 1024L) / sizeof(Tree<UCTNode>::Node);
    cullToSize(ai.getTree(), cull_nodes);

    cout << "Writing to file\n";
    ai.getTree().write(out);
    cout << "Complete\n";
}

