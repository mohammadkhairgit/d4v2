#include "ScoringMethodMomAll.hpp"

namespace d4 {
/**
   Constructor.

   @param[in] o the specification of a mixed problem.
 */
ScoringMethodMomAll::ScoringMethodMomAll(SpecManagerAll &o)
    : om(o) {} // constructor

/**
   Compute the score following the well-known MOM heuristic.

   @param[in] v the variable we want the score.
*/
double ScoringMethodMomAll::computeScore(Var v) {
  return om.getNbBinaryClause(v) * 0.25;
} // computeScore

} // namespace d4
