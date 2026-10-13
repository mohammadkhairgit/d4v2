#pragma once

#include <src/heuristics/ClauseScoringMethod.hpp>

namespace d4 {
// Never select a clause, always return false.
class ClauseScoringMethodNone final : public ClauseScoringMethod {
public:
  ClauseScoringMethodNone(SpecManagerAll &specManager,
                          ScoringMethod &scoringMethod)
      : ClauseScoringMethod(specManager, scoringMethod) {}

  double computeScore(const ClauseType &clause) override;
  double getBestScoreClause(std::vector<Var> &connected,
                            std::vector<Lit> &clause) override;
};
} // namespace d4