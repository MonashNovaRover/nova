#!/usr/bin/env python3
"""Calculate diagonal covariance matrices from ros2-unbag JSON output.

This utility is generic and can work with multiple message shapes by either:
- selecting specific vector fields with --field path.to.value, or
- auto-discovering vector-like fields with --auto.

Only per-axis variances are estimated. Cross-covariance terms are set to zero.

Examples:
  # IMU-like fields
  calculate_covariances.py imu.json \
    --field angular_velocity --field linear_acceleration

  # Auto discover vector-like fields in messages
  calculate_covariances.py unbag.json --auto

    # Include mean vectors and full diagnostic details
    calculate_covariances.py unbag.json --auto --verbose
"""

from __future__ import annotations

import argparse
import json
import math
from dataclasses import dataclass
from pathlib import Path
from typing import Any, Dict, Iterable, List, Sequence, Tuple


@dataclass
class FieldSamples:
    path: str
    labels: List[str]
    samples: List[List[float]]


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("input", type=Path, help="Path to ros2-unbag JSON file")
    parser.add_argument(
        "--field",
        action="append",
        default=[],
        help="Dot path to a vector field (repeatable), e.g. angular_velocity or pose.position",
    )
    parser.add_argument(
        "--auto",
        action="store_true",
        help="Auto-discover vector-like numeric fields",
    )
    parser.add_argument(
        "--include-covariance-fields",
        action="store_true",
        help="Include keys ending in _covariance during auto-discovery",
    )
    parser.add_argument(
        "--population",
        action="store_true",
        help="Use population covariance (divide by N) instead of sample covariance (N-1)",
    )
    parser.add_argument(
        "--min-samples",
        type=int,
        default=2,
        help="Minimum number of samples required to report a field (default: 2)",
    )
    parser.add_argument(
        "--output",
        type=Path,
        help="Optional output JSON path. Prints to stdout when omitted.",
    )
    parser.add_argument(
        "--verbose",
        action="store_true",
        help="Include additional diagnostic fields in the output.",
    )
    return parser.parse_args()


def is_number(value: Any) -> bool:
    return isinstance(value, (int, float)) and not isinstance(value, bool) and math.isfinite(value)


def extract_messages(data: Any) -> List[Dict[str, Any]]:
    if isinstance(data, list):
        return [x for x in data if isinstance(x, dict)]

    if not isinstance(data, dict):
        return []

    if "messages" in data and isinstance(data["messages"], list):
        msgs: List[Dict[str, Any]] = []
        for item in data["messages"]:
            if not isinstance(item, dict):
                continue
            msg = item.get("message", item)
            if isinstance(msg, dict):
                msgs.append(msg)
        return msgs

    # Typical ros2-unbag JSON object: {"timestamp": {msg...}, ...}
    dict_values = [v for v in data.values() if isinstance(v, dict)]
    if dict_values:
        return dict_values

    # Fallback: treat whole object as one message.
    return [data]


def get_by_path(obj: Dict[str, Any], path: str) -> Any:
    current: Any = obj
    for token in path.split("."):
        if not isinstance(current, dict) or token not in current:
            return None
        current = current[token]
    return current


def parse_vector(value: Any) -> Tuple[List[str], List[float]] | None:
    if isinstance(value, dict):
        labels = list(value.keys())
        if len(labels) < 2:
            return None
        vector = []
        for key in labels:
            component = value[key]
            if not is_number(component):
                return None
            vector.append(float(component))
        return labels, vector

    if isinstance(value, list):
        if len(value) < 2:
            return None
        if not all(is_number(x) for x in value):
            return None
        labels = [f"i{idx}" for idx in range(len(value))]
        return labels, [float(x) for x in value]

    return None


def discover_vector_paths(
    obj: Dict[str, Any],
    include_covariance_fields: bool,
    prefix: str = "",
) -> List[str]:
    paths: List[str] = []
    for key, value in obj.items():
        path = key if not prefix else f"{prefix}.{key}"

        if not include_covariance_fields and key.endswith("_covariance"):
            continue

        parsed = parse_vector(value)
        if parsed is not None:
            paths.append(path)
            continue

        if isinstance(value, dict):
            paths.extend(discover_vector_paths(value, include_covariance_fields, path))

    return paths


