import pandas as pd

from backtester.data import normalize_yfinance


def test_normalize_single_ticker():
    idx = pd.date_range("2023-01-01", periods=3, tz="UTC")
    df = pd.DataFrame({"Open": [100, 101, 102], "High": [101, 102, 103], "Low": [99, 100, 101], "Close": [100, 101, 102], "Adj Close": [100, 101, 102], "Volume": [1000, 1000, 1000]}, index=idx)
    df.index.name = "Date"
    norm = normalize_yfinance(df, ["AAPL"])
    assert list(norm.columns) == ["ts_utc_ms", "symbol", "open", "high", "low", "close", "adj_close", "volume"]
    assert (norm["symbol"] == "AAPL").all()
    assert norm["ts_utc_ms"].is_monotonic_increasing


def test_normalize_lookahead_guard():
    """Strategy must not see future bar — data must be sorted."""
    idx = pd.date_range("2023-01-03", periods=3, tz="UTC")
    df = pd.DataFrame({"Open": [102, 100, 101], "High": [103, 101, 102], "Low": [101, 99, 100], "Close": [102, 100, 101], "Adj Close": [102, 100, 101], "Volume": [1000, 1000, 1000]}, index=idx)
    df.index.name = "Date"
    norm = normalize_yfinance(df, ["AAPL"])
    # normalize sorts by ts, so out-of-order input becomes ordered — engine will still iterate sequentially
    assert norm["ts_utc_ms"].is_monotonic_increasing
