"""Same saved CFD metric entry; composed stage2 target assessment only, no CFD advance or remeshing."""
import json,os,sys,tempfile,time
from pathlib import Path
import run_cases as checks
R=Path(__file__).resolve().parent
mode='metric_stage'
assert json.loads((R/'build371_terminal_handles.json').read_text())['all_owned_handles_terminal']
# Reuse370 MPI primitive/publication milestone:371 changes CSolver only; its scalar solve was skipped there.
# The actual371 metric stage itself runs MPI6 to cover the changed normalization collectives.
for build,check in (('371','serial'),('370','mpi6')):
    attempts=sorted((R/'cases').glob('curved'+build+'_joint_'+check+'_*'),key=lambda p:p.stat().st_mtime)
    assert attempts,check
    status=json.loads((attempts[-1]/'status.json').read_text())
    assert status['state']=='terminal' and status['exit_code']==0 and status['pins_verified'],status
    assert json.loads((attempts[-1]/'terminal_handles.json').read_text())['all_owned_handles_terminal']
case='curved371_saved_cfd_metric_stage'
checks.CASES[case]=(6,'[NativeSavedCFDMetricStage3D]')
scratch=R.parents[3]/'integration_evidence/build-integrated-v2/compile-tmp'
os.environ.update(OMPI_MCA_osc='pt2pt',OMPI_MCA_orte_tmpdir_base=tempfile.mkdtemp(prefix=case+'-',dir=scratch))
seen={};original=checks.sample_tree
own={'pid':os.getpid(),'start_ticks':int(Path('/proc/self/stat').read_text().rsplit(')',1)[1].split()[19])}
def sample(pid):
    rows=original(pid)
    for row in rows:
        try:
            v=Path('/proc',str(row['pid']),'stat').read_text().rsplit(')',1)[1].split()
            row.update(start_ticks=int(v[19]),user_cpu_ticks=int(v[11]),system_cpu_ticks=int(v[12]))
            seen[(row['pid'],int(v[19]))]=dict(pid=row['pid'],start_ticks=int(v[19]))
        except (FileNotFoundError,ProcessLookupError):pass
    return rows
checks.sample_tree=sample
# Reuse run_cases pin/runtime/resource monitoring; retain the established full outer envelope.
import types,shutil
original_process=checks.subprocess

def popen(argv,**kwargs):
    assert argv[argv.index('timeout')+1] in ('600','14400')
    argv[argv.index('timeout')+1]='14400'
    return original_process.Popen(argv,**kwargs)
checks.subprocess=types.SimpleNamespace(run=original_process.run,Popen=popen,STDOUT=original_process.STDOUT)
original_save=checks.save
inputs=R/'m6_bl_inputs/rans_metric_entry364_v1'
provenance=json.loads((inputs/'provenance.json').read_text())
def save_pinned(path,value):
    if Path(path).name=='pins.json':
        attempt=Path(path).parent
        for name,receipt in provenance['files'].items():
            source=inputs/name
            assert checks.sha(source)==receipt['sha256'],source
            destination=attempt/name
            if name=='run.cfg':shutil.copyfile(source,destination)
            else:destination.symlink_to(source)
            value[str(source)]=receipt['sha256'];value[str(destination)]=receipt['sha256']
        os.environ['SU2_NATIVE_FROZEN_CONFIG']=str(attempt/'run.cfg')
        for p in (Path(__file__).resolve(),inputs/'provenance.json',inputs/'qualification.json'):
            value[str(p)]=checks.sha(p)
    if Path(path).name=='command.json':
        assert value['timeout_seconds']==600
        value['timeout_seconds']=14400
        value['argv'][value['argv'].index('timeout')+1]='14400'
    original_save(path,value)
checks.save=save_pinned
sys.argv=['run_cases.py','build371',case]
try:checks.main()
except SystemExit as e:code=e.code or 0
attempt=sorted((R/'cases').glob(case+'_*'),key=lambda p:p.stat().st_mtime)[-1]
live=[]
for row in seen.values():
    try:
        v=Path('/proc',str(row['pid']),'stat').read_text().rsplit(')',1)[1].split()
        if int(v[19])==row['start_ticks']:live.append(row)
    except (FileNotFoundError,ProcessLookupError):pass
receipt=dict(actual_host_proc=True,all_owned_handles_terminal=not live,live_owned_handles=live,
             sampled_identities=list(seen.values()),wrapper_excluded=own,checked_unix=time.time())
(attempt/'terminal_handles.json').write_text(json.dumps(receipt,indent=2)+'\n')
print(json.dumps({'attempt':str(attempt),'exit_code':code,'sampled_terminal':not live}),flush=True)
assert not live,live
raise SystemExit(code)
