"""Export old and shell-exposed faces for the existing bounded geometry-query diagnostic."""
from pathlib import Path
import importlib.util, json, shutil, struct

ROOT = Path(__file__).resolve().parent
RUN = ROOT / 'm6_runs/euler_build227_witness_real_cfd50k'
SOURCE = RUN / 'current_witnesses_v1'
OUT = RUN / 'shell_geometry_queries_v1'
spec = importlib.util.spec_from_file_location('boundary_preparation', ROOT / 'prepare_boundary202.py')
old = importlib.util.module_from_spec(spec); spec.loader.exec_module(old)


def main():
    assert not OUT.exists()
    receipt = json.loads((SOURCE / 'witness_preparation.json').read_text())
    before = {str((SOURCE / name).resolve()): old.sha256(SOURCE / name) for name in receipt['outputs']}
    assert all(before[str((SOURCE / name).resolve())] == item['sha256']
               and (SOURCE / name).stat().st_size == item['bytes'] for name, item in receipt['outputs'].items())
    for p in (SOURCE / 'witness_preparation.json', Path(__file__), ROOT / 'prepare_boundary202.py', ROOT / 'inspect_native_failure3d.py'):
        before[str(p.resolve())] = old.sha256(p)
    data = (SOURCE / 'witness_boundary_points.bin').read_bytes()
    count, = old.U64.unpack_from(data); assert len(data) == 8 + count * old.POINT.size
    points = {row[0]: row for row in (old.POINT.unpack_from(data, 8 + i * old.POINT.size) for i in range(count))}
    assert len(points) == count
    queries = []
    for label, core in [('residual_2', 220904843), ('residual_3', 219735950)]:
        path = SOURCE / (label + '_owned.bin'); count, = old.U64.unpack_from(path.read_bytes())
        rows = list(old.reader._read_owned(path, count, 128 << 20))
        cell, = [row for row in rows if row['id'] == core]
        present = [slot for slot, face in enumerate(cell['physical']) if face['present']]
        assert len(present) == 2
        retired = [(tuple(cell['nodes'][k]['id'] for k in old.FACES[slot]), cell['physical'][slot]['facet']) for slot in present]
        # ShellBoundaryCell chooses the first retired face in the sorted physical-face map.
        anchor = min(retired, key=lambda row: tuple(sorted(row[0])))[1]
        for slot in range(4):
            face = tuple(cell['nodes'][k]['id'] for k in old.FACES[slot])
            if slot in present:
                kind, facet = 'old', cell['physical'][slot]['facet']
            else:
                kind, facet, face = 'shell_exposed', anchor, tuple(reversed(face))
            assert all(node in points for node in face)
            queries.append(dict(core=core, kind=kind, facet=facet, face=face))
    versions = {points[node][5] for q in queries for node in q['face']}; assert versions == {18}
    payload = bytearray(struct.pack('<QQQQdQ', 0x4d36515545525931, 2, 18, len(queries), 1e-4, 401030928))
    for q in queries:
        payload.extend(old.SURFACE.pack(*q['face'], q['facet']))
        for node in q['face']:
            payload.extend(old.POINT.pack(*points[node]))
        payload.extend(old.U64.pack(0))
    assert len(payload) == 48 + 256 * len(queries)
    after = {name: old.sha256(Path(name)) for name in before}; assert before == after
    OUT.mkdir(); (OUT / 'current_geometry_queries.bin').write_bytes(payload)
    shutil.copyfile(SOURCE / 'run.cfg', OUT / 'run.cfg')
    (OUT / 'm6.su2.native_ref').symlink_to((RUN / 'm6.su2.native_ref').resolve())
    result = dict(schema='native_current_geometry_query_preparation_v1', source_event=1,
        input_hashes_before=before, input_hashes_after=after, queries=queries,
        outputs={p.name: dict(bytes=p.stat().st_size, sha256=old.sha256(p)) for p in OUT.iterdir()},
        scope='Exact old and shell-exposed faces from native cell deletion; unchanged captured coordinates/bindings. Native code discovers supports. Not optimized-position replay or publication proof.')
    (OUT / 'geometry_query_preparation.json').write_text(json.dumps(result, indent=2) + '\n')
    print(json.dumps(dict(queries=len(queries), output=str(OUT))))


if __name__ == '__main__':
    main()
