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

#include <algorithm>
#include <cstddef>
#include <memory>
#include <ostream>
#include <vector>

#include "src/problem/ProblemTypes.hpp"

namespace d4 {

/**
   Kind of clause stored in the mixed problem manager.
 */
enum class ClauseKind { Cnf, Alternative };

/**
   Shared clause abstraction.

   The class stores the literal vector and the small amount of clause-local
   state.
 */
class ClauseType {
protected:
  /**
   * TODO: add an unsigned int variable for non projected count of variable in
   * this clause and a function that return if the clause have any more
   * selectable variables. Not related to here but in SpecManagerAll if we add a
   * list of sorted not yet satisfied clauses, then we need to update them in
   * per/postupdate. But considering the first solution here I might wait
   * because maybe I can create scoring classes instead of arrays of clauses
   * that are sorted.
   * TODO: add functions that return how many positive
   * , negative assignments need be set still until fulfillment then update
   * SpecManager
   */
  // the literals stored in the clause.
  std::vector<Lit> m_literals;
  // the number of satisfied literals in the clause.
  unsigned m_nbSatLit;
  // the number of unsatisfied literals in the clause.
  unsigned m_nbUnsatLit;
  // the number of non-projected variables in the clause. This variable is not
  // set in the constructor
  unsigned m_nbNonProjectedVars;
  // the number of decided non-projected variables in the clause. (decided pure
  // literals)
  unsigned m_nbDecidedNonProjectedVars;
  // the clause watcher literal.
  Lit m_watcher;

  explicit ClauseType(std::vector<Lit> literals)
      : m_literals(std::move(literals)), m_nbSatLit(0), m_nbUnsatLit(0),
        m_nbDecidedNonProjectedVars(0), m_watcher(lit_Undef) {}

public:
  virtual ~ClauseType() = default;

  /**
     \return the enum kind of clause.
  */
  virtual ClauseKind kind() const = 0;

  /**
     Clone the clause object.
  */
  virtual std::unique_ptr<ClauseType> clone() const = 0;

  /**
     Write the cnf encoding to cnf_clauses and the auxiliary (new helpful
     variables) to created_vars. The next available variable id is passed in
     next_free_variable_number and updated to the next available variable id
     number after the encoding.

     @param[out] cnf_clauses the CNF encoding of the clause.
     @param[out] created_vars the auxiliary variables created by the encoding.
     @param[in,out] next_free_variable_number the next available variable id
     number.
  */
  virtual void
  populate_as_cnf_clause(std::vector<std::vector<Lit>> &cnf_clauses,
                         std::vector<Var> &created_vars,
                         unsigned &next_free_variable_number) const = 0;

  // TODO: Mohammad,: For alternative it is not clear if we return true when not
  // all other literals are false.
  /**
     Test whether the clause is satisfied by the passed partial assignment.

      @param[in] currentValue the current variable assignment.
      \return true if the clause is satisfied, false otherwise.
  */
  virtual bool isSatisfied(const std::vector<lbool> &currentValue) const = 0;

  /**
    Test whether the clause is satisfied by the passed partial assignment.

     \return true if the clause is satisfied, false otherwise.
 */
  virtual bool isSatisfied() const = 0;

  /**
   * \return if the clause has any remaining undecided projected variables.
   */
  bool hasRemainingProjectedVars() const {
    return m_nbSatLit + m_nbUnsatLit + m_nbNonProjectedVars -
               m_nbDecidedNonProjectedVars <
           m_literals.size();
  }

  /**
   * \return if the class still need to be satisfied and it have any remaining
   * undecided projected variables.
   */
  bool isStillRelevant() const {
    return !this->isSatisfied() && hasRemainingProjectedVars();
  }

  /**
     \return the stored literals.
  */
  inline const std::vector<Lit> &getLiterals() const { return m_literals; }

  /**
     \return the stored literals, mutable for local normalization.
  */
  inline std::vector<Lit> &getLiterals() { return m_literals; }

  /**
     \return the number of literals stored in the clause.
  */
  inline std::size_t size() const { return m_literals.size(); }

  /**
     Remap the clause literals according to a new variable numbering.

      @param[in] remap the new variable numbering/indexing.
  */
  inline void remap(const std::vector<Var> &remap) {
    for (auto &lit : m_literals)
      lit = Lit::makeLit(remap[lit.var()], lit.sign());
  }

  /**
     Sort clause literals by variable index.
  */
  inline void normalizeInner() {
    std::sort(m_literals.begin(), m_literals.end(),
              [](Lit a, Lit b) { return a.var() < b.var(); });
  }

  /**
     \return the clause watcher.
  */
  inline Lit getWatcher() const { return m_watcher; }

  /**
     Update the clause watcher.
  */
  inline void setWatcher(Lit watcher) { m_watcher = watcher; }

  /**
     \return the number of satisfied literals tracked for this clause.
  */
  inline unsigned getNbSatLit() const { return m_nbSatLit; }

  /**
     \return the number of unsatisfied literals tracked for this clause.
  */
  inline unsigned getNbUnsatLit() const { return m_nbUnsatLit; }

  /**
     Increase the number of satisfied literals tracked for this clause.
  */
  inline void incNbSatLit() { ++m_nbSatLit; }

  /**
     Increase the number of unsatisfied literals tracked for this clause.
  */
  inline void incNbUnsatLit() { ++m_nbUnsatLit; }

  /**
     Decrease the number of satisfied literals tracked for this clause.
  */
  inline void decNbSatLit() { --m_nbSatLit; }

