"""Metrics matching C++ metrics.h (for parity)."""
import math


def compute_metrics(equity_curve: list[float], periods_per_year: int = 252) -> dict:
    if len(equity_curve) < 2:
        return dict(cagr=0, sharpe=0, sortino=0, max_drawdown=0, volatility=0, total_return=0)
    total = (equity_curve[-1] - equity_curve[0]) / equity_curve[0]
    years = len(equity_curve) / periods_per_year
    cagr = (equity_curve[-1] / equity_curve[0]) ** (1 / years) - 1 if years > 0 else 0
    rets = [(equity_curve[i] - equity_curve[i - 1]) / equity_curve[i - 1] for i in range(1, len(equity_curve))]
    mean = sum(rets) / len(rets)
    var = sum((r - mean) ** 2 for r in rets) / len(rets)
    std = math.sqrt(var)
    vol = std * math.sqrt(periods_per_year)
    sharpe = (mean / std) * math.sqrt(periods_per_year) if std > 1e-12 else 0
    downside = [r for r in rets if r < 0]
    if downside:
        dvar = sum(r * r for r in downside) / len(downside)
        dstd = math.sqrt(dvar)
        sortino = (mean / dstd) * math.sqrt(periods_per_year) if dstd > 1e-12 else 0
    else:
        sortino = 0
    peak = equity_curve[0]
    max_dd = 0
    for v in equity_curve:
        peak = max(peak, v)
        max_dd = max(max_dd, (peak - v) / peak if peak else 0)
    return dict(cagr=cagr, sharpe=sharpe, sortino=sortino, max_drawdown=max_dd, volatility=vol, total_return=total)
