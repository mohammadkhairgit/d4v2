#include "tests/alternative/problemManager/ProblemManagerFixture.hpp"
/** Currently their is not tests for the projected case**/

/** The following test supposedly cover the functions appendClauses, the
 * constructor through makeProblemManager (without projection). COnsidering that
 * buildRemap, normalize, normalizeInner, remapClauses require the projected
 * case for them to be tested, they are not covered in this test. **/
TEST_F(ProblemManagerTest, InitializationCorrectness) {
  vector<vector<d4::Lit>> cnf_clauses;
  vector<vector<d4::Lit>> alternative_clauses;
  for (const auto &clause : problemManager->getClauses()) {
    if (clause->kind() == d4::ClauseKind::Cnf) {
      cnf_clauses.push_back(clause->getLiterals());
    } else if (clause->kind() == d4::ClauseKind::Alternative) {
      alternative_clauses.push_back(clause->getLiterals());
    }
  }
  EXPECT_EQ(cnf_clauses.size(), 4)
      << "Expected 4 CNF clauses, but got " << cnf_clauses.size();
  EXPECT_EQ(alternative_clauses.size(), 3)
      << "Expected 3 Alternative clauses, but got "
      << alternative_clauses.size();
  EXPECT_EQ(problemManager->getNbVar(), 7)
      << "Expected 7 variables, but got " << problemManager->getNbVar();
  EXPECT_EQ(problemManager->getSelectedVar().size(), 0)
      << "Expected 7 selected variables, but got "
      << problemManager->getSelectedVar().size();
  EXPECT_EQ(problemManager->freeVars(), 0)
      << "Expected 0 free variables, but got " << problemManager->freeVars();
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