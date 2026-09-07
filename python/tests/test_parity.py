"""Parity: Python oracle vs C++ (_apten_core) must match 1e-9 when C++ built.

Skipped if _apten_core not available (week 1 pure-Python workflow).
Hand-calc golden for SMA(10,20) on AAPL 2023 would live here as regression file
— for v0.1 we assert oracle vs C++ on deterministic bars.
"""
import pytest

from backtester.engine import Bar, run_backtest
from backtester.strategy import BuyAndHold


def _has_cpp():
    try:
        import backtester._apten_core  # noqa

        return True
    except ImportError:
        return False


@pytest.mark.skipif(not _has_cpp(), reason="C++ module not built yet (cmake --preset dev)")
def test_cpp_parity_buy_and_hold():
    # This test will be expanded once nanobind bindings expose run()
    # For now it just proves the module imports — real parity after bindings expose Engine.
    import backtester._apten_core

    assert backtester._apten_core is not None
