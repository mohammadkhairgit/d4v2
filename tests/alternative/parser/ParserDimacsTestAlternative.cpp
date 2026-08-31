#include "tests/alternative/parser/ParserDimacsFixture.hpp"

TEST_F(ParserDimacsTest, ParseDimacsAlternative) {
  // The clauses content get ordered by the parser and parsed correctly
  EXPECT_EQ(alternative_clauses.size(), 3)
      << "The number of alternative clauses should be 3";
  EXPECT_EQ(alternative_clauses[0].size(), 2)
      << "The first alternative clause should contain 2 literals";
  EXPECT_EQ(alternative_clauses[0][1].var(), 3)
      << "The first literal in the first alternative clause should be variable "
         "3";
  EXPECT_FALSE(alternative_clauses[0][1].sign())
      << "The first literal in the first alternative clause should be positive";
  EXPECT_EQ(alternative_clauses[0][0].var(), 1)
      << "The second literal in the first alternative clause should be "
         "variable 1";
  EXPECT_FALSE(alternative_clauses[0][0].sign())
      << "The second literal in the first alternative clause should be "
         "positive";
  EXPECT_EQ(alternative_clauses[1].size(), 3)
      << "The second alternative clause should contain 3 literals";
  EXPECT_EQ(alternative_clauses[1][1].var(), 2)
      << "The first literal in the second alternative clause should be "
         "variable "
         "2";
  EXPECT_TRUE(alternative_clauses[1][1].sign())
      << "The first literal in the second alternative clause should be "
         "Negative";
  EXPECT_EQ(alternative_clauses[1][2].var(), 6)
      << "The second literal in the second alternative clause should be "
         "variable "
         "6";
  EXPECT_FALSE(alternative_clauses[1][2].sign())
      << "The second literal in the second alternative clause should be "
         "positive";
  EXPECT_EQ(alternative_clauses[1][0].var(), 1)
      << "The third literal in the second alternative clause should be "
         "variable "
         "1";
  EXPECT_FALSE(alternative_clauses[1][0].sign())
      << "The third literal in the second alternative clause should be "
         "positive";
  EXPECT_EQ(alternative_clauses[2].size(), 1)
      << "The third alternative clause should contain 1 literal";
  EXPECT_EQ(alternative_clauses[2][0].var(), 3)
      << "The first literal in the third alternative clause should be variable "
         "3";
  EXPECT_TRUE(alternative_clauses[2][0].sign())
      << "The first literal in the third alternative clause should be Negative";
}