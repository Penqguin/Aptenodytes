"""Python oracle: reference Tier 2 event-driven backtester.

This is the spec that the C++ port must match 1e-9. No lookahead:
strategy sees bar[t], orders fill at bar[t+1].open.
"""
from __future__ import annotations

from dataclasses import dataclass, field
from typing import Dict, List, Optional

import pandas as pd


@dataclass
class Bar:
    ts_utc_ms: int
    symbol: str
    open: float
    high: float
    low: float
    close: float
    adj_close: float
    volume: float


@dataclass
class Fill:
    ts_utc_ms: int
    symbol: str
    quantity: float
    price: float
    commission: float


@dataclass
class Portfolio:
    initial_cash: float
    cash: float = field(init=False)
    positions: Dict[str, float] = field(default_factory=dict)
    avg_cost: Dict[str, float] = field(default_factory=dict)
    fills: List[Fill] = field(default_factory=list)
    equity_curve: List[float] = field(default_factory=list)
    equity_timestamps: List[int] = field(default_factory=list)

    def __post_init__(self):
        self.cash = self.initial_cash

    def apply_fill(self, fill: Fill):
        cost = fill.quantity * fill.price
        self.cash -= cost + fill.commission
        sym = fill.symbol
        qty = self.positions.get(sym, 0)
        new_qty = qty + fill.quantity
        if abs(new_qty) < 1e-12:
            self.positions[sym] = 0
            self.avg_cost[sym] = 0
        elif qty == 0 or (qty > 0) == (fill.quantity > 0):
            total = qty * self.avg_cost.get(sym, 0) + fill.quantity * fill.price
            self.avg_cost[sym] = total / new_qty if new_qty != 0 else 0
            self.positions[sym] = new_qty
        else:
            if (new_qty > 0) != (qty > 0):
                self.avg_cost[sym] = fill.price
            self.positions[sym] = new_qty
            if abs(new_qty) < 1e-12:
                self.avg_cost[sym] = 0
        self.fills.append(fill)

    def mark(self, ts: int, marks: Dict[str, float]):
        eq = self.cash + sum(self.positions.get(s, 0) * p for s, p in marks.items())
        self.equity_curve.append(eq)
        self.equity_timestamps.append(ts)


@dataclass
class BacktestResult:
    equity_curve: List[float]
    equity_timestamps: List[int]
    fills: List[Fill]
    final_cash: float
    positions: Dict[str, float]


def run_backtest(
    bars: List[Bar],
    strategy,
    initial_cash: float = 100_000,
    commission_bps: float = 0,
    slippage_bps: float = 0,
) -> BacktestResult:
    """Event loop matching C++ Engine::run semantics."""
    portfolio = Portfolio(initial_cash=initial_cash)
    pending: List[dict] = []  # orders waiting for next bar

    # Inject context into strategy
    strategy._portfolio = portfolio
    strategy._pending = pending

    for i, bar in enumerate(bars):
        # 1) execute pending at bar.open
        remaining = []
        executed = []
        for ord in pending:
            if ord["symbol"] == bar.symbol:
                price = bar.open
                slip = price * slippage_bps / 10000
                price = price + slip if ord["quantity"] > 0 else price - slip
                price = max(0.01, price)
                notional = abs(ord["quantity"] * price)
                commission = notional * commission_bps / 10000
                fill = Fill(bar.ts_utc_ms, ord["symbol"], ord["quantity"], price, commission)
                portfolio.apply_fill(fill)
                executed.append(fill)
                if hasattr(strategy, "on_fill"):
                    strategy.on_fill(fill)
            else:
                remaining.append(ord)
        pending[:] = remaining

        # 2) strategy decides
        strategy.on_bar(bar)

        # 3) mark at close
        portfolio.mark(bar.ts_utc_ms, {bar.symbol: bar.close})

    return BacktestResult(
        equity_curve=portfolio.equity_curve,
        equity_timestamps=portfolio.equity_timestamps,
        fills=portfolio.fills,
        final_cash=portfolio.cash,
        positions=dict(portfolio.positions),
    )


def bars_from_dataframe(df: pd.DataFrame) -> List[Bar]:
    """Load from processed parquet schema."""
    out = []
    for _, r in df.iterrows():
        out.append(Bar(int(r.ts_utc_ms), str(r.symbol), float(r.open), float(r.high), float(r.low), float(r.close), float(r.adj_close), float(r.volume)))
    out.sort(key=lambda b: (b.symbol, b.ts_utc_ms))
    return out
