# Aptenodytes — Event-Driven Backtester (C++ core + Python)

> Learning-first quant dev project: Tier 2 bar backtester now, Tier 3 tick/order-book later.

## Shared Understanding (grilled spec)

| Decision             | Choice                                                                                                     |
| -------------------- | ---------------------------------------------------------------------------------------------------------- |
| **Goal**             | Learn quant dev (A) > systems (B) > usable product (C) — Python oracle first, then C++ port                |
| **Core**             | C++20 `engine/` with `nanobind` Python bindings; Rust dropped for MVP, port later                          |
| **Fidelity**         | Target: one US equity, daily bars, USD long-only cash account; explicit open/close event phases             |
| **Data**             | Target: validated UTC-ms data and consistently adjusted OHLC; synthetic correctness fixtures first         |
| **Orders**           | Market-only v0.1, fill at next bar open, `commission_bps + slippage_bps`                                   |
| **Strategies**       | `Strategy` base (`on_bar`, `on_fill`), built-ins: `SmaCrossover`, `BuyAndHold`                             |
| **Metrics**          | Target: explicit conventions for Sharpe, Sortino, CAGR, MaxDD, volatility, and turnover                    |
| **Correctness**      | Target: independent expected ledgers, behavioral checks, and mandatory Python/C++ result parity at 1e-9   |
| **Seams for Tier 3** | `DataFeed` + `ExecutionModel` abstract; only `BarFeed` + `MarketOnNextOpen` implemented                    |

## Layout

```
engine/include/aptenodytes/   # C++ headers
engine/src/                   # C++ impl
engine/tests/                 # GoogleTest
python/backtester/            # Python package (oracle + bindings + data helpers)
python/tests/                 # pytest (golden parity)
data/raw/  data/processed/    # gitkeep — CSVs/parquets ignored
research/example.py           # current research stub
```

## Development gates

The agreed [development plan](DEVELOPMENT_PLAN.md) replaces the original weekly
roadmap with simulation contracts, ownership, verification evidence, and ordered
acceptance gates. The implementation is a scaffold; no gate is complete yet.

1. **G0** — Reproducible development environment and fresh native build
2. **G1** — Validated inputs and independently calculated fixtures
3. **G2** — Correct Python event lifecycle, accounting, and order outcomes
4. **G3** — Runnable C++ bindings and mandatory complete-result parity
5. **G4** — Defined metrics with independent numerical checks
6. **G5** — Adjusted data, frozen datasets, and reproducible run manifests
7. **G6** — Offline SMA vs buy-and-hold research demonstration

Dates depend on available development capacity. New order types, multi-asset
portfolios, and tick data remain deferred until the foundation passes.

## Current scaffold quickstart

These commands describe the existing scaffold, not the planned clean-checkout
verification workflow. Building alone does not currently make the extension
importable from the Python package; that delivery gap is tracked in G0/G3.

```bash
# Python env (uv recommended)
uv venv && source .venv/bin/activate
uv pip install -e ".[dev]"  # or: pip install -e ".[dev]"

# Download data (Python owns ingestion)
uv run python -m backtester.data --tickers AAPL,MSFT --period 5y --out data/processed/

# Run Python tests (native parity is currently only an import smoke test)
uv run pytest python/tests -v

# Build C++ core + bindings
cmake --preset dev
cmake --build build/dev
ctest --preset dev --output-on-failure

# Inspect the package (native run API and import delivery are tracked in G0/G3)
uv run python -c "import backtester; print(backtester.__doc__)"
```

## Target invariants (not yet fully enforced)

- **Causality**: orders from any callback become eligible only on a later bar.
- **Input contract**: single-symbol bars arrive in strictly increasing UTC-ms order;
  invalid input fails explicitly. The adjusted-price model is retrospective.
- **Accounting**: equity equals cash plus marked holdings; no borrowing or shorting.
- **Parity**: compare complete Python/C++ results against independent fixtures.
  The current parity test only checks import and can skip; G3 replaces it.

## Tier 3 notes (do not implement now)

Leave `DataFeed` and `ExecutionModel` virtual. Tier 3 will add `TickFeed` + `OrderBookExecution` without touching `Portfolio`/`Strategy`.

## Contributing / Learning log

See `research/example.py` for the current experiment stub. Rust remains deferred.
