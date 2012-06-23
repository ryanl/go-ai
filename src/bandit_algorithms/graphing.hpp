#include <fstream>
#include <map>
#include <string>
#include "algorithms/algorithm.hpp"

typedef std::map< std::string, std::map<float, float> > LabelledGraph_t;

struct AlgorithmSettings {
    std::string algorithm_name;
    BanditAlgorithm *ba;
};

class AlgorithmGrapher {
public:
    LabelledGraph_t generateGraphData(BanditProblem bp, std::vector<AlgorithmSettings> as);

    void outputGraphForGnuplot(const LabelledGraph_t& graph);
    void outputGraphForR(const LabelledGraph_t& graph);
};

