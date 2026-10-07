#include "ClauseScoringMethodLiteralMetric.hpp"

#include <algorithm>
#include <cmath>
#include <numeric>

namespace d4 {

namespace {

double calculateMedian(const std::vector<double> &values, std::size_t begin,
                       std::size_t end) {
  if (begin >= end) {
    return 0.0;
  }

  const std::size_t length = end - begin;
  const std::size_t middle = begin + length / 2;
  if (length % 2 == 0) {
    return (values[middle - 1] + values[middle]) / 2.0;
  }

  return values[middle];
}

std::vector<double> sortedCopy(const std::vector<double> &values) {
  std::vector<double> sortedValues = values;
  std::sort(sortedValues.begin(), sortedValues.end());
  return sortedValues;
}

} // namespace

double calculateVariance(const std::vector<double> &values) {
  if (values.empty()) {
    return 0.0;
  }

  const double sum = std::accumulate(values.begin(), values.end(), 0.0);
  const double mean = sum / static_cast<double>(values.size());

  double squaredDistanceSum = 0.0;
  for (const double value : values) {
    const double distance = value - mean;
    squaredDistanceSum += distance * distance;
  }

  return squaredDistanceSum / static_cast<double>(values.size());
}

double calculateStandardDeviation(const std::vector<double> &values) {
  return std::sqrt(calculateVariance(values));
}

double calculateLowerQuartile(const std::vector<double> &values) {
  if (values.empty()) {
    return 0.0;
  }

  const std::vector<double> sortedValues = sortedCopy(values);
  const std::size_t half = sortedValues.size() / 2;
  return calculateMedian(sortedValues, 0, half);
}

double calculateUpperQuartile(const std::vector<double> &values) {
  if (values.empty()) {
    return 0.0;
  }

  const std::vector<double> sortedValues = sortedCopy(values);
  const std::size_t half = sortedValues.size() / 2;
  const std::size_t upperBegin = (sortedValues.size() % 2 == 0) ? half : half + 1;
  return calculateMedian(sortedValues, upperBegin, sortedValues.size());
}

double ClauseScoringMethodLiteralMetric::getBestScoreClause(
    std::vector<Var> &connected, std::vector<Lit> &clause) {
  std::vector<unsigned> idxClauses;
  csm_specManager.getCurrentClausesByKind(idxClauses, connected,
                                          ClauseKind::Alternative);

  bool foundClause = false;
  ClauseScores bestClauseScore;
  for (auto idx : idxClauses) {
    const ClauseType &currentClause = csm_specManager.getClause(idx);
    if (!currentClause.isStillRelevant()) {
      continue;
    }
    ClauseScores current = computeClauseScore(currentClause);
    if (!foundClause || current.clauseCompareScore > bestClauseScore.clauseCompareScore) {
      foundClause = true;
      bestClauseScore.clauseCompareScore = current.clauseCompareScore;
      bestClauseScore.literalCompareScore = current.literalCompareScore;
      clause = currentClause.getLiterals();
    }
  }

  return foundClause ? bestClauseScore.literalCompareScore : -1;
}

double ClauseScoringMethodLiteralMetric::computeScore(const ClauseType &clause) {
  ClauseScores clauseScore = computeClauseScore(clause);
  return clauseScore.literalCompareScore;
}

bool ClauseScoringMethodLiteralMetric::skipLiteralScoring(const Lit &lit) const {
    return csm_specManager.litIsAssigned(lit) 
      || (csm_specManager.nbSelected() && csm_specManager.isProj() && !csm_specManager.isSelected(lit.var()));
  }

ClauseScoringMethodLiteralMetric::ClauseScores ClauseScoringMethodLiteralMin::computeClauseScore(const ClauseType &clause) {
  double remainingDecidableVars = static_cast<double>(clause.hasRemainingProjectedVars());
  double nonProjectedBranch = static_cast<double>(0 < clause.getNbNonProjectedVars());
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

  return clauseScore;
}

ClauseScoringMethodLiteralMetric::ClauseScores ClauseScoringMethodBranchMin::computeClauseScore(const ClauseType &clause) {
  double remainingDecidableVars = static_cast<double>(clause.hasRemainingProjectedVars());
  double nonProjectedBranch = static_cast<double>(0 < clause.getNbNonProjectedVars());
  double numberOfBranches = remainingDecidableVars + nonProjectedBranch;
  ClauseScores clauseScore;
  // if their is only the projected variables branch.
  if (numberOfBranches < 2)
    return clauseScore;

  std::vector<double> literalScores(1, 0.0);
  std::vector<double> projectedLiteralScores(1, 0.0);
  //std::vector<double> branchesScores;

  for (const auto &lit : clause.getLiterals()) {
    if (csm_specManager.litIsAssigned(lit))
      continue;
    if (csm_specManager.nbSelected() && csm_specManager.isProj() && !csm_specManager.isSelected(lit.var())) {
      projectedLiteralScores.push_back(csm_scoringMethod.computeScore(lit.var()));
      projectedLiteralScores.push_back(csm_scoringMethod.computeScore((~lit).var()));
    }
    literalScores.push_back(csm_scoringMethod.computeScore(lit.var()));
    literalScores.push_back(csm_scoringMethod.computeScore((~lit).var()));
  }

  double score = 0;
  assert((literalScores.size()-1)/2 == clause.hasRemainingProjectedVars());
  double minimalBranchScore = -1;
  for (int i = 0; i < numberOfBranches; ++i) {
    double branchScore = 0;
    branchScore += literalScores[1 + 2 * i];
    for (std::size_t j = 1; j + 1 < literalScores.size(); j+=2){
      // For the last branch (projected branch) this if should always be true.
      if (j != 1 + 2 * i) {
        branchScore += literalScores[j+1];
      }
    }
    //branchesScores.push_back(branchScore);
    score += branchScore;
    minimalBranchScore = minimalBranchScore < 0 ? branchScore : std::min(minimalBranchScore, branchScore);
  }

  // return the avg score over the to be created branches divided by the standard deviation of the scores
  // double stddev = calculateStandardDeviation(std::vector<double>(branchesScores.begin(), branchesScores.end()));
  clauseScore.clauseCompareScore = minimalBranchScore;
  clauseScore.literalCompareScore = minimalBranchScore;
  // In case all branches have almost (or exactly) the same score, then ignore the stddev.
  return clauseScore;
}

ClauseScoringMethodLiteralMetric::ClauseScores ClauseScoringMethodBranchMax::computeClauseScore(const ClauseType &clause) {
  double remainingDecidableVars = static_cast<double>(clause.hasRemainingProjectedVars());
  double nonProjectedBranch = static_cast<double>(0 < clause.getNbNonProjectedVars());
  double numberOfBranches = remainingDecidableVars + nonProjectedBranch;
  ClauseScores clauseScore;
  // if their is only the projected variables branch.
  if (numberOfBranches < 2)
    return clauseScore;

  std::vector<double> literalScores(1, 0.0);
  std::vector<double> projectedLiteralScores(1, 0.0);
  //std::vector<double> branchesScores;

  for (const auto &lit : clause.getLiterals()) {
    if (csm_specManager.litIsAssigned(lit))
      continue;
    if (csm_specManager.nbSelected() && csm_specManager.isProj() && !csm_specManager.isSelected(lit.var())) {
      projectedLiteralScores.push_back(csm_scoringMethod.computeScore(lit.var()));
      projectedLiteralScores.push_back(csm_scoringMethod.computeScore((~lit).var()));
    }
    literalScores.push_back(csm_scoringMethod.computeScore(lit.var()));
    literalScores.push_back(csm_scoringMethod.computeScore((~lit).var()));
  }

  double score = 0;
  assert((literalScores.size()-1)/2 == clause.hasRemainingProjectedVars());
  double maximalBranchScore = -1;
  for (int i = 0; i < numberOfBranches; ++i) {
    double branchScore = 0;
    branchScore += literalScores[1 + 2 * i];
    for (std::size_t j = 1; j + 1 < literalScores.size(); j+=2){
      // For the last branch (projected branch) this if should always be true.
      if (j != 1 + 2 * i) {
        branchScore += literalScores[j+1];
      }
    }
    //branchesScores.push_back(branchScore);
    score += branchScore;
    maximalBranchScore = maximalBranchScore < 0 ? branchScore : std::max(maximalBranchScore, branchScore);
  }

  // return the avg score over the to be created branches divided by the standard deviation of the scores
  // double stddev = calculateStandardDeviation(std::vector<double>(branchesScores.begin(), branchesScores.end()));
  clauseScore.clauseCompareScore = maximalBranchScore;
  clauseScore.literalCompareScore = maximalBranchScore;
  // In case all branches have almost (or exactly) the same score, then ignore the stddev.
  return clauseScore;
}

ClauseScoringMethodLiteralMetric::ClauseScores ClauseScoringMethodLiteralMax::computeClauseScore(const ClauseType &clause) {
  double remainingDecidableVars = static_cast<double>(clause.hasRemainingProjectedVars());
  double nonProjectedBranch = static_cast<double>(0 < clause.getNbNonProjectedVars());
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

  return clauseScore;
}

ClauseScoringMethodLiteralMetric::ClauseScores ClauseScoringMethodLiteralSum::computeClauseScore(const ClauseType &clause) {
  double remainingDecidableVars = static_cast<double>(clause.hasRemainingProjectedVars());
  double nonProjectedBranch = static_cast<double>(0 < clause.getNbNonProjectedVars());
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

  return clauseScore;
}

ClauseScoringMethodLiteralMetric::ClauseScores ClauseScoringMethodLiteralSumOverBranchAvg::computeClauseScore(const ClauseType &clause) {
  double remainingDecidableVars = static_cast<double>(clause.hasRemainingProjectedVars());
  double nonProjectedBranch = static_cast<double>(0 < clause.getNbNonProjectedVars());
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
  assert((literalScores.size()-1)/2 == clause.hasRemainingProjectedVars());
  for (std::size_t i = 1; i + 1 < literalScores.size(); i += 2)
    score += literalScores[i + 1] * numberOfBranches + literalScores[i];

  clauseScore.clauseCompareScore = score / numberOfBranches;
  clauseScore.literalCompareScore = score / numberOfBranches;

  // return the avg score over the to be created branches
  return clauseScore;
}

/**
 * TODO: The vector should be branches scoring not literals scoring
 * TODO: check if upper and lower quartile are correct and then use them somehow
 */
ClauseScoringMethodLiteralMetric::ClauseScores ClauseScoringMethodBranchesAvgDividedByDistribution::computeClauseScore(const ClauseType &clause) {
  double remainingDecidableVars = static_cast<double>(clause.hasRemainingProjectedVars());
  double nonProjectedBranch = static_cast<double>(0 < clause.getNbNonProjectedVars());
  double numberOfBranches = remainingDecidableVars + nonProjectedBranch;
  ClauseScores clauseScore;
  // if their is only the projected variables branch.
  if (numberOfBranches < 2)
    return clauseScore;

  std::vector<double> literalScores(1, 0.0);
  std::vector<double> projectedLiteralScores(1, 0.0);
  std::vector<double> branchesScores;

  for (const auto &lit : clause.getLiterals()) {
    if (csm_specManager.litIsAssigned(lit))
      continue;
    if (csm_specManager.nbSelected() && csm_specManager.isProj() && !csm_specManager.isSelected(lit.var())) {
      projectedLiteralScores.push_back(csm_scoringMethod.computeScore(lit.var()));
      projectedLiteralScores.push_back(csm_scoringMethod.computeScore((~lit).var()));
    }
    literalScores.push_back(csm_scoringMethod.computeScore(lit.var()));
    literalScores.push_back(csm_scoringMethod.computeScore((~lit).var()));
  }

  double score = 0;
  assert((literalScores.size()-1)/2 == clause.hasRemainingProjectedVars());
  for (int i = 0; i < numberOfBranches; ++i) {
    double branchScore = 0;
    branchScore += literalScores[1 + 2 * i];
    for (std::size_t j = 1; j + 1 < literalScores.size(); j+=2){
      // For the last branch (projected branch) this if should always be true.
      if (j != 1 + 2 * i) {
        branchScore += literalScores[j+1];
      }
    }
    branchesScores.push_back(branchScore);
    score += branchScore;
  }

  // return the avg score over the to be created branches divided by the standard deviation of the scores
  double stddev = calculateStandardDeviation(std::vector<double>(branchesScores.begin(), branchesScores.end()));
  assert(stddev >= 0);
  clauseScore.clauseCompareScore = stddev > 1 ? score / (numberOfBranches * stddev) : score / numberOfBranches;
  clauseScore.literalCompareScore = stddev > 1 ? score / (numberOfBranches * stddev) : score / numberOfBranches;;
  // In case all branches have almost (or exactly) the same score, then ignore the stddev.
  return clauseScore;
}

} // namespace d4


