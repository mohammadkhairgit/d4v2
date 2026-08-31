#include "tests/alternative/specManager/SpecManagerFixture.hpp"

TEST_F(SpecManagerFixture, InitializationCorrectness) {
  vector<vector<d4::Lit>> cnf_clauses;
  vector<vector<d4::Lit>> alternative_clauses;
  EXPECT_EQ(specManager->getNbVariable(), 7)
      << "Expected 7 variables, but got " << specManager->getNbVariable();
  std::vector<unsigned int> nbClauses;
  std::vector<d4::Var> component = {1, 2, 3, 4, 5, 6, 7};
  specManager->getCurrentClauses(nbClauses, component);
  EXPECT_EQ(nbClauses.size(), 7)
      << "Expected 7 clauses, but got " << nbClauses.size();
  std::vector<std::vector<d4::Var>> varConnected;
  std::vector<d4::Var> freeVar;
  EXPECT_EQ(
      specManager->computeConnectedComponent(varConnected, component, freeVar),
      1)
      << "Expected 1 connected component, but got "
      << specManager->computeConnectedComponent(varConnected, component,
                                                freeVar);
  EXPECT_EQ(varConnected.size(), 1);
  EXPECT_EQ(varConnected[0].size(), 6)
      << "Expected 6 variables in the connected component, but got "
      << varConnected[0].size();
  EXPECT_EQ(freeVar.size(), 1)
      << "Expected 1 free variable, but got " << freeVar.size();
  for (d4::Var v : component) {
    EXPECT_FALSE(specManager->varIsAssigned(v))
        << "Variable " << v << " should not be assigned initially.";
  }
  for (unsigned int i : nbClauses) {
    EXPECT_FALSE(specManager->isSatisfiedClause(i))
        << "Clause " << i << " should not be satisfied initially.";
  }
  EXPECT_EQ(specManager->getCurrentSize(0), 5)
      << "Expected first clause to have size 5, but got "
      << specManager->getCurrentSize(0);
  EXPECT_EQ(specManager->getCurrentSize(1), 3)
      << "Expected second clause to have size 3, but got "
      << specManager->getCurrentSize(1);
  EXPECT_EQ(specManager->getCurrentSize(2), 3)
      << "Expected third clause to have size 3, but got "
      << specManager->getCurrentSize(2);
  EXPECT_EQ(specManager->getCurrentSize(3), 2)
      << "Expected fourth clause to have size 2, but got "
      << specManager->getCurrentSize(3);
  EXPECT_EQ(specManager->getCurrentSize(4), 2)
      << "Expected fifth clause to have size 2, but got "
      << specManager->getCurrentSize(4);
  EXPECT_EQ(specManager->getCurrentSize(5), 3)
      << "Expected sixth clause to have size 3, but got "
      << specManager->getCurrentSize(5);
  EXPECT_EQ(specManager->getCurrentSize(6), 1)
      << "Expected seventh clause to have size 1, but got "
      << specManager->getCurrentSize(6);

  EXPECT_EQ(specManager->varIsAssigned(1), false)
      << "Variable 1 should not be assigned after postUpdate.";
  EXPECT_EQ(specManager->getNbOccurrence(d4::Lit::makeLitTrue(1)), 5)
      << "Expected 5 occurrences for literal 1 after postUpdate, but got "
      << specManager->getNbOccurrence(d4::Lit::makeLitTrue(1));
  EXPECT_EQ(specManager->getNbOccurrence(d4::Lit::makeLitFalse(2)), 3)
      << "Expected 3 occurrences for literal -2 after postUpdate, but got "
      << specManager->getNbOccurrence(d4::Lit::makeLitFalse(2));
  EXPECT_EQ(specManager->getNbOccurrence(d4::Lit::makeLitTrue(3)), 2)
      << "Expected 2 occurrences for literal 3 after postUpdate, but got "
      << specManager->getNbOccurrence(d4::Lit::makeLitTrue(3));
  EXPECT_EQ(specManager->getNbOccurrence(d4::Lit::makeLitFalse(3)), 3)
      << "Expected 3 occurrences for literal -3 after postUpdate, but got "
      << specManager->getNbOccurrence(d4::Lit::makeLitFalse(3));
  EXPECT_EQ(specManager->getNbOccurrence(d4::Lit::makeLitTrue(4)), 1)
      << "Expected 1 occurrence for literal 4 after postUpdate, but got "
      << specManager->getNbOccurrence(d4::Lit::makeLitTrue(4));
  EXPECT_EQ(specManager->getNbOccurrence(d4::Lit::makeLitFalse(4)), 0)
      << "Expected 0 occurrences for literal -4 after postUpdate, but got "
      << specManager->getNbOccurrence(d4::Lit::makeLitFalse(4));
  EXPECT_EQ(specManager->getNbOccurrence(d4::Lit::makeLitTrue(5)), 2)
      << "Expected 2 occurrences for literal 5 after postUpdate, but got "
      << specManager->getNbOccurrence(d4::Lit::makeLitTrue(5));
  EXPECT_EQ(specManager->getNbOccurrence(d4::Lit::makeLitFalse(5)), 0)
      << "Expected 0 occurrences for literal -5 after postUpdate, but got "
      << specManager->getNbOccurrence(d4::Lit::makeLitFalse(5));
  EXPECT_EQ(specManager->getNbOccurrence(d4::Lit::makeLitTrue(6)), 3)
      << "Expected 3 occurrences for literal 6 after postUpdate, but got "
      << specManager->getNbOccurrence(d4::Lit::makeLitTrue(6));
  EXPECT_EQ(specManager->getNbOccurrence(d4::Lit::makeLitFalse(6)), 0)
      << "Expected 0 occurrences for literal -6 after postUpdate, but got "
      << specManager->getNbOccurrence(d4::Lit::makeLitFalse(6));
  EXPECT_EQ(specManager->getNbOccurrence(d4::Lit::makeLitTrue(7)), 0)
      << "Expected 1 occurrence for literal 7 after postUpdate, but got "
      << specManager->getNbOccurrence(d4::Lit::makeLitTrue(7));
  EXPECT_EQ(specManager->getNbOccurrence(d4::Lit::makeLitFalse(7)), 0)
      << "Expected 1 occurrence for literal -7 after postUpdate, but got "
      << specManager->getNbOccurrence(d4::Lit::makeLitFalse(7));
}

