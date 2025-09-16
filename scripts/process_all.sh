#!/usr/bin/env bash

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "${SCRIPT_DIR}/.." && pwd)"
EXPS_ROOT="${ROOT_DIR}/ns-test/exps"
PARSER="${ROOT_DIR}/scripts/parse_logger.py"
FLOW_CHECKER="${ROOT_DIR}/scripts/check_flow.py"

if [[ ! -d "${EXPS_ROOT}" ]]; then
    echo "No experiments found under ${EXPS_ROOT}" >&2
    exit 1
fi

if [[ ! -f "${PARSER}" ]]; then
    echo "Missing parser script: ${PARSER}" >&2
    exit 1
fi

if [[ ! -f "${FLOW_CHECKER}" ]]; then
    echo "Missing flow checker script: ${FLOW_CHECKER}" >&2
    exit 1
fi

MAX_LOG_JOBS=${PROCESS_ALL_JOBS:-}
if [[ -z "${MAX_LOG_JOBS}" ]]; then
    if command -v nproc >/dev/null 2>&1; then
        MAX_LOG_JOBS=$(nproc)
    else
        MAX_LOG_JOBS=1
    fi
fi
if [[ ${MAX_LOG_JOBS} -lt 1 ]]; then
    MAX_LOG_JOBS=1
fi

echo "Using up to ${MAX_LOG_JOBS} parallel experiment jobs" >&2

MAX_FLOW_JOBS=${PROCESS_FLOW_JOBS:-1}
if [[ ${MAX_FLOW_JOBS} -lt 1 ]]; then
    MAX_FLOW_JOBS=1
fi

mapfile -t LOGFILES < <(find "${EXPS_ROOT}" -type f -name 'logFile.tr' | sort)

if [[ ${#LOGFILES[@]} -eq 0 ]]; then
    echo "No logFile.tr files found under ${EXPS_ROOT}" >&2
    exit 1
fi

declare -a LOG_PIDS=()

wait_for_slot() {
    while [[ ${#LOG_PIDS[@]} -ge ${MAX_LOG_JOBS} ]]; do
        wait "${LOG_PIDS[0]}" || true
        LOG_PIDS=("${LOG_PIDS[@]:1}")
    done
}

wait_remaining() {
    for pid in "${LOG_PIDS[@]}"; do
        wait "${pid}" || true
    done
    LOG_PIDS=()
}

process_flow_checks() {
    local exp_dir=$1
    shift
    local flows=("$@")
    local -a pids=()

    for flow in "${flows[@]}"; do
        (
            cd "${exp_dir}"
            python3 "${FLOW_CHECKER}" "logger.csv" --flow "${flow}" > "flow_${flow}_analysis.txt"
        ) &
        pids+=("$!")

        while [[ ${#pids[@]} -ge ${MAX_FLOW_JOBS} ]]; do
            wait "${pids[0]}" || true
            pids=("${pids[@]:1}")
        done
    done

    for pid in "${pids[@]}"; do
        wait "${pid}" || true
    done
}

process_log() {
    local logfile=$1
    local exp_dir rel_dir csv_path

    exp_dir="$(dirname "${logfile}")"
    rel_dir="${exp_dir#${ROOT_DIR}/}"

    echo "Processing ${rel_dir}" >&2

    csv_path="${exp_dir}/logger.csv"
    python3 "${PARSER}" "${logfile}" -o "${csv_path}" >/dev/null

    mapfile -t flows < <(python3 - "${csv_path}" <<'PY2'
import csv
import sys
from pathlib import Path

csv_path = Path(sys.argv[1])
if not csv_path.is_file():
    sys.exit(0)
flows = []
seen = set()
with csv_path.open("r", encoding="utf-8", newline="") as fh:
    reader = csv.DictReader(fh)
    if reader.fieldnames and "flow_id" in reader.fieldnames:
        for row in reader:
            val = (row.get("flow_id") or "").strip()
            if not val:
                continue
            try:
                flow = int(val)
            except ValueError:
                try:
                    flow = int(float(val))
                except ValueError:
                    continue
            if flow not in seen:
                seen.add(flow)
                flows.append(flow)
for fid in sorted(flows):
    print(fid)
PY2
    )

    if [[ ${#flows[@]} -eq 0 ]]; then
        echo "  No flow IDs found; skipping flow checks." >&2
        return
    fi

    echo "  Checking flows: ${flows[*]}" >&2
    process_flow_checks "${exp_dir}" "${flows[@]}"
}

for logfile in "${LOGFILES[@]}"; do
    wait_for_slot
    process_log "${logfile}" &
    LOG_PIDS+=("$!")
done

wait_remaining

echo "Done." >&2
