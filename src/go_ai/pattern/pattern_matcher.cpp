#include "pattern_matcher.hpp"

#include <map>
#include <boost/assign/std/vector.hpp> // for 'operator+=()'

using namespace boost::assign; // bring 'operator+=()' into scope
using namespace std;

const char* PatternMatcher::hane_a =
    "#O#"
    " x "
    "xxx";

const char* PatternMatcher::hane_b =
    "#O "
    " x "
    "x x";

const char* PatternMatcher::hane_c =
    "#Ox"
    "#x "
    "x x";

const char* PatternMatcher::hane_d =
    "#OO"
    " # "
    "x x";

const char* PatternMatcher::cut1_a =
    "#Ox"
    "Oxx"
    "xxx";

const char* PatternMatcher::cut1_b =
    "#Ox"
    "OxO"
    "x x";

const char* PatternMatcher::cut2 =
    "x#x"
    "OxO"
    "www";

const char* PatternMatcher::edge_a =
    "# x"
    "Oxx"
    "eee";

const char* PatternMatcher::edge_b =
    "x#x"
    "#xO"
    "eee";

const char* PatternMatcher::edge_c =
    "x#O"
    "x#x"
    "eee";

const char* PatternMatcher::edge_d =
    "x#O"
    "xO#"
    "eee";

const char* PatternMatcher::edge_e =
    "x#O"
    "OO#"
    "eee";

const char* PatternMatcher::patterns[] = {hane_a, hane_b, hane_c, hane_d, cut1_a, cut1_b, cut2,
                                          edge_a, edge_b, edge_c, edge_d, edge_e};

const char* PatternMatcher::pattern_names[] =
    { "hane_a", "hane_b", "hane_c", "hane_d", "cut1_a", "cut1_b", "cut2",
      "edge_a", "edge_b", "edge_c", "edge_d", "edge_e" };

Pattern3x3 PatternMatcher::patternFlippedHorizontal(const Pattern3x3& pattern) {
    Pattern3x3 ret;

    for (unsigned int y = 0; y < 3; y++) {
        for (unsigned int x = 0; x < 3; x++) {
            ret.p[2 - x][y] = pattern.p[x][y];
        }
    }

    return ret;
}

Pattern3x3 PatternMatcher::patternRotated90deg(const Pattern3x3& pattern) {
    Pattern3x3 ret;

    for (unsigned int y = 0; y < 3; y++) {
        for (unsigned int x = 0; x < 3; x++) {
            ret.p[2 - y][x] = pattern.p[x][y];
        }
    }

    return ret;
}

Pattern3x3 PatternMatcher::patternColourFlipped(const Pattern3x3& pattern) {
    Pattern3x3 ret;

    map<char, char> substitution_map;
    substitution_map['x'] = 'x';
    substitution_map['O'] = '#';
    substitution_map[' '] = ' ';
    substitution_map['#'] = 'O';
    substitution_map['w'] = 'b';
    substitution_map['b'] = 'w';
    substitution_map['e'] = 'e';

    for (unsigned int y = 0; y < 3; y++) {
        for (unsigned int x = 0; x < 3; x++) {
            char c = pattern.p[x][y];

            assert(substitution_map.find(c) != substitution_map.end());
            char subst = substitution_map[c];

            ret.p[x][y] = subst;
        }
    }

    return ret;
}

std::vector<Pattern3x3> PatternMatcher::generateAllTransformationsOf(Pattern3x3 pattern) {
#ifndef NDEBUG
    Pattern3x3 original = pattern;
#endif

    vector<Pattern3x3> ret;

    // consider all transformations (i.e. all of the dihedral group of order 8), and colour swaps
    for (unsigned int colour_flip = 0; colour_flip < 2; colour_flip++) {
        for (unsigned int reflect = 0; reflect < 2; reflect++) {
            for (unsigned int rotate = 0; rotate < 4; rotate++) {
                ret.push_back(pattern);
                pattern = patternRotated90deg(pattern);
            }

            pattern = patternFlippedHorizontal(pattern);
        }
        pattern = patternColourFlipped(pattern);
    }

#ifndef NDEBUG
    assert(pattern == original);
#endif

    return ret;
}

uint32_t PatternMatcher::convertToInteger(const Pattern3x3 board) {
    uint32_t ret = 0;

    for (unsigned int x = 0; x < 3; ++x) {
        for (unsigned int y = 0; y < 3; ++y) {
            uint32_t n;

            switch (board.p[x][y]) {
                case ' ': n = 0; break;
                case '#': n = 1; break;
                case 'O': n = 2; break;
                case 'e': n = 3; break;
                default: assert(false); abort();
            }

            ret = (ret << 2) | n;
        }
    }

    return ret;
}