def gather_samples(messages: Sequence[Dict[str, Any]], paths: Iterable[str]) -> Dict[str, FieldSamples]:
    field_data: Dict[str, FieldSamples] = {}

    for path in paths:
        labels: List[str] | None = None
        samples: List[List[float]] = []

        for msg in messages:
            raw = get_by_path(msg, path)
            if raw is None:
                continue

            parsed = parse_vector(raw)
            if parsed is None:
                continue

            this_labels, vector = parsed
            if labels is None:
                labels = this_labels
            if labels != this_labels:
                # Skip shape-mismatched entries for a path.
                continue
            samples.append(vector)

        if labels is not None:
            field_data[path] = FieldSamples(path=path, labels=labels, samples=samples)

    return field_data


def mean_vector(samples: Sequence[Sequence[float]]) -> List[float]:
    count = len(samples)
    dim = len(samples[0])
    sums = [0.0] * dim
    for row in samples:
        for i, value in enumerate(row):
            sums[i] += value
    return [x / count for x in sums]


def covariance_matrix(samples: Sequence[Sequence[float]], use_population: bool) -> List[List[float]]:
    n = len(samples)
    dim = len(samples[0])
    mu = mean_vector(samples)

    denom = n if use_population else (n - 1)
    if denom <= 0:
        raise ValueError("Need at least 2 samples for sample covariance")

    cov = [[0.0 for _ in range(dim)] for _ in range(dim)]
    for row in samples:
        centered = [row[i] - mu[i] for i in range(dim)]
        for i in range(dim):
            cov[i][i] += centered[i] * centered[i]

    for i in range(dim):
        cov[i][i] /= denom

    return cov


def diagonal_values(matrix: Sequence[Sequence[float]]) -> List[float]:
    return [matrix[i][i] for i in range(len(matrix))]


def main() -> int:
    args = parse_args()

    with args.input.open("r", encoding="utf-8") as f:
        data = json.load(f)

    messages = extract_messages(data)
    if not messages:
        raise SystemExit("No messages found in JSON input")

    paths = list(args.field)
    if args.auto:
        discovered = set()
        for msg in messages:
            discovered.update(
                discover_vector_paths(
                    msg,
                    include_covariance_fields=args.include_covariance_fields,
                )
            )
        paths.extend(sorted(discovered))

    unique_paths = sorted(set(paths))
    if not unique_paths:
        raise SystemExit("No fields selected. Use --field and/or --auto")

    fields = gather_samples(messages, unique_paths)

    report: Dict[str, Any] = {
        "input": str(args.input),
        "total_messages": len(messages),
        "covariance_type": "population" if args.population else "sample",
        "fields": {},
    }
    skipped: Dict[str, Any] = {}

    for path in unique_paths:
        info = fields.get(path)
        if info is None:
            skipped[path] = {
                "reason": "not_found_or_not_numeric_vector",
                "samples": 0,
            }
            continue

        if len(info.samples) < args.min_samples:
            skipped[path] = {
                "reason": "insufficient_samples",
                "samples": len(info.samples),
                "min_samples": args.min_samples,
                "labels": info.labels,
            }
            continue

        cov = covariance_matrix(info.samples, use_population=args.population)
        means = mean_vector(info.samples)
        variances = diagonal_values(cov)
        field_report: Dict[str, Any] = {
            "samples": len(info.samples),
            "mean": means,
            "covariance_diagonal": variances,
        }
        if args.verbose:
            field_report["labels"] = info.labels
            field_report["covariance_matrix"] = cov
        report["fields"][path] = field_report

    if skipped:
        report["skipped_fields"] = skipped

    output_text = json.dumps(report, indent=2, sort_keys=True)
    if args.output:
        args.output.write_text(output_text + "\n", encoding="utf-8")
    else:
        print(output_text)

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
