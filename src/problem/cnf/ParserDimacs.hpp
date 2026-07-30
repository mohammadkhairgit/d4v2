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

#include <stdio.h>
#include <stdlib.h>

#include <cassert>
#include <fstream>
#include <iostream>
#include <limits>
#include <vector>

#include "../ProblemTypes.hpp"
#include "src/problem/cnf/ProblemManagerCnf.hpp"
#include "src/utils/BufferRead.hpp"

namespace d4 {
class ParserDimacs {
private:
  /**
    Parse a DIMACS stream into clause vectors and metadata vectors and return the number of variables.

    @param[in] in the stream buffer where we get the information, should be data from a DIMACS file.
    @param[out] clauses vector of clauses.
    @param[out] weightLit vector of weights for each literal.
    @param[out] selected vector of the selected/projected variables.
    @param[out] maxVar the maximum number of variables.
    @return the number of variables in the CNF formula.
  */
  int parse_DIMACS_main(BufferRead &in, std::vector<std::vector<Lit>> &clauses,
                std::vector<double> &weightLit,
                std::vector<Var> &selected,
                std::vector<Var> &maxVar);

  /** 
    Parse a DIMACS stream into clause vectors and metadata vectors and return the number of variables.

    @param[in] in the stream buffer where we get the information, should be Buffer of a DIMACS file.
    @param[out] problemManager the CNF problem manager where we store the parsed information.
    @return the number of variables in the CNF formula.
   */
  int parse_DIMACS_main(BufferRead &in, ProblemManagerCnf *problemManager);

  /**
    Parse an alternative-clause input file into clauses.

    @param[in] in the stream buffer where we get the information, should be data from a "DIMACS" file.
    @param[in] nbVars the number of variables in the CNF formula.
    @param[out] clauses vector of clauses.
  */
  void parse_alternative_main(BufferRead &in, int nbVars,
                    std::vector<std::vector<Lit>> &clauses);
  /**
 * @brief Read the next integar in the given stream while the value 0 is not
 * reached.
 *
 * @param in the stream.
 * @param list the list of integer we parsed.
 */
  void readListIntTerminatedByZero(BufferRead &in, std::vector<int> &list);
  /**
 * @brief Parse a literal index and a weight and store the result in the given
 * vector.
 *
 * @param in the stream buffer where we get the information.
 * @param weightLit the place where is stored the data.
 */
  void parseWeightedLit(BufferRead &in, std::vector<double> &weightLit);

public:
  /**
    Parse a DIMACS file into raw clause vectors and metadata.

    @param[in] input_stream the name of the file to parse.
    @param[out] clauses vector of clauses.
    @param[out] weightLit vector of weights for each literal.
    @param[out] selected vector of the selected/projected variables.
    @param[out] maxVar the maximum variables.
  */
  int parse_DIMACS(std::string input_stream,
             std::vector<std::vector<Lit>> &clauses,
             std::vector<double> &weightLit,
             std::vector<Var> &selected, std::vector<Var> &maxVar);

  /**
   * @brief Parse a DIMACS file into a CNF problem manager.
   * 
   * @param input_stream the name of the file to parse.
   * @param problemManager the CNF problem manager where we store the parsed information.
   * @return the number of variables in the CNF formula.
   */
  int parse_DIMACS(std::string input_stream, ProblemManagerCnf *problemManager);

  /**
    Parse an alternative-clause file into raw clause vectors.
    
    @param[in] input_stream the name of the file to parse.
    @param[in] nbVars the number of variables in the CNF formula.
    @param[out] clauses vector of clauses.
  */
  void parse_alternative(std::string input_stream, int nbVars,
                std::vector<std::vector<Lit>> &clauses);
};
} // namespace d4
