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

#include <memory>

#include "ClauseType.hpp"
#include "ParserDimacs.hpp"
#include "src/problem/ProblemManager.hpp"

namespace d4 {

/**
   Mixed problem manager for CNF clauses and exact-one alternative clauses. (With the capability to extend to other clause types in the future.)
   The problem is stored in this class but never manipulated. But it allow the creation of a new problem manager given an assignment.
 */
class ProblemManagerAll : public ProblemManager {
	//TODO Mohammad: should we add vector of vectors that contains indices of clauses for each kind of clause, so that we can easily access them.
protected:
  std::vector<std::unique_ptr<ClauseType>> m_clauses;
  std::vector<std::unique_ptr<ClauseType>> m_learnt;

  /**
     Build a variable remapping that moves selected/projected variables to the front.

      \return the remapped variable indices.
  */
  std::vector<Var> buildRemap() const;

  /**
     Convert raw literal clauses to clause objects.

      @param[in] rawClauses the raw literal clauses to be cloned.
      @param[in] kind the enum kind of clauses to be created.
  */
  void appendClauses(const std::vector<std::vector<Lit>> &rawClauses,
                     ClauseKind kind);

   /**
       Clone every clause object from a source container into a destination
       container.

         @param[in] clauses the source clause container.
         @param[out] target the destination clause container.
   */
   void cloneClauses(const std::vector<std::unique_ptr<ClauseType>> &clauses,
                              std::vector<std::unique_ptr<ClauseType>> &target);

  /**
     Remap every clause object in a clause container.

      @param[in] clauses the clause container to be remapped.
      @param[in] remap the variable remapping to be applied.
  */
  void remapClauses(std::vector<std::unique_ptr<ClauseType>> &clauses,
                    const std::vector<Var> &remap);

public:
  ProblemManagerAll();
  /**
     Construct a problem manager from a DIMACS file and an alternative clause file.

      @param[in] nameFile the DIMACS file name.
      @param[in] alternativeFile the alternative clause file name.
  */
  ProblemManagerAll(std::string &nameFile, std::string &alternativeFile);

   /**
      Construct a mixed problem manager from an existing problem description.

      If the source is already a mixed problem manager, its clause containers are
      deep-copied as well. Otherwise, the new mixed problem manager will have empty clause containers.
   */
  ProblemManagerAll(ProblemManager *problem);

  /**
     Construct a problem manager from an existing problem manager. 
     But the clauses are not cloned, so the new problem manager will have empty clauses.

      @param[in] problem the problem manager to be copied.
  */
  static ProblemManagerAll* createFromProblemWithNoClauses(ProblemManager *problem);

  /**
     Construct a problem manager that copies the passed variable metadata.

      @param[in] nbVar the number of variables.
      @param[in] weightLit the weights for literals.
      @param[in] weightVar the weights for variables.
      @param[in] selected the selected variables.
      @param[in] freevars the number of free variables.
  */
  ProblemManagerAll(int nbVar, std::vector<double> &weightLit,
                    std::vector<double> &weightVar, std::vector<Var> &selected,
                    int freevars = 0);
  ~ProblemManagerAll() override = default;

  /**
	 Normalize the problem by moving selected variables to the front.
  */
  void normalize() override;
  /**
	 Normalize the contents of each clause object by sorting the literals inside each clause.
  */
  void normalizeInner() override;
   /**
	 	Display the mixed problem in a DIMACS-like text form.
   */
  void display(std::ostream &out) override;
	/**
   Print mixed-problem statistics, including the two clause kinds.
 	*/
  void displayStat(std::ostream &out, std::string startLine) override;
  ProblemManager *getUnsatProblem() override;
  ProblemManager *getConditionedFormula(std::vector<Lit> &units) override;

  /**
     \return the clause container.
  */
  std::vector<std::unique_ptr<ClauseType>> &getClauses() { return m_clauses; }
  /**
     \return the learnt clause container.
  */
  std::vector<std::unique_ptr<ClauseType>> &getLearnt() { return m_learnt; }
};
} // namespace d4