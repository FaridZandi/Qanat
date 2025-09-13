#!/usr/bin/env python3
"""
Analyze an ns-test summary.csv and generate plots.

Usage:
  python scripts/analyze_summary.py \
    --summary ns-test/exps/random_test/summary.csv \
    [--outdir ns-test/exps/random_test]

Outputs:
  Saves a set of PNG plots to the output directory (default: summary.csv's folder).
"""
import argparse
from pathlib import Path
import sys

import pandas as pd


def flatten_columns(df: pd.DataFrame) -> pd.DataFrame:
    if isinstance(df.columns, pd.MultiIndex):
        df.columns = [f"{a.strip()}.{b.strip()}" for a, b in df.columns]
    return df


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--summary", required=True, help="Path to summary.csv")
    parser.add_argument("--outdir", default=None, help="Output directory for plots (defaults to summary folder)")
    args = parser.parse_args()

    summ_path = Path(args.summary)
    if not summ_path.exists():
        print(f"summary.csv not found: {summ_path}", file=sys.stderr)
        sys.exit(1)

    outdir = Path(args.outdir) if args.outdir else summ_path.parent
    outdir.mkdir(parents=True, exist_ok=True)

    # Non-interactive backend
    import matplotlib
    matplotlib.use("Agg")
    import matplotlib.pyplot as plt
    import seaborn as sns
    sns.set_theme(style="whitegrid")

    # Load CSV with two-level header and flatten
    df = pd.read_csv(summ_path, header=[0, 1])
    df = flatten_columns(df)

    # Column names
    ms = "settings.migration_status"
    pr = "settings.prioritization"
    orch = "settings.orch_type"
    vm_col = "flow_metrics.vm_afct"
    tnld_col = "tunnell_metrics.tnld_pkt_cnt"
    hpq = "buffer_metrics.avg_hpq"
    lpq = "buffer_metrics.avg_lpq"
    bpq = "buffer_metrics.avg_bpq"
    flight = "ptk_lever_metrics.vm_avg_pkt_flight_t"
    buff = "ptk_lever_metrics.vm_avg_pkt_buff_t"
    mt = "protocol_metrics.tot_mig_time"

    # Coerce prio to numeric
    if pr in df.columns:
        df[pr] = pd.to_numeric(df[pr], errors="coerce")

    # Base frame of interest
    cols = [c for c in [ms, pr, orch, vm_col, tnld_col, hpq, lpq, bpq, flight, buff, mt] if c in df.columns]
    base = df[cols].copy().dropna(subset=[pr])

    # 1) VM AFCT vs prio per orchestrator, faceted by migration
    try:
        g = sns.relplot(
            data=base,
            x=pr,
            y=vm_col,
            hue=orch,
            col=ms if ms in base.columns else None,
            kind="line",
            marker="o",
            facet_kws={"sharey": True} if ms in base.columns else None,
        )
        g.set_axis_labels("Prioritization level", "VM AFCT (s)")
        if hasattr(g, "set_titles") and (ms in base.columns):
            g.set_titles("migration={col_name}")
        (getattr(g, "_figure", None) or getattr(g, "fig", None)).suptitle(
            "VM AFCT vs Prioritization by Orchestrator", y=1.05
        )
        (getattr(g, "_figure", None) or getattr(g, "fig", None)).tight_layout()
        (getattr(g, "_figure", None) or getattr(g, "fig", None)).savefig(
            outdir / "vm_afct_by_prio_and_orch.png", dpi=200, bbox_inches="tight"
        )
        plt.close("all")
    except Exception as e:
        print(f"WARN: plot vm_afct_by_prio_and_orch failed: {e}")

    # mig-only subset
    mig = base[base[ms] == "mig"] if ms in base.columns else base.iloc[0:0]

    # 2) Heatmap of VM AFCT for mig only
    if not mig.empty and vm_col in mig.columns:
        try:
            pv = mig.pivot_table(index=pr, columns=orch, values=vm_col, aggfunc="mean").sort_index()
            plt.figure(figsize=(6, 3))
            sns.heatmap(pv, annot=True, fmt=".3f", cmap="viridis")
            plt.title("VM AFCT (mig) — heatmap by prio x orch")
            plt.xlabel("Orchestrator")
            plt.ylabel("Prioritization level")
            plt.tight_layout()
            plt.savefig(outdir / "vm_afct_mig_heatmap.png", dpi=200)
            plt.close()
        except Exception as e:
            print(f"WARN: plot vm_afct_mig_heatmap failed: {e}")

    # 3) Tunnelled packets bar (mig)
    if not mig.empty and tnld_col in mig.columns:
        try:
            plt.figure(figsize=(6, 3))
            sns.barplot(data=mig, x=pr, y=tnld_col, hue=orch)
            plt.title("Tunnelled packets (mig) by prio x orch")
            plt.xlabel("Prioritization level")
            plt.ylabel("Tunnelled packet count")
            plt.tight_layout()
            plt.savefig(outdir / "tunnelled_packets_mig_bar.png", dpi=200)
            plt.close()
        except Exception as e:
            print(f"WARN: plot tunnelled_packets_mig_bar failed: {e}")

    # 4) Buffer queues bars (mig)
    for col, title, fname in [
        (hpq, "Avg high-priority queue (mig)", "avg_hpq_mig_bar.png"),
        (lpq, "Avg low-priority queue (mig)", "avg_lpq_mig_bar.png"),
        (bpq, "Avg buffer queue (mig)", "avg_bpq_mig_bar.png"),
    ]:
        if not mig.empty and col in mig.columns:
            try:
                plt.figure(figsize=(6, 3))
                sns.barplot(data=mig, x=pr, y=col, hue=orch)
                plt.title(f"{title} by prio x orch")
                plt.xlabel("Prioritization level")
                plt.ylabel(title.split()[1])
                plt.tight_layout()
                plt.savefig(outdir / fname, dpi=200)
                plt.close()
            except Exception as e:
                print(f"WARN: plot {fname} failed: {e}")

    # 5) VM AFCT delta (mig - nomig)
    try:
        vm_pivot = base.pivot_table(index=[pr, orch], columns=ms, values=vm_col, aggfunc="mean")
        if set(["mig", "nomig"]).issubset(vm_pivot.columns):
            vm_pivot = vm_pivot.sort_index()
            vm_pivot["delta"] = vm_pivot["mig"] - vm_pivot["nomig"]
            vm_pivot = vm_pivot.reset_index()
            plt.figure(figsize=(6, 3))
            sns.barplot(data=vm_pivot, x=pr, y="delta", hue=orch)
            plt.axhline(0, color="k", linewidth=0.8)
            plt.title("VM AFCT delta (mig - nomig) by prio x orch")
            plt.xlabel("Prioritization level")
            plt.ylabel("Δ VM AFCT (s)")
            plt.tight_layout()
            plt.savefig(outdir / "vm_afct_delta_mig_nomig_bar.png", dpi=200)
            plt.close()
    except Exception as e:
        print(f"WARN: plot vm_afct_delta_mig_nomig_bar failed: {e}")

    # 6) Tot migration time / flight / buffer times (mig)
    for col, title, fname, ylabel in [
        (mt, "Total migration time (mig)", "tot_mig_time_mig_bar.png", "Total migration time (s)"),
        (flight, "Avg packet in-flight time (mig)", "vm_avg_pkt_flight_mig_bar.png", "Avg in-flight time (μs)"),
        (buff, "Avg packet buffered time (mig)", "vm_avg_pkt_buff_mig_bar.png", "Avg buffered time (μs)"),
    ]:
        if not mig.empty and col in mig.columns:
            try:
                plt.figure(figsize=(6, 3))
                sns.barplot(data=mig, x=pr, y=col, hue=orch)
                plt.title(f"{title} by prio x orch")
                plt.xlabel("Prioritization level")
                plt.ylabel(ylabel)
                plt.tight_layout()
                plt.savefig(outdir / fname, dpi=200)
                plt.close()
            except Exception as e:
                print(f"WARN: plot {fname} failed: {e}")

    # 7) Scatter: VM AFCT vs tunneled (mig)
    if not mig.empty and tnld_col in mig.columns and vm_col in mig.columns:
        try:
            plt.figure(figsize=(5, 4))
            sns.scatterplot(data=mig, x=tnld_col, y=vm_col, hue=orch, style=pr, s=100)
            plt.title("VM AFCT vs Tunnelled packets (mig)")
            plt.xlabel("Tunnelled packet count")
            plt.ylabel("VM AFCT (s)")
            plt.tight_layout()
            plt.savefig(outdir / "vm_afct_vs_tunnelled_mig_scatter.png", dpi=200)
            plt.close()
        except Exception as e:
            print(f"WARN: plot vm_afct_vs_tunnelled_mig_scatter failed: {e}")

    # 8) Correlation heatmap (mig)
    corr_cols = [c for c in [vm_col, tnld_col, hpq, lpq, bpq, flight, buff, mt] if c in mig.columns]
    if not mig.empty and len(corr_cols) >= 2:
        try:
            corr = mig[corr_cols].corr()
            plt.figure(figsize=(6, 5))
            sns.heatmap(corr, annot=True, fmt=".2f", cmap="coolwarm", square=True)
            plt.title("Correlation (mig runs) among key metrics")
            plt.tight_layout()
            plt.savefig(outdir / "metrics_correlation_mig_heatmap.png", dpi=200)
            plt.close()
        except Exception as e:
            print(f"WARN: plot metrics_correlation_mig_heatmap failed: {e}")

    print(f"Saved plots to {outdir}")


if __name__ == "__main__":
    main()

