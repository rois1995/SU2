"""Select actual captured physical faces; C++ alone discovers their query supports."""
from pathlib import Path
import hashlib, importlib.util, json, math, re, shutil, struct, sys, time

HERE = Path(__file__).resolve().parent
RUN = HERE / 'm6_runs/euler_build225_entity_shell_real_cfd50k'
OUT = RUN / 'geometry_queries_v1'
FACES = ((1,2,3),(0,3,2),(0,1,3),(0,2,1))
POINT = struct.Struct('<QdddQQQdd')
FACE = struct.Struct('<QQQQ')


def sha(path):
    h = hashlib.sha256()
    with Path(path).open('rb') as f:
        for block in iter(lambda: f.read(1 << 20), b''):
            h.update(block)
    return h.hexdigest()


def receipt(path):
    return {'bytes': Path(path).stat().st_size, 'sha256': sha(path)}


def area_normal(points):
    a,b,c = points
    u = [b[i]-a[i] for i in range(3)]
    v = [c[i]-a[i] for i in range(3)]
    n = [u[1]*v[2]-u[2]*v[1], u[2]*v[0]-u[0]*v[2], u[0]*v[1]-u[1]*v[0]]
    return math.hypot(*n), n


def curved(facet):
    points = [n[1:] for n in facet['nodes']]
    length,n = area_normal(points)
    if not length or not facet.get('controls'):
        return False
    scale = max(1., *(abs(x) for p in facet['controls'] for x in p))
    return any(abs(sum((p[i]-points[0][i])*n[i] for i in range(3))) > 128*sys.float_info.epsilon*scale*length
               for p in facet['controls'][3:])


