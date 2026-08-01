/*
 * d4
 * Copyright (C) 2020  Univ. Artois & CNRS
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#pragma once

#include <cassert>
#include <memory>

#include "../SpecManager.hpp"
#include "DataOccurrence.hpp"
#include "src/problem/cnf/ClauseType.hpp"
#include "src/problem/cnf/ProblemManagerAll.hpp"

namespace d4 {

/** TODO: Mohammad, just use the struct in SpecManagerCnf?
 */
struct InfoClusterAll {
  Var parent;
  unsigned size;
  int pos;
};

/**
   Spec manager for mixed CNF and alternative clauses.

   The class keeps the same component-discovery workflow as the CNF spec
   manager, but it stores clause objects and clause-local state through
   ClauseType so both clause kinds can be handled uniformly.
 */
class SpecManagerAll : public SpecManager {

	/** TODO: Mohammad,: missing functions
		freeVars
		all getVecIdx...

	*/
private:
	/**
    TODO: Mohammad, Depending on what happen to the returned clause literals, this function return type is incorrect.
		Because it return the literals without the Clause Type. For now leave in private until fix or choice.
  */
  inline const std::vector<Lit> &getClause(int idx) const {
    assert((unsigned)idx < m_clauses.size());
    return m_clauses[idx]->getLiterals();
  }
	
protected:
	// The stored clauses of different ClauseTypes.
  std::vector<std::unique_ptr<ClauseType>> m_clauses;
	// Indices of clauses that are not binary clauses. Index for the attribute m_clauses.
  std::vector<int> m_clausesNotBinary;
	// The size of the largest clause in the mixed formula.
  unsigned m_maxSizeClause;
	// current partial assignment
  std::vector<lbool> m_currentAssignment;
	// Temporary marks that say whether a variable is in the passed connected component. Index start from 1 to m_nbVar. Index 0 is not used.
  std::vector<bool> m_inCurrentComponent;
	// The occurrence data for each literal, contain the indices of clauses that contain the literal. Index start from 1 to (m_nbVar + 1) << 1. With positive literal at index 2 * v and negative literal at index 2 * v + 1 for variable v. Index 0 is not used.
  std::vector<DataOccurrence> m_occurrence;
	// The memory for the occurrence data.
  int *m_dataOccurrenceMemory;

	// Temporary attributes

	// Temporary data for the connected component discovery following the principle of union-find.
  std::vector<InfoClusterAll> m_infoCluster;
	// Temporary vector for the connected component discovery, that save the indices of clauses have been marked in m_mustUnMark. Used for resetting the view.
  std::vector<int> m_mustUnMark;
	// Temporary vector for the connected component discovery, that mark whether a clause have been reviewed.
  std::vector<bool> m_markView;
	// Temporary vector that mark clauses that need a new watcher literal.
  std::vector<int> m_reviewWatcher;

  /**
     Reset the temporary marks used while building a connected component.
  */
  inline void resetUnMark() {
    for (auto &idx : m_mustUnMark)
      m_markView[idx] = false;
    m_mustUnMark.resize(0);
  }

public:
  /**
     Build a mixed spec manager from the current mixed problem.
  */
  SpecManagerAll(ProblemManager &p);

  /**
     Destroy the mixed spec manager and release the occurrence memory.
  */
  ~SpecManagerAll() override;

	/**
	 * Build the connected components using union-find.
	 * 
	 * @param[out] varConnected the connected components to be built.
	 * @param[in] setOfVar the variables to be considered for the connected components.
	 * @param[out] freeVar the variables that are not in the current clauses.
	 */
  int computeConnectedComponent(std::vector<std::vector<Var>> &varConnected,
                                std::vector<Var> &setOfVar,
                                std::vector<Var> &freeVar) override;

	/**
	 * Build the connected components using union-find.
	 * 
	 * @param[out] varConnected the connected components to be built.
	 * @param[in] setOfVar the variables to be considered for the connected components.
	 * @param[out] freeVar the variables that are not in the current clauses.
	 */
  int computeConnectedComponent(std::vector<ProjVars> &varConnected,
                                std::vector<Var> &setOfVar,
                                std::vector<Var> &freeVar) override;

	/**
	 * Consider the passed literals as assigned and update the occurrence data and watchers if conditions are met.
	 *  TODO: The implementation can be simplified. (A lot of duplicated code)
	 * @param[in] lits the new assignments
	 */
  void preUpdate(std::vector<Lit> &lits) override;
	/**
	 * Consider the passed literals as assigned and update the occurrence data and watchers if conditions are met.
	 * 
	 * @param[in] lits the new assignments
	 * @param[in] pure the pure literals
	 */
  void preUpdate(std::vector<Lit> &lits, std::vector<Lit> &pure) override;

	/**
	 * Consider the passed literals as unassigned and update the occurrence data by reverting the updates done in PreUpdate in reverse order.
	 * 
	 * @param[in] lits the literals to be unassigned
	 */
  void postUpdate(std::vector<Lit> &lits) override;

  bool litIsAssigned(Lit l) override;
  bool litIsAssignedToTrue(Lit l) override;
  bool varIsAssigned(Var v) override;
  int getNbOccurrence(Lit l) override;

