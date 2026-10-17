#!/usr/bin/env python3
"""Disjoint original-cell BL cones: exact support/containment predicates, numerical analytic mass lower bound."""
import csv, hashlib, json, os, struct, subprocess, sys, time
from pathlib import Path
import numpy as np
from scipy.spatial import ConvexHull
from run import HERE, ROOT, FROZEN, OLD, BUILD, sha
sys.path.insert(0, str(ROOT / 'TestCases/adaptation/capability'))
from capcheck import read_su2, faces_of

DTYPE = np.dtype([('face', '<u8'), ('cell', '<u8'), ('points', '<f8', (5, 3)), ('lower', '<f8')])
FOLDER = HERE / 'supporting_wall_floor_v1'
BINARY = FOLDER / 'support_planes'


def packet(path, wall, records, threshold, seconds):
    with path.open('wb') as stream:
        stream.write(struct.pack('<QQdd', len(wall), len(records), threshold, seconds))
        np.asarray(wall, dtype='<f8').tofile(stream);np.asarray(records, dtype=DTYPE).tofile(stream)


def selfcheck():
    wall = np.array([[0., 0, 0], [1., 0, 0], [0., 1, 0], [0., 0, -1]])
    rows = np.zeros(4, dtype=DTYPE);rows['face'] = np.arange(4);rows['cell'] = [0, 0, 1, 2];rows['lower'] = [5, 7, 100, 200]
    for row in rows:row['points'] = np.r_[wall[:3], [[0., 0, 1], [.2, .2, .2]]]
    rows['points'][1, [0, 1]] = rows['points'][1, [1, 0]]
    rows['points'][2, 4] = [2, 2, .2]
    rows['points'][3, 3:] = [[0, 0, -2], [.2, .2, -.2]]
    source = FOLDER / 'selfcheck.bin';output = FOLDER / 'selfcheck.csv';packet(source, wall, rows, 1e9, 5.)
    run = subprocess.run([str(BINARY), str(source), str(output)], capture_output=True, text=True, check=True)
    actual = list(csv.DictReader(output.open()))
    assert [(int(r['supporting']), int(r['inside'])) for r in actual] == [(1, 1), (1, 1), (1, 0), (0, 1)]
    assert float(actual[-1]['lower_sum']) == 7, 'Duplicate original cells must not double-count cones.'
    (FOLDER / 'selfcheck.json').write_text(json.dumps({'passed': True, 'stdout': run.stdout, 'input_sha256': sha(source), 'output_sha256': sha(output)}, indent=2) + '\n')


def projection_inside(points):
    # Exact binary64 plane-edge halfspaces; the point's normal component cancels algebraically.
    values = [[float(v).as_integer_ratio() for v in row] for row in points]
    exponent = max(den.bit_length() - 1 for row in values for num, den in row)
    integer = [[num << (exponent - (den.bit_length() - 1)) for num, den in row] for row in values]
    minus = lambda a, b: [a[k] - b[k] for k in range(3)]
    cross = lambda a, b: [a[1]*b[2]-a[2]*b[1], a[2]*b[0]-a[0]*b[2], a[0]*b[1]-a[1]*b[0]]
    normal = cross(minus(integer[1], integer[0]), minus(integer[2], integer[0]))
    for k in range(3):
        inward = cross(normal, minus(integer[(k + 1) % 3], integer[k]))
        if sum(a * b for a, b in zip(inward, minus(integer[3], integer[k]))) < 0:return False
    return True


def conical_height_lower(height, h0, growth):
    # hn(r) <= h0+b*r; integrating that reciprocal gives a lower bound, not a changed wall law.
    b = np.longdouble(2 * (growth - 1) / (growth + 1));k = b * height / h0
    result = np.empty_like(k);small = k < .01
    result[small] = sum((-k[small]) ** n * np.longdouble(2) / ((n + 1)*(n + 2)*(n + 3)) for n in range(8))
    result[~small] = ((1 + k[~small]) ** 2 * np.log1p(k[~small]) - k[~small] - 1.5*k[~small]**2) / k[~small]**3
    assert np.all(result > 0)
    return height / h0 * result


def prepare():
    mesh = read_su2(FROZEN / 'Coarse.su2');faces = mesh.M['WING3D'];wall_ids = np.unique(faces)
    fixture = OLD / 'cell329/replay.dat';h0, growth, thickness = map(float, fixture.read_text().splitlines()[2].split())
    x = mesh.P[faces].astype(np.longdouble);edges = np.roll(x, -1, axis=1) - x
    normal = np.cross(x[:, 1] - x[:, 0], x[:, 2] - x[:, 0]);norm = np.sqrt(np.sum(normal*normal, axis=1))
    assert np.all(norm > 0);area = norm / 2;normal /= norm[:, None]
    log_product = np.log(np.longdouble(4) / 3 * norm ** 2)
    length = np.sqrt(np.sum(edges * edges, axis=2));trace = np.zeros(len(mesh.P), dtype=np.longdouble);weight = np.zeros_like(trace)
    for k in range(3):
        angle = np.arccos(np.clip(-np.sum(edges[:, (k+2)%3]*edges[:, k], axis=1) / (length[:, (k+2)%3]*length[:, k]), -1, 1))
        np.add.at(trace, faces[:, k], angle * log_product);np.add.at(weight, faces[:, k], angle)
    assert np.all(weight[wall_ids] > 0);trace[wall_ids] /= weight[wall_ids]
    # QHull ONLY preselects a subset. Every included plane is then checked against ALL wall vertices exactly.
    hull = ConvexHull(mesh.P[wall_ids]);wanted = {tuple(sorted(row)): i for i, row in enumerate(faces)}
    candidates = sorted({wanted[tuple(sorted(wall_ids[row]))] for row in hull.simplices if tuple(sorted(wall_ids[row])) in wanted})
    boundary = {tuple(sorted(faces[i])): i for i in candidates};owners = {}
    all_faces, cell_ids = faces_of(mesh.E, 3);is_wall = np.zeros(len(mesh.P), dtype=bool);is_wall[wall_ids] = True
    keep = np.all(is_wall[all_faces], axis=1)
    for face, cell in zip(all_faces[keep], cell_ids[keep]):
        index = boundary.get(tuple(face))
        if index is not None:
            assert index not in owners, 'Expected one original fluid cell per boundary facet.'
            owners[index] = int(cell)
    assert len(owners) == len(candidates)
    ids = np.array(candidates);base = x[ids];n = normal[ids]
    cells = np.array([owners[i] for i in candidates])
    opposite = np.array([next(int(k) for k in mesh.E[cell] if k not in set(faces[face])) for face, cell in zip(candidates, cells)])
    apex = mesh.P[opposite].astype(np.longdouble);offset = apex - base[:, 0]
    signed = np.sum(offset * n, axis=1);assert np.all(signed != 0)
    n *= np.sign(signed)[:, None];altitude = abs(signed)
    u, v = base[:, 1] - base[:, 0], base[:, 2] - base[:, 0]
    cross = np.cross(u, v);square = np.sum(cross * cross, axis=1)
    b1 = np.sum(np.cross(offset, v) * cross, axis=1) / square
    b2 = np.sum(np.cross(u, offset) * cross, axis=1) / square
    bary = np.column_stack([1 - b1 - b2, b1, b2])
    radius = .5 * np.minimum(max(h0, .9*thickness), altitude / (3 * bary.max(axis=1)))
    cone = np.asarray(np.mean(base, axis=1) + radius[:, None] * n, dtype=float)
    actual = cone.astype(np.longdouble) - base[:, 0];height = np.sum(actual * n, axis=1)
    assert np.all(height > 0) and np.all(height < max(h0, .9*thickness))
    q1 = np.sum(np.cross(actual, v) * cross, axis=1) / square
    q2 = np.sum(np.cross(u, actual) * cross, axis=1) / square
    qbeta = np.column_stack([1-q1-q2, q1, q2]);assert np.all(qbeta > 0)
    local_trace = trace[faces[ids]]
    density_lower = np.exp(-.5*np.maximum(local_trace.mean(axis=1), np.sum(qbeta*local_trace, axis=1)))
    lower = area[ids] * density_lower * conical_height_lower(height, h0, growth)
    loose_lower = area[ids] * np.exp(-.5*local_trace.max(axis=1)) * conical_height_lower(height, h0, growth)
    geometric = np.array([projection_inside([*triangle, point]) for triangle, point in zip(base.astype(float), cone)])
    assert np.all(geometric), 'A selected cone foot must lie inside its original wall triangle exactly.'
    records = np.empty(len(ids), dtype=DTYPE);records['face'] = ids;records['cell'] = cells
    records['points'][:, :3] = base;records['points'][:, 3] = apex;records['points'][:, 4] = cone;records['lower'] = lower
    order = np.argsort(-records['lower']);records = records[order]
    estimates = {'face': ids, 'cell': cells, 'lower': np.asarray(lower, float), 'loose_lower': np.asarray(loose_lower, float), 'height': np.asarray(height, float), 'area': np.asarray(area[ids], float)}
    np.savez_compressed(FOLDER / 'cone_contributions.npz', **estimates)
    source = FOLDER / 'input.bin';packet(source, mesh.P[wall_ids], records, 125000., 180.)
    return {'source': source, 'wall_vertices': len(wall_ids), 'wall_faces': len(faces), 'preselected_faces': len(records),
            'preselection_sum_before_support_checks': float(np.sum(lower)), 'h0': h0, 'growth': growth, 'thickness': thickness,
            'cone_table_sha256': sha(FOLDER / 'cone_contributions.npz')}


def main():
    FOLDER.mkdir(exist_ok=True);selfcheck();start = time.monotonic();meta = prepare();source = meta.pop('source')
    paths = [Path(__file__).resolve(), HERE / 'support_planes.cpp', BINARY, ROOT / 'Common/src/adaptation/CNativePredicates3D.cpp',
             ROOT / 'Common/include/adaptation/CNativeMesh3D.hpp', BUILD / 'Common/src/libSU2AccurateSum.a.p/adaptation_CNativePredicates3D.cpp.o',
             FROZEN / 'Coarse.su2', OLD / 'cell329/replay.dat', ROOT / 'TestCases/adaptation/capability/capcheck.py']
    proof = {'parameters': meta, 'sha256': {str(path.relative_to(ROOT)): sha(path) for path in paths}, 'input_sha256': sha(source)}
    (FOLDER / 'provenance.json').write_text(json.dumps(proof, indent=2) + '\n')
    output = FOLDER / 'certificate.csv'
    command = ['taskset', '-c', '0', 'nice', '-n', '19', '/usr/bin/time', '-v', '-o', str(FOLDER / 'certificate_resources.txt'),
               'timeout', '--kill-after=5s', '200s', str(BINARY), str(source), str(output)]
    with (FOLDER / 'certificate.log').open('w') as log:run = subprocess.run(command, stdout=log, stderr=subprocess.STDOUT)
    rows = list(csv.DictReader(output.open())) if output.exists() else []
    contribution = np.load(FOLDER / 'cone_contributions.npz', allow_pickle=False);lookup = {int(face): i for i, face in enumerate(contribution['face'])}
    selected = {};loose = {};accepted = 0
    for row in rows:
        if int(row['supporting']) and int(row['inside']):
            index = lookup[int(row['face'])];cell = int(row['cell']);assert cell == int(contribution['cell'][index]);accepted += 1
            selected[cell] = max(selected.get(cell, 0.), float(contribution['lower'][index]))
            loose[cell] = max(loose.get(cell, 0.), float(contribution['loose_lower'][index]))
    report = dict(meta, checked=len(rows), exact_support_and_containment_faces=accepted, disjoint_original_cells=len(selected),
                  partial_domain_lower=float(sum(selected.values())), minimum_vertex_density_lower=float(sum(loose.values())),
                  requested_complexity=100000, numerical_mass_machine_bound_proven=False, geometry_uses_exact_predicates=True,
                  full_field_integrated=False, CFD_steps=0, exit_code=run.returncode, elapsed_seconds=time.monotonic()-start,
                  command=command, provenance_sha256=sha(FOLDER/'provenance.json'), output_sha256=sha(output), log_sha256=sha(FOLDER/'certificate.log'))
    (FOLDER/'result.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report),flush=True)
    assert run.returncode in (0,124), 'Inspect saved certificate error; partial rows do not establish completion.'
    source.unlink()  # deterministic original geometry packet, regenerated from the hashed inputs/producer.


if __name__ == '__main__':
    main()
