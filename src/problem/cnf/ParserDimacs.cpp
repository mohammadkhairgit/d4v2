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

#include "ParserDimacs.hpp"

#include <algorithm>
#include <cctype>

#include "src/problem/ProblemManager.hpp"
#include "src/problem/cnf/ProblemManagerCnf.hpp"

namespace d4 {

void ParserDimacs::readListIntTerminatedByZero(BufferRead &in,
                                               std::vector<int> &list) {
  int v = -1;
  do {
    v = in.nextInt();
    if (v)
      list.push_back(v);
  } while (v);
} // readListIntTerminatedByZero

void ParserDimacs::parseWeightedLit(BufferRead &in,
                                    std::vector<double> &weightLit) {
  int lit = in.nextInt();
  double w = in.nextDouble();

  if (lit > 0)
    weightLit[lit << 1] = w;
  else
    weightLit[((-lit) << 1) + 1] = w;
} // parseWeightedLit

int ParserDimacs::parse_DIMACS_main(BufferRead &in,
                                    std::vector<std::vector<Lit>> &clauses,
                                    std::vector<double> &weightLit,
                                    std::vector<Var> &selected,
                                    std::vector<Var> &maxVar) {
  std::vector<Lit> lits;

  int nbVars = 0;
  int nbClauses = 0;
  /* read the header and the clauses by first seeing what kind of problem we have
  , meaning normal cnf (p cnf), weighted cnf (p wcnf) or a projected cnf (p pcnf). */
  for (;;) {
    in.skipSpace();
    if (in.eof())
      break;

    if (in.currentChar() == 'p') {
      in.consumeChar();
      in.skipSpace();

      bool vpActivated = false;
      // check if the problem is a projected cnf.
      if (in.currentChar() == 'p') {
        vpActivated = true;
        in.consumeChar();
      }
      // check if the problem is a weighted cnf.
      if (in.currentChar() == 'w')
        in.consumeChar();
        // check if the problem is a cnf. (also remove the 'cnf' part of the header)
      if (in.nextChar() != 'c' || in.nextChar() != 'n' || in.nextChar() != 'f')
        std::cerr << "PARSE ERROR! Unexpected char: " << in.currentChar()
                  << "\n",
            exit(3);

      nbVars = in.nextInt();
      nbClauses = in.nextInt();

      if (vpActivated)
        std::cout << "c Some variable are marked: " << in.nextInt() << "\n";
      weightLit.resize(((nbVars + 1) << 1), 1);
      if (nbClauses < 0)
        printf("parse error\n"), exit(2);
      /* parse lines terminated by 0, while considering the type of problem of the line with:
    vp as projected, w as weighted, or comments with usefull information.*/
    } else if (in.currentChar() == 'v') {
      in.consumeChar();
      assert(in.currentChar() == 'p');
      in.consumeChar();
      readListIntTerminatedByZero(in, selected);
    } else if (in.currentChar() == 'w') {
      in.consumeChar();
      in.skipSpace();
      parseWeightedLit(in, weightLit);
    } else if (in.currentChar() == 'c') {
      // process comments
      in.consumeChar();
      in.skipSimpleSpace();

      if (in.currentChar() != 'p') {
        if (in.canConsume("max")) {
          readListIntTerminatedByZero(in, maxVar);
        } else if (in.canConsume("ind")) {
          readListIntTerminatedByZero(in, selected);
        } else {
          in.skipLine();
        }
      } else {
        in.consumeChar();
        if (in.canConsume("weight")) {
          parseWeightedLit(in, weightLit);

          // in this format we have an end line we have to consume.
          [[maybe_unused]] int endLine = in.nextInt();
          assert(!endLine);
        } else if (in.canConsume("show")) {
          readListIntTerminatedByZero(in, selected);
        } else if (in.canConsume("ind")) {
          readListIntTerminatedByZero(in, selected);
        } else {
          in.skipLine();
        }
      }
    } else {
      lits.clear();
      int v = -1;
      /* process clauses, if we get a non-number character it will be considered as a 0 and then we will handle it in the next for loop.*/
      do {
        v = in.nextInt();
        // Did we read a variable that is out of the range of the number of variables?
        if ((v > 0 && nbVars < v) || (-v > 0 && nbVars < -v))
          std::cerr << "PARSE ERROR! Number of variables incorrect: " << v
                    << "\n",
              exit(3);

        if (v)
          lits.push_back((v > 0) ? Lit::makeLit(v, false)
                                 : Lit::makeLit(-v, true));
      } while (v);

      assert(lits.size());
      std::sort(lits.begin(), lits.end());

      // Check for tautology of the current clause.
      unsigned j = 1;
      bool isSat = false;
      for (unsigned i = 1; !isSat && i < lits.size(); i++) {
        if (lits[i] == lits[j - 1])
          continue;
        isSat = lits[i] == ~lits[j - 1];
        lits[j++] = lits[i];
      }

      // add the clause only if it not a tautology.
      if (!isSat) {
        lits.resize(j);
        clauses.push_back(lits);
      }
    }
  }

  return nbVars;
} // parse_DIMACS_main

int ParserDimacs::parse_DIMACS_main(BufferRead &in,
                                    ProblemManagerCnf *problemManager) {
  std::vector<std::vector<Lit>> &clauses = problemManager->getClauses();
  std::vector<double> &weightLit = problemManager->getWeightLit();
  std::vector<Var> &selected = problemManager->getSelectedVar();
  std::vector<Var> &maxVar = problemManager->getMaxVar();

  int nbVars = parse_DIMACS_main(in, clauses, weightLit, selected, maxVar);
  return nbVars;
} // parse_DIMACS_main

int ParserDimacs::parse_DIMACS(std::string input_stream,
                               ProblemManagerCnf *problemManager) {
  BufferRead in(input_stream);
  return parse_DIMACS_main(in, problemManager);
} // parse_DIMACS

int ParserDimacs::parse_DIMACS(
    std::string input_stream, std::vector<std::vector<Lit>> &clauses,
    std::vector<double> &weightLit, std::vector<Var> &selected,
    std::vector<Var> &maxVar) {
  BufferRead in(input_stream);
  return parse_DIMACS_main(in, clauses, weightLit, selected, maxVar);
} // parse_DIMACS

void ParserDimacs::parse_alternative_main(BufferRead &in, int nbVars,
                                         std::vector<std::vector<Lit>> &clauses) {
  std::vector<Lit> lits;

  for (;;) {
    in.skipSpace();
    if (in.eof())
      break;

    if (in.currentChar() == 'c' || in.currentChar() == 'p') {
      in.skipLine();
      continue;
    }

    if (!std::isdigit(static_cast<unsigned char>(in.currentChar())) &&
        in.currentChar() != '-') {
      in.skipLine();
      continue;
    }

    lits.clear();
    int v = -1;
    /* process alternative clauses, if we get a non-number character it will be considered as a 0 and then we will handle it in the next for loop.*/
    do {
      v = in.nextInt();
      // Did we read a variable that is out of the range of the number of variables?
      if ((v > 0 && nbVars < v) || (-v > 0 && nbVars < -v))
        std::cerr << "PARSE ERROR! Number of variables incorrect: " << v
                  << "\n",
            exit(3);

      if (v)
        lits.push_back((v > 0) ? Lit::makeLit(v, false)
                               : Lit::makeLit(-v, true));
    } while (v);

    if (lits.size() == 0)
      std::cerr << "PARSE ERROR! Empty alternative clause.\n", exit(3);
    
    std::sort(lits.begin(), lits.end());
    auto last = std::unique(lits.begin(), lits.end());
    lits.erase(last, lits.end());
    clauses.push_back(lits);
  }
} // parse_alternative_main

void ParserDimacs::parse_alternative(std::string input_stream, int nbVars,
                                     std::vector<std::vector<Lit>> &clauses) {
  BufferRead in(input_stream);
  parse_alternative_main(in, nbVars, clauses);
} // parse_alternative
} // namespace d4
