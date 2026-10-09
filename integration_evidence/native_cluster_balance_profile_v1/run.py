"""One-allocation serial frozen control/profile pilot with verified compact exports."""
import argparse
import csv
import json
import math
import os
from pathlib import Path
import re
import shutil
import signal
import subprocess
import sys
import tempfile
import time
from prepare import ROOT, PACK, CHECKPOINT, closed, sha, verify
from analyze import profile, compare
sys.path.insert(0, str(ROOT / 'integration_evidence'))
from collect_native_cluster_results import TOOLS
from native_process_sampler import compute_processes

KINDS = ('frozen_euler_to_bl', 'frozen_bl_to_euler')
VARIANTS = ((4, 'NO'), (4, 'YES'), (3, 'YES'), (2, 'YES'))
UNITS = '[NativeBalanceProfile2D],[NativeEngine2D],[NativeDistributed2D],[NativeField2D],[PassiveComm]'


def execute(command, case, environment, timeout):
    samples = []
    started = time.monotonic()
    with (case / 'solver.log').open('x') as log:
        child = subprocess.Popen(command, cwd=case, env=environment, stdout=log,
                                 stderr=subprocess.STDOUT, start_new_session=True)
        while True:
            try:
                code = child.wait(timeout=5)
                break
            except subprocess.TimeoutExpired:
                processes = compute_processes(case)
                owned = [p for p in processes if p['owned_solver']]
                samples.append(dict(elapsed_seconds=time.monotonic()-started, host=os.uname().nodename,
                                    loadavg=Path('/proc/loadavg').read_text().strip(),
                                    other_compute_processes=sum(not p['owned_solver'] for p in processes),
                                    owned_rank_affinity=[p['cpu_allowed_list'] for p in owned],
                                    owned_rank_rss_kib=[p['memory_kib'].get('VmRSS', 0) for p in owned]))
                pressure = Path('/proc/pressure/cpu')
                samples[-1]['cpu_pressure'] = pressure.read_text().strip() if pressure.exists() else None
                if time.monotonic() - started >= timeout:
                    os.killpg(child.pid, signal.SIGTERM)
                    try:child.wait(timeout=10)
                    except subprocess.TimeoutExpired:os.killpg(child.pid, signal.SIGKILL);child.wait()
                    code = 124
                    break
    return dict(exit_code=code, wall_seconds=time.monotonic()-started, command=command, machine_samples=samples)


def retained(case):
    return [p for p in case.iterdir() if p.is_file() and
            (p.name in ('run.cfg', 'solver.log', 'run_evidence.json', 'native_frozen_adapted.su2',
                        'independent_frozen_metric_audit.json', 'independent_profile_accounting.json') or p.name.startswith(('native_frozen_', 'native_balance_'))
             or 'reject' in p.name or p.name.endswith('.native_ref'))]


def compact(case, destination, fixture, objects):
    destination.mkdir()
    for name in ('input.su2', 'frozen_sensor.csv', 'frozen_sensor_source_flow.vtu'):
        assert sha(case/name)==sha(fixture/name), 'Frozen input changed during the run: '+name
    paths = retained(case) + [fixture / name for name in ('input.su2', 'frozen_sensor.csv', 'frozen_sensor_source_flow.vtu')]
    hashes = {}
    for source in paths:
        digest = sha(source)
        shared = objects / digest
        if not shared.exists():shutil.copy2(source, shared)
        assert sha(shared) == digest
        target = destination / source.name
        os.link(shared, target)
        assert sha(target) == digest
        hashes[target.name] = dict(sha256=digest, bytes=target.stat().st_size)
    manifest = dict(status='VERIFIED_COMPACT_EXPORT', files=hashes,
                    scope='Self-contained case files; repeated content hardlinked only within this job export. Temporary work removed after verification. Audit tools shared once at job root.')
    (destination / 'collection_manifest.json').write_text(json.dumps(manifest, indent=2) + '\n')
    return hashes


