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

   D. Pretolani. Efficiency and stability of hypergraph sat
   algorithms. In D. S.  Johnson and M. A. Trick, editors, Second
   DIMACS Implementation Challenge.  American Mathematical Society,
   1993.

   @param[in] v, the variable we want the score.
*/
double ScoringMethodMomAll::computeScore(Var v) {
  return om.getNbBinaryClause(v) * 0.25;
} // computeScore

} // namespace d4
