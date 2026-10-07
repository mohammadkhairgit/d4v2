#pragma once

#include <limits>

#include <vector>

#include <src/heuristics/ClauseScoringMethod.hpp>

namespace d4 {

double calculateVariance(const std::vector<double> &values);
double calculateStandardDeviation(const std::vector<double> &values);
double calculateLowerQuartile(const std::vector<double> &values);
double calculateUpperQuartile(const std::vector<double> &values);

class ClauseScoringMethodLiteralMetric : public ClauseScoringMethod {
protected:
  struct ClauseScores {
    double clauseCompareScore = -1;
    double literalCompareScore = -1;
  };

  ClauseScoringMethodLiteralMetric(SpecManagerAll &specManager,
                                   ScoringMethod &scoringMethod)
      : ClauseScoringMethod(specManager, scoringMethod) {}

  bool skipLiteralScoring(const Lit &lit) const;

public:
  virtual double computeScore(const ClauseType &clause) override;
  virtual ClauseScores computeClauseScore(const ClauseType &clause) = 0;
  double getBestScoreClause(std::vector<Var> &connected,
                            std::vector<Lit> &clause) override;
};

class ClauseScoringMethodLiteralSum : public ClauseScoringMethodLiteralMetric {
public:
  ClauseScoringMethodLiteralSum(SpecManagerAll &specManager,
                                ScoringMethod &scoringMethod)
      : ClauseScoringMethodLiteralMetric(specManager, scoringMethod) {}
  ClauseScores computeClauseScore(const ClauseType &clause) override;
};

class ClauseScoringMethodLiteralSumOverBranchAvg
    : public ClauseScoringMethodLiteralMetric {
public:
  ClauseScoringMethodLiteralSumOverBranchAvg(SpecManagerAll &specManager,
                                             ScoringMethod &scoringMethod)
      : ClauseScoringMethodLiteralMetric(specManager, scoringMethod) {}
  ClauseScores computeClauseScore(const ClauseType &clause) override;
};
class ClauseScoringMethodLiteralMin : public ClauseScoringMethodLiteralMetric {
public:
  ClauseScoringMethodLiteralMin(SpecManagerAll &specManager,
                                ScoringMethod &scoringMethod)
      : ClauseScoringMethodLiteralMetric(specManager, scoringMethod) {}
  ClauseScores computeClauseScore(const ClauseType &clause) override;
};
class ClauseScoringMethodBranchMin : public ClauseScoringMethodLiteralMetric {
public:
  ClauseScoringMethodBranchMin(SpecManagerAll &specManager,
                               ScoringMethod &scoringMethod)
      : ClauseScoringMethodLiteralMetric(specManager, scoringMethod) {}
  ClauseScores computeClauseScore(const ClauseType &clause) override;
};
class ClauseScoringMethodLiteralMax : public ClauseScoringMethodLiteralMetric {
public:
  ClauseScoringMethodLiteralMax(SpecManagerAll &specManager,
                                ScoringMethod &scoringMethod)
      : ClauseScoringMethodLiteralMetric(specManager, scoringMethod) {}
  ClauseScores computeClauseScore(const ClauseType &clause) override;
};
class ClauseScoringMethodBranchMax : public ClauseScoringMethodLiteralMetric {
public:
  ClauseScoringMethodBranchMax(SpecManagerAll &specManager,
                               ScoringMethod &scoringMethod)
      : ClauseScoringMethodLiteralMetric(specManager, scoringMethod) {}
  ClauseScores computeClauseScore(const ClauseType &clause) override;
};
class ClauseScoringMethodBranchesAvgDividedByDistribution
    : public ClauseScoringMethodLiteralMetric {
public:
  ClauseScoringMethodBranchesAvgDividedByDistribution(
      SpecManagerAll &specManager, ScoringMethod &scoringMethod)
      : ClauseScoringMethodLiteralMetric(specManager, scoringMethod) {}
  ClauseScores computeClauseScore(const ClauseType &clause) override;
};
} // namespace d4