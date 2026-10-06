"""Sequential repetitions, affine robustness and real-airfoil demand limits.

Wait for the earlier integration chain; preserve every failed run. At most
one MPI job, four ranks and one thread per rank. No production defaults change.
"""
from pathlib import Path
import hashlib, json, os, re, shutil, signal, subprocess, sys, time

source = Path(__file__).resolve().parent.parent
root = source / 'integration_evidence'
state = root / 'robustness_chain_v5.json'
prerequisite = root / 'extended_chain_v9.json'
if state.exists():
    raise SystemExit('Preserve evidence; choose a fresh version.')
status = {'runner_pid': os.getpid(), 'started': time.time(), 'phase': 'waiting_for_extended', 'steps': []}
env = dict(os.environ, OMP_NUM_THREADS='1', OPENBLAS_NUM_THREADS='1', PYTHONDONTWRITEBYTECODE='1')

def save():
    state.write_text(json.dumps(status, indent=2) + '\n')

def quiet():
    since = None
    while True:
        busy = []
        for row in subprocess.check_output(['ps', '-eo', 'pid,stat,comm'], text=True).splitlines()[1:]:
            pid, flags, name = row.split(maxsplit=2)
            if 'Z' in flags or pid == '918696':
                continue
            if name in ('ninja', 'cc1plus', 'test_driver', 'test_driver_AD', 'test_memory') or name.startswith('SU2_CFD'):
                busy.append({'pid': int(pid), 'name': name})
        status.update(phase='waiting_for_machine', busy=busy, checked=time.time()); save()
        if busy:
            since = None
        elif since is None:
            since = time.monotonic()
        elif time.monotonic() - since >= 15:
            return
        time.sleep(5)

def run(label, command, cwd=source, extra=None, timeout=1800, allow_failure=False):
    quiet()
    start = time.monotonic()
    log = root / (label + '.log')
    with log.open('x') as output:
        child = subprocess.Popen(command, cwd=cwd, env=dict(env, **(extra or {})), stdout=output,
                                 stderr=subprocess.STDOUT, start_new_session=True)
        status.update(phase='running', label=label, child_pid=child.pid, command=list(map(str, command))); save()
        try:
            code = child.wait(timeout=timeout)
        except subprocess.TimeoutExpired:
            os.killpg(child.pid, signal.SIGTERM)
            try:
                child.wait(timeout=10)
            except subprocess.TimeoutExpired:
                os.killpg(child.pid, signal.SIGKILL); child.wait()
            code = 124
    row = {'label': label, 'exit': code, 'elapsed_seconds': time.monotonic() - start,
           'command': list(map(str, command)), 'environment': extra or {}, 'log': str(log)}
    status['steps'].append(row); status.pop('child_pid', None); save()
    if '[NativeScaling2D]' in command and not code and log.read_text().count('All tests passed') != int(command[2]):
        raise RuntimeError('Incomplete Catch report in engine case')
    if code and not allow_failure:
        raise RuntimeError(f'{label} exited {code}; stop before escalation')
    return row

def audit(group):
    run('audit_' + group.name, [sys.executable, root/'audit_engine_scaling.py', group], timeout=600)

save()
try:
    while True:
        try:
            previous = json.loads(prerequisite.read_text())
        except (FileNotFoundError, json.JSONDecodeError):
            time.sleep(5); continue
        if previous.get('phase') == 'terminal':
            if previous.get('exit'):
                raise RuntimeError('Integration prerequisite failed; diagnose before continuing.')
            break
        try:
            os.kill(previous['runner_pid'], 0)
        except ProcessLookupError:
            raise RuntimeError('Prerequisite supervisor disappeared; no assumed completion.')
        time.sleep(5)

    campaign = root/'robustness_campaign_v5'; campaign.mkdir()
    build = root/'build-integrated-v2'; binary = build/'UnitTests/test_driver'
    shutil.copy2(binary, campaign/'test_driver')
    status.update(source_revision=subprocess.check_output(['git', 'rev-parse', 'HEAD'], cwd=source, text=True).strip(),
                  binary_sha256=hashlib.sha256(binary.read_bytes()).hexdigest(),
                  source_status=subprocess.check_output(['git', 'status', '--short'], cwd=source, text=True),
                  cpu_info=subprocess.check_output(['lscpu'], text=True), load_average=Path('/proc/loadavg').read_text())
    pilot = root/'scaling_pilot_v6'
    measures = [json.loads(p.read_text()) for p in pilot.glob('*/native_scaling.json')]
    passing = [t for t in (1, 4, 16, 64) if
               sorted(m['ranks'] for m in measures if m['tiles'] == t and m['complete']) == [1, 2, 4]]
    if not passing:
        raise RuntimeError('No common complete pilot size; diagnose before strong-scaling repetitions.')
    # Increase size until a contract/work or 240-second cost boundary is observed.
    # Audit before escalating. A partial rank matrix is preserved, never called a scaling pass.
    for larger in (128, 256, 512, 1024, 2048, 4096):
        group = campaign/f'large_t{larger}'; group.mkdir()
        measurements = []
        for ranks in (1, 2, 4):
            wd = group/f't{larger}_p{ranks}'; wd.mkdir()
            row = run('large_' + wd.name, ['mpiexec', '-n', str(ranks), binary, '[NativeScaling2D]', '--use-colour', 'no'],
                      cwd=wd, extra={'SU2_NATIVE_SCALING_TILES': str(larger)}, timeout=240, allow_failure=True)
            if row['exit']:
                status['first_large_runtime_limit'] = row; save(); break
            row['measurement'] = json.loads((wd/'native_scaling.json').read_text())
            measurements.append(row['measurement']);save()
        if measurements: audit(group)
        if len(measurements)!=3 or not all(m['complete'] for m in measurements):
            status['first_large_incomplete_tiles'] = larger; save();break
        passing.append(larger)
        if max(m['adapt_seconds'] for m in measurements) >= 120:
            status['large_cost_stop_tiles'] = larger; save();break
    tiles = max(passing); status['largest_common_complete_tiles'] = tiles; save()
    repeated = campaign/'repeated'; repeated.mkdir()
    for layout in (1, 2):
        for ranks in (1, 2, 4):
            for repeat in range(3):
                wd = repeated/f't{tiles}_p{ranks}_layout{layout}_repeat{repeat}'; wd.mkdir()
                row = run('repeat_' + wd.name, ['mpiexec', '-n', str(ranks), binary, '[NativeScaling2D]', '--use-colour', 'no'],
                          cwd=wd, extra={'SU2_NATIVE_SCALING_TILES': str(tiles), 'SU2_NATIVE_SCALING_LAYOUT': str(layout)}, timeout=240)
                if (root/row['log']).read_text().count('All tests passed') != ranks:
                    raise RuntimeError('Incomplete Catch report in repetition')
                row['measurement'] = json.loads((wd/'native_scaling.json').read_text()); save()
    audit(repeated)
    affine = campaign/'affine'; affine.mkdir()
    for ar in (10, 100, 1000):
        for ranks in (1, 2, 4):
            wd = affine/f't4_p{ranks}_ar{ar}'; wd.mkdir()
            row = run('affine_' + wd.name, ['mpiexec', '-n', str(ranks), binary, '[NativeScaling2D]', '--use-colour', 'no'],
                      cwd=wd, extra={'SU2_NATIVE_SCALING_TILES': '4', 'SU2_NATIVE_SCALING_AR': str(ar)}, timeout=240)
            row['measurement'] = json.loads((wd/'native_scaling.json').read_text()); save()
    audit(affine)
    incompatible = campaign/'incompatible'; incompatible.mkdir()
    for ranks in (1, 2, 4):
        wd = incompatible/f't4_p{ranks}_ar25'; wd.mkdir()
        row = run('incompatible_' + wd.name, ['mpiexec', '-n', str(ranks), binary, '[NativeScaling2D]', '--use-colour', 'no'],
                  cwd=wd, extra={'SU2_NATIVE_SCALING_TILES': '4', 'SU2_NATIVE_SCALING_AR': '25', 'SU2_NATIVE_SCALING_MATCHED': '2'}, timeout=240)
        row['measurement'] = json.loads((wd/'native_scaling.json').read_text())
        if row['measurement']['complete']:
            raise RuntimeError('Proved incompatible wall/metric contract reported complete')
        save()
    audit(incompatible)

    # Actual frozen P1 target, conservative transfer and resumed viscous solve.
    inputs = root/'airfoil_inputs'; inputs.mkdir(exist_ok=True)
    portable = (source/'QuickStart/native_NACA0012.cfg').read_text()
    portable = portable.replace('MESH_FILENAME= mesh_NACA0012_inv.su2',
                                'MESH_FILENAME= ' + str(source/'QuickStart/mesh_NACA0012_inv.su2'))
    for name, height, sizes in [('baseline', .0002, '4000, 6000, 3000'), ('complexity12000', .0002, '4000, 12000, 3000'),
                               ('height1e4', .0001, '4000, 6000, 3000'), ('height5e5', .00005, '4000, 6000, 3000')]:
        text = portable
        for option, value in [('ADAP_SIZES', '('+sizes+')'), ('ADAP_HMIN', str(height)), ('ADAP_BL_FIRST_HEIGHT', '('+str(height)+')')]:
            text, count = re.subn('^'+option+'=.*$', option+'= '+value, text, flags=re.M)
            if count != 1:
                raise RuntimeError('Missing or duplicate option '+option)
        (inputs/(name+'.cfg')).write_text(text)

    def airfoil(name, height, ranks):
        label = 'integrated_airfoil_' + name + '_v5'
        row = run(label, [sys.executable, root/'run_native_driver_checks.py', '--label', label,
                         '--build', 'build-integrated-v2', '--timeout', '900', '--filter', '[NativeAirfoil2D]',
                         '--save-audit', '--airfoil-config', inputs/(name+'.cfg'), '--ranks', *map(str, ranks)],
                  timeout=3000, allow_failure=True)
        if not row['exit']:
            run(label+'_audit', [sys.executable, root/'run_native_airfoil_audits.py', root/label,
                                '--label', 'independent_v5', '--height', str(height), '--ranks', *map(str, ranks)], timeout=1800)
        return not row['exit']

    if not airfoil('baseline', .0002, [1, 2, 4]):
        raise RuntimeError('Real-airfoil baseline failed; no demand escalation')
    # Preserve a failed demand as an observed limit; stop that axis before increasing it further.
    airfoil('complexity12000', .0002, [1])
    if airfoil('height1e4', .0001, [1]):
        airfoil('height5e5', .00005, [1])
    # This exact eight-cycle/two-policy fixture already ran in the integrated matrix.
    # Audit those saved outputs instead of repeating an unchanged successful runtime.
    run('integrated_opposing_audit_v5', [sys.executable, root/'run_native_opposing_audits.py',
         root/'integrated_primal_controls_v6', '--label', 'independent_opposing_v5'], timeout=1800)
    status.update(phase='terminal', exit=0, ended=time.time()); save()
except BaseException as error:
    status.update(phase='terminal', exit=1, reason=str(error), ended=time.time())
    status.pop('child_pid', None); save(); raise
