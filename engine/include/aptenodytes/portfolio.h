#pragma once
#include <string>
#include <unordered_map>
#include <vector>

#include "bar.h"

namespace apten {

struct Fill {
    int64_t ts_utc_ms;
    std::string symbol;
    double quantity;  // +buy, -sell
    double price;     // fill price (next bar open + slippage)
    double commission;
};

struct Position {
    double qty = 0;
    double avg_cost = 0;  // weighted avg entry price
};

class Portfolio {
public:
    explicit Portfolio(double initial_cash) : cash_(initial_cash), initial_cash_(initial_cash) {}

    double cash() const { return cash_; }
    double initial_cash() const { return initial_cash_; }
    const std::unordered_map<std::string, Position>& positions() const { return positions_; }
    const std::vector<Fill>& fills() const { return fills_; }
    const std::vector<double>& equity_curve() const { return equity_curve_; }
    const std::vector<int64_t>& equity_timestamps() const { return equity_timestamps_; }

    // Apply a fill: update cash, position avg_cost, record fill.
    void apply_fill(const Fill& fill);

    // Mark-to-market equity at given bar closes (call once per bar).
    void mark_to_market(int64_t ts, const std::unordered_map<std::string, double>& mark_prices);

    double position_qty(const std::string& symbol) const;

private:
    double cash_;
    double initial_cash_;
    std::unordered_map<std::string, Position> positions_;
    std::vector<Fill> fills_;
    std::vector<double> equity_curve_;
    std::vector<int64_t> equity_timestamps_;
};

}  // namespace apten
