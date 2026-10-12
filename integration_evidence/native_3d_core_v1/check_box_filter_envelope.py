"""Verify current aspect2/thin correctness receipts and retained geometry controls."""
from pathlib import Path
import hashlib
import json
from audit_lifecycle import parse

BASE = Path(__file__).resolve().parent
sha = lambda p: hashlib.sha256(p.read_bytes()).hexdigest()


def pinned(folder):
    receipt = json.loads((folder / 'validation.json').read_text())
    assert receipt['status'] == 'PASS', folder.name
    for key in ('source_drift', 'object_drift', 'archive_drift', 'tool_drift'):
        assert not receipt[key], (folder.name, key)
    for name, expected in receipt['artifact_sha256'].items():
        assert sha(folder / name) == expected, (folder.name, name)
    return receipt


def main():
    matrix_path = BASE / 'BOX_FILTER_ENVELOPE_MATRIX.json'
    matrix = json.loads(matrix_path.read_text())
    assert matrix['status'] == 'PREDECLARED_NOT_EXECUTED'
    build = json.loads((BASE / matrix['qualified_build']).read_text())
    assert build['status'] == 'PASS'
    assert len(matrix['cases']) == 3
    assert len({r['folder'] for r in matrix['cases']}) == 3
    assert {(r['parameters']['ranks'], r['parameters']['workers'],
             r['parameters']['anisotropy'], r['parameters']['height_power'])
            for r in matrix['cases']} == {(4, 2, 2, 0), (1, 1, 2, 0), (4, 2, 1, 20)}
    reports = []
    for recipe in matrix['cases']:
        folder = BASE / recipe['folder']
        receipt = pinned(folder)
        assert sha(folder / 'runner.py') == recipe['runner_sha256']
        assert receipt['binary_sha256'] == build['binary_sha256']
        assert receipt['build_validation_sha256'] == sha(BASE / matrix['qualified_build'])
        audit = json.loads((folder / 'independent_audit.json').read_text())
        p = audit['parameters']
        assert audit['status'] == 'PASS' and p == recipe['parameters']
        assert len(audit['frames']) == 8 and len(audit['states']) == 3
        assert {r['format'] for r in audit['reload_metrics']} == {'SU2', 'CGNS'}
        assert p['size'] == 4 and p['diagnostic_rounds'] == 512
        assert (p['ranks'], p['workers']) in ((1, 1), (4, 2))
        accepted = [r for r in audit['frames'] if r['file'] in {f'envelope_s{i}.json' for i in range(3)}]
        assert len(accepted) == 3
        for row in accepted:
            assert row['strict_metric_gate'] and row['minimum_mean_ratio'] >= .20
            assert row['minimum_scaled_jacobian'] >= .05 and row['maximum_length'] <= 1.8
        for row in audit['states'] + audit['reload_metrics']:
            assert row['maximum_transported_tensor_absolute_error'] == 0
            assert row.get('maximum_transported_tensor_physical_error', 0) == 0
        prior_folder = BASE / recipe['historical_control']
        prior = pinned(prior_folder)
        assert json.loads((prior_folder / 'parameters.json').read_text()) == p
        geometry_matches = []
        for row in audit['frames']:
            name = row['file']
            assert sha(folder / name) == sha(prior_folder / name), (folder.name, name, 'geometry/donor drift')
            geometry_matches.append(name)
        state_matches = {f'envelope_s{i}_states.json': sha(folder / f'envelope_s{i}_states.json') == sha(prior_folder / f'envelope_s{i}_states.json') for i in range(3)}
        normalized = []
        if p['height_power']:
            assert p['height_power'] == 20 and p['anisotropy'] == 1
            baseline = BASE / 'box_filter_n4_n4_m2_v1'
            control = pinned(baseline)
            assert control['binary_sha256'] == receipt['binary_sha256']
            factor = 2 ** p['height_power']
            for row in audit['frames']:
                name = row['file']
                mesh = json.loads((folder / name).read_text())
                reference = json.loads((baseline / name).read_text())
                assert mesh != reference
                for node in mesh['nodes']:
                    node[3] *= factor
                for facet in mesh['facets']:
                    for node in facet['vertices']:
                        node[3] *= factor
                for tet in mesh['original']:
                    for node in tet['vertices']:
                        node[3] *= factor
                        node[4][2] /= factor
                        node[4][4] /= factor
                        node[4][5] /= factor ** 2
                assert mesh == reference, (folder.name, name, 'affine covariance')
                normalized.append(name)
        else:
            assert p['anisotropy'] == 2 and p['direction'] == 1
        details = [parse(line) for line in (folder / 'case.stdout').read_text().splitlines()
                   if line.startswith('NATIVE3D_DETAIL ') and any('stage='+name+' ' in line for name in ('refine', 'coarsen'))]
        assert details
        for row in details:
            assert all(row[k] >= 0 and int(row[k]) == row[k] for k in ('donor_box_filtered', 'donor_filtered', 'donor_candidates'))
            assert row['donor_box_filtered'] <= row['donor_filtered'] <= row['donor_candidates']
        reports.append(dict(case=folder.name, parameters=p, validation_sha256=sha(folder/'validation.json'),
                            audit_sha256=sha(folder/'independent_audit.json'), accepted_frames=accepted,
                            states=audit['states'], reload_metrics=audit['reload_metrics'],
                            historical_geometry_and_donor_matches=geometry_matches,
                            historical_state_byte_matches=state_matches,
                            exact_normalized_geometry_and_donor_frames=normalized,
                            donor_box_filtered_rank_work=sum(row['donor_box_filtered'] for row in details),
                            historical_control_validation_sha256=sha(prior_folder/'validation.json')))
    result = dict(status='PASS', matrix_sha256=sha(matrix_path), checker_sha256=sha(Path(__file__).resolve()),
                  qualified_build_sha256=sha(BASE/matrix['qualified_build']), binary_sha256=build['binary_sha256'],
                  cases=reports, scope='Fresh whole-case current-binary aspect2 turning and H20 thin geometry/metric/current+BDF/SU2CGNS audits. '
                  '24 exact historical geometry/donor matches and eight current-source affine covariance checks. '
                  'State byte matches recorded separately from independently validated residuals; no cross-partition identity, '
                  'BL construction, calibrated speedup/scaling or fullGoal1 qualification.')
    (BASE / 'BOX_FILTER_ENVELOPE_VALIDATION.json').write_text(json.dumps(result, indent=2)+'\n')
    print('PASS:three current-source whole cases;24 historical geometry/donor matches;8 current affine covariance frames')


if __name__ == '__main__':
    main()
