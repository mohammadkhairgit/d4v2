#include "tests/alternative/problemManager/ProblemManagerFixture.hpp"

int factorial(int n) { return n <= 1 ? 1 : n * factorial(n - 1); }

int binomialCoefficient(int n, int k) {
  if (k > n)
    return 0;
  if (k == 0 || k == n)
    return 1;
  return factorial(n) / (factorial(k) * factorial(n - k));
}

int alternativeClauseToCNFEncodingSize(int n, int k) {
  if (n < 6) {
    return 1 + binomialCoefficient(n, k + 1);
  } else if (k == 1) {
    // 1 for at least one is true, (3 * (n - 2)) three new clauses for each of
    // the new created literals except the first and last one, + 2 for the first
    // and last created literals and their clauses. sequential counter encoding
    // for exactly one true literal.
    return 1 + (3 * (n - 2)) + 2;
  } else {
    // sequential counter encoding for exactly k true literals.
    return (2 * n * k) + (2 * n) - (3 * k) + 1;
  }
}

/**
 * Test the satisfaction of CNF and Alternative clauses under various
 * assignments, while testing the function isSatisfied(vector<d4::lbool>
 * assignment).
 */
TEST_F(ProblemManagerTest, ClausesSatisfactionsAssignments) {
  vector<d4::CNFClause> cnf_clauses;
  vector<d4::AlternativeClause> alternative_clauses;
  for (const auto &clause : problemManager->getClauses()) {
    if (clause->kind() == d4::ClauseKind::Cnf) {
      cnf_clauses.push_back(d4::CNFClause(clause->getLiterals()));
    } else if (clause->kind() == d4::ClauseKind::Alternative) {
      alternative_clauses.push_back(
          d4::AlternativeClause(clause->getLiterals()));
    }
  }

  std::vector<d4::lbool> assignment(problemManager->getNbVar() + 1,
                                    d4::l_Undef);
  EXPECT_FALSE(cnf_clauses[0].isSatisfied(assignment));

  // assignment is -2
  assignment[2] = d4::l_False;
  EXPECT_TRUE(cnf_clauses[0].isSatisfied(assignment));
  EXPECT_TRUE(cnf_clauses[1].isSatisfied(assignment));
  EXPECT_FALSE(cnf_clauses[2].isSatisfied(assignment));
  EXPECT_FALSE(cnf_clauses[3].isSatisfied(assignment));
  EXPECT_FALSE(alternative_clauses[0].isSatisfied(assignment));
  EXPECT_FALSE(alternative_clauses[1].isSatisfied(assignment));
  EXPECT_FALSE(alternative_clauses[2].isSatisfied(assignment));

  // assignment is -2, 3
  assignment[3] = d4::l_True;
  EXPECT_TRUE(cnf_clauses[0].isSatisfied(assignment));
  EXPECT_TRUE(cnf_clauses[1].isSatisfied(assignment));
  EXPECT_FALSE(cnf_clauses[2].isSatisfied(assignment));
  EXPECT_FALSE(cnf_clauses[3].isSatisfied(assignment));
  EXPECT_FALSE(alternative_clauses[0].isSatisfied(assignment));
  EXPECT_FALSE(alternative_clauses[1].isSatisfied(assignment));
  EXPECT_FALSE(alternative_clauses[2].isSatisfied(assignment));

  // assignment is -2, -3
  assignment[3] = d4::l_False;
  EXPECT_TRUE(cnf_clauses[0].isSatisfied(assignment));
  EXPECT_TRUE(cnf_clauses[1].isSatisfied(assignment));
  EXPECT_TRUE(cnf_clauses[2].isSatisfied(assignment));
  EXPECT_TRUE(cnf_clauses[3].isSatisfied(assignment));
  EXPECT_FALSE(alternative_clauses[0].isSatisfied(assignment));
  EXPECT_FALSE(alternative_clauses[1].isSatisfied(assignment));
  EXPECT_TRUE(alternative_clauses[2].isSatisfied(assignment));

  // assignment is 1, -2, -3
  assignment[1] = d4::l_True;
  EXPECT_TRUE(cnf_clauses[0].isSatisfied(assignment));
  EXPECT_TRUE(cnf_clauses[1].isSatisfied(assignment));
  EXPECT_TRUE(cnf_clauses[2].isSatisfied(assignment));
  EXPECT_TRUE(cnf_clauses[3].isSatisfied(assignment));
  EXPECT_TRUE(alternative_clauses[0].isSatisfied(assignment));
  EXPECT_FALSE(alternative_clauses[1].isSatisfied(assignment));
  EXPECT_TRUE(alternative_clauses[2].isSatisfied(assignment));
}

