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

#include "ProblemManagerAll.hpp"

#include <cassert>

#include "src/problem/ProblemManager.hpp"

namespace d4 {

void ProblemManagerAll::appendClauses(
    const std::vector<std::vector<Lit>> &rawClauses, ClauseKind kind) {
  for (const auto &clause : rawClauses) {
    if (kind == ClauseKind::Cnf) {
      m_clauses.push_back(std::make_unique<CNFClause>(clause));
    } else {
      m_clauses.push_back(std::make_unique<AlternativeClause>(clause));
    }
  }
}

void ProblemManagerAll::cloneClauses(
    const std::vector<std::unique_ptr<ClauseType>> &clauses,
    std::vector<std::unique_ptr<ClauseType>> &target) {
  for (const auto &clause : clauses)
    target.push_back(clause->clone());
}

std::vector<Var> ProblemManagerAll::buildRemap() const {
  std::vector<bool> marked(m_nbVar + 1);
  std::vector<Var> remap(m_nbVar + 1);

  int idx = 1;
  // Mark projected variables to reindex them to the front of the variable list.
  for (Var v : m_selected)
    marked[v] = true;

  // Give the projected variables an index using idx
  for (Var v = 1; v <= m_nbVar; v++) {
    if (marked[v]) {
      remap[v] = idx;
      idx++;
    }
  }
  // With the projected variables indexed from 1 to n, where n is the number of projected variables, we can now index the remaining variables.
  for (Var v = 1; v <= m_nbVar; v++) {
    if (!marked[v]) {
      remap[v] = idx;
      idx++;
    }
  }

  return remap;
}

void ProblemManagerAll::remapClauses(
    std::vector<std::unique_ptr<ClauseType>> &clauses,
    const std::vector<Var> &remap) {
  for (auto &clause : clauses)
    clause->remap(remap);
}


ProblemManagerAll::ProblemManagerAll() { m_nbVar = 0; }

ProblemManagerAll::ProblemManagerAll(std::string &nameFile,
                                     std::string &alternativeFile) {
  ParserDimacs parser;
  std::vector<std::vector<Lit>> cnfClauses;
  std::vector<std::vector<Lit>> altClauses;

  m_nbVar = parser.parse_DIMACS(nameFile, cnfClauses, m_weightLit, m_selected,
                                m_maxVar);
  parser.parse_alternative(alternativeFile, m_nbVar, altClauses);

  m_weightVar.resize(m_nbVar + 1, 0);
  for (unsigned i = 0; i <= m_nbVar; i++)
    m_weightVar[i] = m_weightLit[i << 1] + m_weightLit[(i << 1) + 1];

  appendClauses(cnfClauses, ClauseKind::Cnf);
  appendClauses(altClauses, ClauseKind::Alternative);
  normalize();
} // constructor

ProblemManagerAll::ProblemManagerAll(ProblemManager *problem) {
  m_nbVar = problem->getNbVar();
  m_weightLit = problem->getWeightLit();
  m_weightVar = problem->getWeightVar();
  m_selected = problem->getSelectedVar();
  m_maxVar = problem->getMaxVar();
  m_indVar = problem->getIndVar();
  m_isUnsat = problem->isUnsat();
  m_nbFreeVars = problem->freeVars();
  m_gmap = problem->gmap();

  if (auto *mixedProblem = dynamic_cast<ProblemManagerAll *>(problem)) {
    cloneClauses(mixedProblem->m_clauses, m_clauses);
    cloneClauses(mixedProblem->m_learnt, m_learnt);
  }
} // constructor

ProblemManagerAll *ProblemManagerAll::createFromProblemWithNoClauses(
    ProblemManager *problem) {
  auto *ret = new ProblemManagerAll();
  ret->m_nbVar = problem->getNbVar();
  ret->m_weightLit = problem->getWeightLit();
  ret->m_weightVar = problem->getWeightVar();
  ret->m_selected = problem->getSelectedVar();
  ret->m_maxVar = problem->getMaxVar();
  ret->m_indVar = problem->getIndVar();
  ret->m_isUnsat = problem->isUnsat();
  ret->m_nbFreeVars = problem->freeVars();
  ret->m_gmap = problem->gmap();

  return ret;
} // constructor

ProblemManagerAll::ProblemManagerAll(int nbVar, std::vector<double> &weightLit,
                                     std::vector<double> &weightVar,
                                     std::vector<Var> &selected,
                                     int freevars) {
  m_nbVar = nbVar;
  m_weightLit = weightLit;
  m_weightVar = weightVar;
  m_selected = selected;
  m_isUnsat = false;
  m_nbFreeVars = freevars;
} // constructor

void ProblemManagerAll::normalize() {
  if (m_selected.size() != m_nbVar) {
    std::vector<Var> remap = buildRemap();
    remapClauses(m_clauses, remap);

    // Reinitialized m_selected to the new [1, 2, ..., n] indicies, where n is the number of projected variables.
    int sel = m_selected.size();
    m_selected.clear();
    for (int i = 1; i <= sel; i++)
      m_selected.push_back(i);
  }
} // normalize

void ProblemManagerAll::normalizeInner() {
  for (auto &clause : m_clauses)
    clause->normalizeInner();
}

ProblemManager *ProblemManagerAll::getUnsatProblem() {
  ProblemManagerAll *ret = new ProblemManagerAll(this);
  ret->m_isUnsat = true;

  Lit l = Lit::makeLit(1, false);
  ret->m_clauses.push_back(
      std::make_unique<CNFClause>(std::vector<Lit>{l}));
  ret->m_clauses.push_back(
      std::make_unique<CNFClause>(std::vector<Lit>{l.neg()}));

  return ret;
} // getUnsatProblem

ProblemManager *ProblemManagerAll::getConditionedFormula(
    std::vector<Lit> &units) {
  ProblemManagerAll *ret = createFromProblemWithNoClauses(this);

  std::vector<lbool> value(m_nbVar + 1, l_Undef);
  for (auto l : units) {
    value[l.var()] = l.sign() ? l_False : l_True;
    ret->m_clauses.push_back(
        std::make_unique<CNFClause>(std::vector<Lit>{l}));
  }

  for (const auto &clause : m_clauses) {
    /*TODO Mohammad: alternatives need to be kept in the problem even after they are satisfied. 
     Or we return satisfied only if we set exactly one literal to true and all other to false
     Already done that but is that the correct approach?
     */
    if (!clause->isSatisfied(value))
      ret->m_clauses.push_back(clause->clone());
  }

  return ret;
} // getConditionedFormula

void ProblemManagerAll::display(std::ostream &out) {
  out << "weight list: ";
  for (unsigned i = 1; i <= m_nbVar; i++) {
    Lit l = Lit::makeLit(i, false);
    out << i << "[" << m_weightVar[i] << "] ";
    out << l << "(" << m_weightLit[l.intern()] << ") ";
    out << ~l << "(" << m_weightLit[(~l).intern()] << ") ";
  }
  out << "\n";

  out << "p cnf " << m_nbVar << " " << m_clauses.size() << "\n";
  for (const auto &clause : m_clauses) {
    out << "c ";
    out << (clause->kind() == ClauseKind::Cnf ? "cnf" : "alt") << " ";
    for (const auto &lit : clause->getLiterals())
      out << lit << " ";
    out << "0\n";
  }
} // display

void ProblemManagerAll::displayStat(std::ostream &out, std::string startLine) {
  unsigned nbLits = 0;
  unsigned nbBin = 0;
  unsigned nbTer = 0;
  unsigned nbMoreThree = 0;
  unsigned nbCnf = 0;
  unsigned nbAlt = 0;

  for (const auto &clause : m_clauses) {
    const std::size_t clauseSize = clause->size();
    nbLits += clauseSize;
    nbCnf += clause->kind() == ClauseKind::Cnf;
    nbAlt += clause->kind() == ClauseKind::Alternative;
    if (clauseSize == 2)
      nbBin++;
    if (clauseSize == 3)
      nbTer++;
    if (clauseSize > 3)
      nbMoreThree++;
  }

  out << startLine << "Number of selected: " << m_selected.size() << "\n";
  out << startLine << "Number of variables: " << m_nbVar << "\n";
  out << startLine << "Number of clauses: " << m_clauses.size() << "\n";
  out << startLine << "Number of CNF clauses: " << nbCnf << "\n";
  out << startLine << "Number of alternative clauses: " << nbAlt << "\n";
  out << startLine << "Number of binary clauses: " << nbBin << "\n";
  out << startLine << "Number of ternary clauses: " << nbTer << "\n";
  out << startLine << "Number of clauses larger than 3: " << nbMoreThree
      << "\n";
  out << startLine << "Number of literals: " << nbLits << "\n";
} // displayStat

} // namespace d4