#pragma once
#include "bar.h"
#include "portfolio.h"

namespace apten {

class Strategy {
public:
    virtual ~Strategy() = default;

    // Called sequentially per bar. Strategy may call context.order_market().
    virtual void on_bar(const Bar& bar) = 0;

    // Optional: notified after fills applied.
    virtual void on_fill(const Fill& /*fill*/) {}

    // Context injected by Engine before run.
    struct Context {
        Portfolio* portfolio = nullptr;  // read-only intent, but pointer for simplicity
        // Enqueue market order for next bar open.
        void (*order_fn)(void* self, const std::string& symbol, double qty) = nullptr;
        void* self = nullptr;

        void order_market(const std::string& symbol, double qty) const {
            if (order_fn) order_fn(self, symbol, qty);
        }
        double position_qty(const std::string& symbol) const {
            return portfolio ? portfolio->position_qty(symbol) : 0;
        }
        double cash() const { return portfolio ? portfolio->cash() : 0; }
    };

    void set_context(Context ctx) { ctx_ = ctx; }

protected:
    Context ctx_;
};

// Built-ins (implemented in engine/src/strategy.cpp or engine.cpp)
class BuyAndHold : public Strategy {
public:
    explicit BuyAndHold(std::string symbol, double qty) : symbol_(std::move(symbol)), qty_(qty) {}
    void on_bar(const Bar& bar) override;

private:
    std::string symbol_;
    double qty_;
    bool bought_ = false;
};

class SmaCrossover : public Strategy {
public:
    SmaCrossover(std::string symbol, int fast, int slow, double qty)
        : symbol_(std::move(symbol)), fast_(fast), slow_(slow), qty_(qty) {}
    void on_bar(const Bar& bar) override;

private:
    std::string symbol_;
    int fast_;
    int slow_;
    double qty_;
    std::vector<double> closes_;
};

}  // namespace apten