/**
 * Test the satisfaction of CNF and Alternative clauses under various
 * assignments, while testing the function isSatisfied(). using the internal
 * counters of satisfied and unsatisfied literals in the clauses and their
 * increase and decrease functions.
 */
TEST_F(ProblemManagerTest, ClausesSatisfactionsIndirectAssignments) {
  vector<d4::CNFClause> cnf_clauses;
  vector<d4::AlternativeClause> alternative_clauses;
  for (const auto &clause : problemManager->getClauses()) {
    if (clause->kind() == d4::ClauseKind::Cnf) {
      cnf_clauses.push_back(d4::CNFClause(clause->getLiterals()));
    } else if (clause->kind() == d4::ClauseKind::Alternative) {
      alternative_clauses.push_back(
          d4::AlternativeClause(clause->getLiterals()));
    }
  }

  std::vector<d4::lbool> assignment(problemManager->getNbVar() + 1,
                                    d4::l_Undef);
  EXPECT_FALSE(cnf_clauses[0].isSatisfied(assignment));

  // assignment is -2
  // assignment[2] = d4::l_False;
  cnf_clauses[0].incNbSatLit();
  cnf_clauses[1].incNbSatLit();
  alternative_clauses[1].incNbSatLit();
  EXPECT_TRUE(cnf_clauses[0].isSatisfied());
  EXPECT_TRUE(cnf_clauses[1].isSatisfied());
  EXPECT_FALSE(cnf_clauses[2].isSatisfied());
  EXPECT_FALSE(cnf_clauses[3].isSatisfied());
  EXPECT_FALSE(alternative_clauses[0].isSatisfied());
  EXPECT_FALSE(alternative_clauses[1].isSatisfied());
  EXPECT_FALSE(alternative_clauses[2].isSatisfied());

  // assignment is -2, 3
  assignment[3] = d4::l_True;
  cnf_clauses[0].incNbSatLit();
  cnf_clauses[2].incNbUnsatLit();
  cnf_clauses[3].incNbUnsatLit();
  alternative_clauses[0].incNbSatLit();
  alternative_clauses[2].incNbUnsatLit();
  EXPECT_TRUE(cnf_clauses[0].isSatisfied());
  EXPECT_TRUE(cnf_clauses[1].isSatisfied());
  EXPECT_FALSE(cnf_clauses[2].isSatisfied());
  EXPECT_FALSE(cnf_clauses[3].isSatisfied());
  EXPECT_FALSE(alternative_clauses[0].isSatisfied());
  EXPECT_FALSE(alternative_clauses[1].isSatisfied());
  EXPECT_FALSE(alternative_clauses[2].isSatisfied());
  cnf_clauses[0].decNbSatLit();
  cnf_clauses[2].decNbUnsatLit();
  cnf_clauses[3].decNbUnsatLit();
  alternative_clauses[0].decNbSatLit();
  alternative_clauses[2].decNbUnsatLit();

  // assignment is -2, -3
  assignment[3] = d4::l_False;
  cnf_clauses[0].incNbUnsatLit();
  cnf_clauses[2].incNbSatLit();
  cnf_clauses[3].incNbSatLit();
  alternative_clauses[0].incNbUnsatLit();
  alternative_clauses[2].incNbSatLit();
  EXPECT_TRUE(cnf_clauses[0].isSatisfied());
  EXPECT_TRUE(cnf_clauses[1].isSatisfied());
  EXPECT_TRUE(cnf_clauses[2].isSatisfied());
  EXPECT_TRUE(cnf_clauses[3].isSatisfied());
  EXPECT_FALSE(alternative_clauses[0].isSatisfied());
  EXPECT_FALSE(alternative_clauses[1].isSatisfied());
  EXPECT_TRUE(alternative_clauses[2].isSatisfied());

  // assignment is 1, -2, -3
  assignment[1] = d4::l_True;
  cnf_clauses[0].incNbSatLit();
  cnf_clauses[1].incNbSatLit();
  cnf_clauses[2].incNbSatLit();
  alternative_clauses[0].incNbSatLit();
  alternative_clauses[1].incNbSatLit();
  EXPECT_TRUE(cnf_clauses[0].isSatisfied());
  EXPECT_TRUE(cnf_clauses[1].isSatisfied());
  EXPECT_TRUE(cnf_clauses[2].isSatisfied());
  EXPECT_TRUE(cnf_clauses[3].isSatisfied());
  EXPECT_TRUE(alternative_clauses[0].isSatisfied());
  EXPECT_FALSE(alternative_clauses[1].isSatisfied());
  EXPECT_TRUE(alternative_clauses[2].isSatisfied());
}

