
/*!
    The tree class represents a general purpose tree structure
    allowing any number of children

    Children are stored contiguously in memory. Children must all
    be added at the same time.

    Making the tree multithreaded shouldn't be too difficult - just
    use a separate allocator block for each thread.
*/

#include <vector>

template <typename T>
class Tree {
    friend class ChildIterator;

public:

    class Node {
        friend class Tree;

    private:
        unsigned char mark;
        unsigned int first_child;
        unsigned int parent;
        unsigned int num_children;

    public:
           T val;
    };

    class ChildIterator {
        friend class Tree;

        private:
            Node* n;
            Node* end;

            ChildIterator(Tree &tree, Node& parent, unsigned int child) :
                n(&tree.nodes[parent.first_child] + child),
                end(&tree.nodes[parent.first_child] + parent.num_children)
            {}

        public:
            Node& operator * () {
                return *n;
            }

            Node& operator *() const {
                return *n;
            }

            Node* operator-> () {
                return n;
            }

            const Node* operator-> () const {
                return n;
            }

            bool operator == (const ChildIterator& other) const {
                return n == other.n;
            }

            bool operator != (const ChildIterator& other) const {
                return n != other.n;
            }

            void operator ++ () {
                n++;
            }

            bool done() const {
                return n == end;
            }

    };

private:

    /*! next index to allocate at */
    unsigned int allocation_index;

    unsigned int root_index;

    Node* nodes;
    unsigned int max_nodes;

    unsigned int getIndexOf(const Node* node) const {
        unsigned int ret = node - &nodes[0];
        assert(ret < max_nodes);

        return ret;
    }

    static const unsigned char MARK_KEEP        = 1;
    static const unsigned char MARK_KEEP_KIDS   = 2;

public:

    /*! constructor for Tree
        @param[max_nodes] The maximum number of vertices the tree may contain.
                          Since empty trees are not allowed (any tree must contain a root),
                          max_nodes should be >= 1.

    */
    Tree(unsigned int _max_nodes) :
        allocation_index(1),
        root_index(0),
        max_nodes(_max_nodes)
    {
        // don't init
        unsigned long long* tmp = new unsigned long long[1 + ((sizeof(Node) * (unsigned long long)(max_nodes)) / sizeof(long long))];
        nodes = (Node*)tmp;


        assert(max_nodes > 0);

        nodes[0].val = T();
        nodes[0].parent = 0; // self-parent indicates root
        nodes[0].num_children = 0;
        nodes[0].mark = 0;
    }

    ~Tree() {
        delete [] (unsigned long long*) nodes;
    }

    unsigned int getUnusedCapacity() const {
        return max_nodes - allocation_index;
    }

    unsigned int getMaxNodes() const {
        return max_nodes;
    }


    /*!
        removes all nodes except the root of the tree

        warning: invalidates all node pointers and iterators
    */
    void eraseAllButRoot() {
        Node* root = getRoot();
        nodes[0] = *root;
        nodes[0].num_children = 0;
        nodes[0].parent = 0;
        nodes[0].mark = 0;

        root_index = 0;
        allocation_index = 1;
    }

    bool isRoot(const Node* n) const {
        return n == &nodes[root_index];
    }

    ChildIterator childBegin(Node* n) {
        assert(getIndexOf(n) < max_nodes);
        return ChildIterator(*this, *n, 0);
    }

    ChildIterator childEnd(Node* n) {
        assert(getIndexOf(n) < max_nodes);
        return ChildIterator(*this, *n, n->num_children);
    }

    Node* getParent(Node* n) {
        assert(getIndexOf(n) < max_nodes);
        return &nodes[n->parent];
    }

    Node* getFirstChild(Node* n) {
        assert(getIndexOf(n) < max_nodes);
        return &nodes[n->first_child];
    }

    Node* getChild(Node* n, unsigned int child_number) {
        unsigned int index = n->first_child + child_number;

        assert(child_number < n->num_children);
        assert(index < allocation_index);

        return &nodes[index];
    }

    const Node* getChild(const Node* n, unsigned int child_number) const {
        unsigned int index = n->first_child + child_number;

        assert(child_number < n->num_children);
        assert(index < allocation_index);

        return &nodes[index];
    }



    unsigned int getNumChildren(const Node* n) const {
        assert(getIndexOf(n) < max_nodes);
        return n->num_children;
    }

    /*!
        Takes a subtree of the tree rooted at a given node
        and makes it the new root.
    */
    void reRoot(Node* node) {
        root_index = getIndexOf(node);
        node->parent = root_index;
    }

    /*!
        @return a pointer to the root of the tree
    */
    Node* getRoot() {
        return &nodes[root_index];
    }


    const Node* getRoot() const {
        return &nodes[root_index];
    }

    /*!
        Warning: this may move other child nodes of node, e.g. if we reach the end of the nodes
                 array (which is used circularly but doesn't allow children to wrap around, for
                 performance reasons).

        Can only be done if node is childless or no other node has had
        children added since the node has had children added
    */
    Node* addChild(Node* node) {
        assert((node->num_children == 0) || ((node->first_child + node->num_children) == allocation_index));
        assert(getUnusedCapacity() > 0);

        unsigned int parent_index = getIndexOf(node);

        if (node->num_children == 0) {
            node->first_child = allocation_index;
        }
        node->num_children++;

        Node* new_child = &nodes[allocation_index];
        new_child->parent = parent_index;
        new_child->num_children = 0;
        new_child->mark = 0;

        allocation_index++;

        return new_child;
    }

    struct NodeConditional {
        virtual bool operator () (const Node* node) const = 0;
    };

    void recursivelyMarkIf(Node* start, const NodeConditional& nc) {
        if (nc(&*start)) {
            start->mark |= MARK_KEEP | MARK_KEEP_KIDS;

            for (ChildIterator ci = childBegin(start); !ci.done(); ++ci) {
                recursivelyMarkIf(&*ci, nc);
            }
        } else {
            start->mark |= MARK_KEEP;
        }
    }

    /*!
        Destroys children of nodes that are not marked,
        defragments the node array
        always keeps the root

        Note: invalidates all pointers to nodes of the tree, and all
              iterators.
    */
    void eraseChildrenOfUnmarkedNodes() {
        unsigned int write = 0;

        nodes[root_index].mark = MARK_KEEP;

        assert(root_index < allocation_index);

        for (unsigned int read = root_index; read != allocation_index; read++) {
               Node *node = &nodes[read];
            char mark = node->mark;

            if (mark) {
                // TODO: could handle read == write as a special case
                Node *write_node  = &nodes[write];
                *write_node = *node; // copy
                write_node->mark = 0; // clear mark

                if (mark & MARK_KEEP_KIDS) {
                    // adjust children's parent pointers
                    for (ChildIterator it = childBegin(write_node); !it.done(); ++it) {
                        it->parent = write;
                    }
                } else {
                    write_node->num_children = 0;
                }

                // adjust parent's first_child "pointer" if this is its first child
                if (read != root_index) {
                    Node* parent_node = getParent(node);
                    if (write < parent_node->first_child) {
                        parent_node->first_child = write;
                    }
                 }


                   write++;
            }
        }
        root_index = 0;
        nodes[root_index].parent = 0;
        allocation_index = write;
    }
};
