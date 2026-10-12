"""Independently audit a retained private coarsening rejection; never install it in CFD."""
from collections import Counter
from pathlib import Path
import copy
import hashlib
import json
import os
import shutil
import subprocess
import sys

HERE = Path(__file__).resolve().parent
BASE = HERE.parent
ROOT = BASE.parents[1]
TRACE = BASE / 'full_star_facet_trace_n8_v1'
sys.path[:0] = [str(BASE), str(ROOT / 'TestCases/adaptation/capability')]
from capcheck import read_su2
from audit_coupled import frame
from audit_geometry_bridge import signature, su2_signature
from audit_envelope import WEDGE_MARKERS
from audit_lifecycle import parse


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    certificate = json.loads((TRACE / 'analysis.json').read_text())
    assert certificate['status'] == 'PASS' and certificate['adaptation_status'] == 'FAIL'
    for name, expected in certificate['artifact_sha256'].items():
        assert sha(TRACE / name) == expected, name
    source_path = TRACE / 'envelope_s1_initial.json'
    source = json.loads(source_path.read_text())
    raw = TRACE / 'coarsen_rejected_s1.su2'
    mesh = read_su2(raw)
    assert mesh.dim == 3 and len(mesh.E) == 517 and len(mesh.P) == 153
    assert {tuple(p) for p in mesh.P} <= {tuple(n[1:]) for n in source['nodes']}
    facets = {f['id']: f for f in source['facets']}
    roots = {}
    for group in source['facet_groups']:
        markers = {facets[i]['marker'] for i in group}
        assert len(markers) == 1
        marker = next(iter(markers))
        assert marker not in roots, 'This fixture requires one retained planar patch per physical marker'
        roots[marker] = min(group)
    assert set(roots) == set(range(len(source['marker_names'])))
    data = copy.deepcopy(source)
    data['nodes'] = [[i, *map(float, p)] for i, p in enumerate(mesh.P)]
    data['cells'] = [dict(id=i, nodes=list(map(int, c))) for i, c in enumerate(mesh.E)]
    data['surface'] = []
    for name, triangles in mesh.M.items():
        assert name.startswith('reference_marker_')
        marker = int(name[len('reference_marker_'):])
        assert name == 'reference_marker_' + str(marker) and marker in roots
        data['surface'].extend(dict(facet=roots[marker], nodes=list(map(int, triangle))) for triangle in triangles)
    assert set(mesh.M) == {'reference_marker_' + str(i) for i in roots}
    actual_cells, actual_boundary = su2_signature(raw, set(mesh.M))
    renamed = Counter({(source['marker_names'][int(name[len('reference_marker_'):])], points): count
                       for (name, points), count in actual_boundary.items()})
    assert (actual_cells, renamed) == signature(data, WEDGE_MARKERS)
    # Relabelling local node/cell IDs must preserve every geometric simplex and its physical marker.
    bad = copy.deepcopy(data)
    bad['surface'][0]['facet'] = roots[(facets[bad['surface'][0]['facet']]['marker'] + 1) % len(roots)]
    assert signature(bad, WEDGE_MARKERS) != (actual_cells, renamed)
    copied = HERE / 'coarsen512_diagnostic_original_rejected.su2'
    shutil.copyfile(raw, copied)
    assert sha(copied) == sha(raw)
    audit_input = HERE / 'coarsen512_rejected.json'
    audit_input.write_text(json.dumps(data, separators=(',', ':')) + '\n')
    before_hashes = {str(p): sha(p) for p in [raw, source_path, TRACE/'analysis.json', TRACE/'validation.json']}
    metadata = [json.loads(line) for line in (TRACE/'private_mesh_status.jsonl').read_text().splitlines()]
    assert metadata[-1]['file'] == raw.name and metadata[-1]['cells'] == 517
    assert metadata[-1]['nodes'] == 153 and metadata[-1]['valid_cached_exit'] is False
    details = [parse(line) for line in (TRACE/'case.stdout').read_text().splitlines()
               if line.startswith('NATIVE3D_DETAIL ') and 'stage=coarsen ' in line]
    last = details[-1]
    assert last['attempted'] == last['rounds'] == 512 and last['accepted'] == 437 and last['rejected'] == 75
    assert last['cached_invalid_cells'] == 0 and last['remaining_edges'] == 266
    assert last['final_owned_cells'] == last['cached_measured_cells'] == 517
    print('Prepared exact 517-cell/153-node private input; source and original markers preserved', flush=True)
    observed, audited = frame(audit_input, True)
    assert observed['volume'] == '1/2' and observed['maximum_length'] <= 1.8
    assert observed['minimum_mean_ratio'] >= .20 and observed['minimum_scaled_jacobian'] >= .05
    assert audited['original'] == source['original'] and audited['facets'] != source['facets']
    # frame adds only independent derived reference data in memory; its file preserves the frozen source.
    assert json.loads(audit_input.read_text())['facets'] == source['facets']
    assert all(sha(Path(name)) == digest for name, digest in before_hashes.items())
    tools = [BASE/name for name in ('audit_coupled.py','audit_cavities.py','audit_geometry_bridge.py',
                                   'audit_envelope.py','audit_lifecycle.py')]
    tools += [ROOT/'TestCases/adaptation/capability/capcheck.py', Path(__file__).resolve()]
    result = dict(status='PASS',adaptation_status='FAIL',accepted_by_CFD=False,
                  frame=observed, original_marker_names=source['marker_names'],
                  exact_input_signature_preserved=True, marker_corruption_rejected=True,
                  cache_at_exit={k:last[k] for k in ('rounds','attempted','accepted','rejected',
                      'cached_invalid_cells','remaining_edges','final_owned_cells','cached_measured_cells')},
                  historical_validation_sha256=sha(TRACE/'validation.json'),
                  historical_analysis_sha256=sha(TRACE/'analysis.json'),
                  input_sha256={str(p.relative_to(ROOT)):sha(p) for p in [raw,source_path]},
                  tools_sha256={str(p.relative_to(ROOT)):sha(p) for p in tools},
                  artifacts_sha256={p.name:sha(p) for p in (copied,audit_input,audit_input.with_suffix('.su2'))},
                  scope='Fresh exact geometry/incidence/embedding/no-overlap/domain/facet/marker and original-P1 '
                  'quality/sliver/full-edge audit of the retained bb87 serial 512-transaction private rejection. '
                  'Only the local IDs and marker labels are reconstructed; original frozen tensors and reference '
                  'are retained. No accepted CFD replacement, solution/BDF transfer, CGNS check, new MPI '
                  'execution, budget/completion-policy change, performance or current-source trajectory claim.')
    (HERE/'independent_audit.json').write_text(json.dumps(result,indent=2)+'\n')
    print('PASS:',json.dumps(observed,sort_keys=True),flush=True)


if __name__ == '__main__':
    main()