def export_and_clean(case, destination, fixture, objects):
    hashes = compact(case, destination, fixture, objects)
    # Never erase the working folder if copying or hash verification raises.
    shutil.rmtree(case)
    return hashes


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--machinefile', type=Path, required=True)
    parser.add_argument('--repeat', type=int, default=1)
    parser.add_argument('--timeout', type=int, default=7200)
    args = parser.parse_args()
    assert args.repeat >= 1 and args.timeout > 0 and int(os.environ['NSLOTS']) == 4
    job = os.environ['JOB_ID'];assert re.fullmatch(r'[0-9]+', job)
    checkpoint = json.loads(closed(CHECKPOINT).read_text());verify(checkpoint)
    out = ROOT / 'ClusterResults' / ('balance_profile_' + job)
    out.mkdir(parents=True, exist_ok=False)
    (out / 'cases').mkdir();objects=out / 'objects';objects.mkdir()
    shutil.copy2(CHECKPOINT, out / 'checkpoint.json')
    shutil.copy2(closed(args.machinefile), out / 'machinefile')
    for name in TOOLS:
        target = out / 'tools' / name;target.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(closed(ROOT / name), target)
    for script in PACK.glob('*.py'):shutil.copy2(script, out / script.name)
    for command in (['lscpu'], ['gcc', '--version'], ['mpicxx', '--version'], ['mpirun', '--version']):
        result = subprocess.run(command, stdout=subprocess.PIPE, stderr=subprocess.PIPE, universal_newlines=True)
        (out / (command[0] + '.txt')).write_text(result.stdout + result.stderr)
    fixtures = {}
    for kind in KINDS:
        source = ROOT / 'integration_evidence/native_cluster_campaign_v1/inputs' / kind
        dest = out / 'inputs' / kind;dest.mkdir(parents=True)
        for p in source.iterdir():
            p=closed(p); shared=objects/sha(p)
            if not shared.exists():shutil.copy2(p, shared)
            assert sha(shared)==sha(p)
            os.link(shared, dest/p.name)
        fixtures[kind] = dest
    records = dict(status='RUNNING', cases=[], unit_stages=[], checkpoint_sha256=sha(CHECKPOINT),
                   scheduler={k:os.environ.get(k) for k in ('JOB_ID', 'NSLOTS', 'QUEUE', 'PE')},
                   scope='Frozen-only same-allocation paired instrumentation experiment. No time-marching CFD. Numerical PASS and byte identity precede performance interpretation.')
    def save(): (out / 'validation.json').write_text(json.dumps(records, indent=2) + '\n')
    base_env = dict(os.environ, OMP_NUM_THREADS='1', OPENBLAS_NUM_THREADS='1', MKL_NUM_THREADS='1',
                    PYTHONDONTWRITEBYTECODE='1', SU2_NATIVE_BALANCE_PROFILE='NO')
    binaries = {role:closed(ROOT / v['path']) for role,v in checkpoint['binaries'].items()}
    def mpi(binary, ranks, tail, frozen=False):
        exports=['-x', 'SU2_NATIVE_BALANCE_PROFILE']
        if frozen:
            exports+=['-x', 'SU2_NATIVE_AIRFOIL_CONFIG', '-x', 'SU2_NATIVE_FROZEN_METRIC']
        return ['mpirun', '-n', str(ranks), '-machinefile', str(out/'machinefile')] + exports + [str(binary)] + tail
    save()
    try:
        for ranks in (1, 2, 4):
            stage = out / ('unit_n' + str(ranks));stage.mkdir()
            print('Unit test folder:', stage, flush=True)
            result=execute(mpi(binaries['profile'], ranks, [UNITS, '--use-colour', 'no']), stage, base_env, args.timeout)
            result.update(ranks=ranks, status='PASS' if result['exit_code']==0 and (stage/'solver.log').read_text().count('All tests passed')==ranks else 'FAIL')
            result['log_sha256']=sha(stage/'solver.log')
            records['unit_stages'].append(result);save()
            if result['status']=='PASS':
                assert sha(stage/'solver.log')==result['log_sha256']
                for artifact in stage.iterdir():
                    if artifact.name!='solver.log':
                        if artifact.is_dir():shutil.rmtree(artifact)
                        else:artifact.unlink()
            assert result['status']=='PASS', 'Candidate unit gate failed; inspect '+str(stage)
        for rep in range(1, args.repeat+1):
            kinds=KINDS if rep % 2 else KINDS[::-1]
            variants=VARIANTS[(rep-1)%4:]+VARIANTS[:(rep-1)%4]
            for kind in kinds:
                for workers, repartition in variants:
                    roles=('control', 'profile') if rep % 2 else ('profile', 'control')
                    for role in roles:
                        verify(checkpoint)
                        name=f'{kind}_n4_m{workers}_p{repartition}_r{rep}_{role}'
                        print('Retained case folder:', out / 'cases' / name, flush=True)
                        case=Path(tempfile.mkdtemp(prefix=name+'_', dir=out))
                        records['pending_work_folder']=str(case.relative_to(out));save()
                        print('Case working folder:', case, flush=True)
                        for p in fixtures[kind].iterdir():shutil.copy2(p, case/p.name)
                        cfg=(case/'run.cfg').read_text()
                        cfg=re.sub(r'^ADAP_NATIVE_(?:RANKS|REPARTITION)\s*=.*\n?', '', cfg, flags=re.M)
                        cfg=re.sub(r'^MESH_FILENAME\s*=.*$', 'MESH_FILENAME= input.su2', cfg, flags=re.M)
                        cfg+=f'\nADAP_NATIVE_REPARTITION= {repartition}\nADAP_NATIVE_RANKS= {workers}\n'
                        (case/'run.cfg').write_text(cfg)
                        env=dict(base_env, SU2_NATIVE_AIRFOIL_CONFIG=str(case/'run.cfg'),
                                 SU2_NATIVE_FROZEN_METRIC=str(case/'frozen_sensor.csv'),
                                 SU2_NATIVE_BALANCE_PROFILE='YES' if role=='profile' else 'NO')
                        command=mpi(binaries[role], 4, ['[NativeFrozenAirfoil2D]', '--use-colour', 'no'], frozen=True)
                        evidence=dict(phase='preparing', command=command, inputs_sha256={p.name:sha(p) for p in case.iterdir()},
                                      binary_sha256=checkpoint['binaries'][role]['sha256'])
                        (case/'run_evidence.json').write_text(json.dumps(evidence, indent=2)+'\n')
                        outcome=execute(command, case, env, args.timeout)
                        evidence.update(outcome, phase='terminal');(case/'run_evidence.json').write_text(json.dumps(evidence, indent=2)+'\n')
                        row=dict(case=name, role=role, kind=kind, workers=workers, repartition=repartition, repeat=rep,
                                 status='FAIL', outcome=outcome)
                        try:
                            assert outcome['exit_code']==0 and (case/'solver.log').read_text().count('All tests passed')==4, 'Frozen solver/test failed'
                            checker=out/'tools/integration_evidence/audit_native_frozen_case.py'
                            audit_cmd=[sys.executable, str(checker), str(case), 'rae_'+kind[7:], '--self-contained']
                            checked=subprocess.run(audit_cmd, env=base_env, stdout=subprocess.PIPE, stderr=subprocess.PIPE, universal_newlines=True)
                            row['audit_stdout']=checked.stdout;row['audit_stderr']=checked.stderr
                            assert checked.returncode==0, 'Independent frozen audit failed'
                            row['audit']=json.loads((case/'independent_frozen_metric_audit.json').read_text())
                            assert row['audit']['status']=='PASS'
                            times=[float(next(csv.DictReader(p.read_text().splitlines()))['remesh_max_seconds']) for p in case.glob('native_frozen_timing_rank_*.csv')]
                            assert len(times)==4 and all(math.isfinite(v) and v>0 for v in times)
                            row['remesh_seconds']=max(times)
                            sys.path.insert(0, str(out/'tools/integration_evidence'))
                            from audit_native_profile_accounting import inspect
                            accounting=inspect((case/'solver.log').read_text())
                            assert len(accounting['worker_profiles'])==1
                            (case/'independent_profile_accounting.json').write_text(json.dumps(accounting, indent=2)+'\n')
                            row['timing_scopes']=accounting
                            if role=='profile':
                                row['balance']=profile(case, workers)
                                totals=[r['private_wall_seconds'] for r in row['balance']]
                                printed=accounting['worker_profiles'][0]['private reconstruction']
                                for label, value in zip(('min','mean','max'), (min(totals),sum(totals)/workers,max(totals))):
                                    assert abs(printed[label]-value)<=max(1e-6,1e-5*value), 'CSV/log private timing mismatch'
                                # Cache requests/evaluations count private work only; other query stages remain in the existing totals.
                                counts=re.search(r'^Native operations .*?:([^;]+);', (case/'solver.log').read_text(), re.M)
                                assert counts is not None
                                expected=list(map(int,counts[1].split()))
                                assert len(expected)==8
                                assert expected==[sum(int(r['operations'][i]['committed']) for r in row['balance']) for i in range(8)], 'CSV/log commit mismatch'
                                row['balance_summary']=dict(private_wall_max_mean=max(totals)/(sum(totals)/workers) if sum(totals) else 0,
                                    longest_private_seconds=max(r['longest_private_seconds'] for r in row['balance']))
                            else:assert not list(case.glob('native_balance_*.csv')), 'Control unexpectedly emitted profiling files'
                            row['status']='PASS'
                        except Exception as error:row['error']=str(error)
                        copied=export_and_clean(case, out/'cases'/name, fixtures[kind], objects)
                        records.pop('pending_work_folder',None)
                        row['output_sha256']={k:v['sha256'] for k,v in copied.items()}
                        row['retained_bytes_if_expanded']=sum(v['bytes'] for v in copied.values())
                        records['cases'].append(row);save()
                        assert row['status']=='PASS', row.get('error')
                        if len(records['cases'])%2==0:
                            records['pairs']=compare(records['cases']);save()
                            assert all(p['numerical_outputs_identical'] for p in records['pairs']), 'Profiling changed frozen numerical outputs; campaign stopped'
        pairs=compare(records['cases']);records['pairs']=pairs
        assert all(p['numerical_outputs_identical'] for p in pairs), 'Instrumentation changed frozen grid/transport output; do not compare timings'
        verify(checkpoint)
        records['status']='PASS'
    except Exception as error:
        records.update(status='FAIL', error=str(error))
        if 'pending_work_folder' in records:
            print('Unverified working files retained:', out/records['pending_work_folder'], flush=True)
    # Object names are implementation staging only; visible hardlinked case files retain the data.
    shutil.rmtree(objects)
    records['retained_logical_bytes']=sum(p.stat().st_size for p in out.rglob('*') if p.is_file())
    seen=set();size=0
    for p in out.rglob('*'):
        if p.is_file():
            stat=p.stat();key=(stat.st_dev, stat.st_ino)
            if key not in seen:seen.add(key);size+=stat.st_size
    records['retained_unique_file_bytes']=size
    save()
    print(records['status'], len(records['cases']), 'cases;', out, flush=True)
    return 0 if records['status']=='PASS' else 1


if __name__=='__main__':sys.exit(main())
