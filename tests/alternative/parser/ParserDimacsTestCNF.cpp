#include "tests/alternative/parser/ParserDimacsFixture.hpp"

TEST_F(ParserDimacsTest, ParseDimacsCNF) {
  // The clauses content get ordered by the parser and parsed correctly
  EXPECT_EQ(nbVars, 7) << "The number of variables should be 7";
  EXPECT_EQ(cnf_clauses.size(), 4) << "The number of CNF clauses should be 4";
  EXPECT_EQ(cnf_clauses[0].size(), 5)
      << "The first CNF clause should contain 5 literals";
  EXPECT_EQ(cnf_clauses[0][0].var(), 1)
      << "The first literal in the first CNF clause should be variable 1";
  EXPECT_FALSE(cnf_clauses[0][0].sign())
      << "The first literal in the first CNF clause should be positive";
  EXPECT_EQ(cnf_clauses[0][1].var(), 2)
      << "The second literal in the first CNF clause should be variable 2";
  EXPECT_TRUE(cnf_clauses[0][1].sign())
      << "The second literal in the first CNF clause should be Negative";
  EXPECT_EQ(cnf_clauses[0][2].var(), 3)
      << "The third literal in the first CNF clause should be variable 3";
  EXPECT_FALSE(cnf_clauses[0][2].sign())
      << "The third literal in the first CNF clause should be positive";
  EXPECT_EQ(cnf_clauses[2].size(), 3)
      << "The third CNF clause should contain 3 literals";
  EXPECT_EQ(cnf_clauses[2][0].var(), 1)
      << "The first literal in the third CNF clause should be variable 1";
  EXPECT_FALSE(cnf_clauses[2][0].sign())
      << "The first literal in the third CNF clause should be Negative";
  EXPECT_EQ(cnf_clauses[2][1].var(), 3)
      << "The second literal in the third CNF clause should be variable 3";
  EXPECT_TRUE(cnf_clauses[2][1].sign())
      << "The second literal in the third CNF clause should be positive";
  EXPECT_EQ(cnf_clauses[2][2].var(), 5)
      << "The third literal in the third CNF clause should be variable 5";
  EXPECT_FALSE(cnf_clauses[2][2].sign())
      << "The third literal in the third CNF clause should be Negative";
}