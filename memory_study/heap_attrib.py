#!/usr/bin/env python3
"""Attribute heaptrack peak memory to SU2 owners.

Input: heaptrack_print --print-flamegraph output with --flamegraph-cost-type peak
(lines "root;...;leaf; bytes"). For each stack, the owner is the innermost frame
that belongs to SU2 (not std::/__gnu_cxx/allocator/MemoryAllocation/C2DContainer
helpers); stacks without an SU2 frame are grouped as MPI/UCX/runtime.
Usage: heap_attrib.py flame_peak.txt.gz [top_n] [npoin]
"""
import gzip, re, sys
from collections import defaultdict

SKIP = re.compile(r"^(std::|__gnu_cxx|operator new|malloc|calloc|realloc|posix_memalign|aligned_alloc|"
                  r"MemoryAllocation::|container_details::|C2DContainer|CVectorOfMatrix|su2activematrix|"
                  r"void std::|0x|C2DDummyLastView|CSysVector<.*>::Initialize|CSysVector<.*>::CSysVector)")
SU2 = re.compile(r"^(C[A-Z][A-Za-z0-9_]*(<[^;]*>)?::|main$|GeometryToolbox|PrintingToolbox|CGraphColoring|"
                 r"colorSparsePattern|buildCSRPattern|createNaturalColoring|.*::Set|.*::Compute)")


MPI = re.compile(r"^(PMPI_|ompi_|ucs_|uct_|ucp_|opal_|pmix|mca_)")


def owner(frames):
    if any(MPI.match(f.strip()) for f in frames):
        return "[MPI/UCX runtime]"
    for f in reversed(frames):
        f = f.strip()
        if not f or SKIP.match(f):
            continue
        if SU2.match(f) or "SU2" in f:
            return re.sub(r"\(.*", "", f)
    if any("ucs_" in f or "uct_" in f or "ucp_" in f or "ompi" in f or "pmix" in f or "opal" in f for f in frames):
        return "[MPI/UCX runtime]"
    return "[other: " + (frames[-2] if len(frames) > 1 else frames[-1]) + "]"


def main():
    path = sys.argv[1]
    top = int(sys.argv[2]) if len(sys.argv) > 2 else 25
    npoin = int(sys.argv[3]) if len(sys.argv) > 3 else 0
    agg = defaultdict(int)
    total = 0
    with gzip.open(path, "rt", errors="replace") as f:
        for line in f:
            line = line.rstrip("\n")
            stack, _, cost = line.rpartition(" ")
            try:
                c = int(cost)
            except ValueError:
                continue
            frames = [x for x in stack.split(";") if x]
            agg[owner(frames)] += c
            total += c
    print(f"total peak bytes attributed: {total / 2**20:.1f} MiB" + (f"  ({total / npoin:.0f} B/point)" if npoin else ""))
    for k, v in sorted(agg.items(), key=lambda kv: -kv[1])[:top]:
        extra = f"  {v / npoin:7.0f} B/pt" if npoin else ""
        print(f"{v / 2**20:9.1f} MiB {100 * v / total:5.1f}%{extra}  {k}")


if __name__ == "__main__":
    main()
