#!/usr/bin/env python3
"""Check whether a binary tape is sorted in non-decreasing order."""

import argparse
import struct
import sys
from pathlib import Path

try:
    from tqdm import tqdm
except ImportError:
    tqdm = None

BYTES_PER_VALUE = 4
VALUES_PER_CHUNK = 8192


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Check that a binary tape file contains sorted signed int32 values.",
    )
    parser.add_argument("input", type=Path, help="path to the tape file to check")
    parser.add_argument("-q", "--quiet", action="store_true", help="only use the exit code")
    return parser.parse_args()


def iter_int32_values(path: Path):
    with path.open("rb") as stream:
        while chunk := stream.read(VALUES_PER_CHUNK * BYTES_PER_VALUE):
            value_count = len(chunk) // BYTES_PER_VALUE
            yield from struct.unpack(f"<{value_count}i", chunk)


def check_sorted(path: Path, show_progress: bool = True) -> tuple[bool, int, int | None, int | None]:
    previous: int | None = None
    index = 0
    total_cells = path.stat().st_size // BYTES_PER_VALUE
    progress = None if tqdm is None or not show_progress else tqdm(total=total_cells, unit="cell", unit_scale=True, desc="Checking tape")

    try:
        for value in iter_int32_values(path):
            if previous is not None and value < previous:
                return False, index, previous, value
            previous = value
            index += 1
            if progress is not None:
                progress.update(1)
    finally:
        if progress is not None:
            progress.close()

    return True, index, None, None


def main() -> int:
    args = parse_args()

    if not args.input.exists():
        raise SystemExit(f"{args.input} does not exist")
    if args.input.stat().st_size % BYTES_PER_VALUE != 0:
        raise SystemExit(f"{args.input} is not a valid tape: size is not divisible by {BYTES_PER_VALUE}")

    is_sorted, cell_count, previous, current = check_sorted(args.input, show_progress=not args.quiet)
    if is_sorted:
        if not args.quiet:
            print(f"{args.input} is sorted ({cell_count} cells)")
        return 0

    if not args.quiet:
        print(
            f"{args.input} is not sorted at cell {cell_count}: "
            f"{previous} > {current}",
            file=sys.stderr,
        )
    return 1


if __name__ == "__main__":
    raise SystemExit(main())
