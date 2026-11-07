#!/usr/bin/env python3
"""Convert NASA's one-block 2D no-plenum PLOT3D grid to ASCII SU2, without rescaling."""

import argparse
import gzip
import hashlib
import math
from pathlib import Path


def convert(source, target):
    data = source.read_bytes()
    text = gzip.decompress(data).decode() if source.suffix == ".gz" else data.decode()
    tokens = text.split()
    if len(tokens) < 3 or tokens[0] != "1":
        raise ValueError("Require one-block, formatted, 2D PLOT3D with a block-count header")
    ni, nj = map(int, tokens[1:3])
    count = ni * nj
    if min(ni, nj) < 2 or len(tokens) != 3 + 2*count:
        raise ValueError("Invalid 2D PLOT3D dimensions or coordinate count")
    coordinates = [float(value.replace("D", "E").replace("d", "e")) for value in tokens[3:]]
    if not all(map(math.isfinite, coordinates)):
        raise ValueError("Nonfinite grid coordinates")
    x, y = coordinates[:count], coordinates[count:]
    if not (x[0] < -2.14 and x[ni-1] > 1.3 and 0.05 < max(y[:ni]) < 0.2):
        raise ValueError("Expected NASA hump in chord units, ordered upstream to downstream")
    cells = []
    for j in range(nj-1):
        for i in range(ni-1):
            p = j*ni + i
            cell = (p, p+1, p+ni+1, p+ni)
            for k in range(4):
                a, b, c = cell[k], cell[(k+1) % 4], cell[(k+2) % 4]
                if (x[b]-x[a])*(y[c]-y[b]) - (y[b]-y[a])*(x[c]-x[b]) <= 0:
                    raise ValueError(f"Inverted, degenerate or nonconvex cell at i={i}, j={j}")
            cells.append(cell)
    boundaries = {
        "wall": [(i, i+1) for i in range(ni-1)],
        "outlet": [(j*ni+ni-1, (j+1)*ni+ni-1) for j in range(nj-1)],
        "upper": [((nj-1)*ni+i+1, (nj-1)*ni+i) for i in range(ni-1)],
        "inlet": [((j+1)*ni, j*ni) for j in range(nj-1)],
    }
    with target.open("x") as stream:
        stream.write(f"% NASA no-plenum source {source.name}\n% Source SHA256 {hashlib.sha256(data).hexdigest()}\n")
        stream.write(f"NDIME= 2\nNELEM= {len(cells)}\n")
        for cell in cells:
            stream.write("9 " + " ".join(map(str, cell)) + "\n")
        stream.write(f"NPOIN= {count}\n")
        for p in range(count):
            stream.write(f"{x[p]:.17g} {y[p]:.17g} {p}\n")
        stream.write(f"NMARK= {len(boundaries)}\n")
        for marker, edges in boundaries.items():
            stream.write(f"MARKER_TAG= {marker}\nMARKER_ELEMS= {len(edges)}\n")
            for a, b in edges:
                stream.write(f"3 {a} {b}\n")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("source", type=Path, help="hump2newtop_noplenumZ*.p2dfmt[.gz]")
    parser.add_argument("target", type=Path, help="new .su2 file")
    args = parser.parse_args()
    try:
        convert(args.source, args.target)
    except (OSError, ValueError) as error:
        parser.exit(1, f"Conversion failed: {error}\n")
