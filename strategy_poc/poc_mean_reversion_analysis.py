"""
Helpers for rigorous mean-reversion POC: parse leaderboard keys, merge train vs OOS, stability stats.
"""

from __future__ import annotations

import ast
import re
from typing import Any

import numpy as np
import pandas as pd


_KEY_RE = re.compile(
    r"^(?P<strat>[^|]+)\|(?P<params>\{.*\})\|tp=(?P<tp>[\d.]+)\|sl=(?P<sl>[\d.]+)\|(?P<eval_tag>.+)$"
)


def parse_mean_strategy_key(key: str) -> dict[str, Any]:
    """Extract strat name, params dict, tp, sl, eval_label from a leaderboard key."""
    m = _KEY_RE.match(key.strip())
    if not m:
        return {"strat": "", "params": {}, "tp": np.nan, "sl": np.nan, "eval_label": "", "raw": key}
    params = ast.literal_eval(m.group("params"))
    return {
        "strat": m.group("strat"),
        "params": params,
        "tp": float(m.group("tp")),
        "sl": float(m.group("sl")),
        "eval_label": m.group("eval_tag"),
        "delta": float(params.get("delta", 0.002)),
        "vwap_window": int(params.get("vwap_window", 200)),
        "adaptive_tp_sl": bool(params.get("adaptive_tp_sl", False)),
        "raw": key,
    }


def enrich_leaderboard_csv(df: pd.DataFrame) -> pd.DataFrame:
    """Add columns delta, vwap_window, adaptive_tp_sl, tp, sl from `key` column."""
    if "key" not in df.columns:
        raise ValueError("DataFrame must have a 'key' column")
    rows = [parse_mean_strategy_key(k) for k in df["key"].astype(str)]
    out = df.copy()
    out["delta"] = [r["delta"] for r in rows]
    out["vwap_window"] = [r["vwap_window"] for r in rows]
    out["adaptive_tp_sl"] = [r["adaptive_tp_sl"] for r in rows]
    out["strat"] = [r["strat"] for r in rows]
    return out


def merge_train_holdout(
    train_df: pd.DataFrame,
    holdout_df: pd.DataFrame,
) -> pd.DataFrame:
    """
    Join train and holdout rows on strategy parameters (not eval label).
    """
    on = ["strat", "delta", "vwap_window", "adaptive_tp_sl", "tp", "sl"]
    t = enrich_leaderboard_csv(train_df)
    h = enrich_leaderboard_csv(holdout_df)
    metric_cols = [
        "return_%",
        "trades",
        "win_%",
        "PF_m_avg",
        "maxDD_m_avg",
        "Sharpe_ann",
        "Sharpe_trade_m_avg",
        "Sharpe_m_avg",  # legacy CSVs from older runs
    ]
    cols = on + [c for c in metric_cols if c in t.columns and c in h.columns]
    cols_t = [c for c in cols if c in t.columns]
    cols_h = [c for c in cols if c in h.columns]
    merged = pd.merge(
        t[cols_t],
        h[cols_h],
        on=on,
        how="inner",
        suffixes=("_train", "_holdout"),
    )
    return merged


def spearman_train_holdout_returns(merged: pd.DataFrame) -> float:
    """Spearman correlation between train and holdout total return % (needs merged suffix columns)."""
    if "return_%_train" not in merged.columns or "return_%_holdout" not in merged.columns:
        return float("nan")
    x = merged["return_%_train"].astype(float)
    y = merged["return_%_holdout"].astype(float)
    return float(x.corr(y, method="spearman"))


def top_k_oos_degradation(merged: pd.DataFrame, k: int = 15) -> pd.DataFrame:
    """
    Take top-k configs by train return; show train vs holdout return.
    Large drop on holdout suggests overfitting or regime shift.
    """
    sub = merged.nlargest(k, "return_%_train").copy()
    sub["oos_minus_train_pct"] = sub["return_%_holdout"].astype(float) - sub["return_%_train"].astype(float)
    cols = [
        "delta",
        "vwap_window",
        "adaptive_tp_sl",
        "tp",
        "sl",
        "return_%_train",
        "return_%_holdout",
        "oos_minus_train_pct",
    ]
    return sub[[c for c in cols if c in sub.columns]]
