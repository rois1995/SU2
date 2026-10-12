"""Verify the isolated rotating-policy whole-case matrix and descriptive work counts."""
from pathlib import Path
import hashlib
import json
import sys

BASE = Path(__file__).resolve().parent
ROOT = BASE.parents[1]
sys.path.insert(0, str(ROOT / 'integration_evidence/native_cluster_3d_core_v1'))
from audit_lifecycle import parse
from profile_summary import profiles

sha = lambda p: hashlib.sha256(p.read_bytes()).hexdigest()


def pinned(folder):
    receipt = json.loads((folder / 'validation.json').read_text())
    assert receipt['status'] == 'PASS', (folder.name, receipt.get('reason'))
    for key in ('source_drift', 'object_drift', 'archive_drift', 'tool_drift', 'variant_drift'):
        assert not receipt.get(key, []), (folder.name, key)
    for name, expected in receipt['artifact_sha256'].items():
        assert sha(folder / name) == expected, (folder.name, name)
    return receipt


def result(folder):
    receipt = pinned(folder)
    audit = json.loads((folder / 'independent_audit.json').read_text())
    assert audit['status'] == 'PASS'
    assert len(audit['frames']) == 8 and len(audit['states']) == 3
    accepted = [row for row in audit['frames'] if row['file'] in {f'envelope_s{i}.json' for i in range(3)}]
    assert len(accepted) == 3
    for row in accepted:
        assert row['strict_metric_gate'] and row['minimum_mean_ratio'] >= .20
        assert row['minimum_scaled_jacobian'] >= .05 and row['maximum_length'] <= 1.8
    for row in audit['states'] + audit['reload_metrics']:
        assert row['maximum_transported_tensor_absolute_error'] == 0
        assert row.get('maximum_transported_tensor_physical_error', 0) == 0
    lines = (folder / 'case.stdout').read_text().splitlines()
    native = [parse(line) for line in lines if line.startswith('NATIVE3D ')]
    stages = [parse(line) for line in lines if line.startswith('NATIVE3D_STAGE ')]
    details = [parse(line) for line in lines if line.startswith('NATIVE3D_DETAIL ')]
    summary = profiles(native, stages, details)
    # event_index is within a stage; this fixture has one refine/coarsen pair per cycle.
    work = []
    for event in summary['nested_detail_events']:
        if event['stage'] not in ('refine', 'coarsen'):
            continue
        profile = event['profile']
        work.append(dict(stage=event['stage'], cycle=event['event_index'],
                         proposal_outcomes=profile['proposal_outcomes'],
                         counts={key: profile['measures'][key]['rank_values'] for key in
                                 ('rounds', 'attempted', 'accepted', 'rejected', 'conflicts',
                                  'donor_candidates', 'donor_box_filtered', 'donor_payloads',
                                  'tracked_bytes', 'tracked_calls')}))
    assert len(work) == 6
    return dict(folder=folder.name, validation_sha256=sha(folder/'validation.json'),
                audit_sha256=sha(folder/'independent_audit.json'), log_sha256=sha(folder/'case.stdout'),
                binary_sha256=receipt['binary_sha256'], accepted_frames=accepted,
                states=audit['states'], reload_metrics=audit['reload_metrics'], restart=audit['restart'],
                work=work)


