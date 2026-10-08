from pathlib import Path
import hashlib, json, re, subprocess, sys
import numpy as np

root = Path('/media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated')
sys.path.insert(0, str(root / 'integration_evidence'))
from audit_native_unsteady import capcheck, state

out = root / 'integration_evidence/native_cluster_analysis_v1'
progress = json.loads((out / 'audit_progress.json').read_text())
assert progress['phase'] == 'terminal', progress['phase']
assert all(row['status'] == 'PASS' for row in progress['rows'])
cases = sorted((root / 'ClusterResults/cases').iterdir())
assert len(cases) == 16 and len(progress['rows']) == 16
sha = lambda p: hashlib.sha256(p.read_bytes()).hexdigest()
timing = json.loads((out / 'timing_and_contention_evidence.json').read_text())
rows = []
count = size = 0
for case in cases:
    manifest = json.loads((case / 'collection_manifest.json').read_text())
    assert manifest['status'] == 'COMPLETE_SELECTED_EVIDENCE' and not manifest['missing']
    for name, expected in manifest['files'].items():
        assert Path(name).name == name
        p = case / name
        assert p.is_file() and not p.is_symlink()
        assert p.stat().st_size == expected['bytes'] and sha(p) == expected['sha256'], p
        count += 1
        size += p.stat().st_size
    run = json.loads((case / 'run_evidence.json').read_text())
    assert run['success'] and run['solver_exit'] == 0
    frozen = 'frozen' in case.name
    auditfile = case / ('independent_frozen_metric_audit.json' if frozen else 'independent_rae_unsteady_audit.json')
    audit = json.loads(auditfile.read_text())
    assert audit['status'] == 'PASS'
    accounting = case / 'independent_profile_accounting.json'
    assert json.loads(accounting.read_text())['status'] == 'PASS'
    meshes = [audit] if frozen else audit['windows']
    history = [h for m in meshes for h in m.get('history', [])]
    if not frozen:
        assert len(meshes) == 2 and len(history) == 4
        assert all(h['closed_policy_integral_residual'] is not None and
                   h['closed_policy_integral_residual'] < 1e-10 for h in history)
        assert all([h['step'] for h in m['history']] == [m['first_step']-2, m['first_step']-1] for m in meshes)
    transported = [m['transported_metric'] for m in meshes if m.get('transported_metric')]
    t = next(r for r in timing['cases'] if r['case'] == case.name)
    rows.append(dict(case=case.name, status='PASS', audit_sha256=sha(auditfile),
                     accounting_sha256=sha(accounting), min_quality=min(m['min_quality'] for m in meshes),
                     max_length=max(m['max_simpson_length'] for m in meshes),
                     max_first_height_relative_error=max((m.get('max_relative_first_height_error') or 0) for m in meshes),
                     transport=transported, history_checks=history, phase_seconds=audit.get('phase_seconds'),
                     whole_seconds=t['whole_seconds'], workers=t['workers'], weighted=t['weighted'],
                     profile=t['profile'], full_adaptation_seconds=t.get('full_adaptation_seconds'),
                     other_compute_seen=bool(t['competing_compute_samples'])))

matches = []
for kind in ('actual_euler_to_bl', 'actual_bl_to_euler'):
    group = [p for p in cases if kind in p.name]
    configs = [(p/'run.cfg').read_text() for p in group]
    normalized = [re.sub(r'^ADAP_NATIVE_(RANKS|REPARTITION)\s*=.*$', r'ADAP_NATIVE_\1=<varies>', c, flags=re.M) for c in configs]
    assert len(set(normalized)) == 1
    assert len({sha(p/'input.su2') for p in group}) == 1
    freq = int(re.search(r'^ADAP_FREQ\s*=\s*(\d+)', configs[0], re.M)[1])
    mesh = capcheck.read_su2(group[0]/'input.su2')
    fields = []
    for p in group:
        donor = p/f'flow_{freq-1:05d}.vtu'
        values, _ = state(mesh, donor)
        metric, precision = capcheck.metric_of(mesh, donor)
        assert precision == 'Float64'
        fields.append((values, metric))
    for case, (values, metric) in zip(group, fields):
        same = dict(case=case.name, first_donor_flow_exact=np.array_equal(values,fields[0][0]),
                    first_donor_sensor_exact=np.array_equal(metric,fields[0][1]),
                    max_flow_difference=float(np.max(np.abs(values-fields[0][0]))),
                    max_sensor_difference=float(np.max(np.abs(metric-fields[0][1]))))
        assert same['first_donor_flow_exact'] and same['first_donor_sensor_exact'], same
        matches.append(same)

tools = {str(p.relative_to(root/'ClusterResults/tools')): sha(p) for p in (root/'ClusterResults/tools').rglob('*') if p.is_file() and p.suffix == '.py'}
baseline = 'e6995fbff565955a5677ffcbcc66af9acf96a8f1'
for name,digest in tools.items():
    blob = subprocess.check_output(['git','show',baseline+':'+name],cwd=root)
    assert hashlib.sha256(blob).hexdigest() == digest
original_pins = subprocess.check_output(['git','show',baseline+':integration_evidence/native_cluster_campaign_v1/source_pins.json'],cwd=root)
receipt = dict(status='PASS', scope='Independent numerical and file-integrity checks; descriptive timing only',
               original_manifest_files=count, original_bytes=size, copied_tools_sha256=tools,
               original_source_pins_sha256=hashlib.sha256(original_pins).hexdigest(),
               first_donor_equality=matches, cases=rows,
               timing_limits=['One repetition per mode; N=4; two actual adaptations',
                              'Campaigns overlapped on node-a-ag2.local with overlapping allowed CPU sets',
                              'Overlap is not proof of CPU saturation or causal slowdown',
                              'Rank maxima and nested transfer cannot be added to exclusive wall time',
                              'No aerodynamic convergence, native 3D, or long-horizon accuracy certificate'])
(out/'assessment.json').write_text(json.dumps(receipt,indent=2)+'\n')
lines = ['# Downloaded cluster campaign assessment', '',
         'All 16 cases pass independent numerical and profiling-accounting audits: eight frozen remeshes and eight actual unsteady runs (two adaptations each). Original collected evidence remains unchanged: %d files, %d bytes, hashes checked. All four histories per actual case have original donor snapshots and independently pass the CLOSED transfer check, including the measured open-farfield area correction. The checker’s generic sparse-history limitation does not apply to these complete snapshots.' % (count,size), '',
         'Both directions preserve the original geometry, features, first-height constraints where requested, positive cells, conformity, metric quality and edge-length limits. Frozen reader tensors pass the independent directional transport check. Actual runs reconstruct the target independently from saved sensor donors and geometry; no final-tensor CSV residual is claimed for those runs.', '',
         'The first pre-adaptation conservative flow state and Float64 sensor metric are bitwise identical across all four modes within each actual workload; configurations differ only in worker count/repartitioning. After adaptation, meshes and trajectories differ, so comparisons are not fixed-work speedups.', '',
         '## Full actual lifecycle costs', '',
         'Seconds. Adaptation includes metric construction, remeshing (including its working repartition/migration), mesh replacement (including CFD repartition and solution transfer), and adapted output. Transfer is nested in replacement and is not added twice. The terminal window builds a metric without remeshing; its metric cost is included.', '',
         '| Workload | Workers / partition | Whole | CFD | Metric | Remesh | Replace | Output | Full adaptation |',
         '|---|---|---:|---:|---:|---:|---:|---:|---:|']
for r in rows:
    c=r['phase_seconds']
    if c:
        full=sum(c[k] for k in ('metric','remesh','replace','adapted_output'))
        lines.append('| %s | %d / %s | %.2f | %.2f | %.2f | %.2f | %.3f | %.3f | %.2f |' %
                     ('RANS Euler→BL' if 'euler_to_bl' in r['case'] else 'Euler BL→Euler', r['workers'],r['weighted'],r['whole_seconds'],c['CFD'],c['metric'],c['remesh'],c['replace'],c['adapted_output'],full))
lines += ['', '## Where work accumulates', '',
          'For weighted M=N=4 RANS, the first engine adaptation takes 23.77 s. Exclusive rank means are: selection 0.82, protocol 0.09, dependencies 2.42, donor import/IDs 2.60, reconstruction 5.47, validation/decision 10.28, commit 0.75, unclassified 1.33. Private reconstruction ranges 2.62–8.85 s: MPI imbalance remains. Validation wall scopes include collective waiting; their size does not prove equivalent validation CPU work.', '',
          'Working repartition is small in these N=4 cases: weighted RANS estimates/graph/partition/migration component maxima are approximately 0.021/0.013/0.017/0.006 s before the first remesh and 0.082/0.019/0.011/0.008 s before the second. Component maxima are descriptive, not additive critical-path costs. CFD return partition/migration/preprocessing is inside replacement; its first adapted geometry row is 0.049/0.032/0.010 s. The initial geometry row is outside adaptation.', '',
          'Weighted M=N=4 Euler takes 9.08 s in its first engine adaptation: donor import/IDs 2.43, private reconstruction 1.68, validation 2.69 s (rank means). Its second takes 3.26 s, with donor import/IDs 1.13 s. Dependency/donor traffic, recovery/metric work, and reconstruction imbalance deserve attention; changing worker count alone has not established a total-cost gain.', '',
          '## Performance interpretation', '',
          'Weighted four-worker actual runs have the lowest observed adaptation totals: RANS 65.22 s and Euler 16.70 s. M=3 and M=2 do not improve these observed totals. Frozen Euler→BL kernel/whole times are respectively: original M4 34.62/35.65, weighted M4 30.76/31.81, M3 30.48/32.65, M2 37.41/42.04 s. Frozen BL→Euler: original M4 9.46/10.73, weighted M4 9.19/11.53, M3 11.03/15.57, M2 13.47/15.42 s. The best kernel time need not be the best whole-process time.', '',
          'These are single observations, not accepted speedup estimates or a default-setting recommendation. All campaigns share node-a-ag2.local (EPYC 9654, 192 physical cores); recorded processes show overlaps and common allowed CPU groups. With 24 permitted cores per rank group, overlap alone does not establish oversubscription. CPU PSI and actual per-thread placement/activity are unavailable. Even identical pre-adaptation fixed-mesh CFD varies: RANS 20.33–28.78 s, Euler 22.17–30.79 s. Shared caches, memory and filesystem activity are uncontrolled. RANS M3 adapted output alone takes 3.14 s versus 0.15–0.36 s in other modes.', '',
          'The next cluster campaign must pair old/new executables under the same compiler/MPI/build options, rotate order, repeat at least three times, and serialize jobs through SGE dependencies. Repartition remains included. Use matched frozen targets to isolate the remesher, and actual cycles to assess changed metric/recovery trajectories. Keep noise disabled. No new local performance campaign is needed.', '',
          'Memory samples are partial: sum of rank HWM is not a simultaneous peak; host-local RSS samples miss other nodes and may miss short peaks. These pilots establish N=4 numerical feasibility, not a scaling limit, native 3D support, long-time accuracy, or converged viscous forces.', '',
          'Machine-readable numerical receipts and phase profiles: `assessment.json`; complete sampled timing/contention evidence: `timing_and_contention_evidence.json`; exact audit commands/results: `audit_progress.json`. Raw cases remain under `/media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated/ClusterResults/cases/`.']
(out/'ASSESSMENT.md').write_text('\n'.join(lines)+'\n')
print('Cluster assessment PASS',count,size,'16 cases, first donors identical; no isolated timing claim')
