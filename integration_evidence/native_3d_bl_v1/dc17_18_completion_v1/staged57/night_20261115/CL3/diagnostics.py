"""Read marker and flat-cell observations from a terminal CL3 or CL2 case."""

from collections import defaultdict
from itertools import combinations
import os

from run_cl3 import CL2, REPO

# Bound the offline validator and spatial search to one thread.
os.environ.update(dict.fromkeys(('OMP_NUM_THREADS', 'OPENBLAS_NUM_THREADS', 'MKL_NUM_THREADS'), '1'))

import numpy as np

from runtime_checks import CYCLE, FIELDS, log_lines, log_stamp, rank_log, rejected_cells
from validate import STAGED, module

CAP = module('cl3_capcheck', REPO / 'TestCases/adaptation/capability/capcheck.py')
TIMER = module('cl3_timer', STAGED / 'f3b_20261115/check_timer_tree.py')
TOLERANCES = {'FARFIELD': 1., 'SYMMETRY': 1e-4, 'WING3D': 1e-4}
PHASES = ('construction', 'quality', 'final_proof')
FACE_NODES = tuple(combinations(range(4), 3))
EDGE_NODES = tuple(combinations(range(4), 2))
GEOMETRY_MATCH = 1e-12  # Only for matching coordinates written to disk in this report.


def fields(text, kind):
    return dict(FIELDS.findall(text.split(kind + ' ', 1)[1]))


def runtime(case):
    """Keep evidence on separate rank-0 streams, using the stamped cycle boundaries for proof rows."""
    event = None
    starts, result = [], defaultdict(lambda: {'tolerance': [], 'final_cert': [], 'proof_vote': []})
    for line, raw in log_lines(rank_log(case, 0)):
        if match := CYCLE.match(line):
            cycle = int(match[1])
            if cycle != 4:
                event = cycle + 1
            else:
                event = None
            stamp = log_stamp(raw)
            if stamp is not None:
                starts.append((stamp, event))
        if 'NATIVE3D_TOLERANCE ' in line:
            row = fields(line, 'NATIVE3D_TOLERANCE')
            try:
                markers = []
                for entry in row['markers'].split(','):
                    name, i, value, group = entry.rsplit(':', 3)
                    markers.append({'name': name, 'index': int(i), 'value': float(value), 'class': int(group)})
                parsed = {name: int(row[name]) for name in ('rank', 'seam_corners', 'seam_edges', 'refused_seam_edits')}
                parsed.update(strict=float(row['strict']), markers=markers)
            except (ValueError, KeyError):
                parsed = {'parse_error': True}
            result[event]['tolerance'].append({'line': line, **parsed})

    stderr = rank_log(case, 0).with_name('stderr')
    for line, raw in log_lines(stderr):
        if 'NATIVE3D_FINAL_CERT ' not in line and 'NATIVE3D_FINAL_PROOF ' not in line:
            continue
        stamp = log_stamp(raw)
        event = next((idx for boundary, idx in reversed(starts) if stamp is not None and boundary <= stamp), None)
        if event is None:
            raise ValueError(f'Cannot match final proof to an adaptation cycle: {raw.strip()}')
        if 'NATIVE3D_FINAL_PROOF ' in line:
            result[event]['proof_vote'].append(line)
            continue
        row = fields(line, 'NATIVE3D_FINAL_CERT')
        try:
            parsed = {'entity': row['entity'], 'index': int(row['entity'].rsplit('/', 1)[1]),
                      'faces': int(row['faces']), 'within': int(row['within']),
                      **{key: float(row[key]) for key in ('mesh_to_model', 'model_to_mesh', 'tolerance')}}
        except (ValueError, KeyError):
            parsed = {'parse_error': True}
        result[event]['final_cert'].append({'line': line, **parsed})
    return result


def wiring(rows, expected_indices):
    """Only printed values that prove a mismatch are R5 wiring defects."""
    events, defects = {}, []
    for event, event_rows in sorted(rows.items()):
        if event is None:
            raise ValueError('Tolerance row outside an adaptation cycle')
        tolerance, certs = event_rows['tolerance'], event_rows['final_cert']
        status = {'tolerance_table': 'not evaluated', 'final_proof_tolerance': 'not evaluated'}
        for row in tolerance:
            if row.get('parse_error'):
                continue
            classes = sorted(set(TOLERANCES.values()))
            markers = {mark['name']: (mark['index'], mark['value'], mark['class']) for mark in row['markers']}
            target = {name: (idx, TOLERANCES[name], classes.index(TOLERANCES[name]))
                      for idx, name in expected_indices.items()}
            if row['rank'] != 0 or row['strict'] != 1e-4 or markers != target or len(row['markers']) != len(target):
                status['tolerance_table'] = 'FAILED'
                defects.append({'cycle': event, 'check': 'tolerance_table', 'status': 'FAILED',
                                'reason': f'Wrong printed tolerance table: {row["line"]}'})
            elif status['tolerance_table'] != 'FAILED':
                status['tolerance_table'] = 'passed'
        for row in certs:
            if row.get('parse_error'):
                continue
            name = expected_indices.get(row['index'])
            target = TOLERANCES.get(name)
            row['marker'] = name
            if target != row['tolerance']:
                status['final_proof_tolerance'] = 'FAILED'
                defects.append({'cycle': event, 'check': 'final_proof_tolerance', 'status': 'FAILED',
                                'reason': f'{name} entity {row["entity"]} printed {row["tolerance"]}, expected {target}'})
            elif status['final_proof_tolerance'] != 'FAILED':
                status['final_proof_tolerance'] = 'passed'
        events[event] = status
    return {'wiring_defect': bool(defects), 'defects': defects, 'events': events}


def phase_times(case):
    result = defaultdict(dict)
    for rank in range(6):
        path = rank_log(case, rank)
        if not path.is_file():
            continue
        rows = TIMER.snapshots(path.read_text(errors='replace'))
        final = [row for row in rows if row['summary'].get('root') == 'remesh'
                 and row['summary'].get('boundary') == 'final' and row['summary'].get('rank') == str(rank)]
        for event, snapshot in enumerate(final, 1):
            ledger = TIMER.assess(snapshot)
            result[event][str(rank)] = {
                phase: sum(float(row['exclusive_seconds']) for row in ledger['rows'] if row['phase'] == phase)
                for phase in PHASES}
    return result


def vertices(mesh, name):
    return np.unique(mesh.M[name]) if name in mesh.M else np.array([], dtype=int)


def coordinates(row):
    values = fields(row['line'], 'NATIVE3D_M29_CELL')['x'].split(',')
    return np.asarray(values, dtype=float).reshape(4, 3)


def point_index(points):
    order = np.argsort(points[:, 0], kind='stable')
    return points, order, points[order, 0]


def point_ids(index, points):
    coordinates, order, x = index
    lo = np.searchsorted(x, points[:, 0] - GEOMETRY_MATCH)
    hi = np.searchsorted(x, points[:, 0] + GEOMETRY_MATCH, side='right')
    ids = []
    for point, begin, end in zip(points, lo, hi):
        candidates = order[begin:end]
        distances = np.max(np.abs(coordinates[candidates] - point), axis=1)
        if len(distances) and distances.min() <= GEOMETRY_MATCH:
            ids.append(int(candidates[distances.argmin()]))
        else:
            ids.append(-1)
    return np.asarray(ids)


def marker_matches(index, markers, points):
    return [sorted(mark for mark, indices in markers.items() if idx in indices) for idx in point_ids(index, points)]


def cap_coordinates():
    row, = [seed for seed in rejected_cells(CL2 / 'run', 2) if seed['rank'] == 1 and seed['cell'] == '401818']
    points = coordinates(row)
    return points, row['q']


def flat_cells(mesh, name):
    marked = np.zeros(len(mesh.P), dtype=bool)
    marked[vertices(mesh, name)] = True
    faces = {tuple(face) for face in np.sort(mesh.M[name], axis=1)}
    possible = mesh.E[np.all(marked[mesh.E], axis=1)]
    flat = [cell for cell in possible if sum(tuple(sorted(cell[i] for i in face)) in faces for face in FACE_NODES) >= 2]
    return np.asarray(flat, dtype=int).reshape(-1, 4)


def ratio(points, metric):
    edges = np.asarray([points[j] - points[i] for i, j in EDGE_NODES])
    denominator = np.einsum('ij,jk,ik->', edges, metric, edges)
    volume_frame = abs(np.linalg.det(points[1:] - points[0])) * np.sqrt(np.linalg.det(metric))
    return float(12 * (volume_frame / 2) ** (2 / 3) / denominator)


def donor_field(mesh, path):
    metric, precision = CAP.metric_of(mesh, path)
    points = mesh.P[mesh.E]
    return mesh.E, points, points.mean(axis=1), metric, precision


