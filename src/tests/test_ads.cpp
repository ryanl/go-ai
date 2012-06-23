/* Tests for Augmented Disjoint Set Forest */

#include <set>
#include <cstdlib>
#include <assert.h>
#include <iostream>
#include <vector>

#include "../augmented_disjoint_set/ads_faster.hpp"

using namespace std;

/* I wrap a std::set as if AugmentedDisjointSetForest works properly
 * for this token type, it should work properly for any token type
 * (due to abstraction), excluding errors in the token type itself
 */

struct TestTokenType {
    std::set<unsigned int> members;

    void operator |= (const TestTokenType& other) {
        members.insert(other.members.begin(), other.members.end());
    }
};

std::string okay = " okay\n";

void outputSet(set<unsigned int> s) {
    for (set<unsigned int>::iterator it = s.begin(); it != s.end(); ++it) {
        cout << *it << " ";
    }
    cout << endl;
}

/*
void part2() {
    ADSFast<TestTokenType, 20> forest;
    for (unsigned int i = 0; i < 20; i++) {
        forest.createSingleton(i);
        if (i > 0) {
            forest.join(i, i - 1);
        }
    }
    int count[20] = {0};

    cout << "Checking nextGroupMember works...";

    for (unsigned int i = forest.find(11); i != forest.NONE; i = forest.nextGroupMember(i)) {
        count[i]++;
    }

    for (unsigned int i = 0; i < 20; i++) {
        assert(count[i] == 1);
    }
    cout << okay;
}
*/

