#include "aptenodytes/execution_model.h"

#include <cmath>

namespace apten {

void MarketOnNextOpen::submit(Order order) { pending_.push_back(std::move(order)); }

std::vector<Fill> MarketOnNextOpen::execute(const Bar& bar) {
    std::vector<Fill> fills;
    // FIFO: fill all pending that match this bar's symbol at bar.open
    // For simplicity, each pending order is for a single symbol; we fill if symbol matches bar.symbol
    // If multiple symbols in feed, we iterate pending and fill matching.
    std::deque<Order> remaining;
    for (auto& ord : pending_) {
        if (ord.symbol == bar.symbol) {
            double price = bar.open;
            // slippage: buys pay more, sells receive less
            double slip = price * cfg_.slippage_bps / 10000.0;
            if (ord.quantity > 0) price += slip;
            else price -= slip;
            price = std::max(0.01, price);  // floor

            double notional = std::abs(ord.quantity * price);
            double commission = notional * cfg_.commission_bps / 10000.0;

            fills.push_back(Fill{bar.ts_utc_ms, ord.symbol, ord.quantity, price, commission});
        } else {
            remaining.push_back(std::move(ord));
        }
    }
    pending_ = std::move(remaining);
    return fills;
}

}  // namespace apten
