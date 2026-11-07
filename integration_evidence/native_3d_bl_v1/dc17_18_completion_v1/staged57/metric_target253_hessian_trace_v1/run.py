"""Mach-only production L1 target assessment; never invokes remeshing."""
import hashlib,json,os,subprocess,sys,time
from pathlib import Path
HERE=Path(__file__).resolve().parent
ROOT=HERE.parent
sys.path.insert(0,str(ROOT))
from run_cases import sample_tree
from run_m6 import CFD,sha
pins=json.loads((HERE/'pins.json').read_text());pins[str(Path(__file__).resolve())]=sha(__file__)
(HERE/'assessment_pins.json').write_text(json.dumps(pins,indent=2)+'\n')
def check():
    changed=[p for p,h in pins.items() if sha(p)!=h]
    if changed:raise RuntimeError('Frozen inputs changed: '+str(changed[:5]))
check()
for norm in (1,):
    folder=HERE/f'L{norm}';status=folder/'status.json'
    if status.exists():
        prior=json.loads(status.read_text())
        if prior.get('state')=='terminal' and prior.get('exit_code')==0 and prior.get('pins_verified'):
            check()
            if sha(folder/'restart_flow.dat')!=prior['restart_sha256']:raise RuntimeError('Changed completed metric output')
            print('Reusing independently preserved L'+str(norm),flush=True);continue
        raise RuntimeError('Inspect existing owned handle before retrying '+str(folder))
    env=os.environ.copy();env.update(OMP_NUM_THREADS='1',OPENBLAS_NUM_THREADS='1',MKL_NUM_THREADS='1',OMPI_MCA_osc='pt2pt')
    scratch=ROOT.parents[3]/'integration_evidence/build-integrated-v2/compile-tmp'/('metric253_hessian_trace_L'+str(norm))
    scratch.mkdir(exist_ok=False);env.update(TMPDIR=str(scratch),OMPI_MCA_orte_tmpdir_base=str(scratch))
    argv=['nice','-n','19','/usr/bin/time','-v','timeout','900','mpirun','--output-filename','rank_output','-n','6',str(CFD),'run.cfg']
    (folder/'command.json').write_text(json.dumps({'argv':argv,'cwd':str(folder),'environment':{k:env[k] for k in ('OMP_NUM_THREADS','OPENBLAS_NUM_THREADS','MKL_NUM_THREADS','TMPDIR','OMPI_MCA_orte_tmpdir_base','OMPI_MCA_osc')},'load':Path('/proc/loadavg').read_text(),'meminfo':Path('/proc/meminfo').read_text()},indent=2)+'\n')
    started=time.time();check()
    with (folder/'run.log').open('x') as log,(folder/'resources.jsonl').open('x') as resources:
        proc=subprocess.Popen(argv,cwd=folder,env=env,stdout=log,stderr=subprocess.STDOUT,start_new_session=True)
        (status).write_text(json.dumps({'state':'running','pid':proc.pid,'started_at':started,'helper_pid':os.getpid()})+'\n')
        print('L'+str(norm)+' RUNNING '+str(proc.pid),flush=True)
        while proc.poll() is None:
            rows=sample_tree(proc.pid)
            for row in rows:
                try:row['startticks']=int((Path('/proc')/str(row['pid'])/'stat').read_text().rsplit(')',1)[1].split()[19])
                except FileNotFoundError:pass
            resources.write(json.dumps({'elapsed':time.time()-started,'processes':rows,'summed_rss':sum(r['rss_bytes'] for r in rows)})+'\n');resources.flush();time.sleep(2)
        code=proc.wait()
    check();text=(folder/'run.log').read_text(errors='replace')
    if code==0 and ('NATIVE3D rank=' in text or 'Native3D rejected event' in text):raise RuntimeError('Unexpected remeshing in target assessment')
    result={'state':'terminal','exit_code':code,'seconds':time.time()-started,'pins_verified':True,'restart_sha256':sha(folder/'restart_flow.dat') if (folder/'restart_flow.dat').is_file() else None}
    status.write_text(json.dumps(result,indent=2)+'\n');print('L'+str(norm)+' '+json.dumps(result),flush=True)
    if code or not result['restart_sha256']:raise SystemExit(code or 1)
print('MACH-ONLY L1 TARGET ASSESSMENT TERMINAL',flush=True)
