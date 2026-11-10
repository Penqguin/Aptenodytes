# Aptenodytes development plan

Agreed through the design interview on 2026-11-05. This document supersedes the
original week-based roadmap. It describes intended behavior, not features already
implemented. No implementation gate is complete yet.

## Objective and working arrangement

Learn quant development first, systems engineering second, and product delivery
third. Build a trustworthy Python reference, then demonstrate that the C++20 core
implements the same independently checked simulation contract.

The developer owns selected core changes, explains expected behavior before
coding, and reviews the resulting traces. The assistant coaches, challenges
assumptions, reviews changes, and handles repetitive setup and verification.
Do not assign calendar dates until weekly capacity is known. Complete gates by
evidence, not elapsed time, code volume, or a profitable-looking equity curve.

## Observed starting point

Read-only checks during the interview:

| Check | Result | What it does not establish |
| --- | --- | --- |
| Python tests | 5 passed, 1 skipped | The skipped native test compares no backtest results even when import succeeds |
| C++ CTest | 6 passed | Existing binaries were used; no fresh build was verified |
| Ruff | 26 violations | The current tree is not lint-clean |
| Native import | Unavailable in the Python 3.12 environment | An existing build artifact targets CPython 3.13 |

Reproducible defects and delivery gaps:

- `python/backtester/engine.py`: `on_fill` can append to the live execution queue
  and cause a same-open fill or an unbounded execution loop.
- `python/backtester/data.py`: timestamp conversion assumes nanosecond datetime
  storage. Installed pandas produces microsecond-backed dates, yielding values
  1,000 times smaller than UTC milliseconds.
- Both engines value only the current symbol. A two-symbol constant-price example
  reports equity of 900 when cash plus holdings equals 1,000. The first release
  must reject this unsupported input rather than appear to support it.
- `python/tests/test_parity.py` checks import only; bindings expose no backtest
  entrypoint. CI runs Python tests before native compilation and never afterward.
- Both metric implementations use the number of equity levels rather than return
  intervals for annualization; Python also divides by zero for some equity paths.
- Real-data ingestion has no complete adjustment/validation contract, dependency
  lock, or dataset manifest. The research script silently substitutes synthetic
  data after any download/processing exception.

Passing the existing suite is a baseline, not a correctness certificate.

## Agreed simulation contract

### Supported model

- One US equity, daily bars, USD, one instrument per run. Reject multi-symbol
  input before processing. Keep the feed/execution seams, without implementing
  ticks, an order book, or portfolio scheduling now.
- Long-only cash account; positive finite initial capital; fractional quantities
  allowed. No borrowing, shorting, cash interest, partial fills, or volume limits.
- Market orders only. Fill at the next available bar's open with adverse
  proportional slippage and commission on absolute executed notional.
- Require finite nonnegative commission and `0 <= slippage_bps < 10000`. Remove
  the arbitrary 0.01 execution-price floor; invalid inputs fail explicitly.
- Resolve eligible orders FIFO against updated cash and positions. Reject a whole
  order if the actual gap-open execution cost exceeds cash or a sell exceeds
  holdings. Do not resize it silently. Record the rejection and notify strategy.
- Preserve tiny floating-point tolerances explicitly in accounting checks; do not
  allow tolerances to hide meaningful borrowing or short positions.

### Event phases and information boundaries

For each strictly increasing bar timestamp:

1. Freeze the orders eligible at this open: only orders from earlier bars qualify.
2. Resolve that batch FIFO. Apply each accepted fill before notifying its callback;
   record and notify each rejection. Callback-created orders cannot join this batch.
3. At the close, mark cash plus holdings at the current close.
4. Call `on_bar` with the completed bar and current portfolio state. Its orders also
   wait for a later bar.

At open-time callbacks, do not expose the current bar's future high, low, or close.
Timestamp, event phase, and order sequence distinguish open and close events even
if the stored daily bar uses a single timestamp label. Do not manufacture intraday
timestamps from daily data. End-of-data orders expire with explicit records;
remaining holdings stay valued at the final close. There is no forced liquidation.

Use fresh strategy/feed/portfolio state per run. Two runs with identical fresh
inputs must produce identical events and results. Validate SMA parameters before
execution (`0 < fast < slow`, valid quantity); do not let invalid periods reach
C++ indexing operations.

### Data boundaries and adjustment policy

- Engine input must already be chronological. Reject backward/duplicate timestamps,
  multiple symbols, invalid timestamp representation, nonfinite values, nonpositive
  OHLC, invalid high/low relationships, and negative volume. Zero volume is allowed
  under the documented model that does not limit fills by volume.