TEST_F(SpecManagerFixture, PrePostUpdatesSimpleAndConnectedComponents) {
  vector<d4::Lit> assignedLits = {d4::Lit::makeLitTrue(1)};
  specManager->preUpdate(assignedLits);
  EXPECT_EQ(specManager->varIsAssigned(1), true)
      << "Variable 1 should be assigned after preUpdate.";
  std::vector<d4::Var> component = {1, 2, 3, 4, 5, 6, 7};
  std::vector<std::vector<d4::Var>> varConnected;
  std::vector<d4::Var> freeVar;
  EXPECT_EQ(
      specManager->computeConnectedComponent(varConnected, component, freeVar),
      1)
      << "Expected 1 connected component after preUpdate, but got "
      << specManager->computeConnectedComponent(varConnected, component,
                                                freeVar);
  EXPECT_EQ(varConnected.size(), 1);
  EXPECT_EQ(varConnected[0].size(), 3)
      << "Expected 3 variables in the connected component after preUpdate, but "
         "got "
      << varConnected[0].size();
  EXPECT_EQ(freeVar.size(), 3)
      << "Expected 3 free variables after preUpdate, but got "
      << freeVar.size();
  EXPECT_EQ(specManager->getNbOccurrence(d4::Lit::makeLitTrue(1)), 5)
      << "Expected 5 occurrences for literal 1 after preUpdate, but got "
      << specManager->getNbOccurrence(d4::Lit::makeLitTrue(1));
  EXPECT_EQ(specManager->getNbOccurrence(d4::Lit::makeLitFalse(2)), 1)
      << "Expected 1 occurrence for literal -2 after preUpdate, but got "
      << specManager->getNbOccurrence(d4::Lit::makeLitFalse(2));
  EXPECT_EQ(specManager->getNbOccurrence(d4::Lit::makeLitTrue(3)), 1)
      << "Expected 1 occurrence for literal 3 after preUpdate, but got "
      << specManager->getNbOccurrence(d4::Lit::makeLitTrue(3));
  EXPECT_EQ(specManager->getNbOccurrence(d4::Lit::makeLitFalse(3)), 2)
      << "Expected 2 occurrences for literal -3 after preUpdate, but got "
      << specManager->getNbOccurrence(d4::Lit::makeLitFalse(3));
  EXPECT_EQ(specManager->getNbOccurrence(d4::Lit::makeLitTrue(4)), 0)
      << "Expected 0 occurrences for literal 4 after preUpdate, but got "
      << specManager->getNbOccurrence(d4::Lit::makeLitTrue(4));
  EXPECT_EQ(specManager->getNbOccurrence(d4::Lit::makeLitFalse(4)), 0)
      << "Expected 0 occurrences for literal -4 after preUpdate, but got "
      << specManager->getNbOccurrence(d4::Lit::makeLitFalse(4));
  EXPECT_EQ(specManager->getNbOccurrence(d4::Lit::makeLitTrue(5)), 0)
      << "Expected 0 occurrences for literal 5 after preUpdate, but got "
      << specManager->getNbOccurrence(d4::Lit::makeLitTrue(5));
  EXPECT_EQ(specManager->getNbOccurrence(d4::Lit::makeLitFalse(5)), 0)
      << "Expected 0 occurrences for literal -5 after preUpdate, but got "
      << specManager->getNbOccurrence(d4::Lit::makeLitFalse(5));
  EXPECT_EQ(specManager->getNbOccurrence(d4::Lit::makeLitTrue(6)), 2)
      << "Expected 2 occurrences for literal 6 after preUpdate, but got "
      << specManager->getNbOccurrence(d4::Lit::makeLitTrue(6));
  EXPECT_EQ(specManager->getNbOccurrence(d4::Lit::makeLitFalse(6)), 0)
      << "Expected 0 occurrences for literal -6 after preUpdate, but got "
      << specManager->getNbOccurrence(d4::Lit::makeLitFalse(6));
  EXPECT_EQ(specManager->getNbOccurrence(d4::Lit::makeLitFalse(7)), 0)
      << "Expected 0 occurrences for literal -7 after preUpdate, but got "
      << specManager->getNbOccurrence(d4::Lit::makeLitFalse(7));
  specManager->postUpdate(assignedLits);
  EXPECT_EQ(specManager->varIsAssigned(1), false)
      << "Variable 1 should not be assigned after postUpdate.";
  EXPECT_EQ(specManager->getNbOccurrence(d4::Lit::makeLitTrue(1)), 5)
      << "Expected 5 occurrences for literal 1 after postUpdate, but got "
      << specManager->getNbOccurrence(d4::Lit::makeLitTrue(1));
  EXPECT_EQ(specManager->getNbOccurrence(d4::Lit::makeLitFalse(2)), 3)
      << "Expected 3 occurrences for literal -2 after postUpdate, but got "
      << specManager->getNbOccurrence(d4::Lit::makeLitFalse(2));
  EXPECT_EQ(specManager->getNbOccurrence(d4::Lit::makeLitTrue(3)), 2)
      << "Expected 2 occurrences for literal 3 after postUpdate, but got "
      << specManager->getNbOccurrence(d4::Lit::makeLitTrue(3));
  EXPECT_EQ(specManager->getNbOccurrence(d4::Lit::makeLitFalse(3)), 3)
      << "Expected 3 occurrences for literal -3 after postUpdate, but got "
      << specManager->getNbOccurrence(d4::Lit::makeLitFalse(3));
  EXPECT_EQ(specManager->getNbOccurrence(d4::Lit::makeLitTrue(4)), 1)
      << "Expected 1 occurrence for literal 4 after postUpdate, but got "
      << specManager->getNbOccurrence(d4::Lit::makeLitTrue(4));
  EXPECT_EQ(specManager->getNbOccurrence(d4::Lit::makeLitFalse(4)), 0)
      << "Expected 0 occurrences for literal -4 after postUpdate, but got "
      << specManager->getNbOccurrence(d4::Lit::makeLitFalse(4));
  EXPECT_EQ(specManager->getNbOccurrence(d4::Lit::makeLitTrue(5)), 2)
      << "Expected 2 occurrences for literal 5 after postUpdate, but got "
      << specManager->getNbOccurrence(d4::Lit::makeLitTrue(5));
  EXPECT_EQ(specManager->getNbOccurrence(d4::Lit::makeLitFalse(5)), 0)
      << "Expected 0 occurrences for literal -5 after postUpdate, but got "
      << specManager->getNbOccurrence(d4::Lit::makeLitFalse(5));
  EXPECT_EQ(specManager->getNbOccurrence(d4::Lit::makeLitTrue(6)), 3)
      << "Expected 3 occurrences for literal 6 after postUpdate, but got "
      << specManager->getNbOccurrence(d4::Lit::makeLitTrue(6));
  EXPECT_EQ(specManager->getNbOccurrence(d4::Lit::makeLitFalse(6)), 0)
      << "Expected 0 occurrences for literal -6 after postUpdate, but got "
      << specManager->getNbOccurrence(d4::Lit::makeLitFalse(6));
  EXPECT_EQ(specManager->getNbOccurrence(d4::Lit::makeLitTrue(7)), 0)
      << "Expected 1 occurrence for literal 7 after postUpdate, but got "
      << specManager->getNbOccurrence(d4::Lit::makeLitTrue(7));
  EXPECT_EQ(specManager->getNbOccurrence(d4::Lit::makeLitFalse(7)), 0)
      << "Expected 1 occurrence for literal -7 after postUpdate, but got "
      << specManager->getNbOccurrence(d4::Lit::makeLitFalse(7));
}

