"""Join native centroid diagnostics to the sealed final mesh; never infer geometry acceptance."""
from pathlib import Path
import csv, hashlib, json, mmap, struct, time
import meshio
import numpy as np

ROOT = Path(__file__).resolve().parent
RUN = ROOT / 'm6_runs/euler_build227_witness_real_cfd50k'
CASE = ROOT / 'cases/captured_shape_census_20261107T010656_2223729'
OUT = RUN / 'shape_inspection'


def sha(path):
    h = hashlib.sha256()
    with path.open('rb') as stream:
        for block in iter(lambda: stream.read(1 << 20), b''):
            h.update(block)
    return h.hexdigest()


def main():
    started = time.time()
    assert not OUT.exists()
    status = json.loads((CASE / 'status.json').read_text())
    assert status['state'] == 'terminal' and status['exit_code'] == 0 and status['pins_verified']
    ready = json.loads((RUN / 'capture_ready.json').read_text())
    inputs = [CASE / name for name in ('status.json', 'captured_shape_census.tsv', 'pins.json')]
    inputs += [RUN / 'inspection' / name for name in ('rejected_mesh.vtu', 'rejected_mesh.su2')]
    inputs += [RUN / 'capture_ready.json', Path(__file__)]
    before = {str(p.resolve()): sha(p) for p in inputs}
    for name, item in ready['files'].items():
        p = Path(name)
        assert p.stat().st_size == item['bytes'] and sha(p) == item['sha256']
        before[name] = item['sha256']
    values = {}
    with (CASE / 'captured_shape_census.tsv').open() as stream:
        rows = csv.DictReader(stream, delimiter='\t')
        for rank in range(6):
            with (RUN / f'adapted_mesh_inspection_rank{rank}_owned.bin').open('rb') as raw:
                with mmap.mmap(raw.fileno(), 0, access=mmap.ACCESS_READ) as data:
                    count, = struct.unpack_from('=Q', data)
                    assert len(data) == 8 + 416 * count
                    for i in range(count):
                        row = next(rows); offset = 8 + 416 * i
                        magic, cell = struct.unpack_from('=QQ', data, offset)
                        version, = struct.unpack_from('=Q', data, offset + 144)
                        assert magic == 0x4E334443454C4C02
                        assert (int(row['rank']), int(row['id']), int(row['version'])) == (rank, cell, version)
                        q, j = float(row['q']), float(row['J'])
                        assert np.isfinite(q) and np.isfinite(j) and cell not in values
                        values[cell] = (q, j)
        assert next(rows, None) is None
    mesh = meshio.read(RUN / 'inspection/rejected_mesh.vtu')
    assert len(mesh.cells) == 1 and mesh.cells[0].type == 'tetra'
    ids = mesh.cell_data['global_cell_id'][0]
    assert len(values) == len(ids) == ready['owned_cells'] and set(map(int, ids)) == set(values)
    qj = np.array([values[int(cell)] for cell in ids])
    bad = (qj[:, 0] < .2) | (qj[:, 1] < .05)
    mesh.cell_data['q_centroid_diagnostic'] = [qj[:, 0]]
    mesh.cell_data['J_centroid_diagnostic'] = [qj[:, 1]]
    # Existing q/J/geometry/cache fields remain unknown; recovered shape is explicitly separate.
    assert np.isnan(mesh.cell_data['q'][0]).all() and np.all(mesh.cell_data['cache_valid'][0] == 0)
    tetra = mesh.cells[0].data
    used = np.zeros(len(mesh.points), dtype=bool); used[tetra[bad].ravel()] = True
    neighbors = np.any(used[tetra], axis=1)
    assert int(neighbors.sum()) <= 262144
    OUT.mkdir()
    for name, mask in [('bad_cells.vtu', bad), ('cavities.vtu', neighbors)]:
        view = meshio.Mesh(mesh.points, [('tetra', tetra[mask])], point_data=mesh.point_data,
                           cell_data={key: [arrays[0][mask]] for key, arrays in mesh.cell_data.items()})
        meshio.write(OUT / name, view)
        read = meshio.read(OUT / name)
        assert np.array_equal(read.cell_data['global_cell_id'][0], ids[mask])
        assert np.array_equal(read.cell_data['q_centroid_diagnostic'][0], qj[mask, 0])
        assert np.array_equal(read.point_data['captured_nodal_metric'], mesh.point_data['captured_nodal_metric'])
    after = {name: sha(Path(name)) for name in before}; assert before == after
    result = dict(accepted=False, cells=len(ids), q_bad=int((qj[:, 0] < .2).sum()),
                  J_bad=int((qj[:, 1] < .05).sum()), bad_union=int(bad.sum()),
                  cavity_cells=int(neighbors.sum()), geometry_status='Unknown; no geometry recomputation',
                  inputs_before=before, inputs_after=after, seconds=time.time() - started,
                  scope='Native centroid q/J diagnostics and complete one-node-neighbor cavities, not final certification',
                  outputs={p.name: dict(bytes=p.stat().st_size, sha256=sha(p)) for p in OUT.glob('*.vtu')})
    (OUT / 'assessment.json').write_text(json.dumps(result, indent=2) + '\n')
    print(json.dumps({key: result[key] for key in ('cells', 'q_bad', 'J_bad', 'bad_union', 'cavity_cells', 'seconds')}))


if __name__ == '__main__':
    main()
