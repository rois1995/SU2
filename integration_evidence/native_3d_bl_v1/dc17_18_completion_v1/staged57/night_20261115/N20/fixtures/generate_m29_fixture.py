#!/usr/bin/env python3
"""Insert the two closed M29 vertex stars into the N20 optimizer test fragment."""

import argparse
from collections import Counter
from itertools import combinations
from pathlib import Path
import subprocess

HERE = Path(__file__).resolve().parent
M29 = HERE.parents[1] / "N19/reports/M29_dump.txt"
SEEDS = (200812891, 200812895)
MARKER = "@M29_CONSTANTS@"


def groups(values, width):
    data = values.split(",")
    assert len(data) == 4 * width, data
    return [tuple(data[i:i + width]) for i in range(0, len(data), width)]


def read_stars(path):
    """Validate source incidence and preserve the exact decimal literals and ID order."""
    vertices, cells = {}, {}
    source_count = 0
    for line in path.read_text().splitlines():
        if not line.startswith("NATIVE3D_M29_CELL "):
            continue
        source_count += 1
        fields = dict(token.split("=", 1) for token in line.split()[1:])
        ids = tuple(map(int, fields["v"].split(",")))
        if not set(ids).intersection(SEEDS):
            continue
        cell = int(fields["cell"])
        assert cell not in cells and len(set(ids)) == 4
        cells[cell] = ids
        for vertex, point, tensor, free in zip(ids, groups(fields["x"], 3), groups(fields["m"], 6),
                                               fields["free"].split(","), strict=True):
            row = point, tensor, free
            assert vertices.setdefault(vertex, row) == row, vertex
    assert source_count == 521 and len(cells) == 16 and len(vertices) == 10
    for seed in SEEDS:
        link = [tuple(sorted(set(ids) - {seed})) for ids in cells.values() if seed in ids]
        use = Counter(edge for face in link for edge in combinations(face, 2))
        assert len(link) == 10 and len(set(link)) == 10 and set(use.values()) == {2}
        assert vertices[seed][2] == "1"
    return vertices, cells


def constants(vertices, cells):
    """Write each unique vertex and incident cell once across the two stars."""
    lines = [
        "// Exact coordinates and nodal target tensors from the two closed M29 vertex stars.",
        f"constexpr std::array<M29FixtureVertex, {len(vertices)}> M29FixtureVertices{{{{",
    ]
    for vertex, (point, tensor, _) in sorted(vertices.items()):
        lines.append(f"    {{{vertex}, {{{', '.join(point)}}},")
        lines.append(f"     {{{', '.join(tensor)}}}}},")
    lines.extend([
        "}};",
        "",
        "// Original IDs and stored corner orders, shared by both stars.",
        f"constexpr std::array<M29FixtureCell, {len(cells)}> M29FixtureCells{{{{",
    ])
    lines.extend(f"    {{{cell}, {{{', '.join(map(str, ids))}}}}}," for cell, ids in sorted(cells.items()))
    lines.append("}};")
    return "\n".join(lines)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--dump", type=Path, default=M29)
    parser.add_argument("--output", type=Path, default=HERE / "fine_optimizer_fixture.inc")
    args = parser.parse_args()
    template = (HERE / "fine_optimizer_fixture.template").read_text()
    assert template.count(MARKER) == 1
    rendered = template.replace(MARKER, constants(*read_stars(args.dump)))
    formatted = subprocess.run(
        ["clang-format", f"--assume-filename={HERE / 'fine_optimizer_fixture.cpp'}"],
        input=rendered, text=True, capture_output=True, check=True,
    )
    args.output.write_text(formatted.stdout)


if __name__ == "__main__":
    main()
