"""
Shared backtest logic for POC notebooks: futures-style long/short, costs, regime routing,
combined imbalance filters, and honest hold-out evaluation.
"""

from __future__ import annotations

import os
from collections import deque
from dataclasses import dataclass
from typing import Callable, Iterable, Literal

import numpy as np
import pandas as pd
import polars as pl

from poc_trend_detection import per_tick_trend_state

# --- Defaults (override in notebook) ---

INITIAL_BALANCE = 1000.0
COOLDOWN = 500

FEE = 0.0004
SPREAD = 0.0001
SLIPPAGE = 0.0001
TOTAL_COST = FEE + SPREAD + SLIPPAGE

TP = 0.007
SL = 0.004

IMBALANCE_WINDOW = 2000
MOMENTUM_LOOKBACK = 100
BREAKOUT_LOOKBACK = 500

# Adaptive TP/SL: scale by rolling tick volatility vs median (entry snapshot)
ADAPTIVE_VOL_WINDOW = 2000
ADAPTIVE_VOL_MULT_MIN = 0.75
ADAPTIVE_VOL_MULT_MAX = 1.35

# Grids (smaller than old notebook: fewer combinations = less multiple-testing pain)
TP_SL_PRESETS = {
    "tight": (0.003, 0.002),
    "balanced": (0.007, 0.004),
    "swing": (0.012, 0.006),
}

MEAN_DELTAS = [0.001, 0.002, 0.003]
MOMENTUM_THRESHOLDS = [0.0005, 0.001, 0.002]
BREAKOUT_LOOKBACKS = [300, 500, 800]
IMBALANCE_THRESHOLDS = [1.0, 1.25, 1.5]  # for combined filters only


@dataclass
class BacktestResult:
    total_pnl: float
    trades: int
    win_rate: float
    avg_win: float
    avg_loss: float
    max_drawdown: float
    profit_factor: float
    sharpe: float


@dataclass
class YearlyReport:
    strategy_key: str
    start_balance: float
    end_balance: float
    total_return_pct: float
    monthly: pd.DataFrame
    month_metrics: list
    tp: float
    sl: float
    eval_label: str  # "full_year" | "holdout_oos" | "train_only"


yearly_reports: dict[str, YearlyReport] = {}


def _rising_edge_long_short(signals: list[str]) -> list[str]:
    """Emit long/short only on transition into that direction from flat intent."""
    out: list[str] = []
    prev_long = False
    prev_short = False
    for s in signals:
        want_long = s == "long"
        want_short = s == "short"
        if want_long and not prev_long and not want_short:
            out.append("long")
        elif want_short and not prev_short and not want_long:
            out.append("short")
        else:
            out.append("hold")
        prev_long = want_long
        prev_short = want_short
    return out


def simulate_trades_futures(
    prices: np.ndarray,
    signals: list[str],
    initial_balance: float | None = None,
    tp: float | None = None,
    sl: float | None = None,
    edge_entries: bool = True,
    flatten_at_end: bool = True,
    *,
    adaptive_tp_sl: bool = False,
    vol_window: int = ADAPTIVE_VOL_WINDOW,
    vol_mult_min: float = ADAPTIVE_VOL_MULT_MIN,
    vol_mult_max: float = ADAPTIVE_VOL_MULT_MAX,
) -> tuple[list[tuple[float, float]], list[float], float]:
    """
    Linear futures-style: full notional long or short, symmetric costs.
    signals: "long" | "short" | "hold"

    If adaptive_tp_sl is True, TP/SL distances at entry are scaled by
    tp_sl_volatility_multipliers (high recent vol -> wider bands, low vol -> tighter).
    """
    if initial_balance is None:
        initial_balance = float(INITIAL_BALANCE)
    if tp is None:
        tp = TP
    if sl is None:
        sl = SL

    if edge_entries:
        signals = _rising_edge_long_short(signals)

    price_arr = np.asarray(prices, dtype=float)
    mult = (
        tp_sl_volatility_multipliers(price_arr, vol_window, vol_mult_min, vol_mult_max)
        if adaptive_tp_sl
        else None
    )

    balance = float(initial_balance)
    side = 0  # 0 flat, 1 long, -1 short
    qty = 0.0
    entry_price = 0.0
    cash_at_entry = 0.0
    pos_tp = float(tp)
    pos_sl = float(sl)

    trades: list[tuple[float, float]] = []
    equity_curve: list[float] = []
    cooldown = 0
    last_price = float(price_arr[0]) if len(price_arr) else 0.0

    n = len(price_arr)
    for i in range(n):
        last_price = float(price_arr[i])
        signal = signals[i]

        if cooldown > 0:
            cooldown -= 1
            signal = "hold"

        m = float(mult[i]) if adaptive_tp_sl and mult is not None else 1.0

        if side == 0:
            if signal == "long":
                cash_at_entry = balance
                entry_price = last_price * (1.0 + TOTAL_COST)
                qty = cash_at_entry / entry_price
                balance = 0.0
                side = 1
                pos_tp = float(tp) * m
                pos_sl = float(sl) * m
            elif signal == "short":
                cash_at_entry = balance
                entry_price = last_price * (1.0 - TOTAL_COST)
                qty = cash_at_entry / entry_price
                balance = 0.0
                side = -1
                pos_tp = float(tp) * m
                pos_sl = float(sl) * m

        elif side == 1:
            tp_fill = entry_price * (1.0 + pos_tp)
            sl_fill = entry_price * (1.0 - pos_sl)
            if last_price >= tp_fill:
                exit_price = tp_fill * (1.0 - TOTAL_COST)
                balance = qty * exit_price
                trades.append((balance - cash_at_entry, cash_at_entry))
                side = 0
                qty = 0.0
                cooldown = COOLDOWN
            elif last_price <= sl_fill:
                exit_price = sl_fill * (1.0 - TOTAL_COST)
                balance = qty * exit_price
                trades.append((balance - cash_at_entry, cash_at_entry))
                side = 0
                qty = 0.0
                cooldown = COOLDOWN

        elif side == -1:
            tp_fill = entry_price * (1.0 - pos_tp)
            sl_fill = entry_price * (1.0 + pos_sl)
            if last_price <= tp_fill:
                exit_buy = tp_fill * (1.0 + TOTAL_COST)
                balance = cash_at_entry + qty * (entry_price - exit_buy)
                trades.append((balance - cash_at_entry, cash_at_entry))
                side = 0
                qty = 0.0
                cooldown = COOLDOWN
            elif last_price >= sl_fill:
                exit_buy = sl_fill * (1.0 + TOTAL_COST)
                balance = cash_at_entry + qty * (entry_price - exit_buy)
                trades.append((balance - cash_at_entry, cash_at_entry))
                side = 0
                qty = 0.0
                cooldown = COOLDOWN

        if side == 0:
            eq = balance
        elif side == 1:
            eq = qty * last_price
        else:
            eq = cash_at_entry + qty * (entry_price - last_price)
        equity_curve.append(eq)

    if flatten_at_end and side != 0:
        if side == 1:
            exit_price = last_price * (1.0 - TOTAL_COST)
            balance = qty * exit_price
        else:
            exit_buy = last_price * (1.0 + TOTAL_COST)
            balance = cash_at_entry + qty * (entry_price - exit_buy)
        trades.append((balance - cash_at_entry, cash_at_entry))
        side = 0
        qty = 0.0
        if equity_curve:
            equity_curve[-1] = balance

    return trades, equity_curve, balance


def build_metrics(trades: list[tuple[float, float]], equity: list[float]) -> BacktestResult:
    if not trades:
        return BacktestResult(0, 0, 0, 0, 0, 0, 0, 0)

    pnls = np.array([t[0] for t in trades], dtype=float)
    caps = np.array([t[1] for t in trades], dtype=float)

    total_pnl = float(pnls.sum())
    wins = pnls[pnls > 0]
    losses = pnls[pnls < 0]

    win_rate = float(len(wins) / len(trades))
    avg_win = float(np.mean(wins)) if len(wins) else 0.0
    avg_loss = float(np.mean(losses)) if len(losses) else 0.0

    if losses.sum() != 0:
        profit_factor = abs(float(wins.sum() / losses.sum()))
    elif wins.sum() > 0:
        profit_factor = float("inf")
    else:
        profit_factor = 0.0

    eq = np.asarray(equity, dtype=float)
    drawdown = float(np.max(np.maximum.accumulate(eq) - eq))

    with np.errstate(divide="ignore", invalid="ignore"):
        rets = np.where(caps > 0, pnls / caps, 0.0)
    rets = rets[np.isfinite(rets)]
    std_ret = float(np.std(rets, ddof=1)) if len(rets) > 1 else 0.0
    min_std = 1e-9
    if len(rets) > 1 and std_ret > 0:
        sharpe = float(np.clip(np.mean(rets) / max(std_ret, min_std), -50.0, 50.0))
    else:
        sharpe = 0.0

    return BacktestResult(
        total_pnl,
        len(trades),
        win_rate,
        avg_win,
        avg_loss,
        drawdown,
        profit_factor,
        sharpe,
    )


def _rolling_imbalance_arrays(df: pl.DataFrame, window: int) -> tuple[np.ndarray, np.ndarray]:
    prices = df["price"].to_numpy()
    qty = df["quantity"].to_numpy()
    is_sell = df["is_buyer_maker"].to_numpy()
    buy_vol = 0.0
    sell_vol = 0.0
    q: deque = deque()
    imb = np.zeros(len(prices), dtype=float)
    for i in range(len(prices)):
        side_buy = not bool(is_sell[i])
        v = float(qty[i])
        q.append((side_buy, v))
        if side_buy:
            buy_vol += v
        else:
            sell_vol += v
        if len(q) > window:
            ob, ov = q.popleft()
            if ob:
                buy_vol -= ov
            else:
                sell_vol -= ov
        if sell_vol <= 0:
            imb[i] = 1.0
        else:
            imb[i] = buy_vol / sell_vol
    return prices, imb


def imbalance_filter_ok(direction_long: bool, imb: float, threshold: float) -> bool:
    if direction_long:
        return imb > threshold
    return imb < (1.0 / threshold)


def rolling_tick_volatility(prices: np.ndarray, window: int) -> np.ndarray:
    """
    Per-tick rolling std of log returns over `window` trades.
    Early ticks reuse the first full-window estimate so multipliers are always defined.
    """
    p = np.maximum(np.asarray(prices, dtype=float), 1e-12)
    n = len(p)
    lr = np.diff(np.log(p))
    out = np.ones(n, dtype=float)
    if len(lr) < window:
        return out
    rv_lr = pd.Series(lr).rolling(window, min_periods=window).std()
    # vol at price index i matches std of lr[i-window : i]
    out[window:] = rv_lr.iloc[window - 1 :].to_numpy()
    out[:window] = out[window]
    return out


def tp_sl_volatility_multipliers(
    prices: np.ndarray,
    window: int,
    mult_min: float,
    mult_max: float,
) -> np.ndarray:
    """
    Multiplier ~1 at median vol; >1 when recent vol is high (wider TP/SL), <1 when calm.
    """
    v = rolling_tick_volatility(prices, window)
    med = float(np.median(v))
    if med < 1e-15:
        return np.ones(len(prices))
    m = np.clip(v / med, mult_min, mult_max)
    return m


def _simulate_with_optional_adaptive(
    prices: np.ndarray,
    signals: list[str],
    *,
    initial_balance: float,
    tp: float,
    sl: float,
    adaptive_tp_sl: bool = False,
    vol_window: int = ADAPTIVE_VOL_WINDOW,
    vol_mult_min: float = ADAPTIVE_VOL_MULT_MIN,
    vol_mult_max: float = ADAPTIVE_VOL_MULT_MAX,
) -> tuple[list[tuple[float, float]], list[float], float]:
    return simulate_trades_futures(
        prices,
        signals,
        initial_balance=initial_balance,
        tp=tp,
        sl=sl,
        adaptive_tp_sl=adaptive_tp_sl,
        vol_window=vol_window,
        vol_mult_min=vol_mult_min,
        vol_mult_max=vol_mult_max,
    )


# --- Strategies (return metrics, end_bal) ---


def mean_strategy_futures(
    df: pl.DataFrame,
    delta: float = 0.002,
    vwap_window: int = 200,
    initial_balance: float = INITIAL_BALANCE,
    tp: float = TP,
    sl: float = SL,
    adaptive_tp_sl: bool = False,
    vol_window: int = ADAPTIVE_VOL_WINDOW,
    vol_mult_min: float = ADAPTIVE_VOL_MULT_MIN,
    vol_mult_max: float = ADAPTIVE_VOL_MULT_MAX,
) -> tuple[BacktestResult, float]:
    prices = df["price"].to_numpy()
    qty = df["quantity"].to_numpy()
    window: deque = deque()
    sum_pq = 0.0
    sum_q = 0.0
    signals: list[str] = []

    for i in range(len(prices)):
        p, qv = float(prices[i]), float(qty[i])
        window.append((p, qv))
        sum_pq += p * qv
        sum_q += qv
        if len(window) > vwap_window:
            op, oq = window.popleft()
            sum_pq -= op * oq
            sum_q -= oq
        vwap = sum_pq / sum_q if sum_q > 0 else p
        if p < vwap * (1.0 - delta):
            signals.append("long")
        elif p > vwap * (1.0 + delta):
            signals.append("short")
        else:
            signals.append("hold")

    trades, equity, end_bal = _simulate_with_optional_adaptive(
        prices,
        signals,
        initial_balance=initial_balance,
        tp=tp,
        sl=sl,
        adaptive_tp_sl=adaptive_tp_sl,
        vol_window=vol_window,
        vol_mult_min=vol_mult_min,
        vol_mult_max=vol_mult_max,
    )
    return build_metrics(trades, equity), end_bal


def momentum_strategy_futures(
    df: pl.DataFrame,
    momentum_threshold: float = 0.001,
    lookback: int = MOMENTUM_LOOKBACK,
    initial_balance: float = INITIAL_BALANCE,
    tp: float = TP,
    sl: float = SL,
    adaptive_tp_sl: bool = False,
    vol_window: int = ADAPTIVE_VOL_WINDOW,
    vol_mult_min: float = ADAPTIVE_VOL_MULT_MIN,
    vol_mult_max: float = ADAPTIVE_VOL_MULT_MAX,
) -> tuple[BacktestResult, float]:
    prices = df["price"].to_numpy()
    n = len(prices)
    signals = ["hold"] * n
    for i in range(lookback, n):
        ref = prices[i - lookback]
        if ref <= 0:
            continue
        change = (prices[i] - ref) / ref
        if change > momentum_threshold:
            signals[i] = "long"
        elif change < -momentum_threshold:
            signals[i] = "short"
    trades, equity, end_bal = _simulate_with_optional_adaptive(
        prices,
        signals,
        initial_balance=initial_balance,
        tp=tp,
        sl=sl,
        adaptive_tp_sl=adaptive_tp_sl,
        vol_window=vol_window,
        vol_mult_min=vol_mult_min,
        vol_mult_max=vol_mult_max,
    )
    return build_metrics(trades, equity), end_bal


def breakout_strategy_futures(
    df: pl.DataFrame,
    lookback: int = BREAKOUT_LOOKBACK,
    breakout_eps: float = 0.0005,
    initial_balance: float = INITIAL_BALANCE,
    tp: float = TP,
    sl: float = SL,
    adaptive_tp_sl: bool = False,
    vol_window: int = ADAPTIVE_VOL_WINDOW,
    vol_mult_min: float = ADAPTIVE_VOL_MULT_MIN,
    vol_mult_max: float = ADAPTIVE_VOL_MULT_MAX,
) -> tuple[BacktestResult, float]:
    prices = df["price"].to_numpy()
    n = len(prices)
    signals = ["hold"] * n
    dq: deque = deque()
    dq_low: deque = deque()

    for i in range(n):
        if i > 0:
            j = i - 1
            pj = float(prices[j])
            while dq and float(prices[dq[-1]]) <= pj:
                dq.pop()
            dq.append(j)
            while dq_low and float(prices[dq_low[-1]]) >= pj:
                dq_low.pop()
            dq_low.append(j)
        while dq and dq[0] < i - lookback:
            dq.popleft()
        while dq_low and dq_low[0] < i - lookback:
            dq_low.popleft()

        if i >= lookback:
            p = float(prices[i])
            prior_high = float(prices[dq[0]])
            prior_low = float(prices[dq_low[0]])
            if p >= prior_high * (1.0 + breakout_eps):
                signals[i] = "long"
            elif p <= prior_low * (1.0 - breakout_eps):
                signals[i] = "short"

    trades, equity, end_bal = _simulate_with_optional_adaptive(
        prices,
        signals,
        initial_balance=initial_balance,
        tp=tp,
        sl=sl,
        adaptive_tp_sl=adaptive_tp_sl,
        vol_window=vol_window,
        vol_mult_min=vol_mult_min,
        vol_mult_max=vol_mult_max,
    )
    return build_metrics(trades, equity), end_bal


def momentum_with_imbalance_confirm(
    df: pl.DataFrame,
    threshold: float = 1.25,
    momentum_threshold: float = 0.001,
    lookback: int = MOMENTUM_LOOKBACK,
    imb_window: int = IMBALANCE_WINDOW,
    initial_balance: float = INITIAL_BALANCE,
    tp: float = TP,
    sl: float = SL,
    adaptive_tp_sl: bool = False,
    vol_window: int = ADAPTIVE_VOL_WINDOW,
    vol_mult_min: float = ADAPTIVE_VOL_MULT_MIN,
    vol_mult_max: float = ADAPTIVE_VOL_MULT_MAX,
) -> tuple[BacktestResult, float]:
    """Imbalance is NOT traded alone — only confirms momentum direction."""
    prices, imb = _rolling_imbalance_arrays(df, imb_window)
    n = len(prices)
    signals = ["hold"] * n
    for i in range(lookback, n):
        ref = prices[i - lookback]
        if ref <= 0:
            continue
        change = (prices[i] - ref) / ref
        if change > momentum_threshold:
            if imbalance_filter_ok(True, imb[i], threshold):
                signals[i] = "long"
        elif change < -momentum_threshold:
            if imbalance_filter_ok(False, imb[i], threshold):
                signals[i] = "short"
    trades, equity, end_bal = _simulate_with_optional_adaptive(
        prices,
        signals,
        initial_balance=initial_balance,
        tp=tp,
        sl=sl,
        adaptive_tp_sl=adaptive_tp_sl,
        vol_window=vol_window,
        vol_mult_min=vol_mult_min,
        vol_mult_max=vol_mult_max,
    )
    return build_metrics(trades, equity), end_bal


def mean_with_imbalance_confirm(
    df: pl.DataFrame,
    threshold: float = 1.25,
    delta: float = 0.002,
    vwap_window: int = 200,
    imb_window: int = IMBALANCE_WINDOW,
    initial_balance: float = INITIAL_BALANCE,
    tp: float = TP,
    sl: float = SL,
    adaptive_tp_sl: bool = False,
    vol_window: int = ADAPTIVE_VOL_WINDOW,
    vol_mult_min: float = ADAPTIVE_VOL_MULT_MIN,
    vol_mult_max: float = ADAPTIVE_VOL_MULT_MAX,
) -> tuple[BacktestResult, float]:
    """Mean-reversion long only if buy-side flow; short only if sell-side flow (contrarian with flow)."""
    prices, imb = _rolling_imbalance_arrays(df, imb_window)
    qty = df["quantity"].to_numpy()
    window: deque = deque()
    sum_pq = 0.0
    sum_q = 0.0
    signals: list[str] = []

    for i in range(len(prices)):
        p, qv = float(prices[i]), float(qty[i])
        window.append((p, qv))
        sum_pq += p * qv
        sum_q += qv
        if len(window) > vwap_window:
            op, oq = window.popleft()
            sum_pq -= op * oq
            sum_q -= oq
        vwap = sum_pq / sum_q if sum_q > 0 else p
        if p < vwap * (1.0 - delta):
            signals.append("long" if imb[i] > threshold else "hold")
        elif p > vwap * (1.0 + delta):
            signals.append("short" if imb[i] < (1.0 / threshold) else "hold")
        else:
            signals.append("hold")

    trades, equity, end_bal = _simulate_with_optional_adaptive(
        prices,
        signals,
        initial_balance=initial_balance,
        tp=tp,
        sl=sl,
        adaptive_tp_sl=adaptive_tp_sl,
        vol_window=vol_window,
        vol_mult_min=vol_mult_min,
        vol_mult_max=vol_mult_max,
    )
    return build_metrics(trades, equity), end_bal


def regime_meta_strategy(
    df: pl.DataFrame,
    trend_fast: int = 20,
    trend_slow: int = 50,
    mean_delta: float = 0.002,
    mom_thr: float = 0.001,
    br_lookback: int = 500,
    initial_balance: float = INITIAL_BALANCE,
    tp: float = TP,
    sl: float = SL,
    adaptive_tp_sl: bool = False,
    vol_window: int = ADAPTIVE_VOL_WINDOW,
    vol_mult_min: float = ADAPTIVE_VOL_MULT_MIN,
    vol_mult_max: float = ADAPTIVE_VOL_MULT_MAX,
) -> tuple[BacktestResult, float]:
    """
    trend 1  -> momentum + breakout confirmation (momentum takes priority on signal tick)
    trend -1 -> momentum (short bias) + breakout short
    trend 0  -> mean reversion
    Computed by composing signals (same TP/SL engine).
    """
    trend = per_tick_trend_state(df, fast=trend_fast, slow=trend_slow)
    prices = df["price"].to_numpy()
    n = len(prices)

    # Precompute child signal intentions (hold/long/short) as integers: 0,1,2
    mom_sig = ["hold"] * n
    for i in range(MOMENTUM_LOOKBACK, n):
        ref = prices[i - MOMENTUM_LOOKBACK]
        if ref <= 0:
            continue
        ch = (prices[i] - ref) / ref
        if ch > mom_thr:
            mom_sig[i] = "long"
        elif ch < -mom_thr:
            mom_sig[i] = "short"

    mean_sig = ["hold"] * n
    qty = df["quantity"].to_numpy()
    w: deque = deque()
    spq, sq = 0.0, 0.0
    for i in range(n):
        p, qv = float(prices[i]), float(qty[i])
        w.append((p, qv))
        spq += p * qv
        sq += qv
        if len(w) > 200:
            op, oq = w.popleft()
            spq -= op * oq
            sq -= oq
        vwap = spq / sq if sq > 0 else p
        if p < vwap * (1.0 - mean_delta):
            mean_sig[i] = "long"
        elif p > vwap * (1.0 + mean_delta):
            mean_sig[i] = "short"

    br_sig = ["hold"] * n
    dq_h: deque = deque()
    dq_l: deque = deque()
    for i in range(n):
        if i > 0:
            j = i - 1
            pj = float(prices[j])
            while dq_h and float(prices[dq_h[-1]]) <= pj:
                dq_h.pop()
            dq_h.append(j)
            while dq_l and float(prices[dq_l[-1]]) >= pj:
                dq_l.pop()
            dq_l.append(j)
        while dq_h and dq_h[0] < i - br_lookback:
            dq_h.popleft()
        while dq_l and dq_l[0] < i - br_lookback:
            dq_l.popleft()
        if i >= br_lookback:
            p = float(prices[i])
            ph = float(prices[dq_h[0]])
            pl = float(prices[dq_l[0]])
            if p >= ph * 1.0005:
                br_sig[i] = "long"
            elif p <= pl * 0.9995:
                br_sig[i] = "short"

    final: list[str] = []
    for i in range(n):
        tr = int(trend[i])
        if tr == 1:
            s = mom_sig[i] if mom_sig[i] != "hold" else br_sig[i]
        elif tr == -1:
            s = mom_sig[i] if mom_sig[i] != "hold" else br_sig[i]
        else:
            s = mean_sig[i]
        final.append(s)

    trades, equity, end_bal = _simulate_with_optional_adaptive(
        prices,
        final,
        initial_balance=initial_balance,
        tp=tp,
        sl=sl,
        adaptive_tp_sl=adaptive_tp_sl,
        vol_window=vol_window,
        vol_mult_min=vol_mult_min,
        vol_mult_max=vol_mult_max,
    )
    return build_metrics(trades, equity), end_bal


def run_backtest_yearly(
    strat_name: str,
    func: Callable,
    *,
    data_folder: str,
    symbol: str,
    year: int,
    months: Iterable[int],
    eval_label: str = "full_year",
    **params,
) -> None:
    params = dict(params)
    tp = float(params.pop("tp", TP))
    sl = float(params.pop("sl", SL))

    balance = float(INITIAL_BALANCE)
    rows = []
    month_metrics: list[BacktestResult] = []

    for month in months:
        month_str = f"{month:02d}"
        input_file = os.path.join(data_folder, f"{symbol}-aggTrades-{year}-{month_str}.parquet")
        if not os.path.exists(input_file):
            print(f"File {input_file} not found, skipping")
            continue

        print(f"Running [{eval_label}] {strat_name} {params} tp={tp} sl={sl} for {month_str}")

        df = pl.read_parquet(input_file)
        metrics, end_bal = func(df, initial_balance=balance, tp=tp, sl=sl, **params)

        pnl = end_bal - balance
        ret_m = 100.0 * pnl / balance if balance > 0 else 0.0
        rows.append(
            {
                "month": month,
                "start_balance": balance,
                "end_balance": end_bal,
                "pnl": pnl,
                "return_pct": ret_m,
            }
        )
        month_metrics.append(metrics)
        balance = end_bal

    if not rows:
        return

    monthly_df = pd.DataFrame(rows)
    total_ret = 100.0 * (balance - INITIAL_BALANCE) / INITIAL_BALANCE

    key = f"{strat_name}|{params}|tp={tp}|sl={sl}|{eval_label}"
    yearly_reports[key] = YearlyReport(
        strategy_key=key,
        start_balance=float(INITIAL_BALANCE),
        end_balance=float(balance),
        total_return_pct=float(total_ret),
        monthly=monthly_df,
        month_metrics=month_metrics,
        tp=tp,
        sl=sl,
        eval_label=eval_label,
    )


def annualized_sharpe_from_monthly_returns(
    monthly: pd.DataFrame,
    *,
    periods_per_year: float = 12.0,
) -> float:
    """
    Classical Sharpe-style ratio from **monthly portfolio returns** (not per-trade).

    Uses simple monthly returns r_t = (end_t - start_t) / start_t as in `monthly['return_pct'] / 100`.
    Annualized as: (mean(r) / std(r, ddof=1)) * sqrt(periods_per_year). Assumes ~12 independent
    months; for crypto this is a common simplification when bars are calendar months.

    Risk-free rate is 0 (same as typical crypto backtest POC). Returns NaN if <2 months or std=0.
    """
    if monthly is None or monthly.empty or len(monthly) < 2:
        return float("nan")
    if "return_pct" not in monthly.columns:
        return float("nan")
    r = monthly["return_pct"].astype(float).to_numpy() / 100.0
    r = r[np.isfinite(r)]
    if len(r) < 2:
        return float("nan")
    mu = float(np.mean(r))
    sd = float(np.std(r, ddof=1))
    if sd <= 0 or not np.isfinite(sd):
        return float("nan")
    return float(np.clip((mu / sd) * np.sqrt(periods_per_year), -50.0, 50.0))


def build_summary_dataframe() -> pd.DataFrame:
    rows = []
    for key, rep in yearly_reports.items():
        tw = sum(m.trades for m in rep.month_metrics)
        wr = (sum(m.win_rate * m.trades for m in rep.month_metrics) / tw) if tw else 0.0
        pfs = [m.profit_factor for m in rep.month_metrics if m.profit_factor != float("inf")]
        pf_m = float(np.mean(pfs)) if pfs else 0.0
        sharpe_trade_m = float(np.mean([m.sharpe for m in rep.month_metrics]))
        sharpe_ann = annualized_sharpe_from_monthly_returns(rep.monthly)
        dd_m = float(np.mean([m.max_drawdown for m in rep.month_metrics]))
        row = {
            "key": key[:200],
            "eval": rep.eval_label,
            "end_$": round(rep.end_balance, 2),
            "return_%": round(rep.total_return_pct, 2),
            "trades": tw,
            "win_%": round(wr * 100, 2),
            "PF_m_avg": round(pf_m, 3),
            "Sharpe_ann": round(sharpe_ann, 3) if np.isfinite(sharpe_ann) else sharpe_ann,
            "Sharpe_trade_m_avg": round(sharpe_trade_m, 3),
            "maxDD_m_avg": round(dd_m, 2),
            "tp": rep.tp,
            "sl": rep.sl,
        }
        rows.append(row)
    return pd.DataFrame(rows).sort_values("return_%", ascending=False)


def summary_yearly(show_monthly: bool = False) -> None:
    print(f"{'Strategy':<50} | {'End $':>10} | {'Return%':>9} | {'Trades':>7} | {'Win%*':>6} | {'PF**':>6} | {'eval':>12}")
    print("-" * 110)
    print("* Win% weighted by trades  |  ** PF mean of monthly PFs")
    print("-" * 110)
    for name, rep in yearly_reports.items():
        tw = sum(m.trades for m in rep.month_metrics)
        wr = sum(m.win_rate * m.trades for m in rep.month_metrics) / tw if tw else 0.0
        pfs = [m.profit_factor for m in rep.month_metrics if m.profit_factor != float("inf")]
        pf = float(np.mean(pfs)) if pfs else 0.0
        short_name = name[:50]
        print(
            f"{short_name:<50} | {rep.end_balance:10.2f} | {rep.total_return_pct:9.2f} | {tw:7d} | {wr * 100:6.1f} | {pf:6.2f} | {rep.eval_label:>12}"
        )
        if show_monthly and not rep.monthly.empty:
            print(rep.monthly.to_string(index=False))
            print(f"  (start ${rep.start_balance:.2f} -> end ${rep.end_balance:.2f} | tp={rep.tp} sl={rep.sl} | {rep.eval_label})\n")


def analytics_full(csv_name: str = "poc_backtest_leaderboard.csv") -> None:
    print("\n# LEADERBOARD\n")
    df = build_summary_dataframe()
    print(df.to_string(index=False))
    try:
        df.to_csv(csv_name, index=False)
        print(f"\nSaved: {csv_name}\n")
    except OSError as e:
        print("CSV save failed:", e)
    print("\n# YEARLY + MONTHLY\n")
    summary_yearly(show_monthly=True)


def clear_reports() -> None:
    yearly_reports.clear()
