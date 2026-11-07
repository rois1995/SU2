"""Reuse qualified capture serialization for four actual final-cell residual contexts."""
from pathlib import Path
import csv, importlib.util, json, math, shutil, struct, time

ROOT = Path(__file__).resolve().parent
RUN = ROOT / 'm6_runs/euler_build227_witness_real_cfd50k'
OUT = RUN / 'current_witnesses_v1'
spec = importlib.util.spec_from_file_location('boundary_preparation', ROOT / 'prepare_boundary202.py')
old = importlib.util.module_from_spec(spec); spec.loader.exec_module(old)
reader = old.reader


def main():
    started = time.monotonic(); assert not OUT.exists()
    assert json.loads((RUN / 'terminal_handles.json').read_text())['all_owned_terminal']
    selection_path = RUN / 'shape_inspection/residual_location.json'
    location = json.loads(selection_path.read_text())
    chosen = [c for c in location['candidates'] if c['id'] in (220904843, 219735950, 227574818, 225504384)]
    assert len(chosen) == 4 and all(c['context_within128'] for c in chosen)
    selected = {cell for case in chosen for cell in case['support_cells']}
    ready = json.loads((RUN / 'capture_ready.json').read_text())
    before = {name: old.sha256(Path(name)) for name in ready['files']}
    assert all(before[name] == item['sha256'] and Path(name).stat().st_size == item['bytes']
               for name, item in ready['files'].items())
    shape = ROOT / 'cases/captured_shape_census_20261107T010656_2223729/captured_shape_census.tsv'
    for p in (shape, selection_path, RUN / 'capture_ready.json', RUN / 'm6.su2.native_ref', RUN / 'run.cfg',
              Path(__file__), ROOT / 'prepare_boundary202.py', ROOT / 'inspect_native_failure3d.py'):
        before[str(p.resolve())] = old.sha256(p)
    metrics = {}
    with shape.open() as stream:
        for row in csv.DictReader(stream, delimiter='\t'):
            if int(row['id']) in selected:
                metrics[int(row['id'])] = (float(row['q']), float(row['J']))
    manifests, missing = reader._load_manifests([str(RUN / '*_inspection_rank*_manifest.json')])
    assert not missing and len(manifests) == 6
    points, physical, records, expected = {}, {}, {}, {}; total = 0
    for manifest in manifests:
        files, caps = manifest['_resolved_files'], manifest['_caps']
        paired = reader._jsonl(files['metrics'], caps['metrics'], 'metrics')
        with files['owned'].open('rb') as raw:
            for index, cell in enumerate(reader._read_owned(files['owned'], manifest['written_rows'], caps['owned'])):
                m = reader._validate_metric(next(paired), cell, files['metrics']); total += 1
                for node, binding in zip(cell['nodes'], m['bindings']):
                    if binding is None:
                        continue
                    assert binding['id'] == node['id']
                    value = (tuple(node['xyz']), (binding['feature'], binding['version'], binding['dimension'], binding['u'], binding['v']))
                    assert points.setdefault(node['id'], value) == value
                for slot, face in enumerate(cell['physical']):
                    if not face['present']:
                        continue
                    ordered = tuple(cell['nodes'][k]['id'] for k in old.FACES[slot]); key = tuple(sorted(ordered))
                    assert key not in physical; physical[key] = (ordered, face['facet'])
                if cell['id'] not in selected:
                    continue
                assert old.exact_positive(cell['nodes']) and cell['id'] not in records
                raw.seek(8 + 416 * index); records[cell['id']] = raw.read(416)
                values = [cell['id'], *metrics[cell['id']]]
                for first, second, length in m['edges']:
                    assert length is None; values.extend((first, second, math.nan))
                expected[cell['id']] = old.METRIC.pack(*values)
        assert next(paired, None) is None
    assert total == ready['owned_cells'] and set(records) == selected == set(expected)
    assert {node for face in physical for node in face} == set(points)
    OUT.mkdir()
    old.encode(OUT / 'witness_boundary_points.bin', [old.POINT.pack(node, *xyz, *binding) for node, (xyz, binding) in sorted(points.items())], old.POINT)
    old.encode(OUT / 'witness_boundary_surface.bin', [old.SURFACE.pack(*ordered, facet) for ordered, facet in physical.values()], old.SURFACE)
    lines = []
    for i, case in enumerate(chosen):
        label = 'residual_' + str(i); support = sorted(case['support_cells'])
        old.encode(OUT / (label + '_owned.bin'), [records[cell] for cell in support], struct.Struct('416s'))
        old.encode(OUT / (label + '_expected_metrics.bin'), [expected[cell] for cell in support], old.METRIC)
        lines.append(' '.join(map(str, [label, case['id'], len(support),
            *[value for pair in zip(case['vertices'], case['star_sizes']) for value in pair]])))
    (OUT / 'current_residual_cases.tsv').write_text('\n'.join(lines) + '\n')
    shutil.copyfile(RUN / 'run.cfg', OUT / 'run.cfg')
    (OUT / 'm6.su2.native_ref').symlink_to((RUN / 'm6.su2.native_ref').resolve())
    after = {name: old.sha256(Path(name)) for name in before}; assert before == after
    outputs = {p.name: dict(bytes=p.stat().st_size, sha256=old.sha256(p)) for p in OUT.iterdir()}
    result = dict(schema='native_current_witness_preparation_v1', source_build='build227', source_event=1,
        paired_rows=total, cases=chosen, boundary_points=len(points), physical_faces=len(physical),
        input_hashes_before=before, input_hashes_after=after, outputs=outputs, seconds=time.monotonic()-started,
        scope='Complete current boundary and four existing-star contexts; centroid shape oracle, missing edge caches explicit, fresh association reconstruction, not historical publication replay')
    (OUT / 'witness_preparation.json').write_text(json.dumps(result, indent=2) + '\n')
    print(json.dumps({k: result[k] for k in ('paired_rows', 'boundary_points', 'physical_faces', 'seconds')}))


if __name__ == '__main__':
    main()