- Ingestion may explicitly sort, but must report its normalization and all dropped
  rows or missing data. Do not silently deduplicate or fabricate missing bars.
  Report observed gaps without claiming full exchange-calendar completeness.
- Test UTC-millisecond values exactly for multiple datetime storage resolutions;
  column names and sortedness alone are insufficient evidence of correct units.
- Synthetic fixtures establish correctness first. Real-data support uses a labeled,
  retrospectively adjusted-price research model: apply `adj_close / close` to all
  OHLC at one conversion boundary, require positive finite factors, and apply no
  separate split/dividend events or credits. Retain provider volume as metadata;
  it does not constrain fills. Mark adjustment state to prevent double adjustment.
- Prices and quantities are units of that research model, not a reconstruction of
  executable historical shares. Do not claim point-in-time data fidelity. Raw-price
  execution with explicit corporate actions is a later project.

### Result and metric contracts

The result must expose initial capital, timestamped close equity, final cash and
positions, and an ordered audit trail of submitted orders, fills, rejections, and
expirations with stable IDs and reason codes. Strategies receive only the state
and events available at their callback phase, not mutable engine internals.

| Metric | Agreed convention |
| --- | --- |
| Total return | Final marked equity / initial capital - 1; no external cash flows |
| CAGR | Growth over actual elapsed timestamp duration, using 365.25 days per year |
| Volatility | Sample standard deviation of simple returns, annualized with explicit periods/year |
| Sharpe | Zero risk-free rate; sample standard deviation; 252 periods/year for daily observations |
| Sortino | Mean return / downside deviation, times sqrt(periods/year); zero target; downside deviation = sqrt(sum(min(return, 0)^2) / all return observations) |
| Max drawdown | Peak-to-trough fraction, including initial capital as the starting peak |
| Turnover | Sum of absolute executed notionals / mean sampled close equity; no division by two |

Unavailable statistics carry a reason; do not substitute zero for undefined
statistics. Cover empty/short series, zero variance, no downside observations,
zero equity, invalid annualization, and zero elapsed duration. Defined zero-valued
statistics remain zero (for example drawdown on a flat positive path). Return-based
statistics with zero denominators are unavailable. Surface irregular sampling and
insufficient-history limitations. The initial no-position close sample supplies
the initial equity anchor; do not invent an earlier timestamp for CAGR.

For zero bars, return unchanged initial cash, no positions/events, and an empty
equity series; performance statistics are unavailable. With one bar and no fills,
total return, turnover, and drawdown are zero; elapsed-time and return-distribution
statistics are unavailable. Retain metric availability reasons in parity results.

### Reproducibility

Lock Python dependencies and use the same selected interpreter for package,
nanobind build, and tests. Verify Linux CI and the local Mac workflow. A future
documented verification command must bootstrap the environment, build native code,
run checks, and execute an offline example. Dependency installation may need the
network; correctness tests and the example must not download market data.

Research runs record configuration, engine/backend version, source revision and
dirty state, dependency lock identity, input data hash, adjustment policy, and
metric conventions. Frozen datasets include source/request/retrieval metadata.
Keep small synthetic fixtures under tests outside ignored download directories.
Use permitted frozen real data for the research demonstration; do not depend on
relative provider requests such as `period=1y` to reproduce a run.

## Gates and dependency order

`G0 build baseline -> G1 data and independent fixtures -> G2 Python semantics ->
G3 C++ delivery and parity -> G4 metrics -> G5 real-data reproducibility -> G6 research`

Keep changes small enough to review independently. Where practical, add the failing
case before its fix. A downstream gate cannot substitute for an upstream gate.

