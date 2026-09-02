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

#include <iostream>
#include <vector>

#include "Branch.hpp"
#include "Node.hpp"
#include "src/problem/ProblemManager.hpp"

namespace d4 {
template <class T, typename U> class DeterministicOrNode : public Node<T> {
public:
  using Node<T>::header;

  T nbModels;
  U size;
  Branch<T, U> *branches;
  /* The explicit data of each branch is saved in data,
     where the end of the data of a branch i and added one position to it is the
     beginning of the data of the branch i+1. Because The array start with
     position 0 it is enough to do pos += p->branches[i].nbUnits +
     p->branches[i].nbFree; to get position of next branch data.
  */
  U *data;

  DeterministicOrNode() = delete;
  /**
   * Init the branches using elts and its size.
   *
   * @param[in] elts the branches we want to combine under the node.
   * @param[in] size the number of branches.
   */
  DeterministicOrNode(DataBranch<Node<T> *> *elts, unsigned _size) {
    header.typeNode = TypeNode::TypeDetOrNode;
    header.stamp = 0;
    size = _size;
    nbModels = T(0);

    char *base = reinterpret_cast<char *>(this);
    branches = reinterpret_cast<Branch<T, U> *>(base + sizeof(*this));
    data = reinterpret_cast<U *>(reinterpret_cast<char *>(branches) +
                                 size * sizeof(Branch<T, U>));

    unsigned pos = 0;
    for (unsigned i = 0; i < size; i++) {
      branches[i].d = elts[i].d;
      branches[i].nbUnits = elts[i].unitLits.size();
      branches[i].nbFree = elts[i].freeVars.size();

      for (auto &lit : elts[i].unitLits)
        data[pos++] = lit.intern();
      for (auto &var : elts[i].freeVars)
        data[pos++] = var;
    }
  }

  /**
   * Deallocate the memory used by the node and recursively deallocate its
   * children.
   *
   * @param[in] node the node to deallocate, is equivalent to this.
   * @param[in] func the functions vector containing the deallocate function for
   * each node type.?
   * @param[in] globalStamp the stamp number.
   */
  static void deallocate(Node<T> *node, void (**func)(), unsigned globalStamp) {
    if (node->header.stamp == globalStamp)
      return;
    node->header.stamp = globalStamp;
    reinterpret_cast<DeterministicOrNode *>(node)->nbModels.~T();

    auto *p = reinterpret_cast<DeterministicOrNode *>(node);
    for (unsigned i = 0; i < p->size; i++) {
      reinterpret_cast<void (**)(Node<T> *, void (**func)(), unsigned)>(
          func)[p->branches[i].d->header.typeNode](p->branches[i].d, func,
                                                   globalStamp);
    }
  }

  /**
   * Ask for the number of models of the formula.
   *
   * @param[in] node the node to compute the number of models, is equivalent to
   * this.
   * @param[in] func the functions vector containing the computeNbModels
   * function for each node type.
   * @param[in] fixedValue the assignment we consider.
   * @param[in] problem the problem we are solving (use to get information about
   * weight).
   * @param[in] globalStamp the stamp number.
   */
  static T computeNbModels(Node<T> *node, T (**func)(),
                           std::vector<ValueVar> &fixedValue,
                           ProblemManager &problem, unsigned globalStamp) {
    auto *p = reinterpret_cast<DeterministicOrNode *>(node);
    if (node->header.stamp == globalStamp)
      return p->nbModels;

    p->nbModels = T(0);
    unsigned pos = 0;
    for (unsigned i = 0; i < p->size; i++) {
      p->nbModels += p->branches[i].computeNbModels(
          func, &p->data[pos], fixedValue, problem, globalStamp);
      pos += p->branches[i].nbUnits + p->branches[i].nbFree;
    }

    node->header.stamp = globalStamp;
    return p->nbModels;
  }

  /**
   * Ask if the formula is satisfiable under an interpretation (fixedValue).
   *
   * @param[in] node the node to compute the number of models, is equivalent to
   * this.
   * @param[in] func the functions vector containing the isSAT functions for
   * each node type.
   * @param[in] fixedValue the assignment we consider.
   * @param[in] globalStamp the stamp number.
   */
  static bool isSAT(Node<T> *node, bool (**func)(),
                    std::vector<ValueVar> &fixedValue, unsigned globalStamp) {
    auto *p = reinterpret_cast<DeterministicOrNode *>(node);
    if (node->header.stamp == globalStamp)
      return p->nbModels == 1;

    unsigned pos = 0;
    for (unsigned i = 0; i < p->size; i++) {
      p->nbModels =
          p->branches[i].isSAT(func, &p->data[pos], fixedValue, globalStamp);
      if (p->nbModels == 1) {
        node->header.stamp = globalStamp;
        return true;
      }
      pos += p->branches[i].nbUnits + p->branches[i].nbFree;
    }

    node->header.stamp = globalStamp;
    return false;
  }

  /**
   * Print the NNF representation of the node and its children.
   *
   * @param[in] node the node to print, is equivalent to this.
   * @param[in] func the functions vector containing the printNNF functions for
   * each node type.
   * @param[in] out the stream where we print out the formula.
   * @param[in] idx the index of the node.
   * @param[in] globalStamp the stamp number.
   */
  static unsigned printNNF(Node<T> *node, unsigned (**func)(),
                           std::ostream &out, unsigned &idx,
                           unsigned globalStamp) {
    auto *p = reinterpret_cast<DeterministicOrNode *>(node);
    if (p->header.stamp == globalStamp)
      return (unsigned)p->nbModels;

    p->nbModels = idx++;
    out << "o " << (unsigned)p->nbModels << " 0\n";

    unsigned pos = 0;
    for (unsigned i = 0; i < p->size; i++) {
      p->branches[i].printNNF((unsigned)p->nbModels, &p->data[pos], func, out,
                              idx, globalStamp);
      pos += p->branches[i].nbUnits + p->branches[i].nbFree;
    }

    p->header.stamp = globalStamp;
    return (unsigned)p->nbModels;
  }
};
} // namespace d4