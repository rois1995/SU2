"""Extract the two FP0 sensor fields from the pinned native restart records."""

import json
import os
from pathlib import Path
import runpy
import struct

import numpy as np


ITEM = Path(__file__).resolve().parent
NIGHT = ITEM.parent
G5 = NIGHT / 'N4/section6_3_after_N11/round_8_5t_P/G5/full587'
CONVERTER = NIGHT.parents[1] / 'diagnostic55/prepare.py'


def sensor(source, points, output):
    raw = source.read_bytes()
    magic, fields, count, width, unused = struct.unpack_from('=5i', raw)
    assert (magic, count, width, unused) == (535532, len(points), 8, 0)
    names = [raw[20 + 33*i:20 + 33*(i+1)].split(b'\0')[0].decode() for i in range(fields)]
    offset = 20 + 33 * fields
    assert len(raw) == offset + count * fields * 8 and len(set(names)) == fields
    values = np.frombuffer(raw, dtype='=f8', offset=offset).reshape(count, fields)
    xyz = values[:, [names.index(s) for s in ('x', 'y', 'z')]]
    assert np.array_equal(xyz, points), 'Restart row/global node correspondence failed'
    six = values[:, [names.index('Metric_' + s) for s in ('XX', 'XY', 'XZ', 'YY', 'YZ', 'ZZ')]]
    full = six[:, [0, 1, 2, 1, 3, 4, 2, 4, 5]]
    assert np.isfinite(full).all() and np.all(np.linalg.eigvalsh(full.reshape(-1, 3, 3)) > 0)
    dtype = np.dtype([('id', '=u8'), ('x', '=f8', (3,)), ('m', '=f8', (9,))])
    rows = np.empty(count, dtype=dtype)
    rows['id'] = np.arange(count)
    rows['x'], rows['m'] = xyz, full
    with output.open('xb') as out:
        out.write(struct.pack('=Q', count))
        rows.tofile(out)
    actual = np.fromfile(output, dtype=dtype, offset=8)
    assert np.array_equal(actual, rows)


def prepare():
    reusable = runpy.run_path(str(CONVERTER))
    sha, mesh = reusable['sha'], reusable['mesh']
    inputs = ITEM / 'cases'
    inputs.mkdir(exist_ok=True)
    manifest = {}
    for key, name, index, dimensions in (
            ('A', 'adapted_mesh_adap_00001.su2', 1, (146676, 726442, 67170)),
            ('B', 'm6.su2', 0, (61330, None, None))):
        case = inputs / key
        case.mkdir()
        restart = f'restart_flow_adap_{index:05}.dat'
        sources = (name, name + '.native_ref', restart, 'baseline_euler_seed.dat')
        for basename in sources:
            os.link(G5 / basename, case / basename)
        points, cells, markers = mesh(case / name)
        sizes = len(points), len(cells), sum(map(len, markers.values()))
        assert all(expected is None or expected == actual
                   for expected, actual in zip(dimensions, sizes)), sizes
        output = case / 'sensor.bin'
        sensor(case / restart, points, output)
        config = (G5 / 'run.cfg').read_text()
        if key == 'A':
            for before, after in (
                    ('MESH_FILENAME= m6.su2\n', f'MESH_FILENAME= {name}\n'),
                    ('SOLUTION_FILENAME= baseline_euler_seed\n', f'SOLUTION_FILENAME= {Path(restart).stem}\n'),
                    ('ADAP_NATIVE_RANKS= 6\n', 'ADAP_NATIVE_RANKS= 1\n')):
                assert config.count(before) == 1
                config = config.replace(before, after)
        (case / 'run.cfg').write_text(config)
        manifest[key] = {'counts': dict(zip(('points', 'cells', 'surface_faces'), sizes)),
                         'markers': {marker: len(faces) for marker, faces in markers.items()},
                         'pinned_sources': {str(G5 / basename): sha(case / basename) for basename in sources},
                         'source_config_sha256': sha(G5 / 'run.cfg'),
                         'sensor': str(output), 'sensor_sha256': sha(output),
                         'config': str(case / 'run.cfg'), 'config_sha256': sha(case / 'run.cfg'),
                         'exact_coordinate_global_id_match': True, 'exact_metric_roundtrip': True}
    manifest['converter_sha256'] = sha(CONVERTER)
    (ITEM / 'case_input_manifest.json').write_text(json.dumps(manifest, indent=2) + '\n')


if __name__ == '__main__':
    prepare()
