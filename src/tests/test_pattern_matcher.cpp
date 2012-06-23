#undef NDEBUG

#include "go_ai/pattern/pattern_matcher.hpp"

#include "assert.h"
#include <iostream>
#include <map>
#include <string>
#include <set>
#include <boost/assign/std/set.hpp> // for 'operator+=()'


using namespace std;
using namespace boost::assign; // bring 'operator+=()' into scope

void testPatternTransformations(const PatternMatcher &pm) {
    Pattern3x3 hane_a_pat(PatternMatcher::hane_a);
    Pattern3x3 hane_b_pat(PatternMatcher::hane_b);
    Pattern3x3 edge_c_pat(PatternMatcher::edge_c);

    assert(PatternMatcher::patternFlippedHorizontal(hane_a_pat) ==
           Pattern3x3("#O#"
                      " x "
                      "xxx"));

    assert(PatternMatcher::patternFlippedHorizontal(hane_b_pat) ==
           Pattern3x3(" O#"
                      " x "
                      "x x"));

    assert(PatternMatcher::patternRotated90deg(edge_c_pat).toString() ==
           "exx"
           "e##"
           "exO");

}

void test1(const PatternMatcher &pm) {
    map<std::string, set<unsigned int> > expected_matches;

    expected_matches[
        "O#O"
        " # "
        "#  "
    ] += PatternMatcher::HANE_A;

    expected_matches[
        "O#O"
        "O#O"
        "###"
    ] += PatternMatcher::CUT2;

    expected_matches[
        "O#O"
        "#O#"
        "# #"
    ];

    expected_matches[
        "O#O"
        "#O "
        "# #"
    ] += PatternMatcher::CUT1_A;

    bool any_failures = false;

    for (map< std::string, set<unsigned int> >::const_iterator it = expected_matches.begin();
         it != expected_matches.end(); ++it) {

        uint32_t v = pm.convertToInteger(Pattern3x3(it->first));

        assert(pm.checkForPatternMatch(v) ==  (it->second.size() > 0));
        //std::cout << pm.whatPatternsMatch(v) << "\n";
        for (unsigned int i = 0; i < 12; i++) {

            bool match = pm.doesThisPatternMatch(i, v);
            if (match != (it->second.find(i) != it->second.end())) {
                std::cout << "FAILURE: '" << (it->first) << "' (" << v << ") matches " << pm.pattern_names[i] << " = " << match << "\n";
                any_failures = true;
            }
        }
    }

    assert(!any_failures);
}

void test2(const PatternMatcher &pm) {
    vector<Pattern3x3> hane_a_transforms = pm.generateAllTransformationsOf(Pattern3x3(pm.hane_a));
    unsigned int i = 0;

    assert(hane_a_transforms.size() == 16);
    assert(hane_a_transforms[8].toString() == "O#O x xxx");
    vector<Pattern3x3> hane_a8_examples = pm.generateAllUntransformedRealizationsOf(hane_a_transforms[8]);

    std::cout << "---------------\n";
    i = 0;
    bool found = false;
    for (vector<Pattern3x3>::iterator it = hane_a8_examples.begin(); it != hane_a8_examples.end(); ++it) {
        if (it->toString() == "O#O # #  ") found = true;
    }
    assert(found);
}

int main(int argc, char* argv[]) {
    PatternMatcher pm;

    testPatternTransformations(pm);
    test1(pm);
    test2(pm);

    std::cout << "PASSED\n";
}
