/* ---------------------------------------------------------------------
 * Numenta Platform for Intelligent Computing (NuPIC)
 * Copyright (C) 2016, Numenta, Inc.  Unless you have an agreement
 * with Numenta, Inc., for a separate license for this software code, the
 * following terms and conditions apply:
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero Public License version 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU Affero Public License for more details.
 *
 * You should have received a copy of the GNU Affero Public License
 * along with this program.  If not, see http://www.gnu.org/licenses.
 *
 * http://numenta.org/licenses/
 * ---------------------------------------------------------------------
 */

#include <algorithm>
#include <vector>

#include "gtest/gtest.h"

#include "nupic/algorithms/Anomaly.hpp"
#include "nupic/types/Types.hpp"

namespace testing {
    
using namespace nupic::algorithms::anomaly;
using namespace nupic;

TEST(ComputeRawAnomalyScore, NoActiveOrPredicted) {
  std::vector<UInt> active;
  std::vector<UInt> predicted;
  ASSERT_FLOAT_EQ(computeRawAnomalyScore(active, predicted), 0.0);
};

TEST(ComputeRawAnomalyScore, NoActive) {
  std::vector<UInt> active;
  std::vector<UInt> predicted = {3, 5};
  ASSERT_FLOAT_EQ(computeRawAnomalyScore(active, predicted), 0.0f);
};

TEST(ComputeRawAnomalyScore, PerfectMatch) {
  std::vector<UInt> active = {3, 5, 7};
  std::vector<UInt> predicted = {3, 5, 7};
  ASSERT_FLOAT_EQ(computeRawAnomalyScore(active, predicted), 0.0f);
};

TEST(ComputeRawAnomalyScore, NoMatch) {
  std::vector<UInt> active = {2, 4, 6};
  std::vector<UInt> predicted = {3, 5, 7};
  ASSERT_FLOAT_EQ(computeRawAnomalyScore(active, predicted), 1.0f);
};

TEST(ComputeRawAnomalyScore, PartialMatch) {
  std::vector<UInt> active = {2, 3, 6};
  std::vector<UInt> predicted = {3, 5, 7};
  ASSERT_FLOAT_EQ(computeRawAnomalyScore(active, predicted), 2.0f / 3.0f);
};

TEST(Anomaly, ComputeScoreNoActiveOrPredicted) {
  std::vector<UInt> active;
  std::vector<UInt> predicted;
  Anomaly a;
  ASSERT_FLOAT_EQ(a.compute(active, predicted), 0.0f);
}

TEST(Anomaly, ComputeScoreNoActive) {
  std::vector<UInt> active;
  std::vector<UInt> predicted = {3, 5};
  Anomaly a;
  ASSERT_FLOAT_EQ(a.compute(active, predicted), 0.0f);
}

TEST(Anomaly, ComputeScorePerfectMatch) {
  std::vector<UInt> active = {3, 5, 7};
  std::vector<UInt> predicted = {3, 5, 7};
  Anomaly a;
  ASSERT_FLOAT_EQ(a.compute(active, predicted), 0.0f);
}

TEST(Anomaly, ComputeScoreNoMatch) {
  std::vector<UInt> active = {2, 4, 6};
  std::vector<UInt> predicted = {3, 5, 7};
  Anomaly a;
  ASSERT_FLOAT_EQ(a.compute(active, predicted), 1.0);
}

TEST(Anomaly, ComputeScorePartialMatch) {
  std::vector<UInt> active = {2, 3, 6};
  std::vector<UInt> predicted = {3, 5, 7};
  Anomaly a;
  ASSERT_FLOAT_EQ(a.compute(active, predicted), 2.0f / 3.0f);
}

TEST(Anomaly, Cumulative) {
  const UInt TEST_COUNT = 9;
  Anomaly a{3};

  // TODO: Do not understand this statement.
  // It appears it is initializing a vector containing vectors of type UInt.
  // But TEST_COUNT is not a vector.
  // Seems to work in ubuntu but rejected by MSVC
  std::vector<std::vector<UInt> > preds{TEST_COUNT, {1, 2, 6}};

  std::vector<std::vector<UInt>> acts = {
      {1, 2, 6},    {1, 2, 6},    {1, 4, 6}, {10, 11, 6}, {10, 11, 12},
      {10, 11, 12}, {10, 11, 12}, {1, 2, 6}, {1, 2, 6}};

  std::vector<float> expected = {0.0f,        0.0f,        1.0f / 9.0f,
                                 3.0f / 9.0f, 2.0f / 3.0f, 8.0f / 9.0f,
                                 1.0f,        2.0f / 3.0f, 1.0f / 3.0f};

  for (UInt index = 0; index < TEST_COUNT; index++) {
    ASSERT_FLOAT_EQ(a.compute(acts[index], preds[index]), expected[index]);
  }
}

TEST(Anomaly, SelectModePure) {
  Anomaly a{0, AnomalyMode::PURE, 0};
  std::vector<UInt> active = {2, 3, 6};
  std::vector<UInt> predicted = {3, 5, 7};
  ASSERT_FLOAT_EQ(a.compute(active, predicted), 2.0f / 3.0f);
};

// Anomaly Likelihood tests
TEST(AnomalyLikelihood, SelectModeLikelihood)
{
  Anomaly a{0, AnomalyMode::LIKELIHOOD, 0};
  std::vector<UInt> active = {2, 3, 6};
  std::vector<UInt> predicted = {3, 5, 7};
  int ts = 0; //timestamp
  Real likelihood;

  for(int i=0; i< 388; i++) {
     likelihood = a.compute(active, predicted,  ++ts);
     ASSERT_FLOAT_EQ(likelihood, 0.5f); //first (<=388) probationaryPeriod rounds likelihood=0.5
  }

  //real likelihood returned here
  for(int i=0; i< 10; i++) {
     likelihood = a.compute(active, predicted,  ++ts);
  ASSERT_TRUE(abs(likelihood - 0.5)<0.001); //TODO port likelihood tests here
  }

};

}