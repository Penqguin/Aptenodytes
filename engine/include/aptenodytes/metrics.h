#pragma once
#include <vector>

namespace apten {

struct Metrics {
    double cagr = 0;
    double sharpe = 0;      // annualized, risk-free = 0
    double sortino = 0;
    double max_drawdown = 0;  // 0..1
    double volatility = 0;    // annualized
    double total_return = 0;
};

// Compute from equity curve (must be mark-to-market per bar, >1 point).
// Assumes daily bars for annualization (252); caller may override periods_per_year.
Metrics compute_metrics(const std::vector<double>& equity_curve, int periods_per_year = 252);

}  // namespace apten
