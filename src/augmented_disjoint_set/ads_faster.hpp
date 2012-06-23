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
        unsigned int root_id;
        MemberSet set_members; // only valid at root
        Token token;
        unsigned int index_in_root_array;

        node() :
            root_id(NONE),
            set_members()
        {}
    };

    unsigned int num_roots;
    unsigned int roots[els];

    node nodes[els];

    // joins two distinct sets whose roots a, b are given
    void link(unsigned int a, unsigned int b) {
        assert(nodes[a].root_id == a);
        assert(nodes[b].root_id == b);
        assert(a != b);

        unsigned int size_a = nodes[a].set_members.count(), size_b = nodes[b].set_members.count();

        // relabel the smaller group
        if (size_a < size_b) {
            unsigned int tmp = a;
            a = b;
            b = tmp;
#ifndef NDEBUG
            size_b = size_a;
#endif
        }

        // we have rank[a] >= rank[b] so we make b a child of a
        nodes[a].token       |= nodes[b].token;
        nodes[a].set_members |= nodes[b].set_members;

        for (MS_SBI it = nodes[b].set_members.getSetBitIterator(); !it.isDone(); ++it) {
            assert(nodes[b].set_members[*it]);
            nodes[*it].root_id = a;
#ifndef NDEBUG
            size_b--;
#endif
        }
        assert(size_b == 0);

        removeFromRootsList(b);
    }

    // deletes the set whose root a is given
    void _disperse(unsigned int a) {
        for (MS_SBI it = nodes[a].set_members.getSetBitIterator(); !it.isDone(); ++it) {
            assert(nodes[a].set_members[*it]);
            nodes[*it].root_id = NONE;
        }
        removeFromRootsList(a);
    }

    // removes a root from the list of roots
    void removeFromRootsList(unsigned int b) {
        // remove from the root 'list'
        unsigned int tmp = roots[num_roots - 1], old_index = nodes[b].index_in_root_array;
        nodes[tmp].index_in_root_array = old_index;
        roots[old_index] = tmp;
        num_roots--;
    }

public:
    static const unsigned int NONE = (unsigned int)-1;

    inline unsigned int countRoots() const {
        return num_roots;
    }

    inline unsigned int getRootListElement(unsigned int i) const {
        assert(i < num_roots);
        return roots[i];
    }
#
/*
    inline bool isRoot(unsigned int a) {
        assert(a < size());
        return nodes[a].root_id == a;
    }
*/

    // returns true if a is not part of any set
    inline bool isDispersed(unsigned int a) {
        assert(a < size());
        return nodes[a].root_id == NONE;
    }

    ADSFast() :
        num_roots(0)
    {}

    inline void createSingleton(unsigned int a) {
        assert(a < size());
        assert(nodes[a].root_id == NONE);

        nodes[a].token = Token();
        nodes[a].set_members.zero();
        nodes[a].set_members.setBit(a);
        nodes[a].root_id = a;

        // add to roots list
        nodes[a].index_in_root_array = num_roots;
        roots[num_roots] = a;
        num_roots++;
    }


    // disperse is a slow operation but its cost is amortised away
    inline void disperse(unsigned int a) {
        assert(a < size()); // validation
        assert(nodes[a].root_id != NONE);
        _disperse(nodes[a].root_id);
    }

    inline void join(unsigned int a, unsigned int b) {
        assert(a < els);
        assert(b < els);

        unsigned int find_a = find(a);
        unsigned int find_b = find(b);

        assert(find_a < els); // i.e. not DISPERSED or NONE
        assert(find_b < els); // i.e. not DISPERSED or NONE

        // find_a == find_b means a and b were in the same set to begin with

        if (find_a != find_b) {
            link(find_a, find_b);
        }
    }


    // returns NONE if the node is dispersed (i.e. not part of any set)
    inline unsigned int find(unsigned int a) const {
        assert(a < size()); // validation

        return nodes[a].root_id;
    }

    inline unsigned int size() const {
        return els;
    }

    /* returns a reference to the token for the set whose root is a */
    inline Token& tokenForRoot(unsigned int a) {
        assert(a < size());
        assert(nodes[a].root_id == a);

        return nodes[a].token;
    }

    const MemberSet& setMembersOfRoot(unsigned int a) const {
        assert(a < size());
        assert(nodes[a].root_id == a);

        return nodes[a].set_members;
    }
};

#endif
