#include "graphing.hpp"

using namespace std;

LabelledGraph_t AlgorithmGrapher::generateGraphData(BanditProblem bp, std::vector<AlgorithmSettings> as) {
    LabelledGraph_t ret; //std::map< std::string, std::map<float, float> >

    unsigned int repeats = 1000;

    float max_bandit_prob = -1.0f;
    for (unsigned int i = 0; i < bp.bandits.size(); i++) {
        if (bp.bandits[i].getP() > max_bandit_prob) {
            max_bandit_prob = bp.bandits[i].getP();
        }
    }

    for (unsigned int i = 0; i < as.size(); i++) {
        std::map<float, float> line;
        std::vector<float> sum_readings(bp.plays + 1);
        std::vector<float> sumsq_readings(bp.plays + 1);

        for (unsigned int rep = 0; rep < repeats; rep++) {
            std::vector<unsigned int> readings = testBanditAlgorithm(*as[i].ba, bp);

            assert(sum_readings.size() == 1 + readings.size());

            for (unsigned int j = 0; j < readings.size(); j++) {
                sum_readings[j + 1]   += readings[j] / float(j + 1);
                sumsq_readings[j + 1] += (readings[j] / float(j + 1)) * (readings[j] / float(j + 1));
            }
        }

        for (unsigned int j = 0; j < sum_readings.size(); j++) {
            line[j] = (max_bandit_prob * j) - (sum_readings[j] / float(repeats));
        }

        ret[as[i].algorithm_name] = line;

        float f500_mean = sum_readings.at(500) / float(repeats);
        float f500_sd = sqrtf((sumsq_readings.at(500) / float(repeats)) - (f500_mean * f500_mean)); // didn't bother with sample variance

        float f5000_mean = sum_readings.at(5000) / float(repeats);
        float f5000_sd = sqrtf((sumsq_readings.at(5000) / float(repeats)) - (f5000_mean * f5000_mean)); // didn't bother with sample variance

        std::cout << as[i].algorithm_name << "\n";
        std::cout << "500: " <<  f500_mean <<  " +- " << f500_sd  << "\n";
        std::cout << "5000: " << f5000_mean << " +- " << f5000_sd << "\n";
     }


    return ret;
}

void AlgorithmGrapher::outputGraphForGnuplot(const LabelledGraph_t& graph) {
    ofstream f("gnuplot_data.txt");
    ofstream script("gnuplot_script.txt");

    script << "set terminal postscript enhanced color solid lw 2 \"Times-Roman\" 14\n";
    script << "set output \"plot.ps\"\n";
    script << "set samples 1001\n";
    script << "set grid\n";
    script << "set key on outside center bottom vertical\n";
    script << "set xlabel \"Plays\"\n";
    script << "set ylabel \"Regret\"\n";
    script << "set yrange [0:300]\n";

    bool first = true;
    unsigned int i = 0;

    for (LabelledGraph_t::const_iterator it = graph.begin(); it != graph.end(); ++it) {
        f << "#" << it->first << "\n";
        for (std::map<float, float>::const_iterator it2 = it->second.begin(); it2 != it->second.end(); ++it2) {
            f << it2->first << " " << it2->second << "\n";
        }
        f << "\n\n";

        if (first) {
            first = false;
            script << "plot ";
        } else {
            script << ",\\\n";
        }
         script << "\"gnuplot_data.txt\" index " << i << ":" << i << " using 1:2 with lines title \"" << it->first << "\"";

         i++;
    }
        script << "\n";
}

/*
void AlgorithmGrapher::outputGraphForR(const LabelledGraph_t& graph) {
    std::ofstream f("r.dat");
    f << "plays";

    for (unsigned int i = 0; i < graph.size(); i++) {
        f << ",";
        f << graph[i].label;
    }
    f << "\n";

    for (std::map<unsigned int, float>::const_iterator it = graph[0].data.begin();
         it != graph[0].data.end(); ++it) {

        f << it->first;
        for (unsigned int i = 0; i < graph.size(); i++) {
            f << ",";
            f << graph[i].data[it->first];
        }
        f << "\n";
    }
}
*/