| Gate | Scope and likely files | Exit evidence | Ownership |
| --- | --- | --- | --- |
| G0: reproducible development baseline | `pyproject.toml`, lockfile, `CMakeLists.txt`, `CMakePresets.json`, CI, verification entrypoint | Fresh native build/import with the chosen Python; CTest and existing Python tests run; lint passes; no stale binary supplies evidence. Document the temporary absence of real parity until G3. | Assistant handles setup; developer reviews build/interpreter flow |
| G1: input contract and independent fixtures | `python/backtester/data.py`, engine input boundary, `python/tests/test_data.py`, new fixtures/expected ledgers | Exact UTC-ms regression; invalid input/multi-symbol rejection cases; independently calculated fill/cash/position/equity ledger including costs and gaps. Expected values are not generated by either engine. | Developer owns ledger and a selected validation change; assistant reviews and scaffolds |
| G2: Python reference correctness | `python/backtester/engine.py`, `strategy.py`, oracle tests | Frozen execution batches; explicit order outcomes; supported-account constraints; correct marks; phase-aware callbacks; full required fixture matrix passes. | Developer owns event loop/accounting changes with assistant review |
| G3: C++ contract and mandatory parity | C++ engine/execution/portfolio/strategy headers and sources, native tests, `python/src/bindings.cpp`, parity tests, CI | Native run/results callable from Python; fixtures pass independently in both engines; compiled CI fails on unavailable extension or divergent results. | Developer ports validated behavior; assistant assists bindings/build and reviews lifetime handling |
| G4: metrics | Python/C++ metrics and focused tests | Independently calculated numeric cases, undefined-value reasons, initial-capital and elapsed-time handling; metric parity required. | Developer explains formulas and implements selected metrics; assistant checks numerical cases |
| G5: adjusted data and run manifests | `data.py`, data tests, dataset/run manifests, data documentation | Consistent adjusted OHLC, no double adjustment, exact schema validation, frozen hashed dataset and reproducible request metadata; errors never trigger synthetic fallback. | Shared; developer owns adjustment reasoning |
| G6: reproducible research demonstration | `research/example.py`, report/export helpers only if needed, README | One offline SMA vs buy-and-hold run on identical frozen data, with audit logs, costs, timestamped equity/drawdown, metrics and assumptions; documented verification command works from clean checkout. | Developer interprets experiment; assistant handles routine report generation |

### Minimum correctness matrix for G1–G3

| Case | Required observation |
| --- | --- |
| First-bar market order with a gap next open | Fill uses next open, never signal close |
| Order from `on_fill` or rejection callback | Cannot execute in the current frozen batch |
| Several pending orders | Stable FIFO; later cash checks see earlier fills |
| Buy and sell with nonzero costs | Slippage direction, commissions, cash, holdings and equity match independent ledger |
| Gap makes a buy unaffordable | Entire order rejected; balances unchanged; reason recorded |
| Oversell, malformed order, invalid strategy parameters | Explicit failure/outcome; no accidental short or unsafe indexing |
| Fractional round trip | Supported precision; no meaningful residual holding or accounting drift |
| No orders; zero/one bar; final-bar order | Defined result shape; no invented fills; expiration visible |
| SMA warm-up, crossing, and equality boundary | Exact signal bars, orders, fill timestamps and position path |
| Changed unseen future data | Earlier events/results unchanged; callbacks cannot inspect future bar fields |
| Two fresh identical runs | Same ordered events, balances, marks, and results |
| Duplicate/backward timestamps, bad OHLC, nonfinite input, second symbol | Rejected before any execution/state mutation |

Compare event types, IDs, reasons, symbols and timestamps exactly. Compare finite
numerical fixture/parity values with absolute tolerance 1e-9; undefined statuses
must match exactly. On failure print the first divergent event and relevant state.
Do not silently relax tolerances or treat matching summary metrics as full parity.

Keep the Python-only job for reference development. The compiled CI job must build
and import the extension before running native-backed parity; a skip is a failure
there. Run the complete suite at gate boundaries and focused checks during changes.

## First implementation sessions

1. Assistant completes G0 setup in a small reviewable change. Developer explains
   which Python executable supplies the extension ABI and how CI proves it imports.
2. Developer writes a short expected ledger before changing the event loop: begin
   with 1,000 cash; queue a buy of 2 at the first close; next open is 110, close 115;
   commission is 10 bps and slippage 100 bps. Expected fill is 111.10, commission
   0.2222, cash 777.5778, position 2, close equity 1007.5778. Extend it with an order
   from `on_fill` that must wait for another open.
3. Assistant reviews the arithmetic and turns the agreed ledger into fixture
   scaffolding; developer implements the selected G1 validation regression.
4. For G2, developer explains queue ownership and callback timing before coding.
   Review a trace showing no callback-created order enters the active batch.

After each change, record the gate, evidence, and next small task here or in its
linked issue. Do not mark a gate complete because its files exist. The first
engineering work is G0/G1, not another strategy or an execution optimization.

## Deferred work and reopening criteria

- Multi-asset portfolios: revisit only after G6, with a separate timestamp-batch,
  retained-mark, calendar, and capital-allocation design.
- Raw corporate actions: introduce explicit split/dividend accounting and a new
  dataset contract before claiming raw historical execution fidelity.
- Limit/stop orders, partial fills, volume participation, leverage and shorting:
  require new execution/account contracts and independent fixtures.
- Tick/order-book simulation, Rust ports, packaging for public distribution, and
  broad optimization: separate milestones, not hidden requirements of this plan.
- Performance: establish a measured baseline after correctness/parity. Define a
  representative workload and target before optimizing; no arbitrary speed claim.
- Parameter search and profitability claims: require a separate research protocol
  for data selection, out-of-sample evaluation, and selection bias. The first
  demonstration establishes traceability, not a trading recommendation.
