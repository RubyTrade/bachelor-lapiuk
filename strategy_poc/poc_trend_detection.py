"""
Trend / regime labels on the same aggTrades data as the POC backtests.

Uses 1-minute resampled closes (last trade per minute) so trend is not recomputed
per tick (too slow + overly noisy). Labels are forward-filled to each tick.

States:
  1  = uptrend   (fast EMA > slow EMA by at least eps)
 -1  = downtrend (fast EMA < slow EMA by at least eps)
  0  = range / undecided
"""

from __future__ import annotations

from typing import Any

import numpy as np
import pandas as pd
import polars as pl


def resample_1m_last_close(df: pl.DataFrame) -> pl.DataFrame:
    """Last trade price per 1-minute bucket."""
    if "timestamp" not in df.columns:
        raise ValueError("DataFrame must have a 'timestamp' column")
    d = df.sort("timestamp")
    return (
        d.group_by_dynamic("timestamp", every="1m", closed="left")
        .agg(pl.col("price").last().alias("close"))
        .drop_nulls()
    )


def ema(x: np.ndarray, span: int) -> np.ndarray:
    if len(x) == 0:
        return x
    alpha = 2.0 / (span + 1)
    out = np.empty_like(x, dtype=float)
    out[0] = float(x[0])
    for i in range(1, len(x)):
        out[i] = alpha * float(x[i]) + (1 - alpha) * out[i - 1]
    return out


def trend_states_from_closes(
    close: np.ndarray,
    fast: int = 20,
    slow: int = 50,
    eps_pct: float = 0.0003,
) -> np.ndarray:
    """
    Per-bar trend: 1 / -1 / 0 from EMA crossover with deadband eps_pct on slow EMA.
    """
    if len(close) < max(fast, slow) + 2:
        return np.zeros(len(close), dtype=np.int8)
    ef = ema(close.astype(float), fast)
    es = ema(close.astype(float), slow)
    diff = np.where(es != 0, (ef - es) / np.abs(es), 0.0)
    state = np.zeros(len(close), dtype=np.int8)
    state[diff > eps_pct] = 1
    state[diff < -eps_pct] = -1
    return state


def align_bar_trend_to_ticks(
    tick_ts_ms: np.ndarray,
    bar_ts_ms: np.ndarray,
    bar_trend: np.ndarray,
) -> np.ndarray:
    """Map each tick (epoch ms) to the trend of the latest 1m bar whose start <= tick time."""
    n = len(tick_ts_ms)
    out = np.zeros(n, dtype=np.int8)
    if len(bar_ts_ms) == 0:
        return out
    bt_ms = np.asarray(bar_ts_ms, dtype=np.int64)
    tt = np.asarray(tick_ts_ms, dtype=np.int64)
    idx = np.searchsorted(bt_ms, tt, side="right") - 1
    idx = np.clip(idx, 0, len(bar_trend) - 1)
    out[:] = bar_trend[idx]
    return out


def per_tick_trend_state(df: pl.DataFrame, fast: int = 20, slow: int = 50, eps_pct: float = 0.0003) -> np.ndarray:
    """Full-length trend state aligned to each row of aggTrades df."""
    bars = resample_1m_last_close(df)
    if bars.height == 0:
        return np.zeros(df.height, dtype=np.int8)
    close = bars["close"].to_numpy()
    ts_bar_ms = bars.select(pl.col("timestamp").dt.epoch("ms").cast(pl.Int64)).to_series().to_numpy()
    bar_state = trend_states_from_closes(close, fast=fast, slow=slow, eps_pct=eps_pct)
    tick_ts_ms = df.select(pl.col("timestamp").dt.epoch("ms").cast(pl.Int64)).to_series().to_numpy()
    return align_bar_trend_to_ticks(tick_ts_ms, ts_bar_ms, bar_state)


def bars_with_trend(
    df: pl.DataFrame,
    fast: int = 20,
    slow: int = 50,
    eps_pct: float = 0.0003,
) -> pl.DataFrame:
    """
    One row per 1m bar: close, state, fast/slow EMA (for charts).
    State changes are defined here (not on raw ticks).
    """
    bars = resample_1m_last_close(df)
    if bars.height == 0:
        return bars
    close = bars["close"].to_numpy()
    state = trend_states_from_closes(close, fast=fast, slow=slow, eps_pct=eps_pct)
    ef = ema(close.astype(float), fast)
    es = ema(close.astype(float), slow)
    return bars.with_columns(
        pl.Series("state", state, dtype=pl.Int8),
        pl.Series("ema_fast", ef),
        pl.Series("ema_slow", es),
    )


def expected_state_from_ema(
    ema_fast: np.ndarray,
    ema_slow: np.ndarray,
    eps_pct: float = 0.0003,
) -> np.ndarray:
    """Same rule as `trend_states_from_closes` but from precomputed EMA series."""
    ef = np.asarray(ema_fast, dtype=float)
    es = np.asarray(ema_slow, dtype=float)
    diff = np.where(np.abs(es) > 1e-15, (ef - es) / np.abs(es), 0.0)
    out = np.zeros(len(ef), dtype=np.int8)
    out[diff > eps_pct] = 1
    out[diff < -eps_pct] = -1
    return out


def verify_bars_state_matches_rule(
    bars: pl.DataFrame,
    eps_pct: float = 0.0003,
) -> tuple[int, np.ndarray]:
    """
    Returns (n_mismatches, indices_where_state_differs_from_ema_rule).

    Run after `bars_with_trend()` — if mismatches > 0, something is inconsistent
    (should always be 0 for data produced by `bars_with_trend`).
    """
    if bars.height == 0:
        return 0, np.array([], dtype=np.int64)
    st = bars["state"].to_numpy().astype(np.int8)
    ef = bars["ema_fast"].to_numpy()
    es = bars["ema_slow"].to_numpy()
    exp = expected_state_from_ema(ef, es, eps_pct)
    bad = np.flatnonzero(st != exp)
    return int(bad.size), bad.astype(np.int64)


def state_transition_table(bars: pl.DataFrame) -> pd.DataFrame:
    """
    Each row: when regime changed, from → to. First row marks the initial state at bar 0.
    """
    if bars.height == 0:
        return pd.DataFrame(
            columns=["bar_index", "time", "from_state", "to_state", "duration_bars"]
        )
    st = bars["state"].to_numpy().astype(np.int8)
    n = len(st)
    times = bars["timestamp"]
    rows: list[dict[str, Any]] = [
        {
            "bar_index": 0,
            "time": times[0],
            "from_state": None,
            "to_state": int(st[0]),
            "duration_bars": None,
        }
    ]
    run_start = 0
    for i in range(1, n):
        if st[i] != st[i - 1]:
            dur = i - run_start
            rows[-1]["duration_bars"] = dur
            rows.append(
                {
                    "bar_index": i,
                    "time": times[i],
                    "from_state": int(st[i - 1]),
                    "to_state": int(st[i]),
                    "duration_bars": None,
                }
            )
            run_start = i
    if rows:
        rows[-1]["duration_bars"] = n - run_start
    return pd.DataFrame(rows)


STATE_COLORS = {
    1: "#b8e0b8",   # up / risk-on
    0: "#d9d9d9",   # range
    -1: "#f5b8b8",  # down / risk-off
}

STATE_LINE = {1: "#2ca02c", 0: "#7f7f7f", -1: "#d62728"}


def plot_regime_timeline(
    bars: pl.DataFrame,
    transition_df: pd.DataFrame | None = None,
    *,
    title: str = "Regime (1m bars)",
    zoom_last_bars: int | None = None,
    figsize: tuple[float, float] = (14, 8),
):
    """
    Panel 1: close + ema_fast/ema_slow + background by state.
    Panel 2: state as a step plot (-1 / 0 / 1).
    Panel 3: optional — mark transitions (vertical lines) on price.

    If zoom_last_bars is set, only the last N bars are shown (clearer on long months).
    """
    import matplotlib.pyplot as plt
    import matplotlib.dates as mdates

    if bars.height == 0:
        raise ValueError("bars_with_trend() returned no rows")

    orig_h = bars.height
    b = bars
    first_idx = 0
    if zoom_last_bars is not None and b.height > zoom_last_bars:
        first_idx = orig_h - zoom_last_bars
        b = b.tail(zoom_last_bars)
        if transition_df is not None and not transition_df.empty:
            transition_df = transition_df[transition_df["bar_index"] >= first_idx].copy()

    t = b["timestamp"].to_numpy()
    close = b["close"].to_numpy()
    state = b["state"].to_numpy().astype(np.int8)
    ef = b["ema_fast"].to_numpy()
    es = b["ema_slow"].to_numpy()

    x = mdates.date2num(t)

    fig, axes = plt.subplots(3, 1, figsize=figsize, sharex=True, gridspec_kw={"height_ratios": [3, 1, 1.2]})
    ax_price, ax_step, ax_mark = axes

    # Background: one span per constant-state run (use bar boundaries)
    i0 = 0
    for i in range(1, len(state) + 1):
        if i == len(state) or state[i] != state[i0]:
            c = STATE_COLORS.get(int(state[i0]), "#eeeeee")
            left = x[i0]
            if i < len(x):
                right = x[i]
            else:
                w = (x[-1] - x[-2]) if len(x) > 1 else 1e-6
                right = x[-1] + w
            ax_price.axvspan(left, right, facecolor=c, alpha=0.45, linewidth=0)
            i0 = i

    ax_price.plot(t, close, color="black", lw=0.8, label="close")
    # Colors: green = fast EMA, red = slow EMA (not "bull/bear" — compare lines to read regime)
    ax_price.plot(t, ef, color=STATE_LINE[1], lw=1.0, alpha=0.85, label="EMA fast")
    ax_price.plot(t, es, color=STATE_LINE[-1], lw=1.0, alpha=0.85, label="EMA slow")
    ax_price.set_ylabel("Price")
    ax_price.set_title(title + (" (zoom)" if zoom_last_bars else ""))
    ax_price.legend(loc="upper left", fontsize=8)
    ax_price.grid(True, alpha=0.25)

    # Step plot of state
    ax_step.fill_between(x, state.astype(float), step="post", color="#4c72b0", alpha=0.35, linewidth=0)
    ax_step.step(t, state, where="post", color="#1f3766", lw=1.2)
    ax_step.set_yticks([-1, 0, 1])
    ax_step.set_yticklabels(["down (-1)", "range (0)", "up (1)"])
    ax_step.set_ylabel("State")
    ax_step.grid(True, alpha=0.3)

    # Transitions: vertical lines at bar boundaries where regime changed (panel 3)
    ax_mark.plot(t, close, color="black", lw=0.6, alpha=0.7)
    if transition_df is not None and not transition_df.empty:
        for _, row in transition_df.iterrows():
            if row.get("from_state") is None:
                continue
            bi = int(row["bar_index"])
            local = bi - first_idx
            if 0 <= local < len(x):
                ax_mark.axvline(x[local], color="#9467bd", ls="--", lw=0.9, alpha=0.85)
    ax_mark.set_ylabel("Price + transitions")
    ax_mark.grid(True, alpha=0.25)

    for ax in axes:
        ax.xaxis.set_major_formatter(mdates.DateFormatter("%m-%d %H:%M"))
        ax.xaxis.set_major_locator(mdates.AutoDateLocator())
    fig.autofmt_xdate()
    plt.tight_layout()
    return fig, axes
