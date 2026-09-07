"""Research stub: yfinance → Python oracle → plot equity curve.

Usage:
    uv run python research/example.py --tickers AAPL --period 1y
Without network, falls back to synthetic data.
"""
import argparse
from pathlib import Path

import matplotlib.pyplot as plt
import pandas as pd

from backtester.data import download_yfinance, normalize_yfinance
from backtester.engine import bars_from_dataframe, run_backtest
from backtester.metrics import compute_metrics
from backtester.strategy import SmaCrossover


def synthetic_bars(n=252):
    import random

    random.seed(42)
    price = 100
    rows = []
    base_ms = 1672531200000  # 2023-01-01 UTC
    for i in range(n):
        price += random.gauss(0, 1)
        rows.append({"ts_utc_ms": base_ms + i * 86400000, "symbol": "SYNTH", "open": price, "high": price + 0.5, "low": price - 0.5, "close": price, "adj_close": price, "volume": 1_000_000})
    return pd.DataFrame(rows)


def main():
    p = argparse.ArgumentParser()
    p.add_argument("--tickers", default="AAPL")
    p.add_argument("--period", default="1y")
    args = p.parse_args()

    tickers = [t.strip().upper() for t in args.tickers.split(",")]
    try:
        raw = download_yfinance(tickers, period=args.period)
        df = normalize_yfinance(raw, tickers)
        print(f"Downloaded {len(df)} bars for {tickers}")
        df.to_parquet(Path("data/processed") / f"{tickers[0]}.parquet", index=False)
    except Exception as e:
        print(f"yfinance failed ({e}), using synthetic data")
        df = synthetic_bars()

    bars = bars_from_dataframe(df)
    strat = SmaCrossover(bars[0].symbol, fast=10, slow=20, qty=100)
    res = run_backtest(bars, strat, commission_bps=5, slippage_bps=2)
    metrics = compute_metrics(res.equity_curve)

    print(f"Fills: {len(res.fills)}, Final equity: {res.equity_curve[-1]:.2f}")
    print(metrics)

    plt.figure()
    plt.plot(res.equity_curve)
    plt.title(f"Equity — SMA(10,20) on {tickers[0]}")
    plt.xlabel("Bar")
    plt.ylabel("Equity")
    out = Path("research/equity.png")
    plt.savefig(out)
    print(f"Saved {out}")


if __name__ == "__main__":
    main()
