#!/usr/bin/env python3
"""Generate a random binary tape."""

import argparse
import random
import struct
from pathlib import Path

try:
    from tqdm import tqdm
except ImportError:
    tqdm = None

INT32_MIN = -(2**31)
INT32_MAX = 2**31 - 1
VALUES_PER_CHUNK = 8192


def int32_value(text: str) -> int:
    value = int(text, 0)
    if value < INT32_MIN or value > INT32_MAX:
        raise argparse.ArgumentTypeError(f"{value} is outside int32 range")
    return value


def non_negative_size(text: str) -> int:
    value = int(text, 0)
    if value < 0:
        raise argparse.ArgumentTypeError("size must be non-negative")
    return value


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Generate a binary tape file with random signed int32 values.",
    )
    parser.add_argument("output", type=Path, help="path to the tape file to create")
    parser.add_argument("size", type=non_negative_size, help="number of int32 tape cells to write")
    parser.add_argument("--min", dest="min_value", type=int32_value, default=INT32_MIN, help="minimum generated value")
    parser.add_argument("--max", dest="max_value", type=int32_value, default=INT32_MAX, help="maximum generated value")
    parser.add_argument("--seed", type=int, help="seed for reproducible output")
    parser.add_argument("-f", "--force", action="store_true", help="overwrite output if it already exists")
    return parser.parse_args()


def write_random_tape(path: Path, size: int, min_value: int, max_value: int, seed: int | None) -> None:
    rng = random.Random(seed)
    path.parent.mkdir(parents=True, exist_ok=True)

    with path.open("wb") as stream:
        progress = None if tqdm is None else tqdm(total=size, unit="cell", unit_scale=True, desc="Writing tape")
        try:
            values_left = size
            while values_left > 0:
                chunk_size = min(values_left, VALUES_PER_CHUNK)
                chunk = (rng.randint(min_value, max_value) for _ in range(chunk_size))
                stream.write(struct.pack(f"<{chunk_size}i", *chunk))
                values_left -= chunk_size
                if progress is not None:
                    progress.update(chunk_size)
        finally:
            if progress is not None:
                progress.close()


def main() -> int:
    args = parse_args()
    if args.min_value > args.max_value:
        raise SystemExit("--min must be less than or equal to --max")
    if args.output.exists() and not args.force:
        raise SystemExit(f"{args.output} already exists; pass --force to overwrite it")

    write_random_tape(args.output, args.size, args.min_value, args.max_value, args.seed)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