def main():
    started = time.monotonic()
    assert not OUT.exists(), 'Never overwrite a preparation or resume from a partial output blindly'
    assert json.loads((RUN/'terminal_handles.json').read_text())['all_owned_terminal']
    reader_path = HERE/'inspect_native_failure3d.py'
    assert sha(reader_path) == '028b7184ff25a0b5de1333c1da287c433251f4b52cccb34b05607e8a2f55c76d'
    spec = importlib.util.spec_from_file_location('geometry225_reader', reader_path)
    reader = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(reader)
    cap = json.loads((RUN/'capture_ready.json').read_text())
    assert len(cap['files']) == 30 and not cap['changed']
    files = {str((HERE.parents[3]/p).resolve()) if not Path(p).is_absolute() else p: value
             for p,value in cap['files'].items()}
    extras = [RUN/'capture_ready.json', RUN/'terminal_handles.json', RUN/'run.cfg',
              RUN/'m6.su2.native_ref', reader_path, Path(__file__).resolve()]
    def hashes():
        result = {p: receipt(p) for p in files}
        assert result == files, 'Raw capture differs from its sealed receipt'
        result.update({str(p): receipt(p) for p in extras})
        return result
    before = hashes()
    cfg = (RUN/'run.cfg').read_text()
    def value(key):
        m = re.search(r'(?m)^\s*'+re.escape(key)+r'\s*=\s*(.*?)\s*$', cfg)
        assert m, key
        return m.group(1)
    assert float(value('ADAP_ANGLE')) == 60 and int(value('ADAP_COMPLEXITY')) == 50000
    assert value('MARKER_FAR') == '( FARFIELD )' and value('MARKER_SYM') == '( SYMMETRY )'
    assert value('MARKER_EULER') == '( WING3D )' and int(value('ADAP_NATIVE_RANKS')) == 6
    with (RUN/'m6.su2.native_ref').open() as f:
        assert f.readline().strip() == 'SU2_NATIVE_REFERENCE_3D 7'  # Archive format, not geometry definition version.
    manifests,missing = reader._load_manifests([str(RUN)])
    assert len(manifests) == 6 and not missing
    assert sum(m['written_rows'] for m in manifests) == 987865
    facets = {}
    for m in manifests:
        assert all(m[k] for k in ['complete_shard','owned_complete','metrics_complete','reference_complete','rejections_complete'])
        for row in reader._jsonl(m['_resolved_files']['reference'],m['_caps']['reference'],'reference'):
            key,clean = reader._validate_reference(row)
            if key[0] == 'facet':
                assert facets.setdefault(clean['id'],clean) == clean
    wing = {k:v for k,v in facets.items() if v['marker'] == 2}
    assert wing
    y0 = min(n[2] for f in wing.values() for n in f['nodes'])
    y1 = max(n[2] for f in wing.values() for n in f['nodes'])
    assert y1 > y0
    bins = {'root': (0.,.25), 'midspan': (.4,.6), 'tip': (.75,1.)}
    buckets = {(region,role): [] for region in bins for role in ['geometry_bad','clean_curved_control']}
    scanned = physical_cells = 0
    for m in manifests:
        path = m['_resolved_files']['metrics']
        with path.open() as metrics:
            for index,cell in enumerate(reader._read_owned(m['_resolved_files']['owned'],m['written_rows'],m['_caps']['owned'])):
                line = metrics.readline()
                assert line.endswith('\n')
                scanned += 1
                physical = [(slot,f['facet']) for slot,f in enumerate(cell['physical']) if f['present'] and f['facet'] in wing]
                if not physical:
                    continue
                physical_cells += 1
                metric = reader._validate_metric(json.loads(line),cell,path)
                if metric['near_feature'] or not metric['feature_known']:
                    continue
                for slot,facet_id in physical:
                    local = FACES[slot]
                    nodes = [cell['nodes'][i] for i in local]
                    bindings = [metric['bindings'][i] for i in local]
                    if any(b is None or b.get('mobility_dimension') != 2 for b in bindings):
                        continue
                    assert all(b['id'] == n['id'] for b,n in zip(bindings,nodes))
                    versions = {b['version'] for b in bindings}
                    assert len(versions) == 1
                    normal_length,_ = area_normal([n['xyz'] for n in nodes])
                    assert normal_length > 0 and math.isfinite(normal_length)
                    center = [sum(n['xyz'][i] for n in nodes)/3 for i in range(3)]
                    position = (center[1]-y0)/(y1-y0)
                    regions = [region for region,(lo,hi) in bins.items() if lo <= position <= hi]
                    bad = metric['geometry_excess_l2'] > 0
                    clean = metric['geometry_excess_l2'] == 0 and metric['q'] >= .2 and metric['J'] >= .05 and curved(wing[facet_id])
                    if not (bad or clean):
                        continue
                    role = 'geometry_bad' if bad else 'clean_curved_control'
                    rank_key = (-metric['geometry_excess_l2'],min(metric['q']/.2,metric['J']/.05),cell['id'],slot) if bad else (-normal_length,cell['id'],slot)
                    row = {'cell': cell['id'], 'rank': m['rank'], 'rank_index': index, 'physical_slot': slot,
                           'facet': facet_id, 'entity': {k: wing[facet_id][k] for k in ['body','sheet','marker']},
                           'nodes': nodes, 'bindings': bindings, 'definition_version': next(iter(versions)),
                           'q': metric['q'], 'J': metric['J'], 'geometry_excess_l2': metric['geometry_excess_l2'],
                           'near_feature': metric['near_feature'], 'face_area': normal_length/2,
                           'center': center, 'curved_control_proved': curved(wing[facet_id]), 'role': role}
                    for region in regions:
                        bucket = buckets[region,role]
                        bucket.append((rank_key,row))
                        bucket.sort(key=lambda x:x[0])
                        del bucket[4:]  # Retain alternatives to avoid choosing two faces of the same cell.
            assert not metrics.read(), 'Metric and owned streams differ in length'
    assert scanned == 987865
    selected = []
    used = set()
    omissions = []
    for (region,role),bucket in buckets.items():
        count = 0
        for _,row in bucket:
            if row['cell'] in used:
                continue
            used.add(row['cell'])
            selected.append(dict(row,region=region))
            count += 1
            if count == 2:
                break
        if count < 2:
            omissions.append({'region': region, 'role': role, 'requested': 2, 'available': count})
    assert 0 < len(selected) <= 12
    versions = {r['definition_version'] for r in selected}
    assert len(versions) == 1
    version = next(iter(versions))
    payload = bytearray(struct.pack('<QQQQdQ',0x4d36515545525931,2,version,len(selected),1e-4,401030928))
    for row in selected:
        payload.extend(FACE.pack(*(n['id'] for n in row['nodes']),row['facet']))
        for n,b in zip(row['nodes'],row['bindings']):
            payload.extend(POINT.pack(n['id'],*n['xyz'],b['feature'],b['version'],b['dimension'],b['u'],b['v']))
        payload.extend(struct.pack('<Q',0))  # Exact support discovery belongs to the existing C++ path.
    assert len(payload) == 48+256*len(selected)
    after = hashes()
    assert before == after
    OUT.mkdir()
    (OUT/'current_geometry_queries.bin').write_bytes(payload)
    shutil.copyfile(RUN/'run.cfg',OUT/'run.cfg')
    (OUT/'m6.su2.native_ref').symlink_to((RUN/'m6.su2.native_ref').resolve())
    outputs = {name: receipt(OUT/name) for name in ['current_geometry_queries.bin','run.cfg','m6.su2.native_ref']}
    result = {'schema': 'native_current_geometry_query_preparation_v1', 'source_event': 1,
              'archive_format_version': 7, 'geometry_definition_version': version,
              'input_hashes_before': before, 'input_hashes_after': after, 'outputs': outputs,
              'paired_rows': scanned, 'wing_physical_cells': physical_cells, 'queries': selected,
              'omissions': omissions, 'seconds': time.monotonic()-started,
              'scope': 'Actual captured current physical faces, not historical trial replay or publication. No support discovery in Python; no synthesized nearby positions. Genuine-feature cases remain covered by existing affected validation.'}
    (OUT/'geometry_query_preparation.json').write_text(json.dumps(result,indent=2)+'\n')
    print(json.dumps({'output': str(OUT), 'queries':len(selected), 'geometry_definition_version':version, 'omissions':omissions, 'seconds': result['seconds']}))


if __name__ == '__main__':
    main()
