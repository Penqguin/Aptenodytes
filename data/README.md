# Data

- `data/raw/` — raw yfinance CSV dumps (gitignored, keep for audit)
- `data/processed/` — normalized parquet per symbol, schema:
  `ts_utc_ms (int64 UTC ms), symbol, open, high, low, close, adj_close, volume`
  Sorted by (symbol, ts_utc_ms), tz-aware UTC.

Download:
```bash
uv run python -m backtester.data --tickers AAPL,MSFT --period 5y
# or via research stub
uv run python research/example.py --tickers AAPL --period 1y
```
