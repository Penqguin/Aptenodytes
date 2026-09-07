#include "aptenodytes/metrics.h"

#include <algorithm>
#include <cmath>
#include <numeric>

namespace apten {

Metrics compute_metrics(const std::vector<double>& eq, int ppy) {
    Metrics m;
    if (eq.size() < 2) return m;
    m.total_return = (eq.back() - eq.front()) / eq.front();

    // CAGR
    double years = static_cast<double>(eq.size()) / ppy;
    if (years > 0 && eq.front() > 0 && eq.back() > 0) {
        m.cagr = std::pow(eq.back() / eq.front(), 1.0 / years) - 1.0;
    }

    // returns per bar
    std::vector<double> rets;
    rets.reserve(eq.size() - 1);
    for (size_t i = 1; i < eq.size(); ++i) rets.push_back((eq[i] - eq[i - 1]) / eq[i - 1]);

    double mean = std::accumulate(rets.begin(), rets.end(), 0.0) / rets.size();
    double var = 0;
    for (double r : rets) var += (r - mean) * (r - mean);
    var /= rets.size();
    double std = std::sqrt(var);
    m.volatility = std * std::sqrt(static_cast<double>(ppy));
    if (std > 1e-12) m.sharpe = (mean / std) * std::sqrt(static_cast<double>(ppy));

    // Sortino (downside only)
    double downside_var = 0;
    int downside_n = 0;
    for (double r : rets) if (r < 0) { downside_var += r * r; downside_n++; }
    if (downside_n > 0) {
        downside_var /= downside_n;
        double downside_std = std::sqrt(downside_var);
        if (downside_std > 1e-12) m.sortino = (mean / downside_std) * std::sqrt(static_cast<double>(ppy));
    }

    // Max drawdown
    double peak = eq.front();
    double max_dd = 0;
    for (double v : eq) {
        peak = std::max(peak, v);
        double dd = (peak - v) / peak;
        max_dd = std::max(max_dd, dd);
    }
    m.max_drawdown = max_dd;
    return m;
}

}  // namespace apten