/**
 * Test the CNF encoding of CNF and Alternative (smaller than 6 variables)
 * clauses, ensuring that the encoding produces the expected number of CNF
 * clauses and that no new variables are created for small alternative clauses.
 * Test the satisfaction of the created CNF clauses under various assignments.
 */
TEST_F(ProblemManagerTest, ClausesToCNFEncoding) {
  vector<d4::CNFClause> cnf_clauses;
  vector<d4::AlternativeClause> alternative_clauses;
  for (const auto &clause : problemManager->getClauses()) {
    if (clause->kind() == d4::ClauseKind::Cnf) {
      cnf_clauses.push_back(d4::CNFClause(clause->getLiterals()));
    } else if (clause->kind() == d4::ClauseKind::Alternative) {
      alternative_clauses.push_back(
          d4::AlternativeClause(clause->getLiterals()));
    }
  }
  vector<vector<d4::CNFClause>> cnf_encodings;
  for (const auto &cnf_clauses : cnf_clauses) {
    vector<vector<d4::Lit>> cnf_clauses_encoding;
    vector<d4::CNFClause> cnf_clauses_result;
    vector<d4::Var> created_vars;
    unsigned next_free_variable_number = problemManager->getNbVar() + 1;
    cnf_clauses.populate_as_cnf_clause(cnf_clauses_encoding, created_vars,
                                       next_free_variable_number);
    EXPECT_EQ(cnf_clauses_encoding.size(), 1)
        << "CNF clause should be encoded as a single CNF clause";
    for (const auto &clauses : cnf_clauses_encoding) {
      cnf_clauses_result.push_back(d4::CNFClause(clauses));
    }
    EXPECT_EQ(cnf_clauses_result.size(), 1)
        << "CNF clause should be encoded as a single CNF clause";

    cnf_encodings.push_back(cnf_clauses_result);
    EXPECT_EQ(next_free_variable_number, problemManager->getNbVar() + 1)
        << "CNF clause should not create any new variables";
  }

  for (const auto &alternative_clauses : alternative_clauses) {
    vector<vector<d4::Lit>> cnf_clauses_encoding;
    vector<d4::CNFClause> cnf_clauses_result;
    vector<d4::Var> created_vars;
    unsigned next_free_variable_number = problemManager->getNbVar() + 1;
    alternative_clauses.populate_as_cnf_clause(
        cnf_clauses_encoding, created_vars, next_free_variable_number);
    EXPECT_GE(cnf_clauses_encoding.size(), 1)
        << "Alternative clause should be encoded as at least one CNF clause";
    for (const auto &clauses : cnf_clauses_encoding) {
      cnf_clauses_result.push_back(d4::CNFClause(clauses));
    }
    EXPECT_GE(cnf_clauses_result.size(), 1)
        << "Alternative clause should be encoded as at least one CNF clause";
    cnf_encodings.push_back(cnf_clauses_result);
    EXPECT_EQ(next_free_variable_number, problemManager->getNbVar() + 1)
        << "Alternative clauses smaller than 6 size should not create any new "
           "variables";
  }

  EXPECT_EQ(cnf_encodings.size(),
            cnf_clauses.size() + alternative_clauses.size())
      << "CNF encodings should have the same number of clauses as the original "
         "clauses";
  EXPECT_EQ(cnf_encodings[0].size(), 1)
      << "CNF clause should be encoded as a single CNF clause";
  EXPECT_EQ(cnf_encodings[1].size(), 1)
      << "CNF clause should be encoded as a single CNF clause";
  EXPECT_EQ(cnf_encodings[2].size(), 1)
      << "CNF clause should be encoded as a single CNF clause";
  EXPECT_EQ(cnf_encodings[3].size(), 1)
      << "CNF clause should be encoded as a single CNF clause";
  EXPECT_EQ(cnf_encodings[4].size(),
            alternativeClauseToCNFEncodingSize(
                alternative_clauses[0].getLiterals().size(), 1))
      << "Alternative clause should be encoded as a number of CNF clauses "
         "equal to the number of pairs of literals";
  EXPECT_EQ(cnf_encodings[5].size(),
            alternativeClauseToCNFEncodingSize(
                alternative_clauses[1].getLiterals().size(), 1))
      << "Alternative clause should be encoded as a number of CNF clauses "
         "equal to the number of pairs of literals";
  EXPECT_EQ(cnf_encodings[6].size(),
            alternativeClauseToCNFEncodingSize(
                alternative_clauses[2].getLiterals().size(), 1))
      << "Alternative clause should be encoded as a number of CNF clauses "
         "equal to the number of pairs of literals";

  std::vector<d4::lbool> assignment(problemManager->getNbVar() + 1,
                                    d4::l_Undef);
  // assignment is -2, -3
  assignment[2] = d4::l_False;
  assignment[3] = d4::l_False;
  EXPECT_TRUE(cnf_encodings[0][0].isSatisfied(assignment));
  EXPECT_TRUE(cnf_encodings[1][0].isSatisfied(assignment));
  EXPECT_TRUE(cnf_encodings[2][0].isSatisfied(assignment));
  EXPECT_TRUE(cnf_encodings[3][0].isSatisfied(assignment));
  int nbSatClauses = 0;
  for (const d4::CNFClause &cnf_clause : cnf_encodings[4]) {
    if (cnf_clause.isSatisfied(assignment)) {
      nbSatClauses++;
    }
  }
  EXPECT_LE(nbSatClauses, alternativeClauseToCNFEncodingSize(
                              alternative_clauses[0].getLiterals().size(), 1))
      << "At least one cnf clause of the first alternative clause (3 1) should "
         "be satisfied by the assignment";
  nbSatClauses = 0;
  for (const d4::CNFClause &cnf_clause : cnf_encodings[5]) {
    if (cnf_clause.isSatisfied(assignment)) {
      nbSatClauses++;
    }
  }
  EXPECT_LE(nbSatClauses, alternativeClauseToCNFEncodingSize(
                              alternative_clauses[1].getLiterals().size(), 1))
      << "At least one cnf clause of the second alternative clause (-2 6 1) "
         "should be satisfied by the assignment";
  nbSatClauses = 0;
  for (const d4::CNFClause &cnf_clause : cnf_encodings[6]) {
    if (cnf_clause.isSatisfied(assignment)) {
      nbSatClauses++;
    }
  }
  EXPECT_EQ(nbSatClauses, alternativeClauseToCNFEncodingSize(
                              alternative_clauses[2].getLiterals().size(), 1))
      << "All cnf clauses of the third alternative clause (-3) should be "
         "satisfied by the assignment";

  // assignment is 1, -2, -3
  assignment[2] = d4::l_False;
  assignment[3] = d4::l_False;
  assignment[1] = d4::l_True;
  EXPECT_TRUE(cnf_encodings[0][0].isSatisfied(assignment));
  EXPECT_TRUE(cnf_encodings[1][0].isSatisfied(assignment));
  EXPECT_TRUE(cnf_encodings[2][0].isSatisfied(assignment));
  EXPECT_TRUE(cnf_encodings[3][0].isSatisfied(assignment));
  nbSatClauses = 0;
  for (const d4::CNFClause &cnf_clause : cnf_encodings[4]) {
    if (cnf_clause.isSatisfied(assignment)) {
      nbSatClauses++;
    }
  }
  EXPECT_EQ(nbSatClauses, alternativeClauseToCNFEncodingSize(
                              alternative_clauses[0].getLiterals().size(), 1))
      << "All cnf clauses of the third alternative clause (-3 1) should be "
         "satisfied by the assignment";
  nbSatClauses = 0;
  for (const d4::CNFClause &cnf_clause : cnf_encodings[5]) {
    if (cnf_clause.isSatisfied(assignment)) {
      nbSatClauses++;
    }
  }
  EXPECT_LE(nbSatClauses, alternativeClauseToCNFEncodingSize(
                              alternative_clauses[1].getLiterals().size(), 1))
      << "At least one cnf clause of the second alternative clause (-2 6 1) "
         "should be satisfied by the assignment";
  nbSatClauses = 0;
  for (const d4::CNFClause &cnf_clause : cnf_encodings[6]) {
    if (cnf_clause.isSatisfied(assignment)) {
      nbSatClauses++;
    }
  }
  EXPECT_EQ(nbSatClauses, alternativeClauseToCNFEncodingSize(
                              alternative_clauses[2].getLiterals().size(), 1))
      << "All cnf clauses of the third alternative clause (-3) should be "
         "satisfied by the assignment";
}

