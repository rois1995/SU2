"""Fresh Stage G warm/cold, one-rank interruption propagation and native rejection.

Uses a hashed existing converged primal checkpoint on the identical input mesh.
Short iteration budgets validate lifecycle behavior, not goal-estimator accuracy.
Only signal descendants of this runner's own mpiexec launch.
"""
from pathlib import Path
import csv, hashlib, json, math, os, re, shutil, signal, subprocess, sys, time

source = Path(__file__).resolve().parent.parent
root = source/'integration_evidence'
state = root/'goal_runtime_v2.json'
prior = root/'robustness_chain_v3.json'
if state.exists(): raise SystemExit('Preserve previous evidence; choose a new version.')
status = {'runner_pid': os.getpid(), 'phase': 'waiting_for_robustness', 'started': time.time(), 'runs': []}
env = dict(os.environ, OMP_NUM_THREADS='1', OPENBLAS_NUM_THREADS='1', PYTHONDONTWRITEBYTECODE='1')
def save(): state.write_text(json.dumps(status, indent=2)+'\n')
def sha(p): return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def descendants(parent):
    rows = [r.split() for r in subprocess.check_output(['ps','-eo','pid,ppid,comm'], text=True).splitlines()[1:]]
    seen = {parent}
    while True:
        fresh = {int(pid) for pid, ppid, name in rows if int(ppid) in seen}
        if fresh <= seen: break
        seen |= fresh
    return sorted(int(pid) for pid, ppid, name in rows if int(pid) in seen and name == 'SU2_CFD_AD')
def quiet():
    since = None
    while True:
        busy = []
        for line in subprocess.check_output(['ps','-eo','pid,stat,comm'], text=True).splitlines()[1:]:
            pid, flags, name = line.split(maxsplit=2)
            if 'Z' not in flags and pid != '918696' and (name in ('ninja','cc1plus','test_driver','test_driver_AD','test_memory') or name.startswith('SU2_CFD')):
                busy.append(int(pid))
        status.update(phase='waiting_for_machine', busy=busy); save()
        if busy: since = None
        elif since is None: since = time.monotonic()
        elif time.monotonic()-since >= 15: return
        time.sleep(5)

save()
try:
    while True:
        try: p = json.loads(prior.read_text())
        except (FileNotFoundError,json.JSONDecodeError): time.sleep(5); continue
        if p.get('phase') == 'terminal':
            if p.get('exit'): raise RuntimeError('Robustness chain failed; diagnose before further jobs.')
            break
        try: os.kill(p['runner_pid'],0)
        except ProcessLookupError: raise RuntimeError('Prerequisite supervisor disappeared.')
        time.sleep(5)
    binary = root/'build-integrated-ad-v1/SU2_CFD/src/SU2_CFD_AD'
    directory = root/'integrated_goal_runtime_v2'; directory.mkdir()
    old = Path('/media/rausa/4TB/SU2_Versions/Prove/MeshAdaptation/44_stage_g2g3')
    mesh = source/'QuickStart/mesh_NACA0012_inv.su2'
    restart = old/'mesh/solution_flow_conv.dat'
    if sha(mesh) != sha(old/'mesh/mesh_NACA0012_inv.su2'): raise RuntimeError('Checkpoint mesh mismatch')
    base = (old/'base.cfg').read_text()
    shutil.copy2(old/'base.cfg',directory/'input_profile.cfg')
    shutil.copy2(source/'TestCases/adaptation/capability/capcheck.py',directory/'capcheck_source.py')
    values = {'MESH_FILENAME': str(mesh), 'CFL_NUMBER': '50', 'SCREEN_WRT_FREQ_INNER': '1',
              'OUTPUT_FILES': '(RESTART)', 'ITER': '100', 'ADAP_SIZES': '(3000, 4000)',
              'ADAP_SUBITER': '(1)', 'ADAP_FLOW_ITER': '(100)', 'ADAP_FLOW_CFL': '(50)',
              'ADAP_ADJ_ITER': '(150)', 'CONV_RESIDUAL_MINVAL': '-12', 'ADAP_ADJ_WARM_START': 'YES'}
    def config(changes):
        text = base
        for option, value in dict(values, **changes).items():
            text, count = re.subn('^'+option+r'\s*=.*$',option+'= '+value,text,flags=re.M)
            if count > 1: raise RuntimeError('Duplicate config option '+option)
            if not count: text += '\n'+option+'= '+value+'\n'
        return text
    shutil.copy2(binary,directory/'SU2_CFD_AD')
    shutil.copy2(restart,directory/'input_solution_flow.dat')
    shutil.copy2(mesh,directory/'input.su2')
    status.update(source_revision=subprocess.check_output(['git','rev-parse','HEAD'],cwd=source,text=True).strip(),
                  binary_sha256=sha(binary),input_restart_sha256=sha(restart),mesh_sha256=sha(mesh),
                  profile_sha256=sha(old/'base.cfg'),checker_sha256=sha(source/'TestCases/adaptation/capability/capcheck.py'),scope='short lifecycle smoke; no estimator/convergence accuracy claim')
    sys.path.insert(0,str(source/'TestCases/adaptation/capability'))
    import capcheck
    reference = capcheck.read_su2(mesh)
    cases = [(f'{start}_p{p}',p,{'ADAP_ADJ_WARM_START': 'YES' if start == 'warm' else 'NO'},None)
             for start in ('warm','cold') for p in (1,2)]
    cases += [(f'interrupt_{phase}_p{p}',p,{'ITER':'1000000',
                  'ADAP_FLOW_ITER':'(1000000)', 'ADAP_ADJ_ITER':'(1000000)',
                  'CONV_STARTITER':'1000000' if phase == 'primal' else '0',
                  'CONV_RESIDUAL_MINVAL':'-12' if phase == 'primal' else '-9'},phase)
              for phase in ('primal','adjoint') for p in (1,2)]
    cases += [('reject_native_p2',2,{'ADAP_REMESHER':'NATIVE_CAVITY'},'reject')]
    for label, ranks, changes, action in cases:
        quiet(); wd=directory/label; wd.mkdir()
        (wd/'run.cfg').write_text(config(changes)); shutil.copy2(restart,wd/'solution_flow.dat')
        command=['mpiexec','-n',str(ranks),str(binary),'run.cfg']; log=wd/'runtime.log'
        begun=time.monotonic(); sent=None
        with log.open('x') as output:
            child=subprocess.Popen(command,cwd=wd,env=env,stdout=output,stderr=subprocess.STDOUT,start_new_session=True)
            status.update(phase='running',label=label,child_pid=child.pid);save()
            while child.poll() is None and time.monotonic()-begun < 600:
                text=log.read_text()
                primal_end = re.search(r'Primal phase: \d+ iterations',text)
                ready=(action=='primal' and re.search(r'  primal iteration \d+',text)) or (action=='adjoint' and
                       primal_end is not None and 'Direct iteration to store the primal computational graph.' in
                       text[primal_end.end():])
                if ready and sent is None:
                    targets=descendants(child.pid)
                    if targets:
                        os.kill(targets[0],signal.SIGTERM)
                        sent={'pid':targets[0],'seconds':time.monotonic()-begun,'condition':'primal iteration' if action=='primal' else 'adjoint recording banner'}
                        status['signal']=sent;save()
                time.sleep(.05)
            if child.poll() is None:
                os.killpg(child.pid,signal.SIGTERM)
                try: child.wait(timeout=10)
                except subprocess.TimeoutExpired: os.killpg(child.pid,signal.SIGKILL);child.wait()
                code=124
            else: code=child.returncode
        text=log.read_text(); rows=[]
        if (wd/'adap_goal_summary.csv').exists():
            with (wd/'adap_goal_summary.csv').open() as stream: rows=list(csv.DictReader(stream))
        checks={}
        if action=='reject':
            checks={'error_exit':code not in (0,124),'native_diagnostic':'Native adaptation' in text,
                    'before_primal':'Primal phase:' not in text,'no_summary':not rows}
        elif action in ('primal','adjoint'):
            checks={'clean_exit':code==0,'one_rank_signalled':sent is not None,'one_interrupted_cycle':len(rows)==1 and rows[0]['interrupted']=='1',
                    'no_remesh':not list(wd.glob('mesh_out_adap_*.su2')) and bool(rows) and float(rows[0]['t_remesh_swap'])==0,
                    'phase_diagnostic':f'interrupted during the {action} phase' in text,
                    'primal_checkpoint':(wd/'restart_flow_adap_00000.dat').exists()}
            if action=='adjoint':checks['adjoint_checkpoint']=(wd/'restart_adj_cd_adap_00000.dat').exists()
        else:
            checks={'clean_exit':code==0,'three_cycles':len(rows)==3,'no_interrupt':bool(rows) and all(r['interrupted']=='0' for r in rows),
                    'warm_flags':len(rows)==3 and [r['warm_start'] for r in rows]==(['0','1','1'] if label.startswith('warm') else ['0','0','0']),
                    'finite_summary':bool(rows) and all(math.isfinite(float(r[k])) for r in rows for k in ('J','primal_res','adjoint_res','preBL_complexity','final_complexity')),
                    'final_sensitivities':bool(rows) and all(rows[-1].get(k) and math.isfinite(float(rows[-1][k])) for k in ('sens_geo','sens_aoa','sens_mach'))}
            mesh_checks=[]
            for cycle in (1,2):
                path=wd/f'mesh_out_adap_{cycle:05}.su2'
                if not path.is_file(): mesh_checks.append({'error':'missing '+str(path)});continue
                candidate=capcheck.read_su2(path)
                validity=capcheck.check_validity(candidate);markers=capcheck.check_markers(reference,candidate)
                mesh_checks.append({'validity':validity,'markers':markers})
            checks['valid_adapted_meshes']=len(mesh_checks)==2 and all('error' not in m and
                all(v[0]!='FAIL' for group in (m['validity'],m['markers']) for v in group.values()) for m in mesh_checks)
        record={'label':label,'ranks':ranks,'command':command,'exit':code,'elapsed_seconds':time.monotonic()-begun,
                'signal':sent,'checks':checks,'passed':all(checks.values()),'summary':rows,'config_sha256':sha(wd/'run.cfg')}
        (wd/'evidence.json').write_text(json.dumps(record,indent=2)+'\n')
        status['runs'].append(record);status.pop('child_pid',None);status.pop('signal',None);save()
        if not record['passed']:raise RuntimeError(f'{label} failed; preserve evidence and diagnose')
    status.update(phase='terminal',exit=0,ended=time.time());save()
except BaseException as error:
    status.update(phase='terminal',exit=1,reason=str(error),ended=time.time());status.pop('child_pid',None);save();raise