TEST_F(ConnectedFormulaSpecManagerFixture,
       PrePostUpdatesComplexAndConnectedComponents) {

  // Correct initialization
  std::vector<d4::Var> component = {1, 2, 3, 4, 5, 6, 7, 8, 9};
  std::vector<std::vector<d4::Var>> varConnected;
  std::vector<d4::Var> freeVar;

  std::vector<d4::Lit> assignedLits = {d4::Lit::makeLitTrue(4)};

  // PreUpdate
  specManager->preUpdate(assignedLits);

  // Connected Components

  component = {1, 2, 3, 4, 5, 6, 7, 8, 9};
  varConnected = {};
  freeVar = {};

  EXPECT_EQ(
      specManager->computeConnectedComponent(varConnected, component, freeVar),
      2)
      << "Expected 2 connected components after preUpdate, but got "
      << specManager->computeConnectedComponent(varConnected, component,
                                                freeVar);
  EXPECT_EQ(varConnected.size(), 2);

  EXPECT_TRUE(varConnected[0].size() == 2 || varConnected[1].size() == 2)
      << "Expected 2 variables in a connected component after preUpdate, but "
         "got "
      << varConnected[0].size() << " and " << varConnected[1].size();

  EXPECT_TRUE(varConnected[0].size() == 6 || varConnected[1].size() == 6)
      << "Expected 6 variables in a connected component after preUpdate, but "
         "got "
      << varConnected[0].size() << " and " << varConnected[1].size();
  EXPECT_EQ(freeVar.size(), 0)
      << "Expected 0 free variables after preUpdate, but got "
      << freeVar.size();

  // Literals Checks

  EXPECT_EQ(specManager->varIsAssigned(4), true)
      << "Variable 4 should be assigned after preUpdate.";
  EXPECT_EQ(specManager->getNbOccurrence(d4::Lit::makeLitTrue(1)), 2)
      << "Expected 2 occurrences for literal 1 after preUpdate, but got "
      << specManager->getNbOccurrence(d4::Lit::makeLitTrue(1));
  EXPECT_EQ(specManager->getNbOccurrence(d4::Lit::makeLitFalse(1)), 2)
      << "Expected 2 occurrences for literal 1 after preUpdate, but got "
      << specManager->getNbOccurrence(d4::Lit::makeLitFalse(1));

  EXPECT_EQ(specManager->getNbOccurrence(d4::Lit::makeLitTrue(2)), 4)
      << "Expected 4 occurrences for literal 2 after preUpdate, but got "
      << specManager->getNbOccurrence(d4::Lit::makeLitTrue(2));
  EXPECT_EQ(specManager->getNbOccurrence(d4::Lit::makeLitFalse(2)), 0)
      << "Expected 0 occurrence for literal -2 after preUpdate, but got "
      << specManager->getNbOccurrence(d4::Lit::makeLitFalse(2));

  EXPECT_EQ(specManager->getNbOccurrence(d4::Lit::makeLitTrue(3)), 1)
      << "Expected 1 occurrence for literal 3 after preUpdate, but got "
      << specManager->getNbOccurrence(d4::Lit::makeLitTrue(3));
  EXPECT_EQ(specManager->getNbOccurrence(d4::Lit::makeLitFalse(3)), 1)
      << "Expected 1 occurrence for literal -3 after preUpdate, but got "
      << specManager->getNbOccurrence(d4::Lit::makeLitFalse(3));

  EXPECT_EQ(specManager->getNbOccurrence(d4::Lit::makeLitTrue(4)), 2)
      << "Expected 2 occurrences for literal 4 after preUpdate, but got "
      << specManager->getNbOccurrence(d4::Lit::makeLitTrue(4));
  EXPECT_EQ(specManager->getNbOccurrence(d4::Lit::makeLitFalse(4)), 3)
      << "Expected 3 occurrences for literal -4 after preUpdate, but got "
      << specManager->getNbOccurrence(d4::Lit::makeLitFalse(4));

  EXPECT_EQ(specManager->getNbOccurrence(d4::Lit::makeLitTrue(5)), 2)
      << "Expected 2 occurrences for literal 5 after preUpdate, but got "
      << specManager->getNbOccurrence(d4::Lit::makeLitTrue(5));
  EXPECT_EQ(specManager->getNbOccurrence(d4::Lit::makeLitFalse(5)), 2)
      << "Expected 2 occurrences for literal -5 after preUpdate, but got "
      << specManager->getNbOccurrence(d4::Lit::makeLitFalse(5));

  EXPECT_EQ(specManager->getNbOccurrence(d4::Lit::makeLitTrue(6)), 1)
      << "Expected 1 occurrence for literal 6 after preUpdate, but got "
      << specManager->getNbOccurrence(d4::Lit::makeLitTrue(6));
  EXPECT_EQ(specManager->getNbOccurrence(d4::Lit::makeLitFalse(6)), 1)
      << "Expected 1 occurrence for literal -6 after preUpdate, but got "
      << specManager->getNbOccurrence(d4::Lit::makeLitFalse(6));

  EXPECT_EQ(specManager->getNbOccurrence(d4::Lit::makeLitTrue(7)), 1)
      << "Expected 1 occurrence for literal 7 after preUpdate, but got "
      << specManager->getNbOccurrence(d4::Lit::makeLitTrue(7));
  EXPECT_EQ(specManager->getNbOccurrence(d4::Lit::makeLitFalse(7)), 1)
      << "Expected 1 occurrence for literal -7 after preUpdate, but got "
      << specManager->getNbOccurrence(d4::Lit::makeLitFalse(7));

  EXPECT_EQ(specManager->getNbOccurrence(d4::Lit::makeLitTrue(8)), 1)
      << "Expected 1 occurrence for literal 8 after preUpdate, but got "
      << specManager->getNbOccurrence(d4::Lit::makeLitTrue(8));
  EXPECT_EQ(specManager->getNbOccurrence(d4::Lit::makeLitFalse(8)), 1)
      << "Expected 1 occurrence for literal -8 after preUpdate, but got "
      << specManager->getNbOccurrence(d4::Lit::makeLitFalse(8));

  EXPECT_EQ(specManager->getNbOccurrence(d4::Lit::makeLitTrue(9)), 2)
      << "Expected 2 occurrences for literal 9 after preUpdate, but got "
      << specManager->getNbOccurrence(d4::Lit::makeLitTrue(9));
  EXPECT_EQ(specManager->getNbOccurrence(d4::Lit::makeLitFalse(9)), 0)
      << "Expected 0 occurrences for literal -9 after preUpdate, but got "
      << specManager->getNbOccurrence(d4::Lit::makeLitFalse(9));

  // PostUpdate
  specManager->postUpdate(assignedLits);

  EXPECT_EQ(specManager->getNbOccurrence(d4::Lit::makeLitTrue(1)), 2)
      << "Expected 2 occurrences for literal 1 after preUpdate, but got "
      << specManager->getNbOccurrence(d4::Lit::makeLitTrue(1));
  EXPECT_EQ(specManager->getNbOccurrence(d4::Lit::makeLitFalse(1)), 2)
      << "Expected 2 occurrences for literal 1 after preUpdate, but got "
      << specManager->getNbOccurrence(d4::Lit::makeLitFalse(1));

  EXPECT_EQ(specManager->getNbOccurrence(d4::Lit::makeLitTrue(2)), 4)
      << "Expected 4 occurrences for literal 2 after preUpdate, but got "
      << specManager->getNbOccurrence(d4::Lit::makeLitTrue(2));
  EXPECT_EQ(specManager->getNbOccurrence(d4::Lit::makeLitFalse(2)), 0)
      << "Expected 0 occurrence for literal -2 after preUpdate, but got "
      << specManager->getNbOccurrence(d4::Lit::makeLitFalse(2));

  EXPECT_EQ(specManager->getNbOccurrence(d4::Lit::makeLitTrue(3)), 1)
      << "Expected 1 occurrence for literal 3 after preUpdate, but got "
      << specManager->getNbOccurrence(d4::Lit::makeLitTrue(3));
  EXPECT_EQ(specManager->getNbOccurrence(d4::Lit::makeLitFalse(3)), 1)
      << "Expected 1 occurrence for literal -3 after preUpdate, but got "
      << specManager->getNbOccurrence(d4::Lit::makeLitFalse(3));

  EXPECT_EQ(specManager->getNbOccurrence(d4::Lit::makeLitTrue(4)), 2)
      << "Expected 2 occurrences for literal 4 after preUpdate, but got "
      << specManager->getNbOccurrence(d4::Lit::makeLitTrue(4));
  EXPECT_EQ(specManager->getNbOccurrence(d4::Lit::makeLitFalse(4)), 3)
      << "Expected 3 occurrences for literal -4 after preUpdate, but got "
      << specManager->getNbOccurrence(d4::Lit::makeLitFalse(4));

  EXPECT_EQ(specManager->getNbOccurrence(d4::Lit::makeLitTrue(5)), 2)
      << "Expected 2 occurrences for literal 5 after preUpdate, but got "
      << specManager->getNbOccurrence(d4::Lit::makeLitTrue(5));
  EXPECT_EQ(specManager->getNbOccurrence(d4::Lit::makeLitFalse(5)), 2)
      << "Expected 2 occurrences for literal -5 after preUpdate, but got "
      << specManager->getNbOccurrence(d4::Lit::makeLitFalse(5));

  EXPECT_EQ(specManager->getNbOccurrence(d4::Lit::makeLitTrue(6)), 1)
      << "Expected 1 occurrence for literal 6 after preUpdate, but got "
      << specManager->getNbOccurrence(d4::Lit::makeLitTrue(6));
  EXPECT_EQ(specManager->getNbOccurrence(d4::Lit::makeLitFalse(6)), 1)
      << "Expected 1 occurrence for literal -6 after preUpdate, but got "
      << specManager->getNbOccurrence(d4::Lit::makeLitFalse(6));

  EXPECT_EQ(specManager->getNbOccurrence(d4::Lit::makeLitTrue(7)), 1)
      << "Expected 1 occurrence for literal 7 after preUpdate, but got "
      << specManager->getNbOccurrence(d4::Lit::makeLitTrue(7));
  EXPECT_EQ(specManager->getNbOccurrence(d4::Lit::makeLitFalse(7)), 1)
      << "Expected 1 occurrence for literal -7 after preUpdate, but got "
      << specManager->getNbOccurrence(d4::Lit::makeLitFalse(7));

  EXPECT_EQ(specManager->getNbOccurrence(d4::Lit::makeLitTrue(8)), 1)
      << "Expected 1 occurrence for literal 8 after preUpdate, but got "
      << specManager->getNbOccurrence(d4::Lit::makeLitTrue(8));
  EXPECT_EQ(specManager->getNbOccurrence(d4::Lit::makeLitFalse(8)), 1)
      << "Expected 1 occurrence for literal -8 after preUpdate, but got "
      << specManager->getNbOccurrence(d4::Lit::makeLitFalse(8));

  EXPECT_EQ(specManager->getNbOccurrence(d4::Lit::makeLitTrue(9)), 2)
      << "Expected 2 occurrences for literal 9 after preUpdate, but got "
      << specManager->getNbOccurrence(d4::Lit::makeLitTrue(9));
  EXPECT_EQ(specManager->getNbOccurrence(d4::Lit::makeLitFalse(9)), 0)
      << "Expected 0 occurrences for literal -9 after preUpdate, but got "
      << specManager->getNbOccurrence(d4::Lit::makeLitFalse(9));

  assignedLits = {d4::Lit::makeLitFalse(1), d4::Lit::makeLitTrue(4),
                  d4::Lit::makeLitFalse(5)};

  // PreUpdate
  specManager->preUpdate(assignedLits);

  // Connected Components

  component = {1, 2, 3, 4, 5, 6, 7, 8, 9};
  varConnected = {};
  freeVar = {};

  EXPECT_EQ(
      specManager->computeConnectedComponent(varConnected, component, freeVar),
      3)
      << "Expected 3 connected components after preUpdate, but got "
      << specManager->computeConnectedComponent(varConnected, component,
                                                freeVar);
  EXPECT_EQ(varConnected.size(), 3);

  // Two component should have one variable remaining
  EXPECT_TRUE(varConnected[0].size() == 1 || varConnected[1].size() == 1)
      << "Expected 1 variables in a connected component after preUpdate, but "
         "got "
      << varConnected[0].size() << " and " << varConnected[1].size() << " and "
      << varConnected[2].size();
  EXPECT_TRUE(varConnected[1].size() == 1 || varConnected[2].size() == 1)
      << "Expected 1 variables in a connected component after preUpdate, but "
         "got "
      << varConnected[0].size() << " and " << varConnected[1].size() << " and "
      << varConnected[2].size();
  EXPECT_TRUE(varConnected[0].size() == 1 || varConnected[2].size() == 1)
      << "Expected 1 variables in a connected component after preUpdate, but "
         "got "
      << varConnected[0].size() << " and " << varConnected[1].size() << " and "
      << varConnected[2].size();

  EXPECT_TRUE(varConnected[0].size() == 4 || varConnected[1].size() == 4 ||
              varConnected[2].size() == 4)
      << "Expected 4 variables in a connected component after preUpdate, but "
         "got "
      << varConnected[0].size() << " and " << varConnected[1].size() << " and "
      << varConnected[2].size();
  EXPECT_EQ(freeVar.size(), 0)
      << "Expected 0 free variables after preUpdate, but got "
      << freeVar.size();

  // Literals Checks
  EXPECT_EQ(specManager->varIsAssigned(1), true)
      << "Variable 1 should be assigned after preUpdate.";
  EXPECT_EQ(specManager->varIsAssigned(4), true)
      << "Variable 4 should be assigned after preUpdate.";
  EXPECT_EQ(specManager->varIsAssigned(5), true)
      << "Variable 5 should be assigned after preUpdate.";
  EXPECT_EQ(specManager->getNbOccurrence(d4::Lit::makeLitTrue(1)), 2)
      << "Expected 2 occurrences for literal 1 after preUpdate, but got "
      << specManager->getNbOccurrence(d4::Lit::makeLitTrue(1));
  EXPECT_EQ(specManager->getNbOccurrence(d4::Lit::makeLitFalse(1)), 2)
      << "Expected 2 occurrences for literal 1 after preUpdate, but got "
      << specManager->getNbOccurrence(d4::Lit::makeLitFalse(1));

  EXPECT_EQ(specManager->getNbOccurrence(d4::Lit::makeLitTrue(2)), 3)
      << "Expected 3 occurrences for literal 2 after preUpdate, but got "
      << specManager->getNbOccurrence(d4::Lit::makeLitTrue(2));
  EXPECT_EQ(specManager->getNbOccurrence(d4::Lit::makeLitFalse(2)), 0)
      << "Expected 0 occurrence for literal -2 after preUpdate, but got "
      << specManager->getNbOccurrence(d4::Lit::makeLitFalse(2));

  EXPECT_EQ(specManager->getNbOccurrence(d4::Lit::makeLitTrue(3)), 1)
      << "Expected 1 occurrence for literal 3 after preUpdate, but got "
      << specManager->getNbOccurrence(d4::Lit::makeLitTrue(3));
  EXPECT_EQ(specManager->getNbOccurrence(d4::Lit::makeLitFalse(3)), 0)
      << "Expected 0 occurrence for literal -3 after preUpdate, but got "
      << specManager->getNbOccurrence(d4::Lit::makeLitFalse(3));

  EXPECT_EQ(specManager->getNbOccurrence(d4::Lit::makeLitTrue(4)), 2)
      << "Expected 2 occurrences for literal 4 after preUpdate, but got "
      << specManager->getNbOccurrence(d4::Lit::makeLitTrue(4));
  EXPECT_EQ(specManager->getNbOccurrence(d4::Lit::makeLitFalse(4)), 2)
      << "Expected 2 occurrences for literal -4 after preUpdate, but got "
      << specManager->getNbOccurrence(d4::Lit::makeLitFalse(4));

  EXPECT_EQ(specManager->getNbOccurrence(d4::Lit::makeLitTrue(5)), 2)
      << "Expected 2 occurrences for literal 5 after preUpdate, but got "
      << specManager->getNbOccurrence(d4::Lit::makeLitTrue(5));
  EXPECT_EQ(specManager->getNbOccurrence(d4::Lit::makeLitFalse(5)), 2)
      << "Expected 2 occurrences for literal -5 after preUpdate, but got "
      << specManager->getNbOccurrence(d4::Lit::makeLitFalse(5));

  EXPECT_EQ(specManager->getNbOccurrence(d4::Lit::makeLitTrue(6)), 1)
      << "Expected 1 occurrence for literal 6 after preUpdate, but got "
      << specManager->getNbOccurrence(d4::Lit::makeLitTrue(6));
  EXPECT_EQ(specManager->getNbOccurrence(d4::Lit::makeLitFalse(6)), 1)
      << "Expected 1 occurrence for literal -6 after preUpdate, but got "
      << specManager->getNbOccurrence(d4::Lit::makeLitFalse(6));

  EXPECT_EQ(specManager->getNbOccurrence(d4::Lit::makeLitTrue(7)), 1)
      << "Expected 1 occurrence for literal 7 after preUpdate, but got "
      << specManager->getNbOccurrence(d4::Lit::makeLitTrue(7));
  EXPECT_EQ(specManager->getNbOccurrence(d4::Lit::makeLitFalse(7)), 1)
      << "Expected 1 occurrence for literal -7 after preUpdate, but got "
      << specManager->getNbOccurrence(d4::Lit::makeLitFalse(7));

  EXPECT_EQ(specManager->getNbOccurrence(d4::Lit::makeLitTrue(8)), 1)
      << "Expected 1 occurrence for literal 8 after preUpdate, but got "
      << specManager->getNbOccurrence(d4::Lit::makeLitTrue(8));
  EXPECT_EQ(specManager->getNbOccurrence(d4::Lit::makeLitFalse(8)), 1)
      << "Expected 1 occurrence for literal -8 after preUpdate, but got "
      << specManager->getNbOccurrence(d4::Lit::makeLitFalse(8));

  EXPECT_EQ(specManager->getNbOccurrence(d4::Lit::makeLitTrue(9)), 2)
      << "Expected 2 occurrences for literal 9 after preUpdate, but got "
      << specManager->getNbOccurrence(d4::Lit::makeLitTrue(9));
  EXPECT_EQ(specManager->getNbOccurrence(d4::Lit::makeLitFalse(9)), 0)
      << "Expected 0 occurrences for literal -9 after preUpdate, but got "
      << specManager->getNbOccurrence(d4::Lit::makeLitFalse(9));

  // PostUpdate
  specManager->postUpdate(assignedLits);

  EXPECT_EQ(specManager->getNbOccurrence(d4::Lit::makeLitTrue(1)), 2)
      << "Expected 2 occurrences for literal 1 after preUpdate, but got "
      << specManager->getNbOccurrence(d4::Lit::makeLitTrue(1));
  EXPECT_EQ(specManager->getNbOccurrence(d4::Lit::makeLitFalse(1)), 2)
      << "Expected 2 occurrences for literal 1 after preUpdate, but got "
      << specManager->getNbOccurrence(d4::Lit::makeLitFalse(1));

  EXPECT_EQ(specManager->getNbOccurrence(d4::Lit::makeLitTrue(2)), 4)
      << "Expected 4 occurrences for literal 2 after preUpdate, but got "
      << specManager->getNbOccurrence(d4::Lit::makeLitTrue(2));
  EXPECT_EQ(specManager->getNbOccurrence(d4::Lit::makeLitFalse(2)), 0)
      << "Expected 0 occurrence for literal -2 after preUpdate, but got "
      << specManager->getNbOccurrence(d4::Lit::makeLitFalse(2));

  EXPECT_EQ(specManager->getNbOccurrence(d4::Lit::makeLitTrue(3)), 1)
      << "Expected 1 occurrence for literal 3 after preUpdate, but got "
      << specManager->getNbOccurrence(d4::Lit::makeLitTrue(3));
  EXPECT_EQ(specManager->getNbOccurrence(d4::Lit::makeLitFalse(3)), 1)
      << "Expected 1 occurrence for literal -3 after preUpdate, but got "
      << specManager->getNbOccurrence(d4::Lit::makeLitFalse(3));

  EXPECT_EQ(specManager->getNbOccurrence(d4::Lit::makeLitTrue(4)), 2)
      << "Expected 2 occurrences for literal 4 after preUpdate, but got "
      << specManager->getNbOccurrence(d4::Lit::makeLitTrue(4));
  EXPECT_EQ(specManager->getNbOccurrence(d4::Lit::makeLitFalse(4)), 3)
      << "Expected 3 occurrences for literal -4 after preUpdate, but got "
      << specManager->getNbOccurrence(d4::Lit::makeLitFalse(4));

  EXPECT_EQ(specManager->getNbOccurrence(d4::Lit::makeLitTrue(5)), 2)
      << "Expected 2 occurrences for literal 5 after preUpdate, but got "
      << specManager->getNbOccurrence(d4::Lit::makeLitTrue(5));
  EXPECT_EQ(specManager->getNbOccurrence(d4::Lit::makeLitFalse(5)), 2)
      << "Expected 2 occurrences for literal -5 after preUpdate, but got "
      << specManager->getNbOccurrence(d4::Lit::makeLitFalse(5));

  EXPECT_EQ(specManager->getNbOccurrence(d4::Lit::makeLitTrue(6)), 1)
      << "Expected 1 occurrence for literal 6 after preUpdate, but got "
      << specManager->getNbOccurrence(d4::Lit::makeLitTrue(6));
  EXPECT_EQ(specManager->getNbOccurrence(d4::Lit::makeLitFalse(6)), 1)
      << "Expected 1 occurrence for literal -6 after preUpdate, but got "
      << specManager->getNbOccurrence(d4::Lit::makeLitFalse(6));

  EXPECT_EQ(specManager->getNbOccurrence(d4::Lit::makeLitTrue(7)), 1)
      << "Expected 1 occurrence for literal 7 after preUpdate, but got "
      << specManager->getNbOccurrence(d4::Lit::makeLitTrue(7));
  EXPECT_EQ(specManager->getNbOccurrence(d4::Lit::makeLitFalse(7)), 1)
      << "Expected 1 occurrence for literal -7 after preUpdate, but got "
      << specManager->getNbOccurrence(d4::Lit::makeLitFalse(7));

  EXPECT_EQ(specManager->getNbOccurrence(d4::Lit::makeLitTrue(8)), 1)
      << "Expected 1 occurrence for literal 8 after preUpdate, but got "
      << specManager->getNbOccurrence(d4::Lit::makeLitTrue(8));
  EXPECT_EQ(specManager->getNbOccurrence(d4::Lit::makeLitFalse(8)), 1)
      << "Expected 1 occurrence for literal -8 after preUpdate, but got "
      << specManager->getNbOccurrence(d4::Lit::makeLitFalse(8));

  EXPECT_EQ(specManager->getNbOccurrence(d4::Lit::makeLitTrue(9)), 2)
      << "Expected 2 occurrences for literal 9 after preUpdate, but got "
      << specManager->getNbOccurrence(d4::Lit::makeLitTrue(9));
  EXPECT_EQ(specManager->getNbOccurrence(d4::Lit::makeLitFalse(9)), 0)
      << "Expected 0 occurrences for literal -9 after preUpdate, but got "
      << specManager->getNbOccurrence(d4::Lit::makeLitFalse(9));
}

TEST_F(ConnectedFormulaSpecManagerFixture,
       PrePostUpdatesComplexAndConnectedComponentsSequential) {
  // Correct initialization
  std::vector<d4::Var> component = {1, 2, 3, 4, 5, 6, 7, 8, 9};
  std::vector<std::vector<d4::Var>> varConnected;
  std::vector<d4::Var> freeVar;

  std::vector<d4::Lit> assignedLits;

  // PreUpdate
  assignedLits = {d4::Lit::makeLitFalse(1)};
  specManager->preUpdate(assignedLits);
  assignedLits = {d4::Lit::makeLitTrue(4)};
  specManager->preUpdate(assignedLits);
  assignedLits = {d4::Lit::makeLitFalse(5)};
  specManager->preUpdate(assignedLits);
  // Connected Components

  component = {1, 2, 3, 4, 5, 6, 7, 8, 9};
  varConnected = {};
  freeVar = {};

  EXPECT_EQ(
      specManager->computeConnectedComponent(varConnected, component, freeVar),
      3)
      << "Expected 3 connected components after preUpdate, but got "
      << specManager->computeConnectedComponent(varConnected, component,
                                                freeVar);
  EXPECT_EQ(varConnected.size(), 3);

  // Two component should have one variable remaining
  EXPECT_TRUE(varConnected[0].size() == 1 || varConnected[1].size() == 1)
      << "Expected 1 variables in a connected component after preUpdate, but "
         "got "
      << varConnected[0].size() << " and " << varConnected[1].size() << " and "
      << varConnected[2].size();
  EXPECT_TRUE(varConnected[1].size() == 1 || varConnected[2].size() == 1)
      << "Expected 1 variables in a connected component after preUpdate, but "
         "got "
      << varConnected[0].size() << " and " << varConnected[1].size() << " and "
      << varConnected[2].size();
  EXPECT_TRUE(varConnected[0].size() == 1 || varConnected[2].size() == 1)
      << "Expected 1 variables in a connected component after preUpdate, but "
         "got "
      << varConnected[0].size() << " and " << varConnected[1].size() << " and "
      << varConnected[2].size();

  EXPECT_TRUE(varConnected[0].size() == 4 || varConnected[1].size() == 4 ||
              varConnected[2].size() == 4)
      << "Expected 4 variables in a connected component after preUpdate, but "
         "got "
      << varConnected[0].size() << " and " << varConnected[1].size() << " and "
      << varConnected[2].size();
  EXPECT_EQ(freeVar.size(), 0)
      << "Expected 0 free variables after preUpdate, but got "
      << freeVar.size();

  // Literals Checks
  EXPECT_EQ(specManager->varIsAssigned(1), true)
      << "Variable 1 should be assigned after preUpdate.";
  EXPECT_EQ(specManager->varIsAssigned(4), true)
      << "Variable 4 should be assigned after preUpdate.";
  EXPECT_EQ(specManager->varIsAssigned(5), true)
      << "Variable 5 should be assigned after preUpdate.";
  EXPECT_EQ(specManager->getNbOccurrence(d4::Lit::makeLitTrue(1)), 2)
      << "Expected 2 occurrences for literal 1 after preUpdate, but got "
      << specManager->getNbOccurrence(d4::Lit::makeLitTrue(1));
  EXPECT_EQ(specManager->getNbOccurrence(d4::Lit::makeLitFalse(1)), 2)
      << "Expected 2 occurrences for literal 1 after preUpdate, but got "
      << specManager->getNbOccurrence(d4::Lit::makeLitFalse(1));

  EXPECT_EQ(specManager->getNbOccurrence(d4::Lit::makeLitTrue(2)), 3)
      << "Expected 3 occurrences for literal 2 after preUpdate, but got "
      << specManager->getNbOccurrence(d4::Lit::makeLitTrue(2));
  EXPECT_EQ(specManager->getNbOccurrence(d4::Lit::makeLitFalse(2)), 0)
      << "Expected 0 occurrence for literal -2 after preUpdate, but got "
      << specManager->getNbOccurrence(d4::Lit::makeLitFalse(2));

  EXPECT_EQ(specManager->getNbOccurrence(d4::Lit::makeLitTrue(3)), 1)
      << "Expected 1 occurrence for literal 3 after preUpdate, but got "
      << specManager->getNbOccurrence(d4::Lit::makeLitTrue(3));
  EXPECT_EQ(specManager->getNbOccurrence(d4::Lit::makeLitFalse(3)), 0)
      << "Expected 0 occurrence for literal -3 after preUpdate, but got "
      << specManager->getNbOccurrence(d4::Lit::makeLitFalse(3));

  EXPECT_EQ(specManager->getNbOccurrence(d4::Lit::makeLitTrue(4)), 2)
      << "Expected 2 occurrences for literal 4 after preUpdate, but got "
      << specManager->getNbOccurrence(d4::Lit::makeLitTrue(4));
  EXPECT_EQ(specManager->getNbOccurrence(d4::Lit::makeLitFalse(4)), 2)
      << "Expected 2 occurrences for literal -4 after preUpdate, but got "
      << specManager->getNbOccurrence(d4::Lit::makeLitFalse(4));

  EXPECT_EQ(specManager->getNbOccurrence(d4::Lit::makeLitTrue(5)), 2)
      << "Expected 2 occurrences for literal 5 after preUpdate, but got "
      << specManager->getNbOccurrence(d4::Lit::makeLitTrue(5));
  EXPECT_EQ(specManager->getNbOccurrence(d4::Lit::makeLitFalse(5)), 2)
      << "Expected 2 occurrences for literal -5 after preUpdate, but got "
      << specManager->getNbOccurrence(d4::Lit::makeLitFalse(5));

  EXPECT_EQ(specManager->getNbOccurrence(d4::Lit::makeLitTrue(6)), 1)
      << "Expected 1 occurrence for literal 6 after preUpdate, but got "
      << specManager->getNbOccurrence(d4::Lit::makeLitTrue(6));
  EXPECT_EQ(specManager->getNbOccurrence(d4::Lit::makeLitFalse(6)), 1)
      << "Expected 1 occurrence for literal -6 after preUpdate, but got "
      << specManager->getNbOccurrence(d4::Lit::makeLitFalse(6));

  EXPECT_EQ(specManager->getNbOccurrence(d4::Lit::makeLitTrue(7)), 1)
      << "Expected 1 occurrence for literal 7 after preUpdate, but got "
      << specManager->getNbOccurrence(d4::Lit::makeLitTrue(7));
  EXPECT_EQ(specManager->getNbOccurrence(d4::Lit::makeLitFalse(7)), 1)
      << "Expected 1 occurrence for literal -7 after preUpdate, but got "
      << specManager->getNbOccurrence(d4::Lit::makeLitFalse(7));

  EXPECT_EQ(specManager->getNbOccurrence(d4::Lit::makeLitTrue(8)), 1)
      << "Expected 1 occurrence for literal 8 after preUpdate, but got "
      << specManager->getNbOccurrence(d4::Lit::makeLitTrue(8));
  EXPECT_EQ(specManager->getNbOccurrence(d4::Lit::makeLitFalse(8)), 1)
      << "Expected 1 occurrence for literal -8 after preUpdate, but got "
      << specManager->getNbOccurrence(d4::Lit::makeLitFalse(8));

  EXPECT_EQ(specManager->getNbOccurrence(d4::Lit::makeLitTrue(9)), 2)
      << "Expected 2 occurrences for literal 9 after preUpdate, but got "
      << specManager->getNbOccurrence(d4::Lit::makeLitTrue(9));
  EXPECT_EQ(specManager->getNbOccurrence(d4::Lit::makeLitFalse(9)), 0)
      << "Expected 0 occurrences for literal -9 after preUpdate, but got "
      << specManager->getNbOccurrence(d4::Lit::makeLitFalse(9));

  // postUpdate
  assignedLits = {d4::Lit::makeLitFalse(5)};
  specManager->postUpdate(assignedLits);
  assignedLits = {d4::Lit::makeLitTrue(4)};
  specManager->postUpdate(assignedLits);
  assignedLits = {d4::Lit::makeLitFalse(1)};
  specManager->postUpdate(assignedLits);

  EXPECT_EQ(specManager->getNbOccurrence(d4::Lit::makeLitTrue(1)), 2)
      << "Expected 2 occurrences for literal 1 after preUpdate, but got "
      << specManager->getNbOccurrence(d4::Lit::makeLitTrue(1));
  EXPECT_EQ(specManager->getNbOccurrence(d4::Lit::makeLitFalse(1)), 2)
      << "Expected 2 occurrences for literal 1 after preUpdate, but got "
      << specManager->getNbOccurrence(d4::Lit::makeLitFalse(1));

  EXPECT_EQ(specManager->getNbOccurrence(d4::Lit::makeLitTrue(2)), 4)
      << "Expected 4 occurrences for literal 2 after preUpdate, but got "
      << specManager->getNbOccurrence(d4::Lit::makeLitTrue(2));
  EXPECT_EQ(specManager->getNbOccurrence(d4::Lit::makeLitFalse(2)), 0)
      << "Expected 0 occurrence for literal -2 after preUpdate, but got "
      << specManager->getNbOccurrence(d4::Lit::makeLitFalse(2));

  EXPECT_EQ(specManager->getNbOccurrence(d4::Lit::makeLitTrue(3)), 1)
      << "Expected 1 occurrence for literal 3 after preUpdate, but got "
      << specManager->getNbOccurrence(d4::Lit::makeLitTrue(3));
  EXPECT_EQ(specManager->getNbOccurrence(d4::Lit::makeLitFalse(3)), 1)
      << "Expected 1 occurrence for literal -3 after preUpdate, but got "
      << specManager->getNbOccurrence(d4::Lit::makeLitFalse(3));

  EXPECT_EQ(specManager->getNbOccurrence(d4::Lit::makeLitTrue(4)), 2)
      << "Expected 2 occurrences for literal 4 after preUpdate, but got "
      << specManager->getNbOccurrence(d4::Lit::makeLitTrue(4));
  EXPECT_EQ(specManager->getNbOccurrence(d4::Lit::makeLitFalse(4)), 3)
      << "Expected 3 occurrences for literal -4 after preUpdate, but got "
      << specManager->getNbOccurrence(d4::Lit::makeLitFalse(4));

  EXPECT_EQ(specManager->getNbOccurrence(d4::Lit::makeLitTrue(5)), 2)
      << "Expected 2 occurrences for literal 5 after preUpdate, but got "
      << specManager->getNbOccurrence(d4::Lit::makeLitTrue(5));
  EXPECT_EQ(specManager->getNbOccurrence(d4::Lit::makeLitFalse(5)), 2)
      << "Expected 2 occurrences for literal -5 after preUpdate, but got "
      << specManager->getNbOccurrence(d4::Lit::makeLitFalse(5));

  EXPECT_EQ(specManager->getNbOccurrence(d4::Lit::makeLitTrue(6)), 1)
      << "Expected 1 occurrence for literal 6 after preUpdate, but got "
      << specManager->getNbOccurrence(d4::Lit::makeLitTrue(6));
  EXPECT_EQ(specManager->getNbOccurrence(d4::Lit::makeLitFalse(6)), 1)
      << "Expected 1 occurrence for literal -6 after preUpdate, but got "
      << specManager->getNbOccurrence(d4::Lit::makeLitFalse(6));

  EXPECT_EQ(specManager->getNbOccurrence(d4::Lit::makeLitTrue(7)), 1)
      << "Expected 1 occurrence for literal 7 after preUpdate, but got "
      << specManager->getNbOccurrence(d4::Lit::makeLitTrue(7));
  EXPECT_EQ(specManager->getNbOccurrence(d4::Lit::makeLitFalse(7)), 1)
      << "Expected 1 occurrence for literal -7 after preUpdate, but got "
      << specManager->getNbOccurrence(d4::Lit::makeLitFalse(7));

  EXPECT_EQ(specManager->getNbOccurrence(d4::Lit::makeLitTrue(8)), 1)
      << "Expected 1 occurrence for literal 8 after preUpdate, but got "
      << specManager->getNbOccurrence(d4::Lit::makeLitTrue(8));
  EXPECT_EQ(specManager->getNbOccurrence(d4::Lit::makeLitFalse(8)), 1)
      << "Expected 1 occurrence for literal -8 after preUpdate, but got "
      << specManager->getNbOccurrence(d4::Lit::makeLitFalse(8));

  EXPECT_EQ(specManager->getNbOccurrence(d4::Lit::makeLitTrue(9)), 2)
      << "Expected 2 occurrences for literal 9 after preUpdate, but got "
      << specManager->getNbOccurrence(d4::Lit::makeLitTrue(9));
  EXPECT_EQ(specManager->getNbOccurrence(d4::Lit::makeLitFalse(9)), 0)
      << "Expected 0 occurrences for literal -9 after preUpdate, but got "
      << specManager->getNbOccurrence(d4::Lit::makeLitFalse(9));
}

TEST_F(ConnectedFormulaSpecManagerFixture,
       PrePostUpdatesDetailedAndConnectedComponents) {

  std::vector<d4::Var> component = {1, 2, 3, 4, 5, 6, 7, 8, 9};
  std::vector<std::vector<d4::Var>> varConnected;
  std::vector<d4::Var> freeVar;

  // PreUpdate
  std::vector<d4::Lit> assignedLits = {d4::Lit::makeLitFalse(1),
                                       d4::Lit::makeLitTrue(4),
                                       d4::Lit::makeLitFalse(5)};

  specManager->preUpdate(assignedLits);

  // Connected Components

  component = {1, 2, 3, 4, 5, 6, 7, 8, 9};
  varConnected = {};
  freeVar = {};

  specManager->computeConnectedComponent(varConnected, component, freeVar);
  EXPECT_EQ(varConnected[0], std::vector<d4::Var>({2, 7, 8, 9}))
      << "Expected connected component 1 to be {2, 7, 8, 9} after preUpdate.";
  EXPECT_EQ(varConnected[1], std::vector<d4::Var>({3}))
      << "Expected connected component 0 to be {3} after preUpdate.";
  EXPECT_EQ(varConnected[2], std::vector<d4::Var>({6}))
      << "Expected connected component 2 to be {6} after preUpdate.";

  // Check clauses for each connected component
  std::vector<unsigned int> idxClauses;
  specManager->getCurrentClauses(idxClauses, varConnected[0]);
  EXPECT_EQ(idxClauses, std::vector<unsigned int>({2, 3, 8}))
      << "Expected clauses for connected component 1 to be {2, 3, 8} after "
         "preUpdate.";
  specManager->getCurrentClauses(idxClauses, varConnected[1]);
  EXPECT_EQ(idxClauses, std::vector<unsigned int>({9}))
      << "Expected clauses for connected component 0 to be {9} after "
         "preUpdate.";
  specManager->getCurrentClauses(idxClauses, varConnected[2]);
  EXPECT_EQ(idxClauses, std::vector<unsigned int>({6, 7}))
      << "Expected clauses for connected component 2 to be {6, 7} after "
         "preUpdate.";

  specManager->getCurrentClausesNotBin(idxClauses, varConnected[0]);
  EXPECT_EQ(idxClauses, std::vector<unsigned int>({3, 8}))
      << "Expected clauses for connected component 1 to be {3, 8} after "
         "preUpdate.";
  specManager->getCurrentClausesNotBin(idxClauses, varConnected[1]);
  EXPECT_EQ(idxClauses, std::vector<unsigned int>({9}))
      << "Expected clauses for connected component 0 to be {} after preUpdate.";
  specManager->getCurrentClausesNotBin(idxClauses, varConnected[2]);
  EXPECT_EQ(idxClauses, std::vector<unsigned int>({7}))
      << "Expected clauses for connected component 2 to be {6, 7} after "
         "preUpdate.";

  // check Binary clauses for each connected component
  std::vector<unsigned int> idxBinaryClauses;
  EXPECT_EQ(specManager->getNbBinaryClause(d4::Lit::makeLitTrue(2)), 1)
      << "Expected 1 binary clause for literal 2 after preUpdate.";
  EXPECT_EQ(specManager->getNbBinaryClause(d4::Lit::makeLitFalse(2)), 0)
      << "Expected 0 binary clause for literal -2 after preUpdate.";
  EXPECT_EQ(specManager->getNbBinaryClause(d4::Lit::makeLitTrue(7)), 0)
      << "Expected 0 binary clause for literal 7 after preUpdate.";
  EXPECT_EQ(specManager->getNbBinaryClause(d4::Lit::makeLitFalse(7)), 1)
      << "Expected 1 binary clause for literal -7 after preUpdate.";
  EXPECT_EQ(specManager->getNbBinaryClause(d4::Lit::makeLitTrue(6)), 1)
      << "Expected 1 binary clause for literal 6 after preUpdate.";
  EXPECT_EQ(specManager->getNbBinaryClause(d4::Lit::makeLitFalse(6)), 0)
      << "Expected 0 binary clause for literal -6 after preUpdate.";
}

