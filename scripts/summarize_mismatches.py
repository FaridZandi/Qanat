#!/usr/bin/env python3
"""Summarize ORDER MISMATCH counts for flow analysis files."""

import argparse
import csv
import re
from pathlib import Path


def count_mismatches(file_path: Path) -> int:
    count = 0
    try:
        with file_path.open('r', encoding='utf-8', errors='ignore') as fh:
            for line in fh:
                if 'ORDER MISMATCH' in line:
                    count += 1
    except FileNotFoundError:
        return 0
    return count


def parse_setting(setting: str) -> dict:
    tokens = setting.split('_')
    data = {}
    for token in tokens:
        if not token:
            continue
        match = re.match(r"([a-zA-Z]+)(.*)", token)
        if not match:
            continue
        key, suffix = match.groups()
        value = suffix.lstrip('-') if suffix else '1'
        data[key] = value
    return data


def scan_root(root: Path):
    results = []
    for path in sorted(root.rglob('flow_*_analysis.txt')):
        print(path)
        count = count_mismatches(path)
        # rel_path = path.relative_to(root)
        rel_path = path
        parts = rel_path.parts
        if 'exps' in parts:
            idx = parts.index('exps')
            exp_name = parts[idx + 1] if len(parts) > idx + 1 else ''
            setting = parts[idx + 2] if len(parts) > idx + 2 else ''
        else:
            exp_name = parts[0] if len(parts) > 0 else ''
            setting = parts[1] if len(parts) > 1 else ''
        flow_file = parts[-1]
        flow_id = flow_file.split('_')[1] if '_' in flow_file else flow_file
        setting_info = parse_setting(setting)
        print(setting_info)
        results.append({
            'path': path,
            'count': count,
            'exp_name': exp_name,
            'setting': setting,
            'flow_id': flow_id,
            'setting_info': setting_info
        })
    return results


def write_aggregated_csv(rows, all_keys, out_path):
    aggregator = {}
    for row in rows:
        key = (row['exp_name'], row['setting'])
        if key not in aggregator:
            aggregator[key] = {
                'exp_name': row['exp_name'],
                'setting': row['setting'],
                'mismatches': 0,
                'setting_info': row['setting_info'],
            }
        aggregator[key]['mismatches'] += row['mismatches']

    fieldnames = ['exp_name', 'mismatches'] + sorted(all_keys)
    with out_path.open('w', newline='', encoding='utf-8') as csvfile:
        writer = csv.DictWriter(csvfile, fieldnames=fieldnames)
        writer.writeheader()
        for data in aggregator.values():
            row_data = {
                'exp_name': data['exp_name'],
                # 'setting': data['setting'],
                'mismatches': data['mismatches'],
            }
            for key in all_keys:
                row_data[key] = data['setting_info'].get(key, '')
            writer.writerow(row_data)


def main():
    parser = argparse.ArgumentParser(description='Count ORDER MISMATCH lines in flow analysis files.')
    parser.add_argument('root', nargs='?', default='.', help='Root directory to scan (default: current directory)')
    parser.add_argument('--nonzero', action='store_true', help='Only include entries with mismatches > 0')
    parser.add_argument('--csv', default='mismatch_summary.csv', help='Output CSV file path (default: under root)')
    parser.add_argument('--agg-csv', default='mismatch_summary_agg.csv', help='Aggregated CSV file name (default: under root)')
    args = parser.parse_args()

    root = Path(args.root).resolve()
    if not root.exists():
        parser.error(f'Root path {root} does not exist')

    results = scan_root(root)
    total = 0
    rows = []
    all_keys = set()
    for item in results:
        total += item['count']
        if args.nonzero and item['count'] == 0:
            continue
        setting_data = item['setting_info']
        all_keys.update(setting_data.keys())
        rows.append({
            'exp_name': item['exp_name'],
            'setting': item['setting'],
            'flow_id': item['flow_id'],
            'mismatches': item['count'],
            'setting_info': setting_data
        })

    if rows:
        fieldnames = ['exp_name', 'flow_id', 'mismatches'] + sorted(all_keys)
        out_path = root / args.csv
        with out_path.open('w', newline='', encoding='utf-8') as csvfile:
            writer = csv.DictWriter(csvfile, fieldnames=fieldnames)
            writer.writeheader()
            for row in rows:
                row_data = {
                    'exp_name': row['exp_name'],
                    # 'setting': row['setting'],
                    'flow_id': row['flow_id'],
                    'mismatches': row['mismatches'],
                }
                for key in all_keys:
                    row_data[key] = row['setting_info'].get(key, '')
                writer.writerow(row_data)
        print(f'CSV written to {out_path} with {len(rows)} rows.')

        agg_path = root / args.agg_csv
        write_aggregated_csv(rows, all_keys, agg_path)
        print(f'Aggregated CSV written to {agg_path}.')
    else:
        print('No data to write to CSV.')

    print(f'Total ORDER MISMATCH occurrences: {total}')


if __name__ == '__main__':
    main()
