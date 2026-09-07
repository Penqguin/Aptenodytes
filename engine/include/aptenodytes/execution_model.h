#pragma once
#include <deque>
#include <string>

#include "bar.h"
#include "portfolio.h"

namespace apten {

struct Order {
    std::string symbol;
    double quantity;  // +buy, -sell (market only v0.1)
    int64_t created_ts_ms;
};

struct ExecutionConfig {
    double commission_bps = 0;  // e.g., 5 = 0.05%
    double slippage_bps = 0;    // e.g., 2 = 0.02%
};

// Abstract execution — seam for Tier 3 OrderBookExecution.
class ExecutionModel {
public:
    virtual ~ExecutionModel() = default;
    virtual void submit(Order order) = 0;
    // Called on next bar: produce fills at bar.open (+ slippage), apply commissions.
    virtual std::vector<Fill> execute(const Bar& bar) = 0;
    virtual bool has_pending() const = 0;
};

// Tier 2: market-on-next-open (no partial fills, no queue).
class MarketOnNextOpen : public ExecutionModel {
public:
    explicit MarketOnNextOpen(ExecutionConfig cfg) : cfg_(cfg) {}

    void submit(Order order) override;
    std::vector<Fill> execute(const Bar& bar) override;
    bool has_pending() const override { return !pending_.empty(); }

private:
    ExecutionConfig cfg_;
    std::deque<Order> pending_;
};

}  // namespace apten
