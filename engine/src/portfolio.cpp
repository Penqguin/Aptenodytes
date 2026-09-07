#include "aptenodytes/portfolio.h"

#include <cmath>

namespace apten {

void Portfolio::apply_fill(const Fill& fill) {
    double cost = fill.quantity * fill.price;
    cash_ -= cost + fill.commission;

    auto& pos = positions_[fill.symbol];
    double new_qty = pos.qty + fill.quantity;
    if (std::abs(new_qty) < 1e-12) {
        pos.qty = 0;
        pos.avg_cost = 0;
    } else if (pos.qty == 0 || (pos.qty > 0) == (fill.quantity > 0)) {
        // increasing or opening: weighted avg
        double total_cost = pos.qty * pos.avg_cost + fill.quantity * fill.price;
        pos.avg_cost = total_cost / new_qty;
        pos.qty = new_qty;
    } else {
        // reducing / flipping through zero — keep avg_cost for remaining qty,
        // if flipped, new avg is fill price for the flipped portion
        if ((new_qty > 0) != (pos.qty > 0)) {
            pos.avg_cost = fill.price;
        }
        pos.qty = new_qty;
        if (std::abs(pos.qty) < 1e-12) pos.avg_cost = 0;
    }
    fills_.push_back(fill);
}

void Portfolio::mark_to_market(int64_t ts, const std::unordered_map<std::string, double>& mark_prices) {
    double equity = cash_;
    for (const auto& [sym, pos] : positions_) {
        auto it = mark_prices.find(sym);
        if (it != mark_prices.end()) equity += pos.qty * it->second;
    }
    equity_curve_.push_back(equity);
    equity_timestamps_.push_back(ts);
}

double Portfolio::position_qty(const std::string& symbol) const {
    auto it = positions_.find(symbol);
    return it == positions_.end() ? 0 : it->second.qty;
}

}  // namespace apten
