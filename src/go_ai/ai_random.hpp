#include "../go_mechanics/random.hpp"

class GoAI_Random {
private:
    RNG rng;

public:
    GoAI_Random() {}

    GoMove selectMove(const GoState& s) {
        std::vector<GoMove> valid_moves = s.getValidMoves();
        std::vector<GoMove> non_eye_filling_moves;

        for (unsigned int i = 0; i < valid_moves.size(); i++) {
            GoMove move = valid_moves[i];
            if (!move.isPass() && !s.isSelfEyeFilling(move)) {
                non_eye_filling_moves.push_back(move);
            }
        }
        if (non_eye_filling_moves.size() > 0) {
            unsigned int i = rng.getIntBetween(0, non_eye_filling_moves.size());
            return non_eye_filling_moves[i];
        } else {
            return GoMove::pass();
        }
    }
}
