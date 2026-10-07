#include "ClauseScoringMethod.hpp"

#include "clauseScoring/alternative/ClauseScoringMethodLiteralMetric.hpp"
#include "clauseScoring/alternative/ClauseScoringMethodNone.hpp"

#include <iostream>

#include "src/exceptions/FactoryException.hpp"
#include "src/specs/cnf/SpecManagerAll.hpp"

namespace d4 {

ClauseScoringMethod *
ClauseScoringMethod::makeClauseScoringMethod(Config &config, SpecManager &p,
                                             ScoringMethod &scoringMethod,
                                             std::ostream &out) {
  std::string meth = config.clause_scoring;
  out << "c [CONSTRUCTOR] Clause heuristic: " << meth << "\n";

  try {
    auto &mixedSpec = dynamic_cast<SpecManagerAll &>(p);
    if (meth == "literal-sum")
      return new ClauseScoringMethodLiteralSum(mixedSpec, scoringMethod);
    if (meth == "literal-sum-over-branch-avg")
      return new ClauseScoringMethodLiteralSumOverBranchAvg(mixedSpec,
                                                            scoringMethod);
    if (meth == "literal-min")
      return new ClauseScoringMethodLiteralMin(mixedSpec, scoringMethod);
    if (meth == "branch-min")
      return new ClauseScoringMethodBranchMin(mixedSpec, scoringMethod);
    if (meth == "literal-max")
      return new ClauseScoringMethodLiteralMax(mixedSpec, scoringMethod);
    if (meth == "branch-max")
      return new ClauseScoringMethodBranchMax(mixedSpec, scoringMethod);
    if (meth == "branches-avg-divided-by-distribution")
      return new ClauseScoringMethodBranchesAvgDividedByDistribution(
          mixedSpec, scoringMethod);
    if (meth == "none")
      return new ClauseScoringMethodNone(mixedSpec, scoringMethod);
  } catch (std::bad_cast &bc) {
    out << "c [CONSTRUCTOR] Clause scoring requires a mixed formula\n";
    throw FactoryException("A mixed formula was expected for clause scoring",
                           __FILE__, __LINE__);
  }

  out << "c [CONSTRUCTOR] Unsupported clause scoring method: " << meth << "\n";
  throw FactoryException("Unsupported clause scoring method", __FILE__,
                         __LINE__);
}

bool ClauseScoringMethod::selectClause(std::vector<Var> &connected,
                                       std::vector<Lit> &clause,
                                       bool ClauseIfAvailable) {
  double bestClauseScore = getBestScoreClause(connected, clause);
  if (bestClauseScore < 0)
    return false;
  if (ClauseIfAvailable)
    return true;
  Var bestVar = csm_scoringMethod.selectVariable(connected, csm_specManager);
  double bestVarScore = -1;
  if (bestVar != var_Undef)
    bestVarScore = csm_scoringMethod.computeScore(bestVar);

  if (bestClauseScore <= bestVarScore)
    return false;
  return true;
}

} // namespace d4
