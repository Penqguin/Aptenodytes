#include <gtest/gtest.h>
#include "aptenodytes/engine.h"
#include "aptenodytes/metrics.h"

using namespace apten;

// Deterministic 5-bar test for hand-calc parity with Python oracle.
static std::vector<Bar> make_bars() {
    // AAPL-like, daily
    return {
        {0, "AAPL", 100, 101, 99, 100, 100, 1000},
        {1, "AAPL", 101, 102, 100, 101, 101, 1000},
        {2, "AAPL", 102, 103, 101, 102, 102, 1000},
        {3, "AAPL", 103, 104, 102, 103, 103, 1000},
        {4, "AAPL", 104, 105, 103, 104, 104, 1000},
        {5, "AAPL", 105, 106, 104, 105, 105, 1000},
    };
}

TEST(Engine, BuyAndHoldFillAtNextOpen) {
    auto bars = make_bars();
    BarFeed feed(bars);
    MarketOnNextOpen exec({0, 0});
    Portfolio portfolio(100000);
    BuyAndHold strat("AAPL", 10);

    Engine eng(feed, exec, portfolio, strat);
    eng.run();

    // BuyAndHold orders on bar 0, fills at bar 1 open = 101
    ASSERT_EQ(portfolio.fills().size(), 1u);
    EXPECT_DOUBLE_EQ(portfolio.fills()[0].price, 101);
    EXPECT_DOUBLE_EQ(portfolio.fills()[0].quantity, 10);
    // equity_curve has 6 points (one per bar, mark at close)
    EXPECT_EQ(portfolio.equity_curve().size(), 6u);
}

TEST(Engine, NoLookahead) {
    // Strategy that would profit if it peeked at next close — ensure it doesn't.
    // We test implicitly via MarketOnNextOpen: fill price is next open, not current close.
    auto bars = make_bars();
    BarFeed feed(bars);
    MarketOnNextOpen exec({0, 0});
    Portfolio portfolio(10000);

    class PeekStrategy : public Strategy {
    public:
        void on_bar(const Bar& bar) override {
            // If we filled at bar.close we'd get 100; we should get next open 101
            if (bar.ts_utc_ms == 0) ctx_.order_market("AAPL", 10);
        }
    } strat;

    Engine eng(feed, exec, portfolio, strat);
    eng.run();
    ASSERT_EQ(portfolio.fills().size(), 1u);
    EXPECT_DOUBLE_EQ(portfolio.fills()[0].price, 101) << "filled at next open, not current close → no lookahead";
}