def sampled_ratio(field, points):
    """Estimate q with the archived donor P1 field when a containing donor cell is found."""
    connectivity, tetrahedra, centroids, metrics, _ = field
    query = points.mean(axis=0)
    offsets = centroids - query
    distances = np.einsum('ij,ij->i', offsets, offsets)
    last = min(64, len(tetrahedra)) - 1
    candidates = np.argpartition(distances, last)[:last+1]
    for i in candidates[np.argsort(distances[candidates])]:
        tet = tetrahedra[i]
        try:
            bary = np.linalg.solve((tet[1:] - tet[0]).T, query - tet[0])
        except np.linalg.LinAlgError:
            continue
        weights = np.r_[1 - bary.sum(), bary]
        if min(weights) < -1e-10:
            continue
        metric = np.einsum('i,ijk->jk', weights, metrics[connectivity[i]])
        if np.linalg.eigvalsh(metric).min() > 0:
            return ratio(points, metric)
    return None


def census(case, mesh, donor, event_number, points, cap):
    tree = point_index(mesh.P)
    parts = {name: set(vertices(mesh, name)) for name in mesh.M}
    cap_ids = point_ids(tree, cap)
    cap_kept = int((cap_ids >= 0).sum())
    cap_present = cap_kept == 4 and bool(np.any(np.all(np.sort(mesh.E, axis=1) == np.sort(cap_ids), axis=1)))
    metric_path = case / f'flow_adap_{event_number - 1:05d}.vtu'
    field = None
    issue = None
    if donor is not None and metric_path.is_file():
        try:
            field = donor_field(donor, metric_path)
        except (ValueError, IndexError, KeyError) as error:
            issue = str(error)
    counts = {}
    for name in mesh.M:
        flat = flat_cells(mesh, name)
        values = [q for cell in flat if field is not None and (q := sampled_ratio(field, mesh.P[cell])) is not None]
        missing = len(flat) - len(values)
        counts[name] = {'cells': len(flat), 'q_below_0_2': sum(q < .2 for q in values) if not missing else None,
                        'minimum_q': min(values) if values and not missing else None, 'unmeasured': missing}
    return {'marker_faces': {name: len(faces) for name, faces in mesh.M.items()},
            'input_farfield_points_retained': sum(idx in parts.get('FARFIELD', set()) for idx in point_ids(tree, points)),
            'circle_points': len(parts.get('FARFIELD', set()) & parts.get('SYMMETRY', set())),
            'flat': counts, 'cap': {'vertices_kept': cap_kept, 'present': cap_present,
                                   'q': sampled_ratio(field, cap) if field is not None and cap_present else None},
            'q_metric': str(metric_path) if field is not None else None, 'q_precision': field[-1] if field is not None else None,
            'q_issue': issue}


def describe(case, events, cap, *, list_mode=True):
    input_mesh = CAP.read_su2(case / 'm6.su2')
    original_farfield = input_mesh.P[vertices(input_mesh, 'FARFIELD')]
    assert all(idx in set(vertices(input_mesh, 'FARFIELD')) for idx in point_ids(point_index(input_mesh.P), cap))
    runtime_rows = runtime(case)
    indices = {idx: name for idx, name in enumerate(input_mesh.M)}
    if list_mode:
        observed = wiring(runtime_rows, indices)
    else:
        observed = None
        for rows in runtime_rows.values():
            for row in rows['final_cert']:
                row['marker'] = indices.get(row.get('index'))
    phases = phase_times(case)
    live_mesh = input_mesh
    live_cycle = 0
    for event in events:
        number = event['event']
        certification = event['gates']['1']['status']
        outcome = ('rejected' if event.get('native_rejection') else
                   'accepted' if certification == 'passed' and event['gates']['2']['status'] != 'OPEN' else
                   'not evaluated')
        ranks = phases.get(number, {})
        event.update(outcome=outcome, marker_tolerances=runtime_rows[number],
                     phase_seconds={'by_rank': ranks, 'max_rank': {
                         phase: max(row[phase] for row in ranks.values()) if len(ranks) == 6 else None for phase in PHASES}})
        if outcome == 'not evaluated':
            continue
        donor = live_mesh
        if outcome == 'accepted':
            live_mesh = CAP.read_su2(case / f'adapted_mesh_adap_{number:05d}.su2')
            live_cycle = number
        event['surface_mesh'] = census(case, live_mesh, donor, number, original_farfield, cap)
        event['surface_mesh']['cycle'] = live_cycle
        for row in event.get('rejected_cells', []):
            points = coordinates(row)
            membership = marker_matches(point_index(donor.P), {name: set(vertices(donor, name)) for name in donor.M}, points)
            if len([markers for markers in membership if markers]) == row['surface_points']:
                row['markers'] = ', '.join(sorted(set().union(*membership))) or 'none'
            row['matched_input_vertex_markers'] = membership
            if set(point_ids(point_index(cap), points)) == set(range(4)):
                event['surface_mesh']['rejected_cap'] = {'present': True, 'q': row['q'], 'J': row['J']}
    return observed
