#include "catch.hpp"
#include "HighsLp.h"
#include "HighsStatus.h"

// No commas in test case name.
TEST_CASE("correct-print-status", "[highs_data]") {
  std::string str = HighsStatusToString(HighsStatus::OK);
  REQUIRE(str == "OK");
}