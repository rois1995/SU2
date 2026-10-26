"""Extract the archived M6 field exactly and reconcile baseline raw records. No meshing."""
import csv
import hashlib
import json
import re
import struct
from pathlib import Path

import numpy as np

ROOT = Path(__file__).resolve().parent
BASE = ROOT.parent / 'validation54/m6_euler'


def sha(path):
    h = hashlib.sha256()
    with open(path, 'rb') as stream:
        for block in iter(lambda: stream.read(1048576), b''):
            h.update(block)
    return h.hexdigest()


def mesh(path):
    cells, points, markers = [], [], {}
    with open(path) as f:
        for line in f:
            if line.startswith('NELEM='):
                for _ in range(int(line.split('=')[1])):
                    row = next(f).split()
                    assert int(row[0]) == 10
                    cells.append([int(x) for x in row[1:5]])
            elif line.startswith('NPOIN='):
                for i in range(int(line.split('=')[1].split()[0])):
                    row = next(f).split()
                    assert len(row) == 4 and int(row[3]) == i
                    points.append([float(x) for x in row[:3]])
            elif line.startswith('MARKER_TAG='):
                marker = line.split('=')[1].strip()
                header = next(f); assert header.startswith('MARKER_ELEMS=')
                faces = []
                for _ in range(int(header.split('=')[1])):
                    row = next(f).split(); assert int(row[0]) == 5
                    faces.append([int(x) for x in row[1:4]])
                assert marker not in markers
                markers[marker] = faces
    return np.array(points), np.array(cells, dtype=np.int64), markers


def main():
    for path, expected in json.loads((BASE / 'terminal_receipt_pins.json').read_text()).items():
        assert sha(path) == expected, path
    points, cells, markers = mesh(BASE / 'm6.su2')
    raw = (BASE / 'restart_flow_adap_00000.dat').read_bytes()
    magic, fields, count, width, unused = struct.unpack_from('=5i', raw)
    assert (magic, width, unused) == (535532, 8, 0)
    names = [raw[20+33*i:20+33*(i+1)].split(b'\0')[0].decode() for i in range(fields)]
    offset = 20 + fields * 33
    assert len(raw) == offset + count * fields * 8
    values = np.frombuffer(raw, dtype='=f8', offset=offset).reshape(count, fields)
    xyz = values[:, [names.index(s) for s in ('x', 'y', 'z')]]
    assert np.array_equal(xyz, points), 'Restart row/global node correspondence failed'
    six = values[:, [names.index('Metric_'+s) for s in ('XX', 'XY', 'XZ', 'YY', 'YZ', 'ZZ')]]
    full = six[:, [0, 1, 2, 1, 3, 4, 2, 4, 5]]
    assert np.isfinite(full).all() and np.all(np.linalg.eigvalsh(full.reshape(-1, 3, 3)) > 0)
    sensor = ROOT / 'sensor_original.bin'
    with sensor.open('wb') as out:
        out.write(struct.pack('=Q', count))
        for i in range(count):
            out.write(struct.pack('=Q12d', i, *xyz[i], *full[i]))
    # Independent round-trip checks both the harness layout and every original value.
    dtype = np.dtype([('id', '=u8'), ('x', '=f8', (3,)), ('m', '=f8', (9,))])
    saved = np.fromfile(sensor, dtype=dtype, offset=8)
    assert np.array_equal(saved['id'], np.arange(count))
    assert np.array_equal(saved['x'], xyz) and np.array_equal(saved['m'], full)
    records = []
    for number, line in enumerate((BASE / 'run.log').read_text().splitlines(), 1):
        if line.startswith(('NATIVE3D ', 'NATIVE3D_DETAIL ')):
            row = dict(re.findall(r'(\w+)=([^\s]+)', line))
            row.update(raw_line=number, record=line.split()[0])
            records.append(row)
    refine = [r for r in records if r.get('stage') == 'refine']
    expected = json.loads((BASE / 'refine_analysis.json').read_text())
    assert len(refine) == 6
    for a, b in zip(sorted(refine, key=lambda r: int(r['rank'])), expected['ranks']):
        for k, v in b.items():
            assert a[k] == v, (k, a[k], v)
    (ROOT / 'baseline_records.json').write_text(json.dumps(records, indent=2)+'\n')
    columns = ['rank', 'stage', 'record', 'raw_line', 'total', 'process_cpu', 'cpu',
               'private_reconstruction_cpu', 'private_metric_cpu', 'private_measurement_cpu',
               'nested_tracked_collectives', 'tracked_calls', 'donor', 'reconstruction',
               'metric', 'other', 'graph', 'parmetis', 'migration', 'donor_migration',
               'cells_before', 'cells_after', 'work_before', 'work_after', 'moved',
               'attempted', 'accepted', 'metric_shape_rejected', 'cached_invalid_cells', 'final_owned_cells',
               'optimization_trials', 'optimization_validated', 'process_peak_rss']
    with (ROOT / 'm6_rank_phase_timings.csv').open('w') as f:
        writer = csv.DictWriter(f, columns, extrasaction='ignore'); writer.writeheader(); writer.writerows(records)
    metadata = dict(mesh_sha256=sha(BASE/'m6.su2'), reference_sha256=sha(BASE/'m6.su2.native_ref'),
                    restart_sha256=sha(BASE/'restart_flow_adap_00000.dat'), sensor_sha256=sha(sensor),
                    points=count, cells=len(cells), markers={k: len(v) for k,v in markers.items()},
                    exact_coordinate_global_id_match=True, exact_metric_roundtrip=True,
                    field_names=names, original_source_commit='e9263060cf',
                    metric_semantics='Archived sensor supplied directly; no CFD step or renormalization in extraction')
    (ROOT/'input_manifest.json').write_text(json.dumps(metadata, indent=2)+'\n')
    print(json.dumps(metadata, indent=2))


if __name__ == '__main__':
    main()
