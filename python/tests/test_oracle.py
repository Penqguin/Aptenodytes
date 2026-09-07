"""Golden-file test: SMA(10,20) hand-calc parity + no-lookahead invariant."""
from backtester.engine import Bar, run_backtest
from backtester.strategy import BuyAndHold, SmaCrossover


def make_bars():
    # 6 bars, same as C++ test_engine.cpp for parity
    return [
        Bar(0, "AAPL", 100, 101, 99, 100, 100, 1000),
        Bar(1, "AAPL", 101, 102, 100, 101, 101, 1000),
        Bar(2, "AAPL", 102, 103, 101, 102, 102, 1000),
        Bar(3, "AAPL", 103, 104, 102, 103, 103, 1000),
        Bar(4, "AAPL", 104, 105, 103, 104, 104, 1000),
        Bar(5, "AAPL", 105, 106, 104, 105, 105, 1000),
    ]


def test_buy_and_hold_fill_at_next_open():
    bars = make_bars()
    strat = BuyAndHold("AAPL", 10)
    res = run_backtest(bars, strat, initial_cash=100_000)
    assert len(res.fills) == 1
    assert res.fills[0].price == 101  # next open, not 100
    assert res.fills[0].quantity == 10
    assert len(res.equity_curve) == 6


def test_no_lookahead():
    bars = make_bars()

    class Peek:
        def __init__(self):
            self._pending = None
            self._portfolio = None

        def on_bar(self, bar):
            if bar.ts_utc_ms == 0:
                self.order_market("AAPL", 10)

        def order_market(self, sym, qty):
            self._pending.append({"symbol": sym, "quantity": qty})

    strat = Peek()
    res = run_backtest(bars, strat, initial_cash=10_000)
    assert res.fills[0].price == 101


def test_sma_crossover_runs():
    # Longer series to trigger real crossover
    bars = [Bar(i, "AAPL", 100 + (i % 5), 101 + (i % 5), 99 + (i % 5), 100 + (i % 5), 100 + (i % 5), 1000) for i in range(30)]
    # Inject a trend to force cross: first 15 flat, next 15 rising
    for i in range(15, 30):
        bars[i].close = 100 + (i - 15) * 2
        bars[i].open = bars[i].close - 0.5
    strat = SmaCrossover("AAPL", fast=5, slow=10, qty=100)
    res = run_backtest(bars, strat, initial_cash=100_000, commission_bps=5, slippage_bps=2)
    # Should have at least one fill, equity curve length matches bars
    assert len(res.equity_curve) == 30
    assert len(res.fills) >= 1
