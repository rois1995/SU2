#!/usr/bin/env python3
"""One-shot optional-plane-witness branch-vertex exclusion diagnostic.

Read only: mirrors BuildPlanes grouping for the sealed build278 entity2 input,
then excludes faces incident to each group's original branched boundary vertices
once and reports the remaining projected graph. It does not validate a finite
plane reference or alter production behavior.
"""
from collections import defaultdict, deque
from fractions import Fraction
from pathlib import Path
import hashlib
import json

BASE = Path(__file__).parent
SOURCE = BASE / "final_certificate_entity2_v1" / "entity2.txt"
EXPECTED_SHA256 = "f6627a5e4b748148475dceebc036be3c58c8dc88e3585a12f82f11b120da6d92"
OUT = BASE / "entity2_buildplanes_branch_exclusion.json"


def rational(x):
    n, d = x.as_integer_ratio()
    return Fraction(n, d)


def normal(a, b, c):
    u = [rational(b[k]) - rational(a[k]) for k in range(3)]
    v = [rational(c[k]) - rational(a[k]) for k in range(3)]
    return (u[1]*v[2]-u[2]*v[1], u[2]*v[0]-u[0]*v[2], u[0]*v[1]-u[1]*v[0])


def components(faces, edges):
    parent = list(range(len(faces)))
    local = {face: i for i, face in enumerate(faces)}

    def root(x):
        while parent[x] != x:
            x = parent[x]
        return x

    for uses in edges.values():
        if len(uses) > 1:
            first = local[uses[0][2]]
            for _, _, other, *_ in uses[1:]:
                other = local[other]
                a, b = root(first), root(other)
                if a != b:
                    parent[max(a, b)] = min(a, b)
    return len({root(i) for i in range(len(faces))}) if faces else 0


def analyze(group_faces, triangles, branch_vertices):
    excluded = [i for i in group_faces if any(v in branch_vertices for v in triangles[i])]
    removed = set(excluded)
    kept = [i for i in group_faces if i not in removed]
    uses = defaultdict(list)
    for i in kept:
        t = triangles[i]
        for k in range(3):
            u, v = t[k], t[(k + 1) % 3]
            uses[tuple(sorted((u, v)))].append((u, v, i))
    boundary = [row[0][:2] for row in uses.values() if len(row) == 1]
    out, inc = defaultdict(int), defaultdict(int)
    for u, v in boundary:
        out[u] += 1
        inc[v] += 1
    vertices = set(out) | set(inc)
    branch = sorted(v for v in vertices if out[v] > 1 or inc[v] > 1)
    open_vertices = sorted(v for v in vertices if out[v] != 1 or inc[v] != 1)
    nonmanifold_edges = [e for e, rows in uses.items() if len(rows) > 2]
    same_direction_internal = [e for e, rows in uses.items()
                               if len(rows) == 2 and rows[0][:2] == rows[1][:2]]
    return {
        "excluded_faces": len(excluded),
        "remaining_faces": len(kept),
        "remaining_edge_connected_face_components": components(kept, uses),
        "remaining_boundary_edges": len(boundary),
        "remaining_branch_vertices": len(branch),
        "remaining_open_or_bad_degree_vertices": len(open_vertices),
        "remaining_nonmanifold_edges_gt2": len(nonmanifold_edges),
        "remaining_same_direction_two_use_edges": len(same_direction_internal),
        "bad_degree_samples": [{"vertex": v, "out": out[v], "in": inc[v]} for v in open_vertices[:30]],
    }


def main():
    raw = SOURCE.read_bytes()
    digest = hashlib.sha256(raw).hexdigest()
    if digest != EXPECTED_SHA256:
        raise SystemExit(f"source hash mismatch: {digest}")
    lines = raw.decode().splitlines()
    reference_facets, physical_faces = map(int, lines[0].split()[:2])
    rows = [[float(x) for x in line.split()]
            for line in lines[1 + reference_facets:]]
    if len(rows) != physical_faces or any(len(row) != 9 for row in rows):
        raise SystemExit("unexpected entity2 layout")
    vertex_ids, triangles, projections, gaps = {}, [], [], []
    for row in rows:
        tri = []
        for k in range(3):
            xyz = tuple(row[3*k:3*k+3])
            tri.append(vertex_ids.setdefault(xyz, len(vertex_ids)))
        triangles.append(tuple(tri))
        n = normal(row[:3], row[3:6], row[6:9])
        axis = max(range(3), key=lambda d: abs(n[d]))
        sorted_magnitudes = sorted((abs(x) for x in n), reverse=True)
        projections.append((axis, (n[axis] > 0) - (n[axis] < 0)))
        gaps.append(float((sorted_magnitudes[0] - sorted_magnitudes[1]) /
                          sorted_magnitudes[0]) if sorted_magnitudes[0] else 0.0)

    parent = list(range(physical_faces))

    def root(x):
        while parent[x] != x:
            x = parent[x]
        return x

    edges = {}
    for i, tri in enumerate(triangles):
        for k in range(3):
            edge = tuple(sorted((tri[k], tri[(k + 1) % 3])))
            if edge not in edges:
                edges[edge] = i
            else:
                j = edges[edge]
                if projections[i][1] and projections[i] == projections[j]:
                    a, b = root(i), root(j)
                    if a != b:
                        parent[max(a, b)] = min(a, b)
    groups = defaultdict(list)
    for i in range(physical_faces):
        groups[root(i)].append(i)

    results = []
    for group_root, face_ids in sorted(groups.items()):
        if len(face_ids) < 2 or not projections[group_root][1]:
            continue
        edge_uses = defaultdict(list)
        for i in face_ids:
            tri = triangles[i]
            for k in range(3):
                u, v = tri[k], tri[(k + 1) % 3]
                edge_uses[tuple(sorted((u, v)))].append((u, v))
        boundary = [row[0] for row in edge_uses.values() if len(row) == 1]
        out, inc = defaultdict(int), defaultdict(int)
        for u, v in boundary:
            out[u] += 1
            inc[v] += 1
        branches = {v for v in set(out) | set(inc) if out[v] > 1 or inc[v] > 1}
        if not branches:
            continue
        results.append({
            "root_index": group_root,
            "faces": len(face_ids),
            "projection_axis_sign": list(projections[group_root]),
            "original_branch_vertices": len(branches),
            "one_shot_exclusion": analyze(face_ids, triangles, branches),
        })
    result = {
        "input": str(SOURCE),
        "sha256": digest,
        "classification": "exact rational cross products of parsed binary64 coordinates; max absolute component selects axis and exact sign selects orientation",
        "physical_faces": physical_faces,
        "unique_exact_coordinate_vertices": len(vertex_ids),
        "projection_groups": len(groups),
        "branched_groups": results,
        "scope": "one-shot removal of original branch-incident triangles only; no recursive pruning and no FinitePlaneReference validation",
    }
    OUT.write_text(json.dumps(result, indent=2) + "\n")
    print(json.dumps(result, indent=2))


if __name__ == "__main__":
    main()