/**
 * Test the CNF encoding of CNF and Alternative (bigger than 5 variables)
 * clauses, ensuring that the encoding produces the expected number of CNF
 * clauses and the correct number of new variables.
 */
TEST_F(ProblemManagerTestBigAlternatives, ClausesToCNFEncodingBigAlternatives) {
  vector<d4::CNFClause> cnf_clauses;
  vector<d4::AlternativeClause> alternative_clauses;
  for (const auto &clause : problemManager->getClauses()) {
    if (clause->kind() == d4::ClauseKind::Cnf) {
      cnf_clauses.push_back(d4::CNFClause(clause->getLiterals()));
    } else if (clause->kind() == d4::ClauseKind::Alternative) {
      alternative_clauses.push_back(
          d4::AlternativeClause(clause->getLiterals()));
    }
  }
  vector<vector<d4::CNFClause>> cnf_encodings;
  for (const auto &cnf_clauses : cnf_clauses) {
    vector<vector<d4::Lit>> cnf_clauses_encoding;
    vector<d4::CNFClause> cnf_clauses_result;
    vector<d4::Var> created_vars;
    unsigned next_free_variable_number = problemManager->getNbVar() + 1;
    cnf_clauses.populate_as_cnf_clause(cnf_clauses_encoding, created_vars,
                                       next_free_variable_number);
    EXPECT_EQ(cnf_clauses_encoding.size(), 1)
        << "CNF clause should be encoded as a single CNF clause";
    for (const auto &clauses : cnf_clauses_encoding) {
      cnf_clauses_result.push_back(d4::CNFClause(clauses));
    }
    EXPECT_EQ(cnf_clauses_result.size(), 1)
        << "CNF clause should be encoded as a single CNF clause";

    cnf_encodings.push_back(cnf_clauses_result);
    EXPECT_EQ(next_free_variable_number, problemManager->getNbVar() + 1)
        << "CNF clause should not create any new variables";
  }

  unsigned next_free_variable_number = problemManager->getNbVar() + 1;
  for (const auto &alternative_clauses : alternative_clauses) {
    vector<vector<d4::Lit>> cnf_clauses_encoding;
    vector<d4::CNFClause> cnf_clauses_result;
    vector<d4::Var> created_vars;
    unsigned current_next_free_variable_number = next_free_variable_number;
    alternative_clauses.populate_as_cnf_clause(
        cnf_clauses_encoding, created_vars, next_free_variable_number);
    EXPECT_GE(cnf_clauses_encoding.size(), 1)
        << "Alternative clause should be encoded as at least one CNF clause";
    for (const auto &clauses : cnf_clauses_encoding) {
      cnf_clauses_result.push_back(d4::CNFClause(clauses));
    }
    EXPECT_GE(cnf_clauses_result.size(), 1)
        << "Alternative clause should be encoded as at least one CNF clause";
    cnf_encodings.push_back(cnf_clauses_result);
    EXPECT_EQ(next_free_variable_number,
              current_next_free_variable_number +
                  alternative_clauses.getLiterals().size() - 1)
        << "Alternative clauses bigger than 6 size should create n - 1 (size "
           "of clause) new variables for sequential counter encoding of "
           "exactly one true literal";
    EXPECT_EQ(created_vars.size(), alternative_clauses.getLiterals().size() - 1)
        << "Alternative clauses bigger than 6 size should create n - 1 (size "
           "of clause) new variables for sequential counter encoding of "
           "exactly one true literal";
    EXPECT_EQ(cnf_clauses_encoding.size(),
              alternativeClauseToCNFEncodingSize(
                  alternative_clauses.getLiterals().size(), 1))
        << "Alternative clause bigger than size 5 should be encoded as a "
           "number of CNF clauses equal 1 + (3 * (n - 2)) + 2 for exactly one "
           "true literal, where n is the number of literals in the alternative "
           "clause";
  }
}