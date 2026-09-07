#include "aptenodytes/engine.h"

#include <unordered_map>

namespace apten {

void Engine::run() {
    // Inject context so strategy can call order_market
    Strategy::Context ctx;
    ctx.portfolio = &portfolio_;
    ctx.self = this;
    ctx.order_fn = [](void* self, const std::string& sym, double qty) {
        auto* eng = static_cast<Engine*>(self);
        eng->exec_.submit(Order{sym, qty, 0});
    };
    strategy_.set_context(ctx);

    std::optional<Bar> cur = feed_.next();
    std::optional<Bar> next;
    // We need next bar to fill at next open. So peek ahead by one.
    // Strategy sees cur, orders fill at next.open. Mark equity at cur.close.
    while (cur) {
        // 1) execute pending orders at cur.open (these were submitted on prior bar)
        auto fills = exec_.execute(*cur);
        for (auto& f : fills) {
            portfolio_.apply_fill(f);
            strategy_.on_fill(f);
        }

        // 2) strategy decides on cur (orders will fill next bar)
        strategy_.on_bar(*cur);

        // 3) mark-to-market at cur.close
        portfolio_.mark_to_market(cur->ts_utc_ms, {{cur->symbol, cur->close}});

        cur = feed_.next();
    }
    // Any remaining pending orders that never got a next bar are discarded (no fill).
}

void BuyAndHold::on_bar(const Bar& bar) {
    if (!bought_ && bar.symbol == symbol_) {
        ctx_.order_market(symbol_, qty_);
        bought_ = true;
    }
}

void SmaCrossover::on_bar(const Bar& bar) {
    if (bar.symbol != symbol_) return;
    closes_.push_back(bar.close);
    if ((int)closes_.size() < slow_) return;

    auto sma = [&](int period) {
        double s = 0;
        for (int i = 0; i < period; ++i) s += closes_[closes_.size() - 1 - i];
        return s / period;
    };
    double fast_sma = sma(fast_);
    double slow_sma = sma(slow_);

    // Need previous bar's SMAs to detect crossover — require at least slow+1 points
    if ((int)closes_.size() <= slow_) return;
    double prev_fast = 0, prev_slow = 0;
    {
        double s = 0; for (int i=1;i<=fast_;++i) s+=closes_[closes_.size()-1-i]; prev_fast=s/fast_;
        double t = 0; for (int i=1;i<=slow_;++i) t+=closes_[closes_.size()-1-i]; prev_slow=t/slow_;
    }

    double pos = ctx_.position_qty(symbol_);
    bool was_above = prev_fast > prev_slow;
    bool now_above = fast_sma > slow_sma;

    if (!was_above && now_above && pos <= 0) {
        // golden cross: go long
        double target = qty_ - pos;
        if (target != 0) ctx_.order_market(symbol_, target);
    } else if (was_above && !now_above && pos >= 0) {
        // death cross: flatten / go short (here just flatten for v0.1)
        if (pos != 0) ctx_.order_market(symbol_, -pos);
    }
}

}  // namespace apten
