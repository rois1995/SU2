"""Run staged native production cases after classifying the independent AD solver failure."""
import csv
import hashlib
import json
import math
import os
from pathlib import Path
import shutil
import signal
import subprocess
import sys
import time

root = Path(__file__).resolve().parent
source = root.parent
case = root/'integrated_native_production_v1'
state = root/'native_production_chain_v2.json'
env = dict(os.environ, OMP_NUM_THREADS='1', OPENBLAS_NUM_THREADS='1', PYTHONDONTWRITEBYTECODE='1')


def sha(path):
    digest = hashlib.sha256()
    with Path(path).open('rb') as stream:
        for block in iter(lambda: stream.read(1024*1024), b''):
            digest.update(block)
    return digest.hexdigest()


def flow_rows(points, fields):
    names = ('Density', 'Momentum_x', 'Momentum_y', 'Energy')
    if any(name not in fields for name in names):
        raise ValueError('Missing conserved flow fields in actual restart')
    return [dict(point_id=i, x=p[0], y=p[1], **dict(zip(
        ('density', 'momentum_x', 'momentum_y', 'energy_density'), (fields[k][i] for k in names))))
        for i, p in enumerate(points)]


def main():
    from check_airfoil_phase_timing import validate_solution
    if sys.argv[1:] == ['--self-check']:
        fields = {'Density': [1.], 'Momentum_x': [.1], 'Momentum_y': [0.], 'Energy': [2.]}
        validate_solution(flow_rows([(0., 0.)], fields), [(0., 0.)])
        try:
            validate_solution(flow_rows([(1., 0.)], fields), [(0., 0.)])
        except ValueError:
            print('Restart-to-mesh mapping self-check PASS'); return
        raise AssertionError('A restart mapped to the wrong mesh passed')
    if sys.argv[1:]:
        raise SystemExit('Use no arguments to execute, or --self-check.')
    if state.exists() or (case/'evidence.json').exists():
        raise SystemExit('Preserve prior evidence; do not restart this version.')
    initial_runner_sha = sha(Path(__file__))
    initial_checker_sha = sha(root/'check_airfoil_phase_timing.py')
    status = dict(runner_pid=os.getpid(), phase='checking_classified_lifecycle', started=time.time(), steps=[],
                  runner_sha256=initial_runner_sha, flow_checker_sha256=initial_checker_sha)
    def save():
        state.write_text(json.dumps(status, indent=2)+'\n')
    def quiet():
        since = None
        while True:
            busy = []
            for line in subprocess.check_output(['ps', '-eo', 'pid,stat,comm'], text=True).splitlines()[1:]:
                pid, flags, name = line.split(maxsplit=2)
                if 'Z' not in flags and pid != '918696' and (name in ('ninja', 'cc1plus', 'test_driver', 'test_driver_AD', 'test_memory') or name.startswith('SU2_CFD')):
                    busy.append(int(pid))
            status.update(phase='waiting_for_machine', busy=busy, checked=time.time()); save()
            if busy: since = None
            elif since is None: since = time.monotonic()
            elif time.monotonic()-since >= 15: return
            time.sleep(5)
    def run(label, command, cwd=source, timeout=1800):
        quiet(); command=list(map(str, command)); log=case/(label+'.log'); begun=time.monotonic()
        with log.open('x') as output:
            child=subprocess.Popen(command, cwd=cwd, env=env, stdout=output, stderr=subprocess.STDOUT, start_new_session=True)
            status.update(phase='running', label=label, child_pid=child.pid, working_directory=str(cwd)); save()
            try: code=child.wait(timeout=timeout)
            except subprocess.TimeoutExpired:
                os.killpg(child.pid, signal.SIGTERM)
                try: child.wait(timeout=10)
                except subprocess.TimeoutExpired: os.killpg(child.pid, signal.SIGKILL); child.wait()
                code=124
        row=dict(label=label, command=command, working_directory=str(cwd), exit=code, elapsed_seconds=time.monotonic()-begun, log=str(log))
        status['steps'].append(row); status.pop('child_pid', None); save()
        if code: raise RuntimeError(label+' failed; classify preserved output before further jobs')
        return row
    save()
    try:
        classification_path=root/'goal_cold_failure_classification_v1.json'
        classification=json.loads(classification_path.read_text())
        if not classification.get('diagnosis_complete') or not classification.get('primal_production_validation_independent'):
            raise RuntimeError('Independent AD failure has not been diagnosed')
        for path,digest in classification['input_files_sha256'].items():
            if sha(path)!=digest: raise RuntimeError('Failure-classification input changed')
        status['classification_sha256']=sha(classification_path);save()
        if sha(Path(__file__))!=initial_runner_sha or sha(root/'check_airfoil_phase_timing.py')!=initial_checker_sha:
            raise RuntimeError('Loaded runner/checker changed while waiting; preserve and reassess this version')
        prepared=json.loads((case/'prepared.json').read_text())
        old=json.loads((root/'ad_repair_controls_v14.json').read_text())
        production={p: digest for p, digest in old['source_sha256'].items() if p.startswith(('Common/', 'SU2_CFD/'))}
        if len(production)!=759 or any(sha(source/p)!=digest for p,digest in production.items()):
            raise RuntimeError('Production sources changed; revalidate the application executable before running')
        quiet()
        binary=root/'build-integrated-v2/SU2_CFD/src/SU2_CFD'
        shutil.copy2(binary, case/'SU2_CFD'); binary=case/'SU2_CFD'
        status.update(binary_sha256=sha(binary), source_revision=subprocess.check_output(['git','rev-parse','HEAD'],cwd=source,text=True).strip(), source_sha256=production)
        save()
        sys.path.insert(0, str(source/'TestCases/adaptation/capability'))
        import capcheck
        from audit_native_bl import mesh
        import numpy as np
        manifest=dict(runner_pid=os.getpid(), source_revision=status['source_revision'], source_sha256=production,
                      binary_sha256=status['binary_sha256'], archived_binary=str(binary), sequential=True, maximum_ranks=4,
                      environment={k:env[k] for k in ('OMP_NUM_THREADS','OPENBLAS_NUM_THREADS')},
                      scope='Actual native SU2_CFD main-loop/restart/ParaView/metric/mesh outputs; short viscous budgets, not converged CFD accuracy', runs=[], numpy_version=np.__version__)
        manifest['runner_sha256']=initial_runner_sha; manifest['flow_checker_sha256']=initial_checker_sha
        tools_archive=case/'auditor_sources';tools_archive.mkdir()
        for tool in [Path(__file__),root/'check_airfoil_phase_timing.py',root/'audit_native_bl.py',root/'frozen_field_audit.py',source/'TestCases/adaptation/capability/capcheck.py']:
            shutil.copy2(tool,tools_archive/tool.name)
        manifest['auditor_sources_sha256']={p.name:sha(p) for p in tools_archive.iterdir()}
        shutil.copy2(case/'np1/input.su2', case/'airfoil_input.su2')
        readme=case/'README.md'
        readme.write_text(readme.read_text().replace('Prepared native production-driver testcase — NOT RUN',
                          'Native production-driver testcase — inspect evidence.json for actual status').replace(
                          'Execution is deferred until the current robustness/AD/follow-through pipelines\nfinish.',
                          'The AD solver failure was classified by a static-mesh reproduction; this independent primal output gate runs sequentially.'))
        manifest['input_files_sha256']={}; report=case/'evidence.json'
        for ranks in (1,2,4):
            wd=case/f'np{ranks}'
            for name, expected in [('input.su2',prepared['mesh_sha256']),('run.cfg',prepared['config_sha256'])]:
                if sha(wd/name)!=expected: raise RuntimeError('Prepared case input changed: '+str(wd/name))
                manifest['input_files_sha256'][str(wd/name)]=expected
            manifest['current_run']=dict(ranks=ranks, working_directory=str(wd)); report.write_text(json.dumps(manifest,indent=2)+'\n')
            row=run(f'np{ranks}', ['mpiexec','-n',ranks,binary,'run.cfg'],cwd=wd,timeout=900)
            outputs={}; audit_dir=case/f'audit_np{ranks}'; audit_dir.mkdir()
            for cycle in range(4):
                donor=wd/'input.su2' if cycle==0 else wd/f'mesh_adap_{cycle:05}.su2'
                if cycle and (not Path(str(donor)+'.native_ref').exists() or not Path(str(donor)+'.native_ref').stat().st_size):
                    raise RuntimeError('Missing accepted immutable-reference sidecar')
                candidates=list(wd.glob(f'solution*_adap_{cycle:05}.dat'))
                if len(candidates)!=1: raise RuntimeError('Missing or ambiguous actual cycle restart')
                restart=candidates[0]; points,fields,precision=capcheck.read_restart(restart)
                expected_points,_,_=mesh(donor)
                summary=validate_solution(flow_rows(points,fields),expected_points)
                if precision!='Float64' or not np.isfinite(fields['Pressure']).all() or not (fields['Pressure']>0).all():
                    raise RuntimeError('Invalid actual primitive pressure or restart precision')
                tensors,_=capcheck.metric_of(capcheck.read_su2(donor),restart)
                if not np.isfinite(tensors).all() or not (tensors[:,0,0]>0).all() or not (np.linalg.det(tensors)>0).all():
                    raise RuntimeError('Invalid actual restart metric')
                vtu=wd/f'flow_adap_{cycle:05}.vtu'
                vp,vf,_=capcheck.read_vtu(vtu)
                required=('Density','Pressure','Metric_XX','Metric_XY','Metric_YY')
                scale=float(np.ptp(np.asarray(expected_points),axis=0).max())
                if len(vp)!=len(expected_points) or any(k not in vf for k in required) or not all(np.isfinite(v).all() for v in vf.values()) or np.max(np.abs(vp[:,:2]-np.asarray(expected_points)))>1e-6*scale:
                    raise RuntimeError('Invalid actual ParaView field output or mesh pairing')
                outputs[cycle]=dict(restart=str(restart), restart_sha256=sha(restart), mesh=str(donor), mesh_sha256=sha(donor),vtu=str(vtu),vtu_sha256=sha(vtu),flow=summary)
                if cycle<3:
                    prefix=audit_dir/f'native_airfoil_cycle_{cycle}'
                    shutil.copy2(donor,Path(str(prefix)+'_donor.su2'))
                    shutil.copy2(wd/f'mesh_adap_{cycle+1:05}.su2',Path(str(prefix)+'_adapted.su2'))
                    with Path(str(prefix)+'_metric.csv').open('x') as stream:
                        writer=csv.writer(stream);writer.writerow(['xx','xy','yy'])
                        writer.writerows([[format(m[0,0],'.17g'),format(m[0,1],'.17g'),format(m[1,1],'.17g')] for m in tensors])
            row.update(ranks=ranks, exit_code=0, verified=True, outputs=outputs); manifest['runs'].append(row)
            manifest.pop('current_run',None); report.write_text(json.dumps(manifest,indent=2)+'\n')
            print(f'Production np{ranks}: actual mesh/flow/metric outputs PASS',flush=True)
        if sha(binary)!=manifest['binary_sha256']: raise RuntimeError('Archived executable changed')
        run('independent_mesh_audits',[sys.executable,root/'run_native_airfoil_audits.py',case,'--label','independent_v1','--height','.0002'],timeout=1800)
        status.update(phase='terminal',exit=0,ended=time.time());save()
    except BaseException as error:
        status.update(phase='terminal',exit=1,reason=str(error),ended=time.time());status.pop('child_pid',None);save();raise


if __name__=='__main__':
    main()
