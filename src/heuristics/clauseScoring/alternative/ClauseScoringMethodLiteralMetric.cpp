#include "ClauseScoringMethodLiteralMetric.hpp"

#include <algorithm>
#include <cmath>
#include <numeric>

namespace d4 {

double ClauseScoringMethodLiteralMetric::getBestScoreClause(
    std::vector<Var> &connected, std::vector<Lit> &clause) {
  std::vector<unsigned> idxClauses;
  csm_specManager.getCurrentClausesByKind(idxClauses, connected,
                                          ClauseKind::Alternative);

  // true if a clause was found, false otherwise.
  bool foundClause = false;
  ClauseScores bestClauseScore;
  // iterate through all current clauses and return the clause with the best
  // score.
  for (auto idx : idxClauses) {
    const ClauseType &currentClause = csm_specManager.getClause(idx);
    if (!currentClause.isStillRelevant()) {
      continue;
    }
    ClauseScores current = computeClauseScore(currentClause);
    if (!foundClause ||
        current.clauseCompareScore > bestClauseScore.clauseCompareScore) {
      foundClause = true;
      bestClauseScore.clauseCompareScore = current.clauseCompareScore;
      bestClauseScore.literalCompareScore = current.literalCompareScore;
      bestClauseScore.nbBranches = current.nbBranches;
      clause = currentClause.getLiterals();
      // prefer clauses with fewer branches.
    } else if (current.clauseCompareScore ==
                   bestClauseScore.clauseCompareScore &&
               current.nbBranches < bestClauseScore.nbBranches) {
      bestClauseScore.clauseCompareScore = current.clauseCompareScore;
      bestClauseScore.literalCompareScore = current.literalCompareScore;
      bestClauseScore.nbBranches = current.nbBranches;
      clause = currentClause.getLiterals();
    }
  }

  return foundClause ? bestClauseScore.literalCompareScore : -1;
}

double
ClauseScoringMethodLiteralMetric::computeScore(const ClauseType &clause) {
  ClauseScores clauseScore = computeClauseScore(clause);
  return clauseScore.literalCompareScore;
}

bool ClauseScoringMethodLiteralMetric::skipLiteralScoring(
    const Lit &lit) const {
  return csm_specManager.litIsAssigned(lit) ||
         (csm_specManager.nbSelected() && csm_specManager.isProj() &&
          !csm_specManager.isSelected(lit.var()));
}

ClauseScoringMethodLiteralMetric::ClauseScores
ClauseScoringMethodLiteralMin::computeClauseScore(const ClauseType &clause) {
  double remainingDecidableVars =
      static_cast<double>(clause.hasRemainingProjectedVars());
  // Do we have projected variables (do we need a branch for projected
  // variables)?
  double nonProjectedBranch =
      static_cast<double>(0 < clause.getNbNonProjectedVars());
  double numberOfBranches = remainingDecidableVars + nonProjectedBranch;
  ClauseScores clauseScore;
  // if their is only the projected variables branch.
  if (numberOfBranches < 2)
    return clauseScore;

  std::vector<double> literalScores(1, 0.0);
  for (const auto &lit : clause.getLiterals()) {
    if (skipLiteralScoring(lit))
      continue;

    literalScores.push_back(csm_scoringMethod.computeScore(lit.var()));
    literalScores.push_back(csm_scoringMethod.computeScore((~lit).var()));
  }

  double score = -1;
  for (std::size_t i = 1; i + 1 < literalScores.size(); i += 2)
    score = std::min(literalScores[i], literalScores[i + 1]);

  clauseScore.clauseCompareScore = score;
  clauseScore.literalCompareScore = score;
  clauseScore.nbBranches = numberOfBranches;

  return clauseScore;
}

ClauseScoringMethodLiteralMetric::ClauseScores
ClauseScoringMethodBranchMin::computeClauseScore(const ClauseType &clause) {
  double remainingDecidableVars =
      static_cast<double>(clause.hasRemainingProjectedVars());
  // Do we have projected variables (do we need a branch for projected
  // variables)?
  double nonProjectedBranch =
      static_cast<double>(0 < clause.getNbNonProjectedVars());
  double numberOfBranches = remainingDecidableVars + nonProjectedBranch;
  ClauseScores clauseScore;
  // if their is only the projected variables branch.
  if (numberOfBranches < 2)
    return clauseScore;

  std::vector<double> literalScores(1, 0.0);
  // std::vector<double> branchesScores;

  for (const auto &lit : clause.getLiterals()) {
    if (skipLiteralScoring(lit))
      continue;
    literalScores.push_back(csm_scoringMethod.computeScore(lit.var()));
    literalScores.push_back(csm_scoringMethod.computeScore((~lit).var()));
  }

  assert((literalScores.size() - 1) / 2 == clause.hasRemainingProjectedVars());
  double minimalBranchScore = -1;
  // For each branch compute its score by summing over the one positive literal
  // to be set and all the other negative literals to be set.
  for (int i = 0; i < numberOfBranches; ++i) {
    double branchScore = 0;
    branchScore += literalScores[1 + 2 * i];
    for (std::size_t j = 1; j + 1 < literalScores.size(); j += 2) {
      // For the last branch (projected branch) this if should always be true.
      if (j != 1 + 2 * i) {
        branchScore += literalScores[j + 1];
      }
    }
    minimalBranchScore = minimalBranchScore < 0
                             ? branchScore
                             : std::min(minimalBranchScore, branchScore);
  }

  clauseScore.clauseCompareScore = minimalBranchScore;
  clauseScore.literalCompareScore = minimalBranchScore;
  clauseScore.nbBranches = numberOfBranches;

  return clauseScore;
}

ClauseScoringMethodLiteralMetric::ClauseScores
ClauseScoringMethodBranchMax::computeClauseScore(const ClauseType &clause) {
  double remainingDecidableVars =
      static_cast<double>(clause.hasRemainingProjectedVars());
  // Do we have non-projected variables (do we need a branch for non-projected
  // variables)?
  double nonProjectedBranch =
      static_cast<double>(0 < clause.getNbNonProjectedVars());
  double numberOfBranches = remainingDecidableVars + nonProjectedBranch;
  ClauseScores clauseScore;
  // if their is only the projected variables branch.
  if (numberOfBranches < 2)
    return clauseScore;

  std::vector<double> literalScores(1, 0.0);
  // std::vector<double> branchesScores;

  for (const auto &lit : clause.getLiterals()) {
    if (skipLiteralScoring(lit))
      continue;
    literalScores.push_back(csm_scoringMethod.computeScore(lit.var()));
    literalScores.push_back(csm_scoringMethod.computeScore((~lit).var()));
  }

  assert((literalScores.size() - 1) / 2 == clause.hasRemainingProjectedVars());
  double maximalBranchScore = -1;
  // For each branch compute its score by summing over the one positive literal
  // to be set and all the other negative literals to be set.
  for (int i = 0; i < numberOfBranches; ++i) {
    double branchScore = 0;
    branchScore += literalScores[1 + 2 * i];
    for (std::size_t j = 1; j + 1 < literalScores.size(); j += 2) {
      // For the last branch (projected branch) this if should always be true.
      if (j != 1 + 2 * i) {
        branchScore += literalScores[j + 1];
      }
    }
    maximalBranchScore = maximalBranchScore < 0
                             ? branchScore
                             : std::max(maximalBranchScore, branchScore);
  }

  clauseScore.clauseCompareScore = maximalBranchScore;
  clauseScore.literalCompareScore = maximalBranchScore;
  clauseScore.nbBranches = numberOfBranches;

  return clauseScore;
}

ClauseScoringMethodLiteralMetric::ClauseScores
ClauseScoringMethodLiteralMax::computeClauseScore(const ClauseType &clause) {
  double remainingDecidableVars =
      static_cast<double>(clause.hasRemainingProjectedVars());
  // Do we have projected variables (do we need a branch for projected
  // variables)?
  double nonProjectedBranch =
      static_cast<double>(0 < clause.getNbNonProjectedVars());
  double numberOfBranches = remainingDecidableVars + nonProjectedBranch;
  ClauseScores clauseScore;
  // if their is only the projected variables branch.
  if (numberOfBranches < 2)
    return clauseScore;

  std::vector<double> literalScores(1, 0.0);

  for (const auto &lit : clause.getLiterals()) {
    if (skipLiteralScoring(lit))
      continue;

    literalScores.push_back(csm_scoringMethod.computeScore(lit.var()));
    literalScores.push_back(csm_scoringMethod.computeScore((~lit).var()));
  }

  double score = -1;
  for (std::size_t i = 1; i + 1 < literalScores.size(); i += 2)
    score = std::max(score, std::max(literalScores[i], literalScores[i + 1]));

  clauseScore.clauseCompareScore = score;
  clauseScore.literalCompareScore = score;
  clauseScore.nbBranches = numberOfBranches;

  return clauseScore;
}

ClauseScoringMethodLiteralMetric::ClauseScores
ClauseScoringMethodLiteralSum::computeClauseScore(const ClauseType &clause) {
  double remainingDecidableVars =
      static_cast<double>(clause.hasRemainingProjectedVars());
  // Do we have projected variables (do we need a branch for projected
  // variables)?
  double nonProjectedBranch =
      static_cast<double>(0 < clause.getNbNonProjectedVars());
  double numberOfBranches = remainingDecidableVars + nonProjectedBranch;
  ClauseScores clauseScore;
  // if their is only the projected variables branch.
  if (numberOfBranches < 2)
    return clauseScore;

  std::vector<double> literalScores(1, 0.0);

  for (const auto &lit : clause.getLiterals()) {
    if (skipLiteralScoring(lit))
      continue;

    literalScores.push_back(csm_scoringMethod.computeScore(lit.var()));
    literalScores.push_back(csm_scoringMethod.computeScore((~lit).var()));
  }

  double score = 0;
  double factor = static_cast<double>(literalScores.size() - 1);
  for (std::size_t i = 1; i + 1 < literalScores.size(); i += 2)
    score += literalScores[i + 1] * factor + literalScores[i];

  clauseScore.clauseCompareScore = score;
  clauseScore.literalCompareScore = score;
  clauseScore.nbBranches = numberOfBranches;

  return clauseScore;
}

ClauseScoringMethodLiteralMetric::ClauseScores
ClauseScoringMethodLiteralSumOverBranchAvg::computeClauseScore(
    const ClauseType &clause) {
  double remainingDecidableVars =
      static_cast<double>(clause.hasRemainingProjectedVars());
  // Do we have projected variables (do we need a branch for projected
  // variables)?
  double nonProjectedBranch =
      static_cast<double>(0 < clause.getNbNonProjectedVars());
  double numberOfBranches = remainingDecidableVars + nonProjectedBranch;
  ClauseScores clauseScore;
  // if their is only the projected variables branch.
  if (numberOfBranches < 2)
    return clauseScore;
  std::vector<double> literalScores(1, 0.0);
  for (const auto &lit : clause.getLiterals()) {
    if (skipLiteralScoring(lit))
      continue;

    literalScores.push_back(csm_scoringMethod.computeScore(lit.var()));
    literalScores.push_back(csm_scoringMethod.computeScore((~lit).var()));
  }

  double score = 0;
  assert((literalScores.size() - 1) / 2 == clause.hasRemainingProjectedVars());
  for (std::size_t i = 1; i + 1 < literalScores.size(); i += 2)
    score += literalScores[i + 1] * numberOfBranches + literalScores[i];

  clauseScore.clauseCompareScore = score / numberOfBranches;
  clauseScore.literalCompareScore = score / numberOfBranches;
  clauseScore.nbBranches = numberOfBranches;
  // return the avg score over the to be created branches
  return clauseScore;
}

} // namespace d4
