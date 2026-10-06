"""Audit final native output snapshots with the existing independent mesh/target checks."""
import argparse
from collections import Counter
import hashlib
import json
from pathlib import Path
import shutil
import h5py
from audit_native_bl import audit as audit_bl, mesh
from audit_native_cgns import audit as audit_cgns

parser = argparse.ArgumentParser()
parser.add_argument('directory', type=Path)
parser.add_argument('--label', required=True)
args = parser.parse_args()
directory = args.directory.resolve(strict=True)
runtime = json.loads((directory/'evidence.json').read_text())
if runtime.get('current_run') or not runtime.get('archived_binary') or \
        [r['ranks'] for r in runtime['runs']] != [1, 2, 4] or not all(r['verified'] for r in runtime['runs']):
    raise RuntimeError('Finish successful MPI1/2/4 runtime before independent auditing.')
destination = directory/args.label
destination.mkdir(exist_ok=False)
(destination/'sources').mkdir()
scripts = ('run_native_final_output_audits.py', 'audit_native_bl.py', 'audit_native_cgns.py')
for name in scripts:
    shutil.copy2(Path(__file__).with_name(name), destination/'sources'/name)
record = {'runtime_evidence': str(directory/'evidence.json'), 'runs': [],
          'source_sha256': {s: hashlib.sha256((destination/'sources'/s).read_bytes()).hexdigest() for s in scripts}}

def coordinates_signature(points, cells, markers):
    return (Counter(tuple(sorted(points[i] for i in cell)) for cell in cells),
            {tag: Counter(tuple(sorted(points[i] for i in edge)) for edge in group)
             for tag, group in markers.items()})

def compare_produced_output(path):
    prefix = path.name.split('_mesh_adap_')[0]
    candidate = path.with_name(prefix+'_cycle_2_adapted.su2')
    points, cells, markers = mesh(candidate)
    if path.suffix == '.su2':
        cgpoints, cgcells, cgmarkers = mesh(path)
    else:
        with h5py.File(path) as f:
            zone = f['Base/Zone']
            coords = [zone[f'GridCoordinates/Coordinate{a}/ data'][()].ravel().tolist() for a in ('X', 'Y')]
            cgpoints = list(zip(*coords))
            cgcells, cgmarkers = [], {}
            for name, obj in zone.items():
                if isinstance(obj, h5py.Group) and obj.attrs.get('label') == b'Elements_t':
                    code = int(obj[' data'][()].ravel()[0])
                    width = {5: 3, 3: 2}[code]
                    rows = (obj['ElementConnectivity/ data'][()].ravel()-1).reshape(-1, width).tolist()
                    if code == 5:
                        cgcells.extend(rows)
                    else:
                        cgmarkers[name] = rows
    assert Counter(points) == Counter(cgpoints), 'Configured output/snapshot coordinates differ'
    assert coordinates_signature(points, cells, markers) == coordinates_signature(cgpoints, cgcells, cgmarkers), \
        'Configured output/snapshot cells or physical marker connectivity differ'
    return str(candidate)

all_ok = True
for ranks in (1, 2, 4):
    saved = directory/f'audit_np{ranks}'
    cgns_paths = sorted(saved.glob('*.cgns'))
    su2_outputs = sorted(saved.glob('native_produced_bl*_mesh_adap_00003.su2'))
    targets = sorted(saved.glob('native_produced_bl*_cycle_*_adapted.su2'))
    assert len(cgns_paths) == 7 and len(su2_outputs) == 3 and len(targets) == 18, \
        'Missing final output audit fixtures'
    for path in cgns_paths + su2_outputs + targets:
        passed = False
        try:
            if path.suffix == '.cgns':
                result = audit_cgns(path)
                if path.name.startswith('native_produced_bl'):
                    result['exact_su2_snapshot_match'] = compare_produced_output(path)
                passed = True  # Existing CGNS audit asserts every geometry/topology/marker invariant.
            elif '_mesh_adap_' in path.name:
                result = {'file': str(path), 'exact_su2_snapshot_match': compare_produced_output(path)}
                passed = True
            else:
                prefix = str(path)[:-len('_adapted.su2')]
                result = audit_bl(prefix+'_donor.su2', prefix+'_metric.csv', path, .004)
                passed = all(result[k] for k in ('all_exact_positive', 'manifold_oriented_edges',
                                                 'physical_equals_exposed')) and result['disk_euler'] == 1 and \
                         result['min_quality'] >= .18 and result['max_simpson_length'] <= 1.8 and \
                         result['max_relative_height_error'] <= 1e-8 and not result['bad_cells']
        except Exception as error:
            result = {'file': str(path), 'error': str(error)}
        all_ok &= passed
        output = destination/f'np{ranks}_{path.name}.json'
        output.write_text(json.dumps(result, indent=2)+'\n')
        record['runs'].append({'ranks': ranks, 'file': str(path), 'verified': passed, 'output': str(output)})
        (destination/'evidence.json').write_text(json.dumps(record, indent=2)+'\n')
        print(f'np{ranks} {path.name}: {"PASS" if passed else "FAIL"}', flush=True)
record.update(terminal=True, verified=all_ok)
(destination/'evidence.json').write_text(json.dumps(record, indent=2)+'\n')
raise SystemExit(0 if all_ok else 1)
