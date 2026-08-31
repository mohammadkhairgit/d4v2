#include "src/problem/cnf/ParserDimacs.hpp"

#include <gtest/gtest.h>

#include <string>
#include <vector>

class ParserDimacsTest : public testing::Test {
protected:
  std::vector<std::vector<d4::Lit>> cnf_clauses;
  std::vector<std::vector<d4::Lit>> alternative_clauses;
  std::vector<double> weightLit;
  std::vector<d4::Var> selected;
  std::vector<d4::Var> maxVar;
  int nbVars;

  void SetUp() {
    {
      d4::ParserDimacs parser;
      std::string inputPathCNF = "instancesTest/cnfs_and_alternatives/cnf1.cnf";
      std::string inputPathAlternative =
          "instancesTest/cnfs_and_alternatives/alternative1.cnf";
      nbVars = parser.parse_DIMACS(inputPathCNF, cnf_clauses, weightLit,
                                   selected, maxVar);
      parser.parse_alternative(inputPathAlternative, nbVars,
                               alternative_clauses);
    }
  }
};