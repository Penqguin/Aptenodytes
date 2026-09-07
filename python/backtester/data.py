"""Data ingestion: yfinance → raw CSV → processed parquet (UTC, sorted, adj handling)."""
from __future__ import annotations

import argparse
from pathlib import Path

import pandas as pd

try:
    import yfinance as yf
except ImportError:
    yf = None  # allows tests without yfinance


def download_yfinance(tickers: list[str], period: str = "5y", interval: str = "1d") -> pd.DataFrame:
    if yf is None:
        raise ImportError("yfinance not installed: pip install yfinance")
    df = yf.download(tickers, period=period, interval=interval, auto_adjust=False, progress=False)
    return df


def normalize_yfinance(df: pd.DataFrame, tickers: list[str]) -> pd.DataFrame:
    """Normalize yfinance multi-index frame to long form with required schema.

    Output columns: ts_utc_ms, symbol, open, high, low, close, adj_close, volume
    Sorted by (symbol, ts_utc_ms), tz-aware UTC.
    """
    # yfinance returns columns as MultiIndex (Price, Ticker) when multi-ticker
    if isinstance(df.columns, pd.MultiIndex):
        frames = []
        for sym in tickers:
            if sym not in df.columns.get_level_values(1):
                continue
            sub = df.xs(sym, axis=1, level=1)
            sub = sub.copy()
            sub["symbol"] = sym
            frames.append(sub)
        if not frames:
            raise ValueError("No tickers found in download")
        long = pd.concat(frames)
    else:
        # single ticker — columns are Price
        sym = tickers[0]
        long = df.copy()
        long["symbol"] = sym

    long = long.reset_index()  # Date -> column
    # Date column may be named 'Date' or 'Datetime'
    date_col = "Date" if "Date" in long.columns else long.columns[0]
    long["ts_utc_ms"] = pd.to_datetime(long[date_col], utc=True).astype("int64") // 1_000_000

    # Standardize price cols (yfinance: Open High Low Close Adj Close Volume)
    col_map = {c: c.lower().replace(" ", "_") for c in long.columns}
    long = long.rename(columns=col_map)
    # Ensure adj_close exists (some yfinance versions already adjusted)
    if "adj_close" not in long.columns and "adj_close" not in long.columns:
        long["adj_close"] = long["close"]

    keep = ["ts_utc_ms", "symbol", "open", "high", "low", "close", "adj_close", "volume"]
    for k in keep:
        if k not in long.columns:
            raise ValueError(f"Missing column {k} after normalize; got {long.columns.tolist()}")
    long = long[keep].sort_values(["symbol", "ts_utc_ms"]).reset_index(drop=True)
    # Drop rows with NaN closes (holidays / bad data)
    long = long.dropna(subset=["close"])
    return long


def save_processed(df: pd.DataFrame, out_dir: Path) -> None:
    out_dir.mkdir(parents=True, exist_ok=True)
    for sym, group in df.groupby("symbol"):
        path = out_dir / f"{sym}.parquet"
        group.to_parquet(path, index=False)
        print(f"wrote {path} ({len(group)} bars)")


def main() -> None:
    p = argparse.ArgumentParser(description="Download yfinance → processed parquet")
    p.add_argument("--tickers", required=True, help="comma-separated, e.g., AAPL,MSFT")
    p.add_argument("--period", default="5y")
    p.add_argument("--interval", default="1d")
    p.add_argument("--out", default="data/processed")
    p.add_argument("--raw-out", default="data/raw")
    args = p.parse_args()

    tickers = [t.strip().upper() for t in args.tickers.split(",") if t.strip()]
    raw = download_yfinance(tickers, period=args.period, interval=args.interval)

    raw_path = Path(args.raw_out)
    raw_path.mkdir(parents=True, exist_ok=True)
    # Save raw as CSV for audit
    raw.to_csv(raw_path / f"{'_'.join(tickers)}_{args.period}_{args.interval}.csv")

    norm = normalize_yfinance(raw, tickers)
    save_processed(norm, Path(args.out))


if __name__ == "__main__":
    main()
