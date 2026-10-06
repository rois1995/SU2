"""Recheck saved integration evidence and checkout invariants; no simulations run."""
import argparse, hashlib, json, math, re, subprocess, time
from pathlib import Path

E = Path(__file__).resolve().parent
source = E.parent

def sha(path):
    h = hashlib.sha256()
    with Path(path).open('rb') as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b''):
            h.update(chunk)
    return h.hexdigest()

def contract(row):
    return (all(row[k] for k in ('all_exact_positive', 'manifold_oriented_edges', 'physical_equals_exposed'))
            and all(math.isfinite(row[k]) for k in ('min_quality', 'max_simpson_length', 'max_relative_height_error'))
            and row['min_quality'] >= .18 and row['max_simpson_length'] <= 1.8
            and row['max_relative_height_error'] <= 1e-8 and not row['bad_cells'])

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path)
    parser.add_argument('--self-check', action='store_true')
    args = parser.parse_args()
    if args.self_check:
        row = dict(all_exact_positive=True, manifold_oriented_edges=True, physical_equals_exposed=True,
                   min_quality=.4, max_simpson_length=1.7, max_relative_height_error=0., bad_cells=[])
        assert contract(row)
        row['max_simpson_length'] = 1.81
        assert not contract(row)
        row['max_simpson_length'] = float('nan')
        assert not contract(row)
        print('Completion-check self-check PASS'); return
    if args.output is None: parser.error('--output is required')
    if args.output.exists(): raise RuntimeError('Preserve evidence; choose a fresh output')
    pins = {}
    def load(name):
        path = Path(name)
        if not path.is_absolute(): path = E / path
        pins[str(path.resolve())] = sha(path)
        return json.loads(path.read_text())
    def recheck(mapping):
        for path, digest in mapping.items():
            assert sha(path) == digest, path
    matrices = {}
    for label in ('integrated_output_controls_v6', 'integrated_primal_controls_v6',
                  'integrated_primal_memory_v6', 'integrated_mmg_default_v6', 'integrated_auxiliary_v9',
                  'integrated_bl_smooth_v1', 'integrated_bl_step_v1', 'integrated_airfoil_phase_timing_v1',
                  'integrated_general_transfer_v1'):
        d = load(label + '/evidence.json')
        assert not d.get('current_run') and d.get('archived_binary')
        assert [r['ranks'] for r in d['runs']] == [1, 2, 4]
        assert all(r['verified'] and r['exit_code'] == 0 for r in d['runs'])
        assert sha(d['archived_binary']) == d['binary_sha256']
        if label != 'integrated_primal_memory_v6':
            for r in d['runs']:
                assert len(re.findall(r'All tests passed[^\n]*', Path(r['log']).read_text())) == r['ranks']
        matrices[label] = dict(filter=d['filter'], ranks=[1, 2, 4], binary_sha256=d['binary_sha256'])
    options = load('integrated_airfoil_phase_timing_v1/evidence.json')['build_options']
    assert options['with-mpi'] == 'enabled' and options['enable-cgns'] and options['enable-mmg'] and options['b_ndebug'] == 'false'
    independent = {}
    for label in ('integrated_bl_smooth_v1', 'integrated_bl_step_v1'):
        d = load(label + '/independent_v1/evidence.json')
        assert d['terminal'] and d['verified'] and len(d['runs']) == 96
        for r in d['runs']:
            a = load(r['output']); assert r['verified'] and contract(a) and a['disk_euler'] == 1
            assert all(a[k] for k in ('original_flat_geometry_retained', 'marker_junction_retained', 'corners_retained'))
            recheck(a['input_files_sha256'])
        independent[label] = 96
    for label in ('integrated_airfoil_phase_timing_v1', 'integrated_airfoil_baseline_v8'):
        folder = 'independent_v1' if 'timing' in label else 'independent_v8'
        d = load(label + '/' + folder + '/evidence.json')
        assert d['terminal'] and d['verified'] and len(d['runs']) == 9
        for r in d['runs']:
            assert r['verified'] and all(r['checks'].values())
            a = load(r['output']); assert contract(a) and a['disk_euler'] == 0
            recheck(a['input_files_sha256'])
        independent[label] = 9
    output = load('integrated_primal_controls_v6/independent_outputs_v6/evidence.json')
    assert output['terminal'] and output['verified'] and len(output['runs']) == 84
    for r in output['runs']:
        assert r['verified']; a = load(r['output'])
        if 'min_quality' in a: assert contract(a)
        elif 'all_exact_positive' in a:
            assert all(a[k] for k in ('all_exact_positive', 'finite_unique_coordinates', 'oriented_manifold_edges', 'physical_equals_exposed'))
        else: assert a['exact_su2_snapshot_match']
    for path in [E/'scaling_pilot_v6/independent_audit.json', *sorted((E/'robustness_campaign_v8').glob('*/independent_audit.json')), E/'integration_capacity_v2/larger_np4_t1024/independent_audit.json']:
        d = load(path); assert d['all_structural_pass']
        for r in d['cases']:
            assert r['structural_pass'] and not r['errors']
            assert r['complete'] or (r['proven_incompatible'] and 'incompatible' in str(path))
            recheck({str(Path(r['directory'])/name): digest for name, digest in r['files_sha256'].items()})
        independent[str(path.relative_to(E))] = len(d['cases'])
    timing = load('integrated_airfoil_phase_timing_v1/timing_audit_v1.json')
    assert timing['verified'] and len(timing['cases']) == 9; recheck(timing['input_files_sha256'])
    partial = load('native_phase_partial_complete_v1.json'); assert partial['verified']
    for r in partial['partial_contracts']:
        a = load('integrated_airfoil_' + r['name'] + '_v8/partial_target_audit_v1.json')
        assert contract(a) and all(r['checks'].values()); recheck(a['input_files_sha256'])
    production = load('integrated_native_production_v2/evidence.json')
    assert [r['ranks'] for r in production['runs']] == [1, 2, 4]
    for r in production['runs']:
        assert r['verified'] and r['exit_code'] == 0 and len(r['outputs']) == 4
        for a in r['outputs'].values():
            recheck({a[k]: a[k + '_sha256'] for k in ('restart', 'mesh', 'vtu')})
    complete = load('native_production_complete_v1.json'); assert complete['verified'] and len(complete['reports']) == 9
    assert complete['all_nine_audited_metrics_exactly_equal_actual_double_restarts']
    for name in ('ad_repair_controls_v14.json', 'goal_remaining_v1.json', 'integration_capacity_v2.json'):
        d = load(name); assert d['phase'] == 'terminal' and d['exit'] == 0
        if name == 'goal_remaining_v1.json': assert all(r['passed'] and all(r['checks'].values()) for r in d['runs'])
    cold = load('goal_cold_failure_classification_v1.json')
    assert cold['diagnosis_complete'] and not cold['lifecycle_pass'] and cold['trajectory_rows'] == 131
    recheck(cold['input_files_sha256'])
    closure = load('native_validation_closure_v1.json')
    assert closure['phase'] == 'terminal' and closure['exit'] == 1 and all(r['verified'] for r in closure['steps'])
    classification = load('native_closure_failure_classification_v1.json')
    assert classification['original_state_sha256'] == sha(E/'native_validation_closure_v1.json')
    rejects = load('native_twopass_rejection_v2/evidence.json')
    assert rejects['phase'] == 'terminal' and rejects['exit'] == 0
    assert [r['ranks'] for r in rejects['runs']] == [2, 4] and all(r['verified'] and r['exit'] == 1 for r in rejects['runs'])
    for name in ('native_unsupported_derivative_v1', 'native_missing_restart_reference_v1'):
        d = load(name + '/evidence.json'); assert [r['ranks'] for r in d['runs']] == [1, 2, 4]
        assert all(r['verified'] and r['exit_code'] == 1 for r in d['runs'])
    ad = load('ad_repair_controls_v14.json')
    production_sources = {str(source/p): digest for p, digest in ad['source_sha256'].items() if p.startswith(('Common/', 'SU2_CFD/'))}
    assert len(production_sources) == 759; recheck(production_sources)
    coverage = load('executed_source_coverage_v2.json'); assert coverage['verified']
    recheck({str(source/p): digest for p, digest in coverage['current_changes_from_primal_manifest'].items()})
    def git(*args, cwd=source): return subprocess.check_output(['git', *args], cwd=cwd, text=True)
    main = Path('/media/rausa/4TB/SU2_Versions/SU2_AdapNoExt')
    original = load('main_initial.json')
    actual = dict(head=git('rev-parse','HEAD',cwd=main).strip(), branch=git('branch','--show-current',cwd=main).strip(),
                  tracked_status=git('status','--short','--untracked-files=no',cwd=main), gitignore_sha256=sha(main/'.gitignore'))
    assert actual == original and git('branch','--show-current').strip() == 'codex/native-integrated'
    refs = dict(r.split() for r in git('for-each-ref','--format=%(refname) %(objectname)','refs/heads','refs/remotes').splitlines())
    previous = load('reconciliation_guard_v2.json')['refs']
    assert {k:v for k,v in refs.items() if k != 'refs/heads/codex/native-integrated'} == {k:v for k,v in previous.items() if k != 'refs/heads/codex/native-integrated'}
    draft = load('stage_g_draft.json')
    files = ['SU2_CFD/include/drivers/CDiscAdjSinglezoneDriver.hpp', 'SU2_CFD/src/drivers/CDiscAdjSinglezoneDriver.cpp', 'UnitTests/SU2_CFD/drivers/CDiscAdjSinglezoneDriver_tests.cpp', 'config_template.cfg']
    raw = subprocess.check_output(['git','diff','--binary','--',*files],cwd=draft['worktree'])
    assert hashlib.sha256(raw).hexdigest() == draft['patch_sha256']
    assert not any(line[3:].startswith(('Common/','SU2_CFD/','UnitTests/')) for line in git('status','--short','--untracked-files=no').splitlines())
    documents = {name: sha(source/name) for name in ('NATIVE_INTEGRATION_GOAL.md','BRANCH_RECONCILIATION.md','ROBUSTNESS_SCALING_PROTOCOL.md','ROBUSTNESS_SCALING_RESULTS.md','HANDOFF_Codex.md','GRID_GUIDE.md','INTEGRATION_COMPLETION_AUDIT.md')}
    recheck(pins)
    report = dict(verified=True,checked_unix_seconds=time.time(),source_revision=git('rev-parse','HEAD').strip(),
                  matrices=matrices,independent_counts=independent,production_source_files_rechecked=759,
                  untouched_main=actual,refs=refs,stage_g_draft_sha256=draft['patch_sha256'],
                  evidence_sha256=pins,documentation_sha256=documents,checker_sha256=sha(__file__),
                  scope='Completion of declared local branch integration and observed native2D envelope; timeouts, unsupported capabilities and classified CFD/harness failures preserved. No3D/CAD/general scalability/converged sensitivity claim.')
    with args.output.open('x') as stream: json.dump(report,stream,indent=2); stream.write('\n')
    print('Completion audit PASS: runtime matrices, independent reports/pins,759production sources,refs,draft,main and documents')

if __name__ == '__main__': main()
