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
enum class ClauseKind {
  Cnf,
  Alternative
};

/**
   Shared clause abstraction.

   The class stores the literal vector and the small amount of clause-local
   state.
 */
class ClauseType {
protected:
  // the literals stored in the clause.
  std::vector<Lit> m_literals;
  // the number of satisfied literals in the clause. 
  unsigned m_nbSatLit;
  // the number of unsatisfied literals in the clause.
  unsigned m_nbUnsatLit;
  // the clause watcher literal.
  Lit m_watcher;

  explicit ClauseType(std::vector<Lit> literals)
      : m_literals(std::move(literals)), m_nbSatLit(0), m_nbUnsatLit(0),
        m_watcher(lit_Undef) {}

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

  //TODO Mohammad: For alternative it is not clear if we return true when not all other literals are false.
  /**
     Test whether the clause is satisfied by the passed partial assignment.

      @param[in] currentValue the current variable assignment.
      \return true if the clause is satisfied, false otherwise.
  */
  virtual bool isSatisfied(const std::vector<lbool> &currentValue) const = 0;

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

  /**
     \return true if at least one literal in the clause is satisfied by the passed assignment.
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

  /**
     \return true if exactly one literal in the clause is satisfied by the passed assignment and all other literals are unsatisfied.
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
};
} // namespace d4