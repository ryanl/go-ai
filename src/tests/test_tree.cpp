#undef NDEBUG

#include "assert.h"
#include "../go_ai/tree.hpp"
#include <iostream>

void test1() {
    typedef Tree<unsigned int> Tree_t;
    Tree_t tree(10);

    assert(tree.getUnusedCapacity() == 9);

    tree.eraseAllButRoot();
    assert(tree.getUnusedCapacity() == 9);

    Tree_t::Node *root = tree.getRoot();

    assert(tree.getNumChildren(root) == 0);

    Tree_t::Node *n = tree.addChild(root);

    assert(tree.getUnusedCapacity() == 8);

    n->val = 5; // its unsigned int value

    assert(n->val == 5);

    assert(tree.getNumChildren(root) == 1);
    assert(tree.getChild(root, 0) == n);
    assert(tree.getFirstChild(root) == n);

    Tree_t::Node *n2 = tree.addChild(root);
    assert(tree.getNumChildren(root) == 2);
    assert(tree.getChild(root, 0) == n);
    assert(tree.getChild(root, 1) == n2);
    assert(tree.getFirstChild(root) == n);

    unsigned expected[2] = { 5, 4 };
    n2->val = 4;

    unsigned int i = 0;
    for (Tree_t::ChildIterator it = tree.childBegin(root); it != tree.childEnd(root); ++it) {
        assert (it->val == expected[i]);
        i++;
    }

    // erase everything except the root
    tree.eraseChildrenOfUnmarkedNodes();
    assert(tree.getUnusedCapacity() == 9);
    root = tree.getRoot();
    assert(tree.getNumChildren(root) == 0);
}

/*
void parseAndCreateTreeFrom(Tree<std::string> &tree, const std::string& text) {
    typedef Tree<std::string>::Node Node;
    std::stack<Node*> node_stack;

    for (unsigned int i = 0; i < text.length(); i++) {
        node_stack.push(tree.addChild(node_stack.top()));

        std::string label;
        for (i++; i < text.length && text[i] != '(' && text[i] != ',' && text[i] != ')'; i++) {
            label.push_back(text[i]);
        }

        node_stack.top()->val = label;

        if (i < text.length) {
            if (text[i] == '(') {

            }
        }
    }

    assert(node_stack.size() == 0); // parentheses match up
}

std::string createStringFrom(Tree<std::string> &tree, Tree<std::string>::Node* node) {
    std::string ret = node->val;

    unsigned int children = tree.getNumChildren(node);
    if (children > 0) {
        ret += "(";
        for (unsigned int i = 0; i < children; i++) {

        }
    }
}

void test2() {
    Tree<std::string> tree(10);;

    parseAndCreateTreeFrom(tree, "1,2,3(31,32),4,7(72,75(753,759),76(761))");
}
*/

int main(int argc, char* argv[]) {
    test1();

    std::cout << "PASSED\n";
}
