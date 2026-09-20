#include "ClauseScoringMethodNone.hpp"

namespace d4 {

double ClauseScoringMethodNone::computeScore(const ClauseType &clause) {
  (void)clause;
  return -1;
}

double ClauseScoringMethodNone::getBestScoreClause(
    std::vector<Var> &connected, std::vector<Lit> &clause) {
  (void)connected;
  (void)clause;
  return -1;
}

} // namespace d4