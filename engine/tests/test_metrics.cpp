#include <gtest/gtest.h>
#include "aptenodytes/metrics.h"

using namespace apten;

TEST(Metrics, MaxDrawdown) {
    // equity 100 -> 120 -> 90 -> 110 : peak 120, trough 90 => dd 0.25
    std::vector<double> eq{100, 120, 90, 110};
    auto m = compute_metrics(eq, 252);
    EXPECT_NEAR(m.max_drawdown, 0.25, 1e-9);
    EXPECT_NEAR(m.total_return, 0.1, 1e-9);
}

TEST(Metrics, Empty) {
    auto m = compute_metrics({}, 252);
    EXPECT_DOUBLE_EQ(m.total_return, 0);
}
