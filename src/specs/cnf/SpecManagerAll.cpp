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

#include "SpecManagerAll.hpp"

#include <iostream>
#include <typeinfo>
#include <unordered_set>

#include "src/problem/ProblemManager.hpp"

namespace d4 {

SpecManagerAll::SpecManagerAll(ProblemManager &p) {
  m_nbVar = p.getNbVar();
  m_nbProj = p.getNbSelectedVar();

  try {
    ProblemManagerAll &pmix = dynamic_cast<ProblemManagerAll &>(p);
    for (const auto &clause : pmix.getClauses())
      m_clauses.push_back(clause->clone());
  } catch (std::bad_cast &bc) {
    std::cerr << "bad_cast caught: " << bc.what() << '\n';
    std::cerr << "A mixed formula was expected\n";
  }

  m_maxSizeClause = 0;
  unsigned count = 0;
  // Temporary store the occurrence for each literal, contain the indices of clauses that contain the literal.
  std::vector<std::vector<int>> occurrence((m_nbVar + 1) << 1);
  // store the not binary clauses indices.
  for (unsigned i = 0; i < m_clauses.size(); i++) {
    if (m_clauses[i]->size() > 2)
      m_clausesNotBinary.push_back(i);

    /* Before filling the actual occurrence attributes m_dataOccurrenceMemory and m_occurrence, we first store the occurrence in a temporary vector of vectors
    , so that the management of the memory m_dataOccurrenceMemory is correctly done.
    */ 
    for (const auto &l : m_clauses[i]->getLiterals())
      occurrence[l.intern()].push_back(i);
    count += m_clauses[i]->size();

    if (m_clauses[i]->size() > m_maxSizeClause)
      m_maxSizeClause = m_clauses[i]->size();
  }
  
  m_occurrence.resize((m_nbVar + 1) << 1, {NULL, 0, NULL, 0});
  m_dataOccurrenceMemory = new int[count];

  // The pointer pointing to the current literal occurrence memory to be filled.
  int *ptr = m_dataOccurrenceMemory;
  // For each literal
  for (unsigned i = 0; i < occurrence.size(); i++) {
    // The occurrence list for the current literal.
    std::vector<int> &occList = occurrence[i];

    // The memory end where the first non-binary clause will be stored. 
    unsigned posNotBin = occList.size() - 1;
    for (auto const &idx : occList) {
      if (m_clauses[idx]->size() == 2)
        // Fill the next available memory for the binary clause occurrence and move forward.
        ptr[m_occurrence[i].nbBin++] = idx;
      else
      // Fill the current end available memory for the non-binary clause occurrence, and move backward.
        ptr[posNotBin--] = idx;
    }

    // Set the correct memory pointers and sizes
    m_occurrence[i].bin = ptr;
    m_occurrence[i].notBin = &ptr[posNotBin + 1];
    m_occurrence[i].nbNotBin = occList.size() - m_occurrence[i].nbBin;
    // Move to the next available memory for the next literal occurrence.
    ptr = &ptr[occList.size()];
  }

  m_inCurrentComponent.resize(m_nbVar + 1, false);
  m_currentAssignment.resize(m_nbVar + 1, l_Undef);
  // Not really needed because it only optimize the first connected component creation.
  m_mustUnMark.reserve(m_clauses.size());
  m_markView.resize(m_clauses.size(), false);
  /** TODO: Mohammad,: should the watcher be initialized to the first literal?
  */
  for (auto &clause : m_clauses) {
    if (clause->size() > 0)
      clause->setWatcher(clause->getLiterals()[0]);
  }
  m_infoCluster.resize(p.getNbVar() + m_clauses.size() + 1, {0, 0, -1});
} // constructor

SpecManagerAll::~SpecManagerAll() { delete[] m_dataOccurrenceMemory; }

int SpecManagerAll::computeConnectedComponent(
    std::vector<std::vector<Var>> &varConnected, std::vector<Var> &setOfVar,
    std::vector<Var> &freeVar) {
  for (auto const v : setOfVar) {
    assert(v < m_infoCluster.size());
    m_infoCluster[v].parent = v;
    m_infoCluster[v].size = 1;
  }

  for (auto const &v : setOfVar) {
    if (m_currentAssignment[v] != l_Undef)
      continue;

    Var rootV = v;
    Lit l = Lit::makeLit(v, false);

    // Handle both the positive and negative literal of the variable v.
    for (unsigned i = 0; i < 2; i++) {
      
      // Pointers to the start and end of the occurrence list for the current literal.
      IteratorIdxClause listIndex = m_occurrence[l.intern()].getClauses();
      // Go through each clause index in the occurrence list for the current literal.
      for (int *ptr = listIndex.start; ptr != listIndex.end; ptr++) {
        int idx = *ptr;
        // Did we not visit this clause before?
        if (!m_markView[idx]) {
          m_markView[idx] = true;
          m_infoCluster[idx + m_nbVar + 1].parent = rootV;
          m_infoCluster[rootV].size++;
          m_mustUnMark.push_back(idx);
        } else {
          // Find the root of the clause?
          Var rootW = m_infoCluster[idx + m_nbVar + 1].parent;
          while (rootW != m_infoCluster[rootW].parent) {
            /**
             * TODO: I do not understand why we set the parent to the parent of the parent, but only once?
             */
            m_infoCluster[rootW].parent =
                m_infoCluster[m_infoCluster[rootW].parent].parent;
            rootW = m_infoCluster[rootW].parent;
          }

          if (rootV == rootW)
            continue;
          // combine the smaller component with the larger one.
          if (m_infoCluster[rootV].size < m_infoCluster[rootW].size) {
            m_infoCluster[rootW].size += m_infoCluster[rootV].size;
            m_infoCluster[rootV].parent = m_infoCluster[rootW].parent;
            rootV = rootW;
          } else {
            m_infoCluster[rootV].size += m_infoCluster[rootW].size;
            m_infoCluster[rootW].parent = m_infoCluster[rootV].parent;
          }
        }
      }

      l = ~l;
    }
  }

  std::vector<Var> rootSet;
  freeVar.resize(0);

  // With the union-find structure built, create the components and set the free variables with the help of the structure.
  for (auto const &v : setOfVar) {
    if (m_currentAssignment[v] != l_Undef)
      continue;

    if (m_infoCluster[v].parent == v && m_infoCluster[v].size == 1) {
      freeVar.push_back(v);
      assert(getNbClause(v) == 0);
      continue;
    }

    assert(getNbClause(v) != 0);
    assert(m_currentAssignment[v] == l_Undef);

    unsigned rootV = m_infoCluster[v].parent;
    while (rootV != m_infoCluster[rootV].parent) {
      m_infoCluster[rootV].parent =
          m_infoCluster[m_infoCluster[rootV].parent].parent;
      rootV = m_infoCluster[rootV].parent;
    }

    // If the root does not belong to any component yet, create a new component and push the root.
    if (m_infoCluster[rootV].pos == -1) {
      m_infoCluster[rootV].pos = varConnected.size();
      varConnected.push_back(std::vector<Var>());
      rootSet.push_back(rootV);
    }

    varConnected[m_infoCluster[rootV].pos].push_back(v);
  }

  resetUnMark();
  for (auto &v : rootSet)
    m_infoCluster[v].pos = -1;

  return varConnected.size();
} // computeConnectedComponent

int SpecManagerAll::computeConnectedComponent(std::vector<ProjVars> &varConnected,
                                              std::vector<Var> &setOfVar,
                                              std::vector<Var> &freeVar) {
  /** TODO: Mohammad,: (I am waiting on this TODO, because I would like to know that everything is working correctly first.)
    This code is technically a copy of the other computeConnectedComponent with the only difference at the end.
    We can create multiple for loops were the other TODO is and instead have 2-3 other "mini" functions instead of duplicating the code.
  */
  for (auto const v : setOfVar) {
    assert(v < m_infoCluster.size());
    m_infoCluster[v].parent = v;
    m_infoCluster[v].size = 1;
  }
  
  for (auto const &v : setOfVar) {
    if (m_currentAssignment[v] != l_Undef)
      continue;

    Var rootV = v;
    Lit l = Lit::makeLit(v, false);

    // Handle both the positive and negative literal of the variable v.
    for (unsigned i = 0; i < 2; i++) {
      // Pointers to the start and end of the occurrence list for the current literal.
      IteratorIdxClause listIndex = m_occurrence[l.intern()].getClauses();
      // Go through each clause index in the occurrence list for the current literal.
      for (int *ptr = listIndex.start; ptr != listIndex.end; ptr++) {
        int idx = *ptr;
        // Did we not visit this clause before?
        if (!m_markView[idx]) {
          m_markView[idx] = true;
          m_infoCluster[idx + m_nbVar + 1].parent = rootV;
          m_infoCluster[rootV].size++;
          m_mustUnMark.push_back(idx);
        } else {
          Var rootW = m_infoCluster[idx + m_nbVar + 1].parent;
          while (rootW != m_infoCluster[rootW].parent) {
            m_infoCluster[rootW].parent =
                m_infoCluster[m_infoCluster[rootW].parent].parent;
            rootW = m_infoCluster[rootW].parent;
          }

          if (rootV == rootW)
            continue;
          // combine the smaller component with the larger one.
          if (m_infoCluster[rootV].size < m_infoCluster[rootW].size) {
            m_infoCluster[rootW].size += m_infoCluster[rootV].size;
            m_infoCluster[rootV].parent = m_infoCluster[rootW].parent;
            rootV = rootW;
          } else {
            m_infoCluster[rootV].size += m_infoCluster[rootW].size;
            m_infoCluster[rootW].parent = m_infoCluster[rootV].parent;
          }
        }
      }

      l = ~l;
    }
  }

  std::vector<Var> rootSet;
  freeVar.resize(0);

  /*TODO: Mohammad,: To solve the code duplication, copy the function until here ("Union_find") and then create three for loops
    One that contain an if "m_infoCluster[v].parent == v && m_infoCluster[v].size == 1" continue and not the last part of the loop.
    This loop should be part of the first substitution function "Union_Find"
    The second loop can stay here it is the same loop but it only set the freevar
    The third loop is the one difference between the two functions and it is the last part of the for loop, which will be done
    in the third for loop.
    */
  // With the union-find structure built, create the components and set the free variables with the help of the structure.
  for (auto const &v : setOfVar) {
    if (m_currentAssignment[v] != l_Undef)
      continue;

    if (m_infoCluster[v].parent == v && m_infoCluster[v].size == 1) {
      freeVar.push_back(v);
      assert(getNbClause(v) == 0);
      continue;
    }

    assert(getNbClause(v) != 0);
    assert(m_currentAssignment[v] == l_Undef);

    unsigned rootV = m_infoCluster[v].parent;
    while (rootV != m_infoCluster[rootV].parent) {
      m_infoCluster[rootV].parent =
          m_infoCluster[m_infoCluster[rootV].parent].parent;
      rootV = m_infoCluster[rootV].parent;
    }

    // If the root does not belong to any component yet, create a new component and push the root.
    if (m_infoCluster[rootV].pos == -1) {
      m_infoCluster[rootV].pos = varConnected.size();
      varConnected.push_back(ProjVars());
      rootSet.push_back(rootV);
    }

    varConnected[m_infoCluster[rootV].pos].vars.push_back(v);
    varConnected[m_infoCluster[rootV].pos].nbProj += isSelected(v);
  }

  resetUnMark();
  for (auto &v : rootSet)
    m_infoCluster[v].pos = -1;

  return varConnected.size();
} // computeConnectedComponent

void SpecManagerAll::preUpdate(std::vector<Lit> &lits) {
  m_reviewWatcher.resize(0);

  // Go through each literal and set as assigned and removed satisfied clauses occurrences from other literals and updating the watcher if the literal was the current watcher of a clause.
  for (auto &l : lits) {
    assert(!litIsAssigned(l));
    m_currentAssignment[l.var()] = l.sign() ? l_False : l_True;

    // Update clauses positive literal occurrences and watcher if the literal was the current watcher. Also remove the occurrences of satisfied clauses from other literals.
    for (unsigned i = 0; i < m_occurrence[l.intern()].nbNotBin; i++) {
      int idxCl = m_occurrence[l.intern()].notBin[i];
      auto &clause = *m_clauses[idxCl];
      clause.incNbSatLit();
      if (clause.getWatcher() == l && !clause.isSatisfied())
        m_reviewWatcher.push_back(idxCl);
      if (!clause.isSatisfied())
        continue;
      for (auto &ll : clause.getLiterals())
        if (m_currentAssignment[ll.var()] == l_Undef)
          m_occurrence[ll.intern()].removeNotBin(idxCl);
    }
    // Update clauses negative literal occurrences and watcher if the literal was the current watcher. Also remove the occurrences of satisfied clauses from other literals.
    for (unsigned i = 0; i < m_occurrence[(~l).intern()].nbNotBin; i++) {
      int idxCl = m_occurrence[(~l).intern()].notBin[i];
      auto &clause = *m_clauses[idxCl];
      clause.incNbUnsatLit();
      if (clause.getWatcher() == ~l && !clause.isSatisfied())
        m_reviewWatcher.push_back(idxCl);
      if (!clause.isSatisfied())
        continue;
      for (auto &ll : clause.getLiterals())
        if (m_currentAssignment[ll.var()] == l_Undef)
          m_occurrence[ll.intern()].removeNotBin(idxCl);
    }

    // Update clauses positive literal occurrences and watcher if the literal was the current watcher. Also remove the occurrences of satisfied clauses from other literals.
    for (unsigned i = 0; i < m_occurrence[l.intern()].nbBin; i++) {
      int idxCl = m_occurrence[l.intern()].bin[i];
      auto &clause = *m_clauses[idxCl];
      clause.incNbSatLit();
      if (clause.getWatcher() == l && !clause.isSatisfied())
        m_reviewWatcher.push_back(idxCl);
      if (!clause.isSatisfied())
        continue;
      for (auto &ll : clause.getLiterals())
        if (m_currentAssignment[ll.var()] == l_Undef)
          m_occurrence[ll.intern()].removeBin(idxCl);
    }

    // Update clauses negative literal occurrences and watcher if the literal was the current watcher. Also remove the occurrences of satisfied clauses from other literals.
    for (unsigned i = 0; i < m_occurrence[(~l).intern()].nbBin; i++) {
      int idxCl = m_occurrence[(~l).intern()].bin[i];
      auto &clause = *m_clauses[idxCl];
      clause.incNbUnsatLit();
      if (clause.getWatcher() == ~l && !clause.isSatisfied())
        m_reviewWatcher.push_back(idxCl);
      if (!clause.isSatisfied())
        continue;
      for (auto &ll : clause.getLiterals())
        if (m_currentAssignment[ll.var()] == l_Undef)
          m_occurrence[ll.intern()].removeBin(idxCl);}
  }
  // Update the watcher for clauses that need a new watcher literal
  for (auto &idxCl : m_reviewWatcher) {
    auto &clause = *m_clauses[idxCl];
    if (clause.isSatisfied())
      continue;

    for (auto &l : clause.getLiterals()) {
      if (m_currentAssignment[l.var()] == l_Undef) {
        clause.setWatcher(l);
        break;
      }
    }
  }
} // preUpdate

void SpecManagerAll::preUpdate(std::vector<Lit> &lits,
                               std::vector<Lit> &pure) {
  m_reviewWatcher.resize(0);

  // Go through each literal and set as assigned and removed satisfied clauses occurrences from other literals and updating the watcher if the literal was the current watcher of a clause.
  for (auto &l : lits) {
    assert(!litIsAssigned(l));
    m_currentAssignment[l.var()] = l.sign() ? l_False : l_True;

    // Update clauses positive literal occurrences and watcher if the literal was the current watcher. Also remove the occurrences of satisfied clauses from other literals.
    for (unsigned i = 0; i < m_occurrence[l.intern()].nbNotBin; i++) {
      int idxCl = m_occurrence[l.intern()].notBin[i];
      auto &clause = *m_clauses[idxCl];
      clause.incNbSatLit();
      if (clause.getWatcher() == l && !clause.isSatisfied())
        m_reviewWatcher.push_back(idxCl);
      if (!clause.isSatisfied())
        continue;
      for (auto &ll : clause.getLiterals()) {
        if (m_currentAssignment[ll.var()] == l_Undef) {
          m_occurrence[ll.intern()].removeNotBin(idxCl);
          if (!isSelected(ll.var()) && !litIsAssigned(ll) &&
              m_occurrence[ll.intern()].nbNotBin == 0 &&
              m_occurrence[ll.intern()].nbBin == 0 &&
              m_occurrence[(~ll).intern()].nbBin +
                      m_occurrence[(~ll).intern()].nbNotBin >
                  0) {
            pure.push_back(~ll);
          }
        }
      }
    }
    // Update clauses negative literal occurrences and watcher if the literal was the current watcher. Also remove the occurrences of satisfied clauses from other literals.
    for (unsigned i = 0; i < m_occurrence[(~l).intern()].nbNotBin; i++) {
      int idxCl = m_occurrence[(~l).intern()].notBin[i];
      auto &clause = *m_clauses[idxCl];
      clause.incNbUnsatLit();
      if (clause.getWatcher() == ~l && !clause.isSatisfied())
        m_reviewWatcher.push_back(idxCl);
      if (!clause.isSatisfied())
        continue;
      for (auto &ll : clause.getLiterals()) {
        if (m_currentAssignment[ll.var()] == l_Undef) {
          m_occurrence[ll.intern()].removeNotBin(idxCl);
          if (!isSelected(ll.var()) && !litIsAssigned(ll) &&
              m_occurrence[ll.intern()].nbNotBin == 0 &&
              m_occurrence[ll.intern()].nbBin == 0 &&
              m_occurrence[(~ll).intern()].nbBin +
                      m_occurrence[(~ll).intern()].nbNotBin >
                  0) {
            pure.push_back(~ll);
          }
        }
      }
    }

    // Update clauses positive literal occurrences and watcher if the literal was the current watcher. Also remove the occurrences of satisfied clauses from other literals.
    for (unsigned i = 0; i < m_occurrence[l.intern()].nbBin; i++) {
      int idxCl = m_occurrence[l.intern()].bin[i];
      auto &clause = *m_clauses[idxCl];
      clause.incNbSatLit();
      if (clause.getWatcher() == l && !clause.isSatisfied())
        m_reviewWatcher.push_back(idxCl);
      if (!clause.isSatisfied())
        continue;
      for (auto &ll : clause.getLiterals()) {
        if (m_currentAssignment[ll.var()] == l_Undef) {
          m_occurrence[ll.intern()].removeBin(idxCl);
          if (!isSelected(ll.var()) && !litIsAssigned(ll) &&
              m_occurrence[ll.intern()].nbNotBin == 0 &&
              m_occurrence[ll.intern()].nbBin == 0 &&
              m_occurrence[(~ll).intern()].nbBin +
                      m_occurrence[(~ll).intern()].nbNotBin >
                  0) {
            pure.push_back(~ll);
          }
        }
      }
    }

    // Update clauses negative literal occurrences and watcher if the literal was the current watcher. Also remove the occurrences of satisfied clauses from other literals.
    for (unsigned i = 0; i < m_occurrence[(~l).intern()].nbBin; i++) {
      int idxCl = m_occurrence[(~l).intern()].bin[i];
      auto &clause = *m_clauses[idxCl];
      clause.incNbUnsatLit();
      if (clause.getWatcher() == ~l && !clause.isSatisfied())
        m_reviewWatcher.push_back(idxCl);
      if (!clause.isSatisfied())
        continue;
      for (auto &ll : clause.getLiterals()) {
        if (m_currentAssignment[ll.var()] == l_Undef) {
          m_occurrence[ll.intern()].removeBin(idxCl);
          if (!isSelected(ll.var()) && !litIsAssigned(ll) &&
              m_occurrence[ll.intern()].nbNotBin == 0 &&
              m_occurrence[ll.intern()].nbBin == 0 &&
              m_occurrence[(~ll).intern()].nbBin +
                      m_occurrence[(~ll).intern()].nbNotBin >
                  0) {
            pure.push_back(~ll);
          }
        }
      }
    }
  }
  // Update the watcher for clauses that need a new watcher literal
  for (auto &idxCl : m_reviewWatcher) {
    auto &clause = *m_clauses[idxCl];
    if (clause.isSatisfied())
      continue;

    for (auto &l : clause.getLiterals()) {
      if (m_currentAssignment[l.var()] == l_Undef) {
        clause.setWatcher(l);
        break;
      }
    }
  }
} // preUpdate

void SpecManagerAll::postUpdate(std::vector<Lit> &lits) {
  for (int i = lits.size() - 1; i >= 0; i--) {
    Lit l = lits[i];

    for (unsigned i = 0; i < m_occurrence[l.intern()].nbNotBin; i++) {
      int idxCl = m_occurrence[l.intern()].notBin[i];
      auto &clause = *m_clauses[idxCl];
      clause.decNbSatLit();
      // should Never occur? (If it should never then use assert)
      if (clause.isSatisfied())
        continue;

      for (auto &ll : clause.getLiterals())
        if (m_currentAssignment[ll.var()] == l_Undef)
          m_occurrence[ll.intern()].addNotBin(idxCl);
    }

    for (unsigned i = 0; i < m_occurrence[(~l).intern()].nbNotBin; i++) {
      int idxCl = m_occurrence[(~l).intern()].notBin[i];
      auto &clause = *m_clauses[idxCl];
      clause.decNbUnsatLit();
      // should Never occur? (If it should never then use assert)
      if (clause.isSatisfied())
        continue;
      for (auto &ll : clause.getLiterals())
        if (m_currentAssignment[ll.var()] == l_Undef)
          m_occurrence[ll.intern()].addNotBin(idxCl);
    }

    for (unsigned i = 0; i < m_occurrence[l.intern()].nbBin; i++) {
      int idxCl = m_occurrence[l.intern()].bin[i];
      auto &clause = *m_clauses[idxCl];
      clause.decNbSatLit();
      // should Never occur? (If it should never then use assert)
      if (clause.isSatisfied())
        continue;
      for (auto &ll : clause.getLiterals())
        if (m_currentAssignment[ll.var()] == l_Undef)
          m_occurrence[ll.intern()].addBin(idxCl);
    }

    for (unsigned i = 0; i < m_occurrence[(~l).intern()].nbBin; i++) {
      int idxCl = m_occurrence[(~l).intern()].bin[i];
      auto &clause = *m_clauses[idxCl];
      clause.decNbUnsatLit();
      // should Never occur? (If it should never then use assert)
      if (clause.isSatisfied())
        continue;
      for (auto &ll : clause.getLiterals())
        if (m_currentAssignment[ll.var()] == l_Undef)
          m_occurrence[ll.intern()].addBin(idxCl);
    }

    m_currentAssignment[l.var()] = l_Undef;
  }
} // postUpdate

/**
   Determine whether a literal is currently assigned.
*/
bool SpecManagerAll::litIsAssigned(Lit l) {
  return m_currentAssignment[l.var()] != l_Undef;
}

/**
   Determine whether a literal is currently assigned to true.
*/
bool SpecManagerAll::litIsAssignedToTrue(Lit l) {
  if (l.sign())
    return m_currentAssignment[l.var()] == l_False;
  return m_currentAssignment[l.var()] == l_True;
}

/**
   Determine whether a variable is currently assigned.
*/
bool SpecManagerAll::varIsAssigned(Var v) {
  return m_currentAssignment[v] != l_Undef;
}

/**
   Return the number of occurrences for a literal.
*/
int SpecManagerAll::getNbOccurrence(Lit l) { return m_occurrence[l.intern()].nbBin + m_occurrence[l.intern()].nbNotBin; }

/**
   Display the mixed formula in DIMACS-like form.
*/
void SpecManagerAll::showFormula(std::ostream &out) {
  out << "p cnf " << getNbVariable() << " " << getNbClause() << "\n";
  for (auto &cl : m_clauses) {
    for (auto &lit : cl->getLiterals())
      out << lit << " ";
    out << "0\n";
  }
} // showFormula

/**
   Display the current formula after assignments have been applied.
*/
void SpecManagerAll::showCurrentFormula(std::ostream &out) {
  out << "p cnf " << getNbVariable() << " " << getNbClause() << "\n";
  for (unsigned i = 0; i < m_clauses.size(); i++) {
    if (isSatisfiedClause(i))
      continue;
    for (auto &l : m_clauses[i]->getLiterals())
      if (!litIsAssigned(l))
        out << l << " ";
    out << "0\n";
  }
} // showCurrentFormula

/**
   Display the current assignment trail.
*/
void SpecManagerAll::showTrail(std::ostream &out) {
  for (int i = 0; i < getNbVariable(); i++) {
    if (!varIsAssigned(i))
      continue;
    Lit l = Lit::makeLit(i, false);
    if (litIsAssignedToTrue(l))
      out << l << " ";
    else
      out << ~l << " ";
  }
  out << "\n";
} // showTrail

/**
   Collect the active clause indices for the current connected component.
*/
void SpecManagerAll::getCurrentClauses(std::vector<unsigned> &idxClauses,
                                       std::vector<Var> &component) {
  idxClauses.resize(0);
  for (auto &v : component)
    m_inCurrentComponent[v] = true;
  for (unsigned i = 0; i < m_clauses.size(); i++) {
    if (isNotSatisfiedClauseAndInComponent(i, m_inCurrentComponent))
      idxClauses.push_back(i);
  }
  for (auto &v : component)
    m_inCurrentComponent[v] = false;
} // getCurrentClauses

/**
   Collect the non-binary active clause indices for the current component.
*/
void SpecManagerAll::getCurrentClausesNotBin(std::vector<unsigned> &idxClauses,
                                             std::vector<Var> &component) {
  idxClauses.resize(0);
  for (auto &v : component)
    m_inCurrentComponent[v] = true;
  for (auto &i : m_clausesNotBinary) {
    if (isNotSatisfiedClauseAndInComponent(i, m_inCurrentComponent))
      idxClauses.push_back(i);
  }
  for (auto &v : component)
    m_inCurrentComponent[v] = false;
} // getCurrentClausesNotBin

} // namespace d4
