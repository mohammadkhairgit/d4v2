#include "src/problem/ProblemManager.hpp"
#include "src/problem/cnf/ClauseType.hpp"
#include "src/specs/cnf/SpecManagerAll.hpp"

#include <gtest/gtest.h>

#include <string>
#include <vector>

class SpecManagerFixture : public testing::Test {
protected:
  d4::SpecManagerAll *specManager;

  void SetUp() {
    d4::Config config;
    config.input = "instancesTest/cnfs_and_alternatives/cnf1.cnf";
    config.input_type = "cnf";
    config.alternative_input =
        "instancesTest/cnfs_and_alternatives/alternative1.cnf";
    config.occurrence_manager = "dynamic";
    std::ostringstream out;
    specManager =
        dynamic_cast<d4::SpecManagerAll *>(d4::SpecManager::makeSpecManager(
            config, *d4::ProblemManager::makeProblemManager(config, out), out));
  }
};

class ConnectedFormulaSpecManagerFixture : public testing::Test {
protected:
  d4::SpecManagerAll *specManager;

  void SetUp() {
    d4::Config config;
    config.input =
        "instancesTest/cnfs_and_alternatives/cnfConnectedComponent1.cnf";
    config.input_type = "cnf";
    config.alternative_input = "instancesTest/cnfs_and_alternatives/"
                               "alternativeConnectedComponent1.cnf";
    config.occurrence_manager = "dynamic";
    std::ostringstream out;
    specManager =
        dynamic_cast<d4::SpecManagerAll *>(d4::SpecManager::makeSpecManager(
            config, *d4::ProblemManager::makeProblemManager(config, out), out));
  }
};