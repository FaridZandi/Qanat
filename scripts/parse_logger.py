#!/usr/bin/env python3
"""
Extract LOGGERNF records (with optional flow ID) from a log into CSV.

Line format now (order fixed):
LOGGERNF: Packet arrival time: 3.76007, seqno: 8761, node ID: 0, flow ID: 1
"""

import argparse
import csv
import gzip
import io
import re
import sys
from typing import Iterable, Iterator, Dict, Optional

_FLOAT = r'[-+]?\d+(?:\.\d+)?(?:[eE][-+]?\d+)?'

LOGGERNF_RE = re.compile(
    rf'^LOGGERNF:\s*Packet\s+arrival\s+time:\s*'
    rf'(?P<time>{_FLOAT})\s*,\s*seqno:\s*(?P<seqno>\d+)\s*,\s*node\s+ID:\s*(?P<node_id>\d+)'
    rf'(?:\s*,\s*flow\s+ID:\s*(?P<flow_id>\d+))?\b',
    re.ASCII
)

def _open_text(path: str):
    if path == "-":
        return io.TextIOWrapper(sys.stdin.buffer, encoding="utf-8", errors="replace")
    if path.endswith(".gz"):
        return io.TextIOWrapper(gzip.open(path, "rb"), encoding="utf-8", errors="replace")
    return open(path, "r", encoding="utf-8", errors="replace")

def parse_loggernf_lines(lines: Iterable[str]) -> Iterator[Dict[str, Optional[object]]]:
    for lineno, raw in enumerate(lines, 1):
        if not raw.startswith("LOGGERNF:"):
            continue
        m = LOGGERNF_RE.search(raw)
        if not m:
            continue
        yield {
            "line": lineno,
            "arrival_time": float(m.group("time")),
            "seqno": int(m.group("seqno")),
            "node_id": int(m.group("node_id")),
            "flow_id": int(m.group("flow_id")) if m.group("flow_id") is not None else None,
        }

def main():
    ap = argparse.ArgumentParser(description="Extract LOGGERNF entries from a log into CSV.")
    ap.add_argument("logfile", help="Path to log file (use '-' for stdin; .gz supported)")
    ap.add_argument("-o", "--output", help="CSV output path (default: stdout)")
    ap.add_argument("--no-header", action="store_true", help="Do not write CSV header row")
    args = ap.parse_args()

    outfh = open(args.output, "w", newline="", encoding="utf-8") if args.output else sys.stdout
    writer = csv.writer(outfh)
    wrote_any = False

    try:
        if not args.no_header:
            writer.writerow(["line", "arrival_time", "seqno", "node_id", "flow_id"])
        with _open_text(args.logfile) as f:
            for r in parse_loggernf_lines(f):
                writer.writerow([r["line"], r["arrival_time"], r["seqno"], r["node_id"], r["flow_id"]])
                wrote_any = True
    finally:
        if args.output:
            outfh.close()

    if not wrote_any:
        print("No LOGGERNF records found.", file=sys.stderr)

if __name__ == "__main__":
    main()