  /**
     Mixed spec managers use the dynamic update flow and do not support the
     static initialization path used by other spec variants.
  */
  inline void initialize(std::vector<Var> &setOfVar,
                         std::vector<Lit> &units) override {
    assert(0);
  }

  void showFormula(std::ostream &out) override;
  void showCurrentFormula(std::ostream &out) override;
  void showTrail(std::ostream &out) override;
  int getNbVariable() override { return m_nbVar; }

  /**
     Collect all clause indices from the current connected component that are
     still unsatisfied.

		 @param[out] idxClauses the indices of clauses that are still unsatisfied.
		 @param[in] component the variables in the current connected component.
  */
  void getCurrentClauses(std::vector<unsigned> &idxClauses,
                         std::vector<Var> &component);

  /**
     Collect only the non-binary clause indices from the current connected
     component that are still unsatisfied.

		 @param[out] idxClauses the indices of clauses that are still unsatisfied.
		 @param[in] component the variables in the current connected component.
  */
  void getCurrentClausesNotBin(std::vector<unsigned> &idxClauses,
                               std::vector<Var> &component);

  /**
     Return the number of clauses in the mixed formula.
  */
  inline unsigned getNbClause() const { return m_clauses.size(); }

  /**
     Return the size of the largest clause in the mixed formula.
  */
  inline int getMaxSizeClause() const { return m_maxSizeClause; }

  /**
     Return the sum of clause sizes in the mixed formula.
  */
  inline int getSumSizeClauses() const {
    int sum = 0;
    for (const auto &clause : m_clauses)
      sum += clause->size();
    return sum;
  }


	/**
	 * Return the original clause size for the given index
	 */
	inline int getInitSize(int idx) const {
		assert((unsigned)idx < m_clauses.size());
		return m_clauses[idx]->size();
	}

  /**
     Return the current literal count for a clause after removal of negatively assigned literals.
  */
  inline int getCurrentSize(int idx) const {
    return (int)m_clauses[idx]->size() - (int)m_clauses[idx]->getNbUnsatLit();
  }

	/**
	 * Return the number of current binary clauses that contain the given variable.
	 */
	inline int getNbBinaryClause(Var v) {
		return getNbBinaryClause(Lit::makeLitFalse(v)) + getNbBinaryClause(Lit::makeLitTrue(v));
	}

	/**
	 * Return the number of currentbinary clauses that contain the given literal.
	 */
	inline int getNbBinaryClause(Lit l) {
		int nbBin = m_occurrence[l.intern()].nbBin;
		for (unsigned i = 0; i < m_occurrence[l.intern()].nbNotBin; i++) {
			int idxCl = m_occurrence[l.intern()].notBin[i];
			if (getCurrentSize(idxCl) == 2)
				nbBin++;
	}
	return nbBin;
}

	// Return the number of current non-binary clauses that contain the given literal.
	inline int getNbNotBinaryClause(Lit l) {
		return getNbClause(l) - getNbBinaryClause(l);
	}

	// return the number of current non-binary clauses that contain the given variable.
	inline int getNbNotBinaryClause(Var v) {
		return getNbClause(v) - getNbBinaryClause(v);
	}

	// Return the number of current clauses that contain the given variable.
	inline int getNbClause(Var v) {
		return getNbClause(Lit::makeLitFalse(v)) + getNbClause(Lit::makeLitTrue(v));
	}

	// Return the number of current clauses that contain the given literal.
	inline unsigned getNbClause(Lit l) {
		return m_occurrence[l.intern()].nbBin + m_occurrence[l.intern()].nbNotBin;
	}

  /**
     Return the clause satisfaction status under the current assignment.
  */
  inline bool isSatisfiedClause(unsigned idx) {
    assert(idx < m_clauses.size());
    return m_clauses[idx]->isSatisfied();
  }

	/**
   Test if a passed clause is satisfied under the current
   interpretation only if it is of a known ClauseKind. Otherwise, return false.

   @param[in] c the clause to be tested for satisfaction.

   \return true if the clause of a known ClauseKind and is satisfied.
*/
	inline bool isSatisfiedClause(std::vector<Lit> &c, ClauseKind kind) {
		if (kind == ClauseKind::Cnf)
			return std::make_unique<CNFClause>(c)->isSatisfied(m_currentAssignment);
		else if (kind == ClauseKind::Alternative)
			return std::make_unique<AlternativeClause>(c)->isSatisfied(m_currentAssignment);
		return false;
	}

  /**
     Return true when a clause is unsatisfied and its watcher still belongs to
     the current connected component.
  */
  inline bool isNotSatisfiedClauseAndInComponent(
      int idx, std::vector<bool> &inCurrentComponent) {
    if (isSatisfiedClause((unsigned)idx))
      return false;
    assert(m_clauses[idx]->getWatcher() != lit_Undef);
    assert(!litIsAssigned(m_clauses[idx]->getWatcher()));
    return inCurrentComponent[m_clauses[idx]->getWatcher().var()];
  }

};
} // namespace d4
