#pragma once

#include <limits>

#include <vector>

#include <src/heuristics/ClauseScoringMethod.hpp>

namespace d4 {

class ClauseScoringMethodLiteralMetric : public ClauseScoringMethod {
protected:
  struct ClauseScores {
    double clauseCompareScore = -1;
    double literalCompareScore = -1;
    int nbBranches = 0;
  };

  ClauseScoringMethodLiteralMetric(SpecManagerAll &specManager,
                                   ScoringMethod &scoringMethod)
      : ClauseScoringMethod(specManager, scoringMethod) {}

  /**
   * @brief Return false if the literal is assigned or if the literal is not a
   * projected variable and we have projected case set to true.
   */
  bool skipLiteralScoring(const Lit &lit) const;

public:
  /**
   * @brief Computes the score of a clause.
   * @param[in] clause the clause to score.
   * @return the score of the clause.
   */
  virtual double computeScore(const ClauseType &clause) override;
  /**
   * @brief Computes the score of a clause.
   * @param[in] clause the clause to score.
   * @return ClauseScores struct.
   */
  virtual ClauseScores computeClauseScore(const ClauseType &clause) = 0;
  double getBestScoreClause(std::vector<Var> &connected,
                            std::vector<Lit> &clause) override;
};

class ClauseScoringMethodLiteralSum : public ClauseScoringMethodLiteralMetric {
public:
  ClauseScoringMethodLiteralSum(SpecManagerAll &specManager,
                                ScoringMethod &scoringMethod)
      : ClauseScoringMethodLiteralMetric(specManager, scoringMethod) {}
  /**
   * @brief Computes the score of a clause by summing over the scores of all
   * branches (Sum over (literal_negative_score * numberOfBranches +
   * literal_positive_score) for all literals).
   */
  ClauseScores computeClauseScore(const ClauseType &clause) override;
};

class ClauseScoringMethodLiteralSumOverBranchAvg
    : public ClauseScoringMethodLiteralMetric {
public:
  ClauseScoringMethodLiteralSumOverBranchAvg(SpecManagerAll &specManager,
                                             ScoringMethod &scoringMethod)
      : ClauseScoringMethodLiteralMetric(specManager, scoringMethod) {}
  /**
   * @brief Computes the score of a clause by summing over the scores of all
   * branches and dividing by the number of branches.
   */
  ClauseScores computeClauseScore(const ClauseType &clause) override;
};
class ClauseScoringMethodLiteralMin : public ClauseScoringMethodLiteralMetric {
public:
  ClauseScoringMethodLiteralMin(SpecManagerAll &specManager,
                                ScoringMethod &scoringMethod)
      : ClauseScoringMethodLiteralMetric(specManager, scoringMethod) {}
  /**
   * @brief Computes the score of a clause by taking the minimum of the scores
   * of all literals.
   */
  ClauseScores computeClauseScore(const ClauseType &clause) override;
};
class ClauseScoringMethodBranchMin : public ClauseScoringMethodLiteralMetric {
public:
  ClauseScoringMethodBranchMin(SpecManagerAll &specManager,
                               ScoringMethod &scoringMethod)
      : ClauseScoringMethodLiteralMetric(specManager, scoringMethod) {}
  /**
   * @brief Computes the score of a clause by taking the minimum of the scores
   * of all branches.
   */
  ClauseScores computeClauseScore(const ClauseType &clause) override;
};
class ClauseScoringMethodLiteralMax : public ClauseScoringMethodLiteralMetric {
public:
  ClauseScoringMethodLiteralMax(SpecManagerAll &specManager,
                                ScoringMethod &scoringMethod)
      : ClauseScoringMethodLiteralMetric(specManager, scoringMethod) {}
  /**
   * @brief Computes the score of a clause by taking the maximum of the scores
   * of all literals.
   */
  ClauseScores computeClauseScore(const ClauseType &clause) override;
};
class ClauseScoringMethodBranchMax : public ClauseScoringMethodLiteralMetric {
public:
  ClauseScoringMethodBranchMax(SpecManagerAll &specManager,
                               ScoringMethod &scoringMethod)
      : ClauseScoringMethodLiteralMetric(specManager, scoringMethod) {}
  /**
   * @brief Computes the score of a clause by taking the maximum of the scores
   * of all branches.
   */
  ClauseScores computeClauseScore(const ClauseType &clause) override;
};
} // namespace d4