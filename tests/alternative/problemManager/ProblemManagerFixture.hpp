#include "src/problem/cnf/ClauseType.hpp"
#include "src/problem/cnf/ProblemManagerAll.hpp"

#include <gtest/gtest.h>

#include <string>
#include <vector>

class ProblemManagerTest : public testing::Test {
protected:
  d4::ProblemManagerAll *problemManager;

  void SetUp() {
    d4::Config config;
    config.input = "instancesTest/cnfs_and_alternatives/cnf1.cnf";
    config.input_type = "cnf";
    config.alternative_input =
        "instancesTest/cnfs_and_alternatives/alternative1.cnf";
    std::ostringstream out;
    problemManager = dynamic_cast<d4::ProblemManagerAll *>(
        d4::ProblemManager::makeProblemManager(config, out));
  }
};

class ProblemManagerTestBigAlternatives : public testing::Test {
protected:
  d4::ProblemManagerAll *problemManager;

  void SetUp() {
    d4::Config config;
    config.input = "instancesTest/cnfs_and_alternatives/cnf1.cnf";
    config.input_type = "cnf";
    config.alternative_input =
        "instancesTest/cnfs_and_alternatives/bigAlternatives1.cnf";
    std::ostringstream out;
    problemManager = dynamic_cast<d4::ProblemManagerAll *>(
        d4::ProblemManager::makeProblemManager(config, out));
  }
};