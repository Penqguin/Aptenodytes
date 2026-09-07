# Aptenodytes — Event-Driven Backtester (C++ core + Python)

> Learning-first quant dev project: Tier 2 bar backtester now, Tier 3 tick/order-book later.

## Shared Understanding (grilled spec)

| Decision | Choice |
|---|---|
| **Goal** | Learn quant dev (A) > systems (B) > usable product (C) — Python oracle first, then C++ port |
| **Core** | C++20 `engine/` with `nanobind` Python bindings; Rust dropped for MVP, port later |
| **Fidelity** | Tier 2 event-driven bars: `DataFeed → Strategy.on_bar → ExecutionModel (MarketOnNextOpen) → Portfolio` |
| **Data** | Python owns ingestion: `yfinance` → `data/raw/*.csv` → `data/processed/*.parquet` (UTC, adj_close, sorted) |
| **Orders** | Market-only v0.1, fill at next bar open, `commission_bps + slippage_bps` |
| **Strategies** | `Strategy` base (`on_bar`, `on_fill`), built-ins: `SmaCrossover`, `BuyAndHold` |
| **Metrics** | Sharpe, Sortino, CAGR, MaxDD, turnover from equity curve |
| **Correctness** | Golden file: SMA(10,20) on AAPL 2023 hand-calc’d 5 trades; Python vs C++ curves must match 1e-9 |
| **Seams for Tier 3** | `DataFeed` + `ExecutionModel` abstract; only `BarFeed` + `MarketOnNextOpen` implemented |

## Layout

```
engine/include/aptenodytes/  # C++ headers
engine/src/                   # C++ impl
engine/tests/                 # GoogleTest
python/backtester/            # Python package (oracle + bindings + data helpers)
python/tests/                 # pytest (golden parity)
data/raw/  data/processed/    # gitkeep — CSVs/parquets ignored
research/example.ipynb        # yfinance → backtester → plot
```

## Incremental Milestones (4–6 weeks)

1. **Week 1 — Python oracle**: `python/backtester/engine.py` fully working, passes golden test, no C++ needed
2. **Week 2 — C++ scaffold + Portfolio/BarFeed**: `apten_core` builds, GoogleTests pass
3. **Week 3 — Strategy + Execution + nanobind**: `import backtester` runs same SMA as oracle, parity test passes
4. **Week 4 — Metrics + research notebook**: equity curve → Sharpe/MaxDD + pretty plot
5. **Stretch — Tier 2.5**: limit orders, `BarFeed` → `TickFeed` seam exercise

## Quickstart

```bash
# Python env (uv recommended)
uv venv && source .venv/bin/activate
uv pip install -e ".[dev]"  # or: pip install -e ".[dev]"

# Download data (Python owns ingestion)
uv run python -m backtester.data --tickers AAPL,MSFT --period 5y --out data/processed/

# Run Python oracle directly (no C++ needed for week 1)
uv run pytest python/tests -v

# Build C++ core + bindings
cmake --preset dev
cmake --build build/dev
ctest --preset dev --output-on-failure

# Use C++ engine from Python (after build)
uv run python -c "import backtester; print(backtester.__doc__)"
```

## Design Invariants (enforced)

- **No lookahead**: strategy sees `bar[t]` only after `on_bar(bar[t])` decides for `t+1`; filled at `bar[t+1].open`.
- **Point-in-time**: `DataFeed` yields bars sorted UTC; never reorders.
- **Parity**: `python/tests/test_parity.py` fails if Python oracle vs C++ (`_apten_core`) diverge.

## Tier 3 notes (do not implement now)

Leave `DataFeed` and `ExecutionModel` virtual. Tier 3 will add `TickFeed` + `OrderBookExecution` without touching `Portfolio`/`Strategy`.

## Contributing / Learning log

See `research/` notebooks for experiments. Rust port lives in `rust/` (future).