std::set<uint32_t> PatternMatcher::getAllPatternsFor(const Pattern3x3 pattern) {
    std::set<uint32_t> ret;

    std::vector<Pattern3x3> transformed_pattern = generateAllTransformationsOf(pattern);
    assert(transformed_pattern.size() == transformations);

    for (unsigned int j = 0; j < transformations; j++) {
        std::vector<Pattern3x3> realized_patterns = generateAllUntransformedRealizationsOf(transformed_pattern[j]);

        for (std::vector<Pattern3x3>::const_iterator it = realized_patterns.begin(); it != realized_patterns.end(); ++it) {
            ret.insert(convertToInteger(*it));
        }
    }

    return ret;
}

void PatternMatcher::addPatternsToHashSet(const std::set<uint32_t>& s, unsigned int id) {
    for (std::set<uint32_t>::const_iterator it = s.begin(); it != s.end(); ++it) {
        pattern_bitset.setBit(*it);
        pattern_vec[*it] |= (1 << id);
    }
}

/* not fast */
void eraseAllFrom(std::set<uint32_t> &source, const std::set<uint32_t> &to_remove) {
    for (std::set<uint32_t>::const_iterator it = to_remove.begin(); it != to_remove.end(); ++it) {
        source.erase(*it);
    }
}

PatternMatcher::PatternMatcher() {
    pattern_vec.resize(1 << 18); // 4 to the power 9

    addPatternsToHashSet(getAllPatternsFor(Pattern3x3(hane_a)), HANE_A);
    addPatternsToHashSet(getAllPatternsFor(Pattern3x3(hane_b)), HANE_B);
    addPatternsToHashSet(getAllPatternsFor(Pattern3x3(hane_c)), HANE_C);
    addPatternsToHashSet(getAllPatternsFor(Pattern3x3(hane_d)), HANE_D);

    // for the cut1 patterns we want "cut1_a and (not cut1_b) and (not cut1_c)"
    std::set<uint32_t> cut1_set = getAllPatternsFor(Pattern3x3(cut1_a));
    eraseAllFrom(cut1_set, getAllPatternsFor(Pattern3x3(cut1_b)));

    addPatternsToHashSet(cut1_set, CUT1_A);

    addPatternsToHashSet(getAllPatternsFor(Pattern3x3(cut2)), CUT2);

    addPatternsToHashSet(getAllPatternsFor(Pattern3x3(edge_a)), EDGE_A);
    addPatternsToHashSet(getAllPatternsFor(Pattern3x3(edge_b)), EDGE_B);
    addPatternsToHashSet(getAllPatternsFor(Pattern3x3(edge_c)), EDGE_C);
    addPatternsToHashSet(getAllPatternsFor(Pattern3x3(edge_d)), EDGE_D);
    addPatternsToHashSet(getAllPatternsFor(Pattern3x3(edge_e)), EDGE_E);
}

bool PatternMatcher::checkForPatternMatch(Pattern3x3 board) const {
    return checkForPatternMatch(convertToInteger(board));
}

vector<string> PatternMatcher::listPatternsMatchedBy(Pattern3x3 board) const {
    vector<string> ret;
    unsigned int v = pattern_vec[convertToInteger(board)];

    for (unsigned int i = 0; i < num_patterns; i++) {
        if (v & (1 << i)) {
            ret.push_back(pattern_names[i]);
        }
    }

    return ret;
}

/* not designed for speed */
std::vector<Pattern3x3> PatternMatcher::generateAllUntransformedRealizationsOf(Pattern3x3 pattern) {
    std::vector<Pattern3x3> vec(1); // note: vec has to start with an element or it will stay empty

    for (unsigned int y = 0; y < 3; y++) {
       for (unsigned int x = 0; x < 3; x++) {
            char c = pattern.p[x][y];

            std::vector<char> valid_chars;
            switch (c) {
                case 'x': // don't care
                    valid_chars += '#', 'O', ' '; break;

                case 'O':
                    valid_chars += 'O'; break;

                case '#':
                    valid_chars += '#'; break;

                case 'w':
                    valid_chars += '#', ' '; break;

                case 'b':
                    valid_chars += 'O', ' '; break;

                case ' ':
                    valid_chars += ' '; break;

                case 'e':
                    valid_chars += 'e'; break;

                default: abort();
            }

            std::vector<Pattern3x3> new_vec;
            for (vector<Pattern3x3>::const_iterator it = vec.begin(); it != vec.end(); ++it) {
                for (vector<char>::const_iterator vc_it = valid_chars.begin(); vc_it != valid_chars.end(); ++vc_it) {
                    Pattern3x3 new_pattern = *it;
                    new_pattern.p[x][y] = *vc_it;
                    new_vec.push_back(new_pattern);
                }
            }

            swap(vec, new_vec);
        }
    }

    return vec;
}
