#include <gtest/gtest.h>
#include "aptenodytes/portfolio.h"

using namespace apten;

TEST(Portfolio, BuyAndSell) {
    Portfolio p(100000);
    p.apply_fill(Fill{0, "AAPL", 100, 150, 0});
    EXPECT_DOUBLE_EQ(p.cash(), 100000 - 15000);
    EXPECT_DOUBLE_EQ(p.position_qty("AAPL"), 100);

    p.apply_fill(Fill{1, "AAPL", -40, 160, 0});
    EXPECT_DOUBLE_EQ(p.position_qty("AAPL"), 60);
    EXPECT_DOUBLE_EQ(p.cash(), 100000 - 15000 + 6400);

    p.mark_to_market(2, {{"AAPL", 155}});
    EXPECT_DOUBLE_EQ(p.equity_curve().back(), p.cash() + 60 * 155);
}

TEST(Portfolio, Commission) {
    Portfolio p(10000);
    Fill f{0, "AAPL", 10, 100, 5};
    p.apply_fill(f);
    EXPECT_DOUBLE_EQ(p.cash(), 10000 - 1000 - 5);
}
