#!/usr/bin/env python3
"""parse-sample.py — Parse macOS `sample` output into a ranked hot-functions list.

Usage:
    python3 parse-sample.py <sample_output.txt> [--top N] [--filter PATTERN]

The `sample` command produces indented call trees with sample counts.
This script extracts per-function sample counts, ranks them, and prints
a summary showing where the CPU is spending time — ideal for finding
idle-loop hotspots.
"""

from __future__ import annotations

import re
import sys
import argparse
from collections import defaultdict
from pathlib import Path


def parse_sample_file(path: str) -> dict:
    """Parse a macOS `sample` output file.

    Returns a dict with:
      - 'total_samples': int
      - 'functions': dict[str, int]  (function name -> sample count)
      - 'modules': dict[str, int]    (binary/dylib name -> sample count)
      - 'threads': list of thread summaries
    """
    text = Path(path).read_text(errors="replace")

    functions = defaultdict(int)
    modules = defaultdict(int)
    threads = []
    total_samples = 0

    # Match lines like:  +   123 some_function  (in libFoo.dylib)  [0x...]
    # or:                 !   45  some_function  (in libFoo.dylib)  [0x...]
    # The sample tool uses indentation with +, |, !, : markers.
    # The key pattern: optional leading whitespace/markers, then a number, then function name.
    frame_re = re.compile(
        r'[+|!: \t]*'           # tree markers
        r'(\d+)'                # sample count
        r'\s+'
        r'(\S+)'               # function name
        r'(?:\s+\(in\s+'
        r'([^)]+)'             # module name
        r'\))?'
    )

    # Also grab total sample count from header
    total_re = re.compile(r'Total number in stack.*?:\s*(\d+)')

    # Thread header
    thread_re = re.compile(r'^\s*(Thread_\d+|Thread \d+).*?DispatchQueue', re.IGNORECASE)
    thread_name_re = re.compile(r'^\s*(Thread_\d+|Thread \d+)', re.IGNORECASE)

    current_thread = None
    thread_samples = 0

    for line in text.splitlines():
        m = total_re.search(line)
        if m:
            total_samples = max(total_samples, int(m.group(1)))

        tm = thread_name_re.match(line)
        if tm:
            if current_thread:
                threads.append((current_thread, thread_samples))
            current_thread = tm.group(1).strip()
            thread_samples = 0

        fm = frame_re.match(line)
        if fm:
            count = int(fm.group(1))
            func = fm.group(2)
            module = fm.group(3) if fm.group(3) else "<unknown>"

            functions[func] += count
            modules[module] += count

            thread_samples = max(thread_samples, count)

    if current_thread:
        threads.append((current_thread, thread_samples))

    return {
        "total_samples": total_samples,
        "functions": dict(functions),
        "modules": dict(modules),
        "threads": threads,
    }


def print_summary(data: dict, top_n: int = 30, filter_pattern: str | None = None):
    """Print a human-readable summary of profiling data."""
    total = data["total_samples"] or 1

    print("=" * 70)
    print(f"  aosdl CPU Profile Summary  (total samples: {data['total_samples']})")
    print("=" * 70)

    # --- Thread overview ---
    if data["threads"]:
        print(f"\n{'Thread':<30} {'Peak Samples':>14}")
        print("-" * 46)
        for name, count in sorted(data["threads"], key=lambda x: -x[1]):
            pct = 100.0 * count / total
            bar = "#" * int(pct / 2)
            print(f"  {name:<28} {count:>8}  ({pct:5.1f}%)  {bar}")

    # --- Module breakdown ---
    print(f"\n{'Module':<40} {'Samples':>10} {'%':>7}")
    print("-" * 60)
    sorted_mods = sorted(data["modules"].items(), key=lambda x: -x[1])
    for mod, count in sorted_mods[:15]:
        pct = 100.0 * count / total
        print(f"  {mod:<38} {count:>10} {pct:>6.1f}%")

    # --- Hot functions ---
    funcs = data["functions"]
    if filter_pattern:
        pat = re.compile(filter_pattern, re.IGNORECASE)
        funcs = {k: v for k, v in funcs.items() if pat.search(k)}

    print(f"\n{'Function':<50} {'Samples':>10} {'%':>7}")
    print("-" * 70)
    sorted_funcs = sorted(funcs.items(), key=lambda x: -x[1])
    for func, count in sorted_funcs[:top_n]:
        pct = 100.0 * count / total
        bar = "#" * max(1, int(pct / 2))
        print(f"  {func:<48} {count:>10} {pct:>6.1f}% {bar}")

    # --- Highlight suspicious idle patterns ---
    print("\n" + "=" * 70)
    print("  Potential idle-loop hotspots:")
    print("=" * 70)
    idle_keywords = [
        "sleep", "poll", "wait", "select", "kevent", "mach_msg",
        "usleep", "nanosleep", "semaphore_wait", "pthread_cond_wait",
        "SDL_Delay", "SDL_WaitEvent", "SDL_PollEvent",
        "CFRunLoop", "dispatch_", "IOKit",
    ]
    found_idle = False
    for func, count in sorted_funcs:
        pct = 100.0 * count / total
        if pct < 0.5:
            continue
        for kw in idle_keywords:
            if kw.lower() in func.lower():
                print(f"  {func:<48} {count:>10} {pct:>6.1f}%  ← {kw}")
                found_idle = True
                break

    if not found_idle:
        print("  (none detected — CPU may be busy-looping without sleep calls)")

    # --- Non-system hotspots (likely your code) ---
    print("\n" + "=" * 70)
    print("  Top non-system functions (likely app code):")
    print("=" * 70)
    system_prefixes = (
        "___", "__", "_dispatch", "_pthread", "mach_", "kevent",
        "CFRunLoop", "CA::", "IOKit", "_semaphore", "szone_",
        "tiny_", "free_", "malloc", "objc_", "OSAtomicDequeue",
    )
    app_funcs = [
        (f, c) for f, c in sorted_funcs
        if not any(f.startswith(p) for p in system_prefixes)
        and "(in aosdl)" in f or "(in libao" in f
        or not f.startswith("_")  # heuristic: user symbols often lack underscore prefix
    ]
    # Also include anything from the aosdl or libao modules
    for func, count in sorted_funcs:
        pct = 100.0 * count / total
        if pct < 0.1:
            continue
    for func, count in app_funcs[:20]:
        pct = 100.0 * count / total
        if pct >= 0.1:
            print(f"  {func:<48} {count:>10} {pct:>6.1f}%")


def main():
    parser = argparse.ArgumentParser(description="Parse macOS sample output")
    parser.add_argument("sample_file", help="Path to sample output file")
    parser.add_argument("--top", type=int, default=30, help="Show top N functions")
    parser.add_argument("--filter", type=str, default=None,
                        help="Regex filter for function names")
    args = parser.parse_args()

    data = parse_sample_file(args.sample_file)
    print_summary(data, top_n=args.top, filter_pattern=args.filter)


if __name__ == "__main__":
    main()
