"""Independent saved-mesh checks for eight cycles on both channel walls and both transfers."""
import argparse
import hashlib
import json
import math
from pathlib import Path
import shutil
from audit_native_bl import audit, mesh

parser = argparse.ArgumentParser()
parser.add_argument('directory', type=Path)
parser.add_argument('--label', required=True)
args = parser.parse_args()
directory = args.directory.resolve(strict=True)
runtime = json.loads((directory/'evidence.json').read_text())
if runtime.get('current_run') or not runtime.get('archived_binary'):
    raise RuntimeError('Finish the MPI run before auditing.')
if [r['ranks'] for r in runtime['runs']] != [1, 2, 4] or not all(r['verified'] for r in runtime['runs']):
    raise RuntimeError('The opposing-wall gate requires successful MPI1/2/4 controls.')
destination = directory/args.label
destination.mkdir(exist_ok=False)
scripts = ('run_native_opposing_audits.py', 'audit_native_bl.py')
(destination/'sources').mkdir()
for script in scripts:
    shutil.copy2(Path(__file__).with_name(script), destination/'sources'/script)
record = {'source_sha256': {p: hashlib.sha256((destination/'sources'/p).read_bytes()).hexdigest() for p in scripts},
          'runtime_evidence': str(directory/'evidence.json'),
          'runtime_evidence_sha256': hashlib.sha256((directory/'evidence.json').read_bytes()).hexdigest(),
          'runs': []}
heights = (.004, .004, .003, .003, .005, .005, .004, .0045)
all_ok = True
for ranks in (1, 2, 4):
    for method in ('barycentric', 'conservative'):
        for cycle, height in enumerate(heights):
            prefix = directory/f'audit_np{ranks}'/f'native_bl_opposing_{method}_cycle_{cycle}'
            candidate = str(prefix)+'_adapted.su2'
            row = audit(str(prefix)+'_donor.su2', str(prefix)+'_metric.csv', candidate, height,
                        wall_tags=('lower_a', 'lower_b', 'upper'))
            points, cells, markers = mesh(candidate)
            physical_points = {tag: {points[i] for edge in group for i in edge} for tag, group in markers.items()}
            flat = set(markers) == {'lower_a', 'lower_b', 'upper', 'left', 'right'} and all(
                all(p[1] == 0 for p in physical_points[tag]) for tag in ('lower_a', 'lower_b')) and all(
                p[1] == .02 for p in physical_points['upper']) and all(p[0] == 0 for p in physical_points['left']) and all(
                p[0] == .04 for p in physical_points['right'])
            junctions = physical_points['lower_a'] & physical_points['lower_b'] == {(.02, 0.)}
            corners = {(0., 0.), (.04, 0.), (0., .02), (.04, .02)} <= set().union(*physical_points.values())
            profile = []
            edges = {tuple(sorted((ids[k], ids[(k+1)%3]))) for ids in cells for k in range(3)}
            for lower, upper in ((0, 1), (1, 2), (2, 4), (4, float('inf'))):
                values = []
                for a, b in edges:
                    x, y = points[a], points[b]
                    midpoint = (x[1]+y[1])/2
                    distance = min(midpoint, .02-midpoint)/height
                    if lower <= distance < upper and abs(y[1]-x[1]) >= abs(y[0]-x[0]):
                        values.append(abs(y[1]-x[1])/height)
                values.sort()
                profile.append({'nearest_wall_distance_over_h0': [lower, upper if math.isfinite(upper) else None],
                                'inclined_edges': len(values), 'normal_projection_over_h0_quantiles':
                                [values[int((len(values)-1)*q)] for q in (0, .25, .5, .75, 1)] if values else []})
            row.update(original_flat_geometry_retained=flat, marker_junction_retained=junctions,
                       corners_retained=corners, opposing_normal_spacing_profile=profile,
                       wall_faces_by_marker={tag: len(markers[tag]) for tag in ('lower_a', 'lower_b', 'upper')})
            passed = all(row[k] for k in ('all_exact_positive', 'manifold_oriented_edges', 'physical_equals_exposed')) and \
                     row['disk_euler'] == 1 and row['min_quality'] >= .18 and row['max_simpson_length'] <= 1.8 and \
                     row['max_relative_height_error'] <= 1e-8 and not row['bad_cells'] and flat and junctions and corners
            all_ok &= passed
            output = destination/f'np{ranks}_{method}_cycle{cycle}.json'
            output.write_text(json.dumps(row, indent=2)+'\n')
            record['runs'].append({'ranks': ranks, 'method': method, 'cycle': cycle, 'verified': passed,
                                   'output': str(output)})
            (destination/'evidence.json').write_text(json.dumps(record, indent=2)+'\n')
            print(f'np{ranks} {method} cycle{cycle}: {"PASS" if passed else "FAIL"}', flush=True)
record.update(terminal=True, verified=all_ok)
(destination/'evidence.json').write_text(json.dumps(record, indent=2)+'\n')
raise SystemExit(0 if all_ok else 1)
