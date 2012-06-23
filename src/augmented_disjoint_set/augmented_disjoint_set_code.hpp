

template <class Token>
AugmentedDisjointSetForest<Token>::AugmentedDisjointSetForest(unsigned int number_of_elements)
{
    assert(number_of_elements < DISPERSED);

    nodes.reserve(number_of_elements);
    for (unsigned int i = 0; i < number_of_elements; i++) {
        nodes.push_back(dispersedNode());
    }
}

template <class Token>
void AugmentedDisjointSetForest<Token>::outputHumanReadableState() {
    std::cout << nodes.size() << " elements" << std::endl;
    for (unsigned int i = 0; i < nodes.size(); i++) {
        std::cout << i << ": "
                  << "parent=" << int(nodes[i].parent) << ", "
                  << "prev="   << int(nodes[i].prev_sibling) << ", "
                  << "next="  <<  int(nodes[i].next_sibling) << ", "
                  << "child=" <<  int(nodes[i].first_child) << std::endl;

        /* convert to int so e.g. NONE appears as -1 */
    }
}

// infix traversal of tree

template <class Token>
unsigned int AugmentedDisjointSetForest<Token>::nextGroupMember(unsigned int a, bool childOkay) {
    assert(0 <= a && a < nodes.size());

    if (childOkay && nodes[a].first_child != NONE) {
        return nodes[a].first_child;
    } else if (nodes[a].parent != NONE) {
        // return next sibling if it's not the circular list wrapping around
        if (nodes[a].next_sibling != NONE) {
            return nodes[a].next_sibling;
        } else {
            return nextGroupMember(nodes[a].parent, false);
        }
    } else {
        return NONE; // all elements have been traversed
    }
}


template <class Token>
void AugmentedDisjointSetForest<Token>::createSingleton(unsigned int a) {
    assert(a < nodes.size());
    assert(nodes[a].parent == DISPERSED);
    assert(nodes[a].next_sibling == NONE);
    assert(nodes[a].prev_sibling == NONE);

    nodes[a].token = Token();
    nodes[a].parent = NONE;
    nodes[a].rank = 0;
}

template <class Token>
void AugmentedDisjointSetForest<Token>::join(unsigned int a, unsigned int b) {
    assert(a < nodes.size());
    assert(b < nodes.size());

    unsigned int find_a = find(a);
    unsigned int find_b = find(b);

    assert(find_a < nodes.size()); // i.e. not DISPERSED or NONE
    assert(find_b < nodes.size()); // i.e. not DISPERSED or NONE

    // find_a == find_b means a and b were in the same set to begin with

    if (find_a != find_b) {
        link(find_a, find_b);
    }
}

template <class Token>
void AugmentedDisjointSetForest<Token>::link(unsigned int a, unsigned int b) {
    assert(nodes[a].parent == NONE);
    assert(nodes[b].parent == NONE);
    assert(a != b);

    // internal error if these asserts fail
    assert(nodes[a].prev_sibling == NONE && nodes[b].prev_sibling == NONE &&
           nodes[a].next_sibling == NONE && nodes[b].next_sibling == NONE);

    // union by rank
    if (nodes[a].rank < nodes[b].rank) {
        int tmp = b; // swap a and b
        b = a;
        a = tmp;
    }

    // we have rank[a] >= rank[b] so we make b a child of a
    if (nodes[a].rank == nodes[b].rank) {
        nodes[a].rank++;
    }

    nodes[a].token |= nodes[b].token;

    unsigned int a_old_child = nodes[a].first_child;

    nodes[a].first_child = b;
    nodes[b].next_sibling = a_old_child;
    nodes[b].parent = a;

    if (a_old_child != NONE) {
        nodes[a_old_child].prev_sibling = b;
    }
}

template <class Token>
void AugmentedDisjointSetForest<Token>::disperse(unsigned int a) {
    assert(a < nodes.size()); // user input validation

    _disperse(find(a));
}


template <class Token>
void AugmentedDisjointSetForest<Token>::_disperse(unsigned int a) {
    assert(a < nodes.size());

    if (nodes[a].first_child != NONE) {
        unsigned int old_child = nodes[a].first_child;
        nodes[a].first_child = NONE;

        _disperse(old_child);
    } else {
        unsigned int old_parent = nodes[a].parent, old_sibling = nodes[a].next_sibling;

        nodes[a] = dispersedNode();
        if (old_sibling != NONE) {
            _disperse(old_sibling);
        } else if (old_parent != NONE) {
            _disperse(old_parent);
        }
    }

}


/* returns -2 if the node is dispersed (i.e. not part of any set) */
template <class Token>
unsigned int AugmentedDisjointSetForest<Token>::find(unsigned int a) {
    assert(a < nodes.size()); // sensible argument check

    unsigned int root = a, new_parent = nodes[a].parent;

    if (new_parent == DISPERSED) { // dispersed
        return NONE;
    } else if (new_parent == NONE) { // root
        return a;
    }

    do {
        root = new_parent;
        new_parent = nodes[root].parent;
    } while  (new_parent != NONE);


    /* perform the path compression */
    unsigned int old_parent;

    for (unsigned int y = a; nodes[y].parent != root; y = old_parent) {
        old_parent = nodes[y].parent;
        nodes[y].parent = root;

        unsigned int old_prev = nodes[y].prev_sibling, old_next = nodes[y].next_sibling;

        if (old_prev != NONE) {
            nodes[old_prev].next_sibling = old_next;
        }
        if (old_next != NONE) {
            nodes[old_next].prev_sibling = old_prev;
        }

        /* adjust old_parent's first child if y was old_parent's first child */
        if (nodes[old_parent].first_child == y) {
            nodes[old_parent].first_child = old_next;
        }

        nodes[y].prev_sibling = NONE;
        nodes[y].next_sibling = nodes[root].first_child;

        if (nodes[root].first_child != NONE) {
            nodes[nodes[root].first_child].prev_sibling = y;
        }
        nodes[root].first_child = y;
    }

    return root;
}

