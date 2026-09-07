"""Aptenodytes backtester — Python oracle + C++ bindings.

Week 1: use the pure-Python oracle (`backtester.engine`) directly.
After `cmake --preset dev && cmake --build build/dev`, `backtester._apten_core`
(C++ via nanobind) becomes available and should match the oracle 1e-9.
"""
from .engine import Bar, Portfolio, BacktestResult, run_backtest
from .strategy import Strategy, BuyAndHold, SmaCrossover

try:
    import backtester._apten_core  # noqa: F401

    HAS_CPP = True
except ImportError:
    HAS_CPP = False

__all__ = ["Bar", "Portfolio", "BacktestResult", "run_backtest", "Strategy", "BuyAndHold", "SmaCrossover", "HAS_CPP"]
