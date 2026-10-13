#pragma once
#include <functional>
#include <src/heuristics/ScoringMethod.hpp>
#include <src/problem/ProblemTypes.hpp>
#include <src/problem/cnf/ClauseType.hpp>
#include <src/specs/cnf/SpecManagerAll.hpp>
#include <vector>

namespace d4 {
class ClauseScoringMethod {
protected:
  ScoringMethod &csm_scoringMethod;
  SpecManagerAll &csm_specManager;

public:
  ClauseScoringMethod(SpecManagerAll &specManager, ScoringMethod &scoringMethod)
      : csm_scoringMethod(scoringMethod), csm_specManager(specManager) {}
  static ClauseScoringMethod *
  makeClauseScoringMethod(Config &config, SpecManager &p,
                          ScoringMethod &scoringMethod, std::ostream &out);
  /**
   * Destructor for the ClauseScoringMethod.
   */
  virtual ~ClauseScoringMethod() { ; }
  /**
   * Computes the score of a clause.
   *
   * @param[in] clause the clause to score.
   * @return the score of the clause.
   */
  virtual double computeScore(const ClauseType &clause) = 0;
  /**
   * Gets the best score of a clause.
   *
   * @param[in] connected the current connected component.
   * @param[out] clause the clause with the best score.
   * @return the best score of the clause.
   */
  virtual double getBestScoreClause(std::vector<Var> &connected,
                                    std::vector<Lit> &clause) = 0;

  /**
   * Selects a clause through a scoring method and return true if a clause was
   * selected and pass the clause back.
   *
   * @param[in] connected the current connected component.
   * @param[out] clause the selected clause.
   * @param[in] ClauseIfAvailable if true, return true if a clause with higher
   * score than 0 was found, otherwise only return true if the found clause has
   * a higher score than the best literal.
   * @return true if a clause was selected, false otherwise.
   */
  bool selectClause(std::vector<Var> &connected, std::vector<Lit> &clause,
                    bool ClauseIfAvailable = false);
};
} // namespace d4