int main(int argc, char *argv[]) {
    unsigned int elements = 40;

    /* ------------------------------------------------------------*/
    cout << "Creating a forest of " << elements << " elements... ";

    // create a new forest with 10 elements
    ADSFast<TestTokenType, 40> forest;

    cout << okay;
    /* ------------------------------------------------------------*/
    cout << "Checking size() returns " << elements << "... ";

    // check size returns the correct value
    assert(forest.size() == elements);

    // NONE should be distinguished from element values
    assert(forest.NONE >= elements);

    cout << okay;
    /* ------------------------------------------------------------*/
    cout << "Checking nodes begin not in any set (find_a == NONE) ... " << flush;

    // all elements should begin as not part of any set
    for (unsigned int i = 0; i < elements; i++) {
        assert(forest.find(i) == forest.NONE);
    }
    cout << okay;
    /* ------------------------------------------------------------*/
    cout << "Creating some singletons... " << endl;

    unsigned int values_to_pick = 20;

    std::vector<bool> element_picked(elements, false);
    std::vector<unsigned int> picked_values(values_to_pick);

    // let's use rand() to select a subset of the elements to create
    for (unsigned int i = 0; i < values_to_pick; i++) {
        unsigned int j;

        do {
            j = rand() % elements;
        } while (element_picked[j]);

        cout << "- creating element " << i << " at position " << j << endl;

        picked_values[i] = j;

        element_picked[j] = true;
        forest.createSingleton(j);

        TestTokenType initial_token;
        initial_token.members.insert(j);

        forest.tokenForRoot(forest.find(j)) = initial_token;

        // now check that only the elements created so far exist, and
        // find gives themselves

        for (unsigned int k = 0; k < elements; k++) {
            if (element_picked[k]) {
                assert(forest.find(k) == k);
            } else {
                assert(forest.find(k) == forest.NONE);
            }
        }
    }

    cout << "All singletons created" << endl;

    cout << "Unioning elements with creation indices equal mod 9... " << flush;

    // now lets union the ones with equal creation indices mod 9
    for (unsigned int i = 9; i < values_to_pick; i++) {
        forest.join(picked_values[i], picked_values[i - 9]);
    }

    cout << okay;

    cout << "Checking that find gives the expected result " << endl;


    // check that  i and i mod 9 are in the same tree
    for (unsigned int i = 0; i < values_to_pick; i++) {
        unsigned int find_i = forest.find(picked_values[i]);
        unsigned int find_i_mod_9 = forest.find(picked_values[i % 9]);
        cout << " - find_" << i << "(" << picked_values[i] << ") = " << find_i
             <<  ", find_" << (i % 9) << "(" << picked_values[i % 9] << ") = " << find_i_mod_9 << endl;
        assert(find_i == find_i_mod_9);
    }

    cout << "Results as expected" << endl;

    cout << "Checking that find is idempotent" << endl;

    // check that the roots of the trees really are roots, and they are distinct
    unsigned int root_for[9];
    for (unsigned int i = 0; i < 9; i++) {
        root_for[i] = forest.find(picked_values[i]);
    cout << "- root_for[" << i << "] = " << root_for[i] << endl;
        assert(forest.find(root_for[i]) == root_for[i]);

        for (unsigned int j = 0; j < i; j++) {
            assert(root_for[j] != root_for[i]);
        }
    }

    cout << "All okay" << endl;

    cout << "Joining elements with equal creation indices mod 3" << endl;
    // now lets union the ones with equal creation indices mod 3
    for (unsigned int i = 3; i < values_to_pick; i++) {
    cout << "Joining " << i << "(" << picked_values[i] << ") and "
         << (i - 3) << "(" << picked_values[i - 3] << ")" << endl;
        forest.join(picked_values[i], picked_values[i - 3]);
    }

    cout << okay;

    // check that i and i mod 3 are in the same tree
    for (unsigned int i = 0; i < values_to_pick; i++) {
        assert(forest.find(picked_values[i]) == forest.find(picked_values[i % 3]));
        assert(forest.find(picked_values[i]) != forest.NONE);
    }


    for (unsigned int i = 0; i < 3; i++) {
        root_for[i] = forest.find(picked_values[i]);
        assert(forest.find(root_for[i]) == root_for[i]);

        for (unsigned int j = 0; j < i; j++) {
            assert(root_for[j] != root_for[i]);
        }

        std::set<unsigned int> expected_set_contents;

        for (unsigned int v = i; v < values_to_pick; v += 3) {
            expected_set_contents.insert(picked_values[v]);
        }

        std::set<unsigned int> token_contents = forest.tokenForRoot(forest.find(root_for[i])).members;
        cout << "Expected set contents: ";
        outputSet(expected_set_contents);
        cout << "Token for set: ";
        outputSet(token_contents);
        assert(token_contents == expected_set_contents);

        /*
        // check nextGroupMember()
        std::set<unsigned int> enumerated_set_contents;

        for (unsigned int j = root_for[i]; j != forest.NONE; j = forest.nextGroupMember(j)) {
            assert(enumerated_set_contents.find(j) == enumerated_set_contents.end());
            enumerated_set_contents.insert(j);
        }
        assert(enumerated_set_contents == expected_set_contents);
        */
    }

    // checked that all the nodes that weren't used still don't exist
     for (unsigned int k = 0; k < elements; k++) {
         if (!element_picked[k]) {
             assert(forest.find(k) == forest.NONE);
         }
     }

     vector<bool> should_be_dispersed(elements, true);
     for (unsigned int i = 0; i < values_to_pick; i++) {
     if (i % 3 != 0)
         should_be_dispersed[picked_values[i]] = false;
     }

    // disperse the values with index equal to 0 mod 3
    forest.disperse(picked_values[0]);

    // checked that all the nodes that weren't used still don't exist
     for (unsigned int k = elements - 1; k < elements; k--) {
     unsigned int find_k = forest.find(k);
     cout << "Find " << k << " = " << int(find_k) << endl;

         if (should_be_dispersed[k]) {
             assert(forest.find(k) == forest.NONE);
         } else {
             assert(forest.find(k) != forest.NONE);
         }
     }

    // TODO: further checks
    //part2();

    cout << endl << "PASSED" << endl;

    return 0;
}