TEST_F(ConnectedFormulaSpecManagerFixture,
       PrePostUpdatesPureAndConnectedComponents) {
  // Correct initialization
  std::vector<d4::Var> component = {1, 2, 3, 4, 5, 6, 7, 8, 9};
  std::vector<std::vector<d4::Var>> varConnected;
  std::vector<d4::Var> freeVar;

  std::vector<d4::Lit> assignedLits;
  std::vector<d4::Lit> pureLits;

  // PreUpdate
  assignedLits = {d4::Lit::makeLitFalse(1), d4::Lit::makeLitTrue(4),
                  d4::Lit::makeLitFalse(5)};
  specManager->preUpdate(assignedLits, pureLits);
  // Connected Components

  component = {1, 2, 3, 4, 5, 6, 7, 8, 9};
  varConnected = {};
  freeVar = {};

  EXPECT_EQ(
      specManager->computeConnectedComponent(varConnected, component, freeVar),
      3)
      << "Expected 3 connected components after preUpdate, but got "
      << specManager->computeConnectedComponent(varConnected, component,
                                                freeVar);
  EXPECT_EQ(varConnected.size(), 3);

  // Two component should have one variable remaining
  EXPECT_TRUE(varConnected[0].size() == 1 || varConnected[1].size() == 1)
      << "Expected 1 variables in a connected component after preUpdate, but "
         "got "
      << varConnected[0].size() << " and " << varConnected[1].size() << " and "
      << varConnected[2].size();
  EXPECT_TRUE(varConnected[1].size() == 1 || varConnected[2].size() == 1)
      << "Expected 1 variables in a connected component after preUpdate, but "
         "got "
      << varConnected[0].size() << " and " << varConnected[1].size() << " and "
      << varConnected[2].size();
  EXPECT_TRUE(varConnected[0].size() == 1 || varConnected[2].size() == 1)
      << "Expected 1 variables in a connected component after preUpdate, but "
         "got "
      << varConnected[0].size() << " and " << varConnected[1].size() << " and "
      << varConnected[2].size();

  EXPECT_TRUE(varConnected[0].size() == 4 || varConnected[1].size() == 4 ||
              varConnected[2].size() == 4)
      << "Expected 4 variables in a connected component after preUpdate, but "
         "got "
      << varConnected[0].size() << " and " << varConnected[1].size() << " and "
      << varConnected[2].size();
  EXPECT_EQ(freeVar.size(), 0)
      << "Expected 0 free variables after preUpdate, but got "
      << freeVar.size();

  // Literals Checks
  EXPECT_EQ(specManager->varIsAssigned(1), true)
      << "Variable 1 should be assigned after preUpdate.";
  EXPECT_EQ(specManager->varIsAssigned(4), true)
      << "Variable 4 should be assigned after preUpdate.";
  EXPECT_EQ(specManager->varIsAssigned(5), true)
      << "Variable 5 should be assigned after preUpdate.";
  EXPECT_EQ(specManager->getNbOccurrence(d4::Lit::makeLitTrue(1)), 2)
      << "Expected 2 occurrences for literal 1 after preUpdate, but got "
      << specManager->getNbOccurrence(d4::Lit::makeLitTrue(1));
  EXPECT_EQ(specManager->getNbOccurrence(d4::Lit::makeLitFalse(1)), 2)
      << "Expected 2 occurrences for literal 1 after preUpdate, but got "
      << specManager->getNbOccurrence(d4::Lit::makeLitFalse(1));

  EXPECT_EQ(specManager->getNbOccurrence(d4::Lit::makeLitTrue(2)), 3)
      << "Expected 3 occurrences for literal 2 after preUpdate, but got "
      << specManager->getNbOccurrence(d4::Lit::makeLitTrue(2));
  EXPECT_EQ(specManager->getNbOccurrence(d4::Lit::makeLitFalse(2)), 0)
      << "Expected 0 occurrence for literal -2 after preUpdate, but got "
      << specManager->getNbOccurrence(d4::Lit::makeLitFalse(2));

  EXPECT_EQ(specManager->getNbOccurrence(d4::Lit::makeLitTrue(3)), 1)
      << "Expected 1 occurrence for literal 3 after preUpdate, but got "
      << specManager->getNbOccurrence(d4::Lit::makeLitTrue(3));
  EXPECT_EQ(specManager->getNbOccurrence(d4::Lit::makeLitFalse(3)), 0)
      << "Expected 0 occurrence for literal -3 after preUpdate, but got "
      << specManager->getNbOccurrence(d4::Lit::makeLitFalse(3));

  EXPECT_EQ(specManager->getNbOccurrence(d4::Lit::makeLitTrue(4)), 2)
      << "Expected 2 occurrences for literal 4 after preUpdate, but got "
      << specManager->getNbOccurrence(d4::Lit::makeLitTrue(4));
  EXPECT_EQ(specManager->getNbOccurrence(d4::Lit::makeLitFalse(4)), 2)
      << "Expected 2 occurrences for literal -4 after preUpdate, but got "
      << specManager->getNbOccurrence(d4::Lit::makeLitFalse(4));

  EXPECT_EQ(specManager->getNbOccurrence(d4::Lit::makeLitTrue(5)), 2)
      << "Expected 2 occurrences for literal 5 after preUpdate, but got "
      << specManager->getNbOccurrence(d4::Lit::makeLitTrue(5));
  EXPECT_EQ(specManager->getNbOccurrence(d4::Lit::makeLitFalse(5)), 2)
      << "Expected 2 occurrences for literal -5 after preUpdate, but got "
      << specManager->getNbOccurrence(d4::Lit::makeLitFalse(5));

  EXPECT_EQ(specManager->getNbOccurrence(d4::Lit::makeLitTrue(6)), 1)
      << "Expected 1 occurrence for literal 6 after preUpdate, but got "
      << specManager->getNbOccurrence(d4::Lit::makeLitTrue(6));
  EXPECT_EQ(specManager->getNbOccurrence(d4::Lit::makeLitFalse(6)), 1)
      << "Expected 1 occurrence for literal -6 after preUpdate, but got "
      << specManager->getNbOccurrence(d4::Lit::makeLitFalse(6));

  EXPECT_EQ(specManager->getNbOccurrence(d4::Lit::makeLitTrue(7)), 1)
      << "Expected 1 occurrence for literal 7 after preUpdate, but got "
      << specManager->getNbOccurrence(d4::Lit::makeLitTrue(7));
  EXPECT_EQ(specManager->getNbOccurrence(d4::Lit::makeLitFalse(7)), 1)
      << "Expected 1 occurrence for literal -7 after preUpdate, but got "
      << specManager->getNbOccurrence(d4::Lit::makeLitFalse(7));

  EXPECT_EQ(specManager->getNbOccurrence(d4::Lit::makeLitTrue(8)), 1)
      << "Expected 1 occurrence for literal 8 after preUpdate, but got "
      << specManager->getNbOccurrence(d4::Lit::makeLitTrue(8));
  EXPECT_EQ(specManager->getNbOccurrence(d4::Lit::makeLitFalse(8)), 1)
      << "Expected 1 occurrence for literal -8 after preUpdate, but got "
      << specManager->getNbOccurrence(d4::Lit::makeLitFalse(8));

  EXPECT_EQ(specManager->getNbOccurrence(d4::Lit::makeLitTrue(9)), 2)
      << "Expected 2 occurrences for literal 9 after preUpdate, but got "
      << specManager->getNbOccurrence(d4::Lit::makeLitTrue(9));
  EXPECT_EQ(specManager->getNbOccurrence(d4::Lit::makeLitFalse(9)), 0)
      << "Expected 0 occurrences for literal -9 after preUpdate, but got "
      << specManager->getNbOccurrence(d4::Lit::makeLitFalse(9));

  // postUpdate
  assignedLits = {d4::Lit::makeLitFalse(5)};
  specManager->postUpdate(assignedLits);
  assignedLits = {d4::Lit::makeLitTrue(4)};
  specManager->postUpdate(assignedLits);
  assignedLits = {d4::Lit::makeLitFalse(1)};
  specManager->postUpdate(assignedLits);

  EXPECT_EQ(specManager->getNbOccurrence(d4::Lit::makeLitTrue(1)), 2)
      << "Expected 2 occurrences for literal 1 after preUpdate, but got "
      << specManager->getNbOccurrence(d4::Lit::makeLitTrue(1));
  EXPECT_EQ(specManager->getNbOccurrence(d4::Lit::makeLitFalse(1)), 2)
      << "Expected 2 occurrences for literal 1 after preUpdate, but got "
      << specManager->getNbOccurrence(d4::Lit::makeLitFalse(1));

  EXPECT_EQ(specManager->getNbOccurrence(d4::Lit::makeLitTrue(2)), 4)
      << "Expected 4 occurrences for literal 2 after preUpdate, but got "
      << specManager->getNbOccurrence(d4::Lit::makeLitTrue(2));
  EXPECT_EQ(specManager->getNbOccurrence(d4::Lit::makeLitFalse(2)), 0)
      << "Expected 0 occurrence for literal -2 after preUpdate, but got "
      << specManager->getNbOccurrence(d4::Lit::makeLitFalse(2));

  EXPECT_EQ(specManager->getNbOccurrence(d4::Lit::makeLitTrue(3)), 1)
      << "Expected 1 occurrence for literal 3 after preUpdate, but got "
      << specManager->getNbOccurrence(d4::Lit::makeLitTrue(3));
  EXPECT_EQ(specManager->getNbOccurrence(d4::Lit::makeLitFalse(3)), 1)
      << "Expected 1 occurrence for literal -3 after preUpdate, but got "
      << specManager->getNbOccurrence(d4::Lit::makeLitFalse(3));

  EXPECT_EQ(specManager->getNbOccurrence(d4::Lit::makeLitTrue(4)), 2)
      << "Expected 2 occurrences for literal 4 after preUpdate, but got "
      << specManager->getNbOccurrence(d4::Lit::makeLitTrue(4));
  EXPECT_EQ(specManager->getNbOccurrence(d4::Lit::makeLitFalse(4)), 3)
      << "Expected 3 occurrences for literal -4 after preUpdate, but got "
      << specManager->getNbOccurrence(d4::Lit::makeLitFalse(4));

  EXPECT_EQ(specManager->getNbOccurrence(d4::Lit::makeLitTrue(5)), 2)
      << "Expected 2 occurrences for literal 5 after preUpdate, but got "
      << specManager->getNbOccurrence(d4::Lit::makeLitTrue(5));
  EXPECT_EQ(specManager->getNbOccurrence(d4::Lit::makeLitFalse(5)), 2)
      << "Expected 2 occurrences for literal -5 after preUpdate, but got "
      << specManager->getNbOccurrence(d4::Lit::makeLitFalse(5));

  EXPECT_EQ(specManager->getNbOccurrence(d4::Lit::makeLitTrue(6)), 1)
      << "Expected 1 occurrence for literal 6 after preUpdate, but got "
      << specManager->getNbOccurrence(d4::Lit::makeLitTrue(6));
  EXPECT_EQ(specManager->getNbOccurrence(d4::Lit::makeLitFalse(6)), 1)
      << "Expected 1 occurrence for literal -6 after preUpdate, but got "
      << specManager->getNbOccurrence(d4::Lit::makeLitFalse(6));

  EXPECT_EQ(specManager->getNbOccurrence(d4::Lit::makeLitTrue(7)), 1)
      << "Expected 1 occurrence for literal 7 after preUpdate, but got "
      << specManager->getNbOccurrence(d4::Lit::makeLitTrue(7));
  EXPECT_EQ(specManager->getNbOccurrence(d4::Lit::makeLitFalse(7)), 1)
      << "Expected 1 occurrence for literal -7 after preUpdate, but got "
      << specManager->getNbOccurrence(d4::Lit::makeLitFalse(7));

  EXPECT_EQ(specManager->getNbOccurrence(d4::Lit::makeLitTrue(8)), 1)
      << "Expected 1 occurrence for literal 8 after preUpdate, but got "
      << specManager->getNbOccurrence(d4::Lit::makeLitTrue(8));
  EXPECT_EQ(specManager->getNbOccurrence(d4::Lit::makeLitFalse(8)), 1)
      << "Expected 1 occurrence for literal -8 after preUpdate, but got "
      << specManager->getNbOccurrence(d4::Lit::makeLitFalse(8));

  EXPECT_EQ(specManager->getNbOccurrence(d4::Lit::makeLitTrue(9)), 2)
      << "Expected 2 occurrences for literal 9 after preUpdate, but got "
      << specManager->getNbOccurrence(d4::Lit::makeLitTrue(9));
  EXPECT_EQ(specManager->getNbOccurrence(d4::Lit::makeLitFalse(9)), 0)
      << "Expected 0 occurrences for literal -9 after preUpdate, but got "
      << specManager->getNbOccurrence(d4::Lit::makeLitFalse(9));
}