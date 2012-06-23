#ifndef __AUGMENTED_DISJOINT_SET_HPP
#define __AUGMENTED_DISJOINT_SET_HPP

#include <vector>
#include <iostream>

#include "assert.h"

template <class Token>
class AugmentedDisjointSetForest {
private:
    struct AugmentedDisjointSetNode {
        unsigned int parent;
        unsigned int first_child;
        unsigned int next_sibling, prev_sibling; // circularly linked
        unsigned int rank; // number of children?
        Token token;
    };

    static AugmentedDisjointSetNode dispersedNode() {
        AugmentedDisjointSetNode ret;
        ret.parent       = DISPERSED;
        ret.first_child  = NONE;
        ret.next_sibling = NONE;
        ret.prev_sibling = NONE;
        ret.rank         = 0;

        return ret;
    }

    bool isRoot(unsigned int a) {
        return nodes[a].parent == NONE;
    }

    bool isDispersed(unsigned int a) {
        return nodes[a].parent == DISPERSED;
    }

    bool isRootOrDispersed(unsigned int a) {
        return nodes[a].parent >= DISPERSED;
    }
    void link(unsigned int a, unsigned int b);

    void _disperse(unsigned int a);

    unsigned int nextGroupMember(unsigned int element, bool child_okay);

    std::vector< AugmentedDisjointSetNode > nodes;

public:
    static const unsigned int DISPERSED = -2;
    static const unsigned int NONE      = -1;

    /* creates a disjoint set forest for objects 0, 1,
     * ..., numberOfElements - 1, with initially all singleton sets */
    AugmentedDisjointSetForest(unsigned int number_of_elements);

    /* infix (??) traversal of tree */
    unsigned int nextGroupMember(unsigned int element) {
        return nextGroupMember(element, true);
    }

    void outputHumanReadableState();

    void createSingleton(unsigned int a);

    // disperse is a slow operation but its cost is amortised away
    void disperse(unsigned int a);

    void join(unsigned int a, unsigned int b);

    unsigned int find(unsigned int a);

    unsigned int size() {
        return nodes.size();
    }

    /* returns a reference to the token for the set containing a */
    Token& tokenForSet(unsigned int a) {
        assert(a < nodes.size());
        assert(nodes[a].parent != DISPERSED);

        unsigned int find_a = find(a);
        assert(find_a < nodes.size());

        return nodes[find_a].token;
    }
};

// because we use templates, we can't create a separate .cpp
// but we can put the code in a separate header

#include "augmented_disjoint_set_code.hpp"

#endif
