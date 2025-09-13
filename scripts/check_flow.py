#!/usr/bin/env python3
"""
Analyze seqno ordering across nodes for a given flow_id.

Input CSV columns expected (header names, case-sensitive):
  line, arrival_time, seqno, node_id, flow_id

Typical upstream row:
  line,arrival_time,seqno,node_id,flow_id
  42,3.76007,8761,0,1

Two checks:
  1) strict  : sequences are exactly identical across all nodes
  2) order   : relative order is consistent pairwise (no inversions on shared seqnos)

Usage:
  python3 check_flow_order.py path/to/log.csv --flow 1
  python3 check_flow_order.py log.csv --flow 1 --sort line
  python3 check_flow_order.py log.csv --flow 1 --nodes 0,5,7 --show 10
"""

import argparse
import csv
import gzip
import io
import sys
from collections import defaultdict
from typing import Dict, List, Tuple, Iterable
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

def _order_consistent_2(a: List[int], b: List[int]) -> Tuple[bool, Tuple[int, int] or None]:
    """
    Returns (True, None) if order consistent; else (False, (x,y)) giving
    the first inversion pair that appears in opposite order between a and b.
    """
    pos_a = {v: i for i, v in enumerate(a)}
    common = [x for x in b if x in pos_a]
    last = -1
    for i, x in enumerate(common):
        p = pos_a[x]
        if p < last:
            # inversion: find the previous element causing it
            y = common[i - 1]
            return False, (y, x)  # y should not precede x in 'a' if it follows in 'b'
        last = p
    return True, None

def _order_consistent(a: List[int], b: List[int]) -> Tuple[bool, Tuple[int, int] or None]:
    bad_count = 0
    
    # find who's next for each value in a 
    next_in_a = {}
    for i in range(len(a) - 1):
        if a[i] not in next_in_a:
            next_in_a[a[i]] = [a[i + 1]]
        else:
            next_in_a[a[i]].append(a[i + 1])
    next_in_a[a[-1]] = None  # last has no next
    
    next_in_b = {} 
    for i in range(len(b) - 1):
        if b[i] not in next_in_b:
            next_in_b[b[i]] = [b[i + 1]]
        else:
            next_in_b[b[i]].append(b[i + 1])
    next_in_b[b[-1]] = None  # last has no next
    
    for a_seqno, a_nexts in next_in_a.items():
        if a_seqno not in next_in_b:
            continue
        b_nexts = next_in_b[a_seqno]
        if b_nexts is None or a_nexts is None:
            continue
        # are they the same? 
        for x in a_nexts:
            if x in b_nexts:
                break
        else:
            # no match found, inversion
            print(f"DEBUG: inversion detected for {a_seqno}: a_nexts={a_nexts}, b_nexts={b_nexts}")
            bad_count += 1
                 
    return bad_count


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

    # Gather rows per node
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
                # line-only, but keep arrival_time as tie info in tuple
                key = (float(line), line)

            per_node[node_id].append((key, seqno, arrival_time, line))

    if not per_node:
        print(f"No rows for flow_id={args.flow}.", file=sys.stderr)
        return

    # Build ordered, de-duplicated seq lists per node
    seqs: Dict[int, List[int]] = {}
    dup_counts: Dict[int, int] = {}
    counts: Dict[int, int] = {}
    for node_id, rows in per_node.items():
        rows.sort(key=lambda t: t[0])
        seq_list = [seq for _, seq, _, _ in rows]
        counts[node_id] = len(seq_list)
        # dedup = _dedup_preserve_order(seq_list)
        dedup = seq_list
        dup_counts[node_id] = len(seq_list) - len(dedup)
        seqs[node_id] = dedup

    # Summary
    nodes_sorted = sorted(seqs.keys())
    print(f"Flow {args.flow}: {len(nodes_sorted)} node(s) with data -> {', '.join(map(str, nodes_sorted))}")
    for n in nodes_sorted:
        sample = seqs[n][:args.show] if args.show > 0 else []
        sample_str = f" first {len(sample)}: {sample}" if args.show > 0 else ""
        extra = f" (dedup dropped {dup_counts[n]})" if dup_counts[n] else ""
        print(f"  Node {n}: {counts[n]} rows -> {len(seqs[n])} unique seqnos{extra}.{sample_str}")

    # for each, show the range of the arrival times 
    for n in nodes_sorted:
        rows = per_node[n]
        if not rows:
            continue
        times = [t for _, _, t, _ in rows]
        print(f"  Node {n}: arrival_time range: {min(times)} .. {max(times)}")
        
    # Strict equality check
    all_strict_equal = _strict_equal(seqs)
    print(f"\nSTRICT equality across nodes: {'OK' if all_strict_equal else 'MISMATCH'}")
    if not all_strict_equal:
        # Group nodes by their exact sequence signature (hash on tuple of seqs)
        sig_to_nodes: Dict[Tuple[int, ...], List[int]] = defaultdict(list)
        for node_id, s in seqs.items():
            sig_to_nodes[tuple(s)].append(node_id)
        print("  Groups by identical sequence:")
        for i, (sig, group_nodes) in enumerate(sig_to_nodes.items(), 1):
            preview = list(sig[:10])
            suffix = " …" if len(sig) > 10 else ""
            print(f"    [{i}] nodes {group_nodes}: len={len(sig)}, head={preview}{suffix}")

    # Relative order consistency (pairwise)
    print("\nPAIRWISE relative-order consistency (shared seqnos):")
    nodes = nodes_sorted
    all_order_ok = True
    for i in range(len(nodes)):
        for j in range(i + 1, len(nodes)):
            a_id, b_id = nodes[i], nodes[j]
            a, b = seqs[a_id], seqs[b_id]
            bad_count = _order_consistent(a, b)
            if bad_count == 0:
                print(f"  {a_id} vs {b_id}: OK")
            else:
                print("bad_count", bad_count)
    if all_order_ok:
        print("All pairs consistent in relative order.")


    # make a csv, print the sequence of each node. The columns are node IDs, the rows are seqnos
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

    # Plot receiving rates per node
    first_half_of_sorted_nodes = nodes_sorted[:len(nodes_sorted)//2]
    second_half_of_sorted_nodes = nodes_sorted[len(nodes_sorted)//2:]

    def compute_receiving_rate(rows, window_size=0.03):
        # rows: list of (key, seqno, arrival_time, line)
        # window_size: seconds
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
    
    fig, axes = plt.subplots(
        nrows=max(len(first_half_of_sorted_nodes), len(second_half_of_sorted_nodes)),
        ncols=1, figsize=(6, 6), sharex=True, sharey=True
    )

    for col, node_list in enumerate([first_half_of_sorted_nodes, second_half_of_sorted_nodes]):
        for row, node_id in enumerate(node_list):
            ax = axes[row]
            bins, rates = compute_receiving_rate(per_node[node_id])
            if node_id in first_half_of_sorted_nodes:
                zone = "src"
                color = "blue"
            else:
                zone = "dst"
                color = "orange"
            ax.plot(bins, rates, label=f"Node {node_id} ({zone})", color=color)
            ax.set_title(f"Node {node_id}")
            ax.set_ylabel("Receiving rate (pkts/sec)")
            ax.set_xlabel("Time (s)")
            ax.grid(True)
            ax.legend()

    plt.tight_layout()
    plt.savefig(f"flow_{args.flow}_receiving_rates.png", dpi=200)
    
if __name__ == "__main__":
    main()