  /**
     Decrease the number of unsatisfied literals tracked for this clause.
  */
  inline void decNbUnsatLit() { --m_nbUnsatLit; }

  /**
     Increase the number of decided non-projected variables in the clause.
   */
  inline void incNbDecidedNonProjectedVars() { ++m_nbDecidedNonProjectedVars; }

  /**
     Decrease the number of decided non-projected variables in the clause.
   */
  inline void decNbDecidedNonProjectedVars() { --m_nbDecidedNonProjectedVars; }

  /**
     Set the number of non-projected variables in the clause.
   */
  inline void setNbNonProjectedVars(unsigned nb) { m_nbNonProjectedVars = nb; }

  /**
     \return the number of non-projected variables in the clause.
   */
  inline unsigned getNbNonProjectedVars() const { return m_nbNonProjectedVars; }
};

/**
   Standard CNF clause.
 */
class CNFClause final : public ClauseType {
public:
  /**
     Build a CNF clause from a literal list.
  */
  explicit CNFClause(std::vector<Lit> literals)
      : ClauseType(std::move(literals)) {}

  ClauseKind kind() const override { return ClauseKind::Cnf; }

  std::unique_ptr<ClauseType> clone() const override {
    return std::make_unique<CNFClause>(*this);
  }

  void
  populate_as_cnf_clause(std::vector<std::vector<Lit>> &cnf_clauses,
                         std::vector<Var> &created_vars,
                         unsigned &next_free_variable_number) const override {
    (void)created_vars;
    (void)next_free_variable_number;
    cnf_clauses.push_back(m_literals);
  }

  /**
     \return true if at least one literal in the clause is satisfied by the
     passed assignment.
  */
  bool isSatisfied(const std::vector<lbool> &currentValue) const override {
    for (const auto &lit : m_literals) {
      if (currentValue[lit.var()] == l_Undef)
        continue;
      if ((!lit.sign() && currentValue[lit.var()] == l_True) ||
          (lit.sign() && currentValue[lit.var()] == l_False))
        return true;
    }
    return false;
  }

  /**
    \return true if at least one literal in the clause is satisfied.
 */
  bool isSatisfied() const override { return m_nbSatLit > 0; }
};

/**
   Alternative clause with exact-one semantics.
 */
class AlternativeClause final : public ClauseType {
public:
  /**
     Build an exact-one clause from a literal list.
  */
  explicit AlternativeClause(std::vector<Lit> literals)
      : ClauseType(std::move(literals)) {}

  ClauseKind kind() const override { return ClauseKind::Alternative; }

  std::unique_ptr<ClauseType> clone() const override {
    return std::make_unique<AlternativeClause>(*this);
  }

  void
  populate_as_cnf_clause(std::vector<std::vector<Lit>> &cnf_clauses,
                         std::vector<Var> &created_vars,
                         unsigned &next_free_variable_number) const override {
    if (m_literals.empty()) {
      cnf_clauses.emplace_back();
      return;
    }

    cnf_clauses.push_back(m_literals);

    if (m_literals.size() < 6) {
      for (unsigned i = 0; i < m_literals.size(); i++) {
        for (unsigned j = i + 1; j < m_literals.size(); j++)
          cnf_clauses.push_back(
              std::vector<Lit>{~m_literals[i], ~m_literals[j]});
      }
      return;
    }

    std::vector<Var> auxVars;
    auxVars.reserve(m_literals.size() - 1);
    for (unsigned i = 0; i + 1 < m_literals.size(); i++) {
      auxVars.push_back(next_free_variable_number);
      created_vars.push_back(next_free_variable_number);
      next_free_variable_number++;
    }

    cnf_clauses.push_back(
        std::vector<Lit>{~m_literals[0], Lit::makeLit(auxVars[0], false)});
    /** TODO: Mohammad, make sure again that everything here is correct */
    for (unsigned i = 1; i + 1 < m_literals.size(); i++) {
      Lit si = Lit::makeLit(auxVars[i], false);
      Lit sim1 = Lit::makeLit(auxVars[i - 1], false);

      cnf_clauses.push_back(std::vector<Lit>{~m_literals[i], si});
      cnf_clauses.push_back(std::vector<Lit>{~sim1, si});
      cnf_clauses.push_back(std::vector<Lit>{~m_literals[i], ~sim1});
    }

    cnf_clauses.push_back(std::vector<Lit>{
        ~m_literals.back(), ~Lit::makeLit(auxVars.back(), false)});
  }

  /**
     \return true if exactly one literal in the clause is satisfied by the
     passed assignment and all other literals are unsatisfied.
  */
  bool isSatisfied(const std::vector<lbool> &currentValue) const override {
    unsigned nbTrue = 0;
    unsigned nbFalse = 0;
    for (const auto &lit : m_literals) {
      if (currentValue[lit.var()] == l_Undef)
        continue;
      if ((!lit.sign() && currentValue[lit.var()] == l_True) ||
          (lit.sign() && currentValue[lit.var()] == l_False)) {
        ++nbTrue;
      } else {
        ++nbFalse;
      }
    }
    return nbTrue == 1 && nbFalse == m_literals.size() - 1;
  }

  /**
     \return true if exactly one literal in the clause is satisfied by the
     passed assignment and all other literals are unsatisfied.
  */
  bool isSatisfied() const override {
    return m_nbSatLit == 1 && m_nbUnsatLit == m_literals.size() - 1;
  }
};
} // namespace d4