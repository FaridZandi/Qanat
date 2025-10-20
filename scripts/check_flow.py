#!/usr/bin/env python3
"""
Analyze seqno ordering across nodes for a given flow_id.
"""

import argparse
import csv
import gzip
import io
import sys
from collections import defaultdict
from typing import Dict, List, Tuple, Iterable, Set
import matplotlib.pyplot as plt

SortKey = Tuple[float, int]  # (arrival_time, line) default

def _open_csv(path: str):
    if path == "-":
        return io.TextIOWrapper(sys.stdin.buffer, encoding="utf-8", errors="replace")
    if path.endswith(".gz"):
        return io.TextIOWrapper(gzip.open(path, "rb"), encoding="utf-8", errors="replace")
    return open(path, "r", encoding="utf-8", errors="replace")

def _coerce_int(name: str, v: str) -> int:
    try:
        return int(v)
    except Exception:
        raise ValueError(f"Bad integer in column {name!r}: {v!r}")

def _coerce_float(name: str, v: str) -> float:
    try:
        return float(v)
    except Exception:
        raise ValueError(f"Bad float in column {name!r}: {v!r}")

def _dedup_preserve_order(seq: Iterable[int]) -> List[int]:
    seen = set()
    out = []
    for x in seq:
        if x not in seen:
            seen.add(x)
            out.append(x)
    return out

def _strict_equal(seqs: Dict[int, List[int]]) -> bool:
    it = iter(seqs.values())
    try:
        first = next(it)
    except StopIteration:
        return True
    return all(s == first for s in it)

def _order_consistent(a: List[int], b: List[int], rows_a, rows_b):
    next_in_a = {}
    for i in range(len(a) - 1):
        next_in_a.setdefault(a[i], []).append(a[i + 1])
    if a:
        next_in_a.setdefault(a[-1], None)

    next_in_b = {}
    for i in range(len(b) - 1):
        next_in_b.setdefault(b[i], []).append(b[i + 1])
    if b:
        next_in_b.setdefault(b[-1], None)

    arrival_map_a = {seq: rows_a[i][2] for i, seq in enumerate(a)}
    arrival_map_b = {seq: rows_b[i][2] for i, seq in enumerate(b)}

    bad_pairs = []

    for seqno, a_nexts in next_in_a.items():
        if seqno not in next_in_b:
            continue
        b_nexts = next_in_b[seqno]
        if a_nexts is None or b_nexts is None:
            continue
        if not any(x in b_nexts for x in a_nexts):
            times = []
            for nxt in a_nexts:
                if nxt in arrival_map_a:
                    times.append(arrival_map_a[nxt])
            for nxt in b_nexts:
                if nxt in arrival_map_b:
                    times.append(arrival_map_b[nxt])
            conflict_time = min(times) if times else None
            bad_pairs.append((seqno, conflict_time))
    return bad_pairs

def main():
    ap = argparse.ArgumentParser(description="Check per-node seqno ordering for a flow_id.")
    ap.add_argument("csvfile", help="Input CSV path (use '-' for stdin; .gz supported)")
    ap.add_argument("--flow", type=int, required=True, help="Flow ID to analyze")
    ap.add_argument("--nodes", help="Comma-separated node IDs to include (default: all)")
    ap.add_argument("--sort", choices=["arrival", "line"], default="arrival",
                    help="Sort key within each node: 'arrival' (arrival_time, then line) or 'line'")
    ap.add_argument("--show", type=int, default=0,
                    help="Print the first N seqnos for each node (0 = none)")
    args = ap.parse_args()

    include_nodes = None
    if args.nodes:
        include_nodes = set(int(x.strip()) for x in args.nodes.split(",") if x.strip())

    per_node: Dict[int, List[Tuple[SortKey, int, float, int]]] = defaultdict(list)
    total_rows = 0
    with _open_csv(args.csvfile) as f:
        rdr = csv.DictReader(f)
        required = {"line", "arrival_time", "seqno", "node_id", "flow_id"}
        missing = required - set(rdr.fieldnames or [])
        if missing:
            raise SystemExit(f"Missing required CSV columns: {', '.join(sorted(missing))}")

        for row in rdr:
            total_rows += 1
            try:
                flow_id = _coerce_int("flow_id", row["flow_id"])
            except Exception:
                continue
            if flow_id != args.flow:
                continue

            node_id = _coerce_int("node_id", row["node_id"])
            if include_nodes and node_id not in include_nodes:
                continue
            seqno = _coerce_int("seqno", row["seqno"])
            arrival_time = _coerce_float("arrival_time", row["arrival_time"])
            line = _coerce_int("line", row["line"])

            if args.sort == "arrival":
                key = (arrival_time, line)
            else:
                key = (float(line), line)

            per_node[node_id].append((key, seqno, arrival_time, line))

    if not per_node:
        print(f"No rows for flow_id={args.flow}.", file=sys.stderr)
        return

    seqs: Dict[int, List[int]] = {}
    seq_rows: Dict[int, List[Tuple[SortKey, int, float, int]]] = {}
    dup_counts: Dict[int, int] = {}
    counts: Dict[int, int] = {}
    for node_id, rows in per_node.items():
        rows.sort(key=lambda t: t[0])
        seq_rows[node_id] = rows
        seq_list = [seq for _, seq, _, _ in rows]
        counts[node_id] = len(seq_list)
        dedup = seq_list
        dup_counts[node_id] = len(seq_list) - len(dedup)
        seqs[node_id] = dedup

    nodes_sorted = sorted(seqs.keys())
    print(f"Flow {args.flow}: {len(nodes_sorted)} node(s) with data -> {', '.join(map(str, nodes_sorted))}")
    for n in nodes_sorted:
        sample = seqs[n][:args.show] if args.show > 0 else []
        sample_str = f" first {len(sample)}: {sample}" if args.show > 0 else ""
        extra = f" (dedup dropped {dup_counts[n]})" if dup_counts[n] else ""
        print(f"  Node {n}: {counts[n]} rows -> {len(seqs[n])} unique seqnos{extra}.{sample_str}")

    for n in nodes_sorted:
        rows = seq_rows[n]
        if not rows:
            continue
        times = [t for _, _, t, _ in rows]
        print(f"  Node {n}: arrival_time range: {min(times)} .. {max(times)}")

    all_strict_equal = _strict_equal(seqs)
    print(f"\nSTRICT equality across nodes: {'OK' if all_strict_equal else 'MISMATCH'}")

    if not all_strict_equal:
        sig_to_nodes: Dict[Tuple[int, ...], List[int]] = defaultdict(list)
        for node_id, s in seqs.items():
            sig_to_nodes[tuple(s)].append(node_id)
        print("  Groups by identical sequence:")
        for i, (sig, group_nodes) in enumerate(sig_to_nodes.items(), 1):
            preview = list(sig[:10])
            suffix = " …" if len(sig) > 10 else ""
            print(f"    [{i}] nodes {group_nodes}: len={len(sig)}, head={preview}{suffix}")

    print("\nPAIRWISE relative-order consistency (shared seqnos):")

    nodes = nodes_sorted
    all_order_ok = True
    inconsistency_times: Set[float] = set()
    for i in range(len(nodes)):
        for j in range(i + 1, len(nodes)):
            a_id, b_id = nodes[i], nodes[j]
            a, b = seqs[a_id], seqs[b_id]
            bad_pairs = _order_consistent(a, b, seq_rows[a_id], seq_rows[b_id])
            if not bad_pairs:
                print(f"  {a_id} vs {b_id}: OK")
            else:
                all_order_ok = False
                bad_pairs_sorted = sorted(bad_pairs, key=lambda x: (x[1] if x[1] is not None else float('inf')))
                for seqno, conflict_time in bad_pairs_sorted:
                    if conflict_time is not None:
                        inconsistency_times.add(conflict_time)
                    print(f"  {a_id} vs {b_id}: ORDER MISMATCH around seq {seqno} (time {conflict_time})")
    if all_order_ok:
        print("All pairs consistent in relative order.")

    with open(f"flow_{args.flow}_sequences.csv", "w", newline="", encoding="utf-8") as outfh:
        writer = csv.writer(outfh)
        writer.writerow(nodes_sorted)
        max_len = max(len(seqs[n]) for n in nodes_sorted)
        for i in range(max_len):
            row = []
            for n in nodes_sorted:
                s = seqs[n]
                v = s[i] if i < len(s) else ""
                row.append(v)
            writer.writerow(row)
    print(f"\nWrote sequences to flow_{args.flow}_sequences.csv")


    first_half_of_sorted_nodes = nodes_sorted[:len(nodes_sorted)//2]
    second_half_of_sorted_nodes = nodes_sorted[len(nodes_sorted)//2:]

    def compute_receiving_rate(rows, window_size=0.03):
        times = [t for _, _, t, _ in rows]
        if not times:
            return [], []
        min_time = min(times)
        max_time = max(times)
        bins = []
        rates = []
        t = min_time
        while t < max_time:
            next_t = t + window_size
            count = sum(1 for time in times if t <= time < next_t)
            bins.append(t + window_size / 2)
            rates.append(count / window_size)
            t = next_t
        return bins, rates

    max_rows = max(len(first_half_of_sorted_nodes), len(second_half_of_sorted_nodes))
    if max_rows == 0:
        print("No nodes to plot.")
        return

    fig, axes = plt.subplots(nrows=max_rows, ncols=1, figsize=(12, 3 * max_rows), sharex=True, sharey=True)
    if max_rows == 1:
        axes = axes.reshape(1, 1)

    for col, node_list in enumerate([first_half_of_sorted_nodes, second_half_of_sorted_nodes]):
        for row in range(max_rows):
            ax = axes[row]
            if row >= len(node_list):
                ax.axis('off')
                continue
            node_id = node_list[row]
            bins, rates = compute_receiving_rate(seq_rows[node_id])
            color = 'blue' if node_id in first_half_of_sorted_nodes else 'orange'
            zone = 'src' if node_id in first_half_of_sorted_nodes else 'dst'
            ax.plot(bins, rates, label=f"Node {node_id} ({zone})", color=color)
            ax.set_title(f"Node {node_id}")
            ax.set_ylabel("Receiving rate (pkts/sec)")
            ax.set_xlabel("Time (s)")
            ax.grid(True)
            ax.legend()
            # for t in sorted(inconsistency_times):
            #     ax.axvline(t, linestyle='--', color='red', alpha=0.4)

    plt.tight_layout()
    plt.savefig(f"flow_{args.flow}_receiving_rates.png", dpi=200)

if __name__ == "__main__":
    main()
