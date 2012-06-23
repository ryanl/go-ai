#ifndef __ADS_FAST_HPP
#define __ADS_FAST

#include <vector>
#include <iostream>

#include "../static_vector.hpp"

#include "../improved_bitset.hpp"

#include "assert.h"

/* Somewhat faster thean the standard ADS when
   the number of elements is known at compile time
   and that number is moderately small */

template <class Token, unsigned int els>


class ADSFast {
public:

    typedef ImprovedBitset<els> MemberSet;
    typedef typename ImprovedBitset<els>::SetBitIterator MS_SBI; // need typename otherwise compiler fails

private:

    struct node {
        unsigned int parent;
        MemberSet set_members; // only valid at root
        unsigned int rank; // number of children
        Token token;

        node() :
            parent(DISPERSED),
            set_members(),
            rank(0)
        {}
    };


    node nodes[els];


    void link(unsigned int a, unsigned int b) {
        assert(nodes[a].parent == NONE);
        assert(nodes[b].parent == NONE);
        assert(a != b);

        // union by rank
        if (nodes[a].rank < nodes[b].rank) {
             // swap a and b
            int tmp = b;
            b = a;
            a = tmp;
        }

        // we have rank[a] >= rank[b] so we make b a child of a
        if (nodes[a].rank == nodes[b].rank) {
            nodes[a].rank++;
        }

        nodes[a].token |= nodes[b].token;
        nodes[a].set_members |= nodes[b].set_members;

        nodes[b].parent = a;
    }

    void _disperse(unsigned int a) {
        for (MS_SBI it = nodes[a].set_members.getSetBitIterator(); !it.isDone(); ++it) {
            assert(nodes[a].set_members[*it]);
            nodes[*it].parent = DISPERSED;
        }
    }

public:

    static const unsigned int DISPERSED = (unsigned int)-2;
    static const unsigned int NONE      = (unsigned int)-1;

    bool isRoot(unsigned int a) {
        assert(a < size());
        return nodes[a].parent == NONE;
    }

    bool isDispersed(unsigned int a) {
        assert(a < size());
        return nodes[a].parent == DISPERSED;
    }

    bool isRootOrDispersed(unsigned int a) {
        assert(a < size());
        return nodes[a].parent >= DISPERSED;
    }

    ADSFast() {
        for (unsigned int i = 0; i < els; i++) {
            nodes[i] = node();
        }
    }

    void createSingleton(unsigned int a) {
        assert(a < size());
        assert(nodes[a].parent == DISPERSED);

        nodes[a].token = Token();
        nodes[a].set_members = MemberSet();
        nodes[a].set_members[a] = true;
        nodes[a].parent = NONE;
        nodes[a].rank = 0;
    }


    // disperse is a slow operation but its cost is amortised away
    void disperse(unsigned int a) {
        assert(a < size()); // validation
        _disperse(find(a));
    }

    void join(unsigned int a, unsigned int b) {
        assert(a < els);
        assert(b < els);

        unsigned int find_a = find(a);
        unsigned int find_b = find(b);

        assert(find_a < size()); // i.e. not DISPERSED or NONE
        assert(find_b < size()); // i.e. not DISPERSED or NONE

        // find_a == find_b means a and b were in the same set to begin with

        if (find_a != find_b) {
            link(find_a, find_b);
        }
    }


    // returns NONE if the node is dispersed (i.e. not part of any set)
    unsigned int find(unsigned int a) {
        assert(a < size()); // validation

        unsigned int root = a, new_parent = nodes[a].parent;

        if (new_parent == DISPERSED) { // dispersed
            return NONE;
        }

        while  (new_parent != NONE) {
            root = new_parent;
            new_parent = nodes[root].parent;
        }


        /* perform the path compression */
        unsigned int old_parent;

        for (unsigned int y = a; nodes[y].parent != NONE; y = old_parent) {
            old_parent = nodes[y].parent;
            nodes[y].parent = root;
        }

        return root;
    }

    unsigned int size() {
        return els;
    }

    /* returns a reference to the token for the set containing a */
    Token& tokenForSet(unsigned int a) {
        assert(a < size());
        assert(nodes[a].parent != DISPERSED);

        unsigned int find_a = find(a);
        assert(find_a < els);

        return nodes[find_a].token;
    }

    /* returns a reference to the token for the set whose root is a */
    Token& tokenForRoot(unsigned int a) {
        assert(a < size());
        assert(nodes[a].parent == NONE);

        return nodes[a].token;
    }

    const MemberSet& setMembers(unsigned int a) {
        assert(a < size());
        assert(nodes[a].parent != DISPERSED);

        unsigned int find_a = find(a);
        assert(find_a < els);

        return nodes[find_a].set_members;
    }
};

#endif
