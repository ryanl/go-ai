#ifndef __GO_UCT_HPP
#define __GO_UCT_HPP

#include <vector>
#include <set>
#include <cmath>
#include "go_mechanics/go_state.hpp"

#include "go_ai/pattern/pattern_matcher.hpp"

#include "go_uct_settings.hpp"

#include "go_ai/tree.hpp"

#include "interface_gtp/go_gtp_utils.hpp" // for moveToString

//#include <omp.h>

#include "go_ai/default_policy/uniformly_random.hpp"
#include "go_ai/default_policy/mogo.hpp"

#include "go_uct_team.hpp"

#include <fstream>
#include <set>

/* RyanBot v6    - ~1016 ELO on CGOS 9x9
           v134  - ~1301 ELO on CGOS 9x9
           v320  - ~1420 ELO on CGOS 9x9
           v760  - ~1510 ELO on CGOS 9x9
           v912  - ~1580 ELO on CGOS 9x9
*/

const unsigned int SIMULATIONS_PER_PONDER = 500;

struct UCTNode {
    /* Annotations */
    GoMove move_that_got_to_here;
    signed char is_win_for; // for that play: 1 for yes, 0 for game not complete, -1 for loss

    unsigned int times_played;
    unsigned int wins; // for that player

    //float urgency;

    // RAVE bits
    float rave_times_played;
    float rave_wins;
    //float rave_children_times_played;

    // modified UCT could store extra info about the board,
    // e.g. live group analysis, heuristics...

    inline UCTNode() :
        move_that_got_to_here(GoMove::none()),
        is_win_for(0),
        times_played(0),
        wins(0),
        rave_times_played(0.0f),
        rave_wins(0.0f)
    {}
};

class GoUCT {
public:
    typedef Tree<UCTNode> Tree_t;
    typedef Tree_t::Node Node;
    friend class GoUCTTeam;

private:
    GoUCTSettings settings;

    static const float epsilon = 0.000001f;

    bool force_cull;

    /*! tree of explored move sequences */
    Tree_t tree;

    /*! random number generator */
    RNG rng;

    DefaultPolicy_Mogo    default_policy_mogo;
    DefaultPolicy_Random  default_policy_random;
    GoState initial_state;

    unsigned int times_played_originally;

    /*!
        calculates how many nodes the tree may contain to stay within the memory limit
    */
    unsigned int getMaxNodes() const {
        return (settings.max_mem_mb * 1024L * 1024L) / sizeof(Node);
    }

    void summariseTreeStructure();

public:

    /*!
        constructs a new GoUCT object (an AI class)
    */
    GoUCT(const GoState &_s, const GoUCTSettings _settings);

    /*!
        returns true if the move results in the completion of the game
    */
    bool moveEndsGame(GoMove move) {
        return initial_state.getPreviousMoveWasPass() && move.isPass();
    }

    /*!
        returns true if a minimax value has been found for the root of the game tree
    */
    bool perfectPlayFound() {
        Tree_t::Node* root = tree.getRoot();
        return root->val.is_win_for != 0;
    }

    /*! keeps the parts of the game tree that
        are still valid, and updates initial_state
    */
    void updateAfterPlay(GoMove move);

    /*! spends a little while (perhaps 200ms) thinking */
    void ponder();


    /*! destroys the game tree and prepares for pondering on a entirely new game state */
    void resetToNewState(const GoState &s_new) {
        tree.eraseAllButRoot();
        Tree_t::Node* root = tree.getRoot();
        root->val = UCTNode();
        initial_state = s_new;
    }


    float raveCountToRaveWeight(float rave_times_played) const;

    /*! for each valid move at state s, add a child to node */
    void createChildrenForNode(GoState &s, Node* node);

    float getValueUpperBound(const Tree_t::Node *child, const float log_n, const float grandfather_mean, const float grandfather_weighting, const bool add_uct_term) const;

    /*! perform one iteration of the UCT loop */
    void playOneSequence();

    /*! uses UCB1_Tuned to select the child of node with the greatest upper confidence bound estimate */
    Node* descendByUCB(Node* node);

    /*! uses the UCT algorithm to select a leaf of the tree */
    Node* selectMoveSequenceByUCT(GoState *s, StaticVector<GoMove, MAX_GAME_LENGTH>* move_seq);

    /*! after a sequence has been played until a terminal state, update the UCT values
        of nodes on the path and the RAVE of values of them and their children
    */
    void updateWins(Node* leaf, int final_player, int winner,
                    const StaticVector<GoMove, MAX_GAME_LENGTH>& move_seq, unsigned int num_moves_in_tree);

    /*! the memory limit should be set high enough that this doesn't get called very often */
    void cullIfNeeded();

    /*! picks the move that is so far considered 'best' */
    GoMove selectMove();

    void recursiveOutputAsGraphViz(std::ostream &f, const Node* node, const std::set<const Node*> node_set, int node_player) const;

    void debugGenerateGraphVizFile(std::ostream &f, const StaticVector<GoMove, MAX_GAME_LENGTH>& move_seq, unsigned int num_moves_in_tree, int winner, int final_player) const;
};

#endif
