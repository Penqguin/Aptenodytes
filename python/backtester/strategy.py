"""Python strategy base + built-ins (mirrors C++ Strategy interface)."""
from __future__ import annotations


class Strategy:
    def on_bar(self, bar): ...
    def on_fill(self, fill): ...

    # helpers injected by run_backtest
    def order_market(self, symbol: str, qty: float):
        self._pending.append({"symbol": symbol, "quantity": qty})

    @property
    def cash(self):
        return self._portfolio.cash

    def position_qty(self, symbol: str) -> float:
        return self._portfolio.positions.get(symbol, 0)


class BuyAndHold(Strategy):
    def __init__(self, symbol: str, qty: float):
        self.symbol = symbol
        self.qty = qty
        self._bought = False

    def on_bar(self, bar):
        if not self._bought and bar.symbol == self.symbol:
            self.order_market(self.symbol, self.qty)
            self._bought = True


class SmaCrossover(Strategy):
    def __init__(self, symbol: str, fast: int, slow: int, qty: float):
        self.symbol = symbol
        self.fast = fast
        self.slow = slow
        self.qty = qty
        self.closes: list[float] = []

    def on_bar(self, bar):
        if bar.symbol != self.symbol:
            return
        self.closes.append(bar.close)
        if len(self.closes) < self.slow:
            return
        fast_sma = sum(self.closes[-self.fast :]) / self.fast
        slow_sma = sum(self.closes[-self.slow :]) / self.slow
        if len(self.closes) <= self.slow:
            return
        prev_fast = sum(self.closes[-self.fast - 1 : -1]) / self.fast
        prev_slow = sum(self.closes[-self.slow - 1 : -1]) / self.slow
        pos = self.position_qty(self.symbol)
        was_above = prev_fast > prev_slow
        now_above = fast_sma > slow_sma
        if not was_above and now_above and pos <= 0:
            self.order_market(self.symbol, self.qty - pos)
        elif was_above and not now_above and pos >= 0:
            if pos != 0:
                self.order_market(self.symbol, -pos)
