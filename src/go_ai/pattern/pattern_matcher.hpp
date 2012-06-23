#ifndef __PATTERN_MATCHER_HPP
#define __PATTERN_MATCHER_HPP

#include <vector>
#include <set>
#include <string>
#include "assert.h"
#include "../../go_mechanics/go_state.hpp"

//#include <boost/unordered_map.hpp>

typedef unsigned int uint32_t;

/*
struct SymbolBitOps {
    bool xorop[2];
    bool orop [2];toString
};

struct PatternBitOps {
    u_int32 xorop;
    u_int32 orop;
};
*/

struct Pattern3x3 {
    char p[3][3];

    Pattern3x3() {
        for (unsigned int y = 0; y < 3; y++) {
            for (unsigned int x = 0; x < 3; x++) {
                p[x][y] = ' ';
            }
        }
    }

    void setToString(const char* src) {
        for (unsigned int y = 0; y < 3; y++) {
            for (unsigned int x = 0; x < 3; x++) {
                p[x][y] = *(src++);
            }
        }
    }

    Pattern3x3(const char *src) {
        setToString(src);
    }

    Pattern3x3(const std::string &s) {
        assert(s.length() == 9);
        setToString(s.c_str());
    }

    bool operator == (const Pattern3x3& other) const {
        for (unsigned int y = 0; y < 3; y++) {
            for (unsigned int x = 0; x < 3; x++) {
                if (other.p[x][y] != p[x][y]) return false;
            }
        }

        return true;
    }

    bool operator != (const Pattern3x3& other) const {
        return !(*this == other);
    }

    std::string toString() const {
        std::string ret;

        for (unsigned int y = 0; y < 3; y++) {
            for (unsigned int x = 0; x < 3; x++) {
                ret.push_back(p[x][y]);
            }
        }
        return ret;
    }
};

class PatternMatcher {
    private:
        ImprovedBitset<(1 << 18)> pattern_bitset;

        // for performance patterns stored as 32-bit integers rather than an array of characters
        // values: 1 bit per pattern representing that pattern is matched
        //boost::unordered_map<uint32_t, unsigned int> pattern_map;
        std::vector<unsigned int> pattern_vec;


        void addPatternsToHashSet(const std::set<uint32_t>& s, unsigned int id);

    public:
        static const unsigned int num_patterns = 12;
        static const unsigned int transformations = 16;
        // static const unsigned int pattern_size = 18;

        // the patterns themselves (defined in pattern_matcher.cpp)
        // cut1_c is not needed - transformations produce it from cut1_b
        static const char *hane_a, *hane_b, *hane_c, *hane_d, *cut1_a, *cut1_b, *cut2,
                          *edge_a, *edge_b, *edge_c, *edge_d, *edge_e;

        // the same patterns in an array rather than named
        static const char* patterns[num_patterns];


        enum PatternIds {
            HANE_A = 0,
            HANE_B,
            HANE_C,
            HANE_D,
            CUT1_A,
            CUT1_B,
            CUT2,
            EDGE_A,
            EDGE_B,
            EDGE_C,
            EDGE_D,
            EDGE_E
        };

        static const char* pattern_names[num_patterns];

        PatternMatcher();

        static Pattern3x3 patternFlippedHorizontal(const Pattern3x3& pattern);
        static Pattern3x3 patternRotated90deg(const Pattern3x3& pattern);
        static Pattern3x3 patternColourFlipped(const Pattern3x3& pattern);

        /*! get all realizations of transformations of pattern, encoded as 32-bit integers */
        static std::set<uint32_t> getAllPatternsFor(const Pattern3x3 pattern);

        static std::vector<Pattern3x3> generateAllTransformationsOf(Pattern3x3 pattern);

        static std::vector<Pattern3x3> generateAllUntransformedRealizationsOf(Pattern3x3 pattern);

        static uint32_t convertToInteger(Pattern3x3 board);

        //static SymbolBitOps opsForSymbol(char symbol);

        bool doesThisPatternMatch(unsigned int pattern_number, uint32_t v) const {
            return pattern_vec.at(v) & (1 << pattern_number);
        }
        uint32_t whatPatternsMatch(uint32_t v) const {
            return pattern_vec.at(v);
        }

        bool checkForPatternMatch(Pattern3x3 board) const;

        bool checkForPatternMatch(uint32_t v) const {
            // using a bitset should have better cache performance
            return pattern_bitset.getBit(v);
        }

        uint32_t convertToInteger(unsigned int x, unsigned int y, const GoState &s) {
            uint32_t ret = 0;

            unsigned int y_saved = y - 1;
            char         cell_contents;
            x--;

            // hopefully the compiler will unroll this
            for (int i = 0; i < 3; i++) {
                y = y_saved;
                for (int j = 0; j < 3; j++) {

                    unsigned int n;

                    if (j == 1 && i == 1) {
                        assert(s.get(x, y) == EMPTY);
                        n = (s.getNextToPlay() == BLACK) ? 1 : 2;
                    } else {
                        if (x >= BOARDSIZE || y >= BOARDSIZE) {
                            n = 3;
                        } else {
                            cell_contents = s.get(x, y);
                            switch (cell_contents) {
                                case BLACK: n = 1;
                                            break;

                                case WHITE: n = 2;
                                            break;

                                default:    assert(cell_contents == EMPTY);
                                            n = 0;
                                            break;

                            }
                        }
                    }
                    ret = (ret << 2) | n;
                    y++;
                }
                x++;
            }

            return ret;
        }

        std::vector<std::string> listPatternsMatchedBy(Pattern3x3 board) const;
};

#endif