def main():
    manifest_path = BASE / 'ROTATING_RESERVATION_WHOLE_MATRIX.json'
    manifest = json.loads(manifest_path.read_text())
    assert manifest['status'] == 'PREDECLARED_NOT_EXECUTED' and len(manifest['cases']) == 3
    qualified_path = BASE / manifest['qualified_build']
    qualified = json.loads(qualified_path.read_text())
    assert sha(qualified_path) == manifest['qualified_build_sha256'] and qualified['status'] == 'PASS'
    variant_path = BASE / manifest['isolated_build']
    variant = json.loads(variant_path.read_text())
    assert variant['status'] == 'PASS' and not variant['source_drift']
    assert not variant['object_drift'] and not variant['library_drift']
    assert sha(variant_path.parent/'directory_rotating.cpp') == manifest['variant_source_sha256']
    assert variant['qualified_build_receipt_sha256'] == sha(qualified_path)
    for name, expected in variant['artifact_sha256'].items():
        assert sha(variant_path.parent/name) == expected, name
    for name, expected in variant['source_sha256'].items():
        assert sha(ROOT/name) == expected, name
    build_folder = ROOT/'integration_evidence/build-integrated-v2'
    assert all(sha(build_folder/name) == expected for name, expected in qualified['actual_objects_sha256'].items())
    assert all(sha(Path(name)) == expected for name, expected in {**qualified['library_sha256'], **qualified['thin_archive_member_sha256']}.items())
    assert all(sha(ROOT / name) == expected for name, expected in qualified['source_sha256'].items())
    assert sha(ROOT/'integration_evidence/build-integrated-v2/UnitTests/test_driver') == qualified['binary_sha256']
    records = []
    for recipe in manifest['cases']:
        folder, control = BASE / recipe['folder'], BASE / recipe['control']
        receipt = pinned(folder)
        assert sha(folder/'runner.py') == recipe['runner_sha256']
        assert receipt['binary_sha256'] == variant['binary_sha256']
        assert receipt['production_binary_sha256'] == qualified['binary_sha256']
        assert receipt['build_validation_sha256'] == sha(qualified_path)
        assert receipt['isolated_variant_receipt_sha256'] == sha(variant_path)
        assert receipt['isolated_variant_source_sha256'] == manifest['variant_source_sha256']
        assert sha(control/'validation.json') == recipe['control_validation_sha256']
        parameters = json.loads((folder/'parameters.json').read_text())
        assert parameters == recipe['parameters'] == json.loads((control/'parameters.json').read_text())
        assert parameters['ranks'] == 4 and parameters['workers'] == 2 and parameters['diagnostic_rounds'] == 512
        inputs = {name: sha(folder/name) for name in ('envelope.cfg', 'envelope_input.su2', 'envelope_s0_initial.json')}
        assert all(sha(control/name) == expected for name, expected in inputs.items())
        actual, prior = result(folder), result(control)
        assert json.loads((folder/'independent_audit.json').read_text())['parameters'] == parameters
        assert (actual['restart'] is not None) == (parameters['restart_format'] == 'cgns')
        matches = {f'envelope_s{i}.json': sha(folder/f'envelope_s{i}.json') == sha(control/f'envelope_s{i}.json') for i in range(3)}
        normalized_matches = []
        if parameters['height_power']:
            unscaled_folder = BASE/'rotating_reservation_probe_v1'
            pinned(unscaled_folder)
            unscaled_parameters = json.loads((unscaled_folder/'parameters.json').read_text())
            assert dict(parameters, height_power=0) == unscaled_parameters
            factor = 2 ** parameters['height_power']
            for row in json.loads((folder/'independent_audit.json').read_text())['frames']:
                name = row['file']
                mesh = json.loads((folder/name).read_text())
                unscaled = json.loads((unscaled_folder/name).read_text())
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
                assert mesh == unscaled, (name, 'Affine covariance of the same isolated variant failed')
                normalized_matches.append(name)
        records.append(dict(parameters=parameters, initial_inputs_sha256=inputs,
                            accepted_mesh_byte_matches=matches, exact_affine_covariance_frames=normalized_matches, isolated=actual, control=prior))
    lifecycle_folder = BASE/'rotating_reservation_lifecycle_mpi1_v1'
    lifecycle_manifest = json.loads((lifecycle_folder/'matrix.json').read_text())
    assert lifecycle_manifest['status'] == 'PREDECLARED_NOT_EXECUTED'
    lifecycle_receipt = pinned(lifecycle_folder)
    assert lifecycle_manifest['runner_sha256'] == sha(lifecycle_folder/'runner.py')
    assert lifecycle_manifest['variant_build_sha256'] == sha(variant_path)
    assert lifecycle_receipt['isolated_variant_receipt_sha256'] == sha(variant_path)
    assert lifecycle_receipt['isolated_variant_source_sha256'] == manifest['variant_source_sha256']
    assert lifecycle_receipt['binary_sha256'] == variant['binary_sha256']
    assert lifecycle_receipt['production_binary_sha256'] == qualified['binary_sha256']
    lifecycle = json.loads((lifecycle_folder/'lifecycle_audit.json').read_text())
    assert lifecycle['status'] == 'PASS' and len(lifecycle['frames']) == 6
    assert len(lifecycle['fixed_work_controls']) == 5 and len(lifecycle['outer_case_rows']) == 6
    for row in lifecycle['frames']:
        assert row['strict_metric_gate'] and row['minimum_mean_ratio'] >= .20
        assert row['minimum_scaled_jacobian'] >= .05 and row['maximum_length'] <= 1.8
    prior_lifecycle_folder = BASE/lifecycle_manifest['control']
    prior_lifecycle = pinned(prior_lifecycle_folder)
    lifecycle_matches = {row['file']: sha(lifecycle_folder/row['file']) == sha(prior_lifecycle_folder/row['file']) for row in lifecycle['frames']}
    assert all(lifecycle_matches.values()), 'Serial reservation priority must retain the established identical geometry/metric fixture'
    lifecycle_report = dict(folder=lifecycle_folder.name, manifest_sha256=sha(lifecycle_folder/'matrix.json'),
                            validation_sha256=sha(lifecycle_folder/'validation.json'),
                            audit_sha256=sha(lifecycle_folder/'lifecycle_audit.json'), frames=lifecycle['frames'],
                            control_validation_sha256=sha(prior_lifecycle_folder/'validation.json'),
                            control_frame_byte_matches=lifecycle_matches,
                            scope='Six actual metric/Hessian/CFD/BDF OFF/ON controls on MPI1; no local timing claim.')
    report = dict(status='PASS', matrix_sha256=sha(manifest_path), checker_sha256=sha(Path(__file__).resolve()),
                  profile_summary_sha256=sha(ROOT/'integration_evidence/native_cluster_3d_core_v1/profile_summary.py'),
                  qualified_build_sha256=sha(qualified_path), isolated_build_sha256=sha(variant_path), cases=records, lifecycle=lifecycle_report,
                  scope='Three independently audited N4M2 whole cases: stronger turning anisotropy, H20 thin affine domain and actual CGNS persisted restart. Exact initial controls match; subsequent accepted meshes and frozen P1 references may differ. No fixed-work speed ratio, repeated cost/scaling, BL generation, production scheduling change or fullGoal1 qualification.')
    (BASE/'ROTATING_RESERVATION_WHOLE_VALIDATION.json').write_text(json.dumps(report, indent=2)+'\n')
    print('PASS: three whole-case fresh exact geometry/metric/state/output audits; pinned controls and changed-trajectory work counts')


if __name__ == '__main__':
    main()
