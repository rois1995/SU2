"""Pinned actual original-M6 cavity geometry experiment; no production metric acceptance claim."""
import json,os,sys,tempfile,time
from pathlib import Path
import run_cases as checks
R=Path(__file__).resolve().parent
mode='mpi6'
assert mode in ('serial','mpi6')
assert json.loads((R/'build393_terminal_handles.json').read_text())['all_owned_handles_terminal']
selected='tangential_target'
case='curved393_prepared_'+mode
checks.CASES[case]=(6,'[NativeGeometryBackend3D],[NativeClosestWitness3D],[NativeClosestPrepared3D],[NativeClosestDistance3D],[NativeBLEdgeExclusion3D],[NativeCurvedFirstHeightBudget3D],[NativeFirstHeightPreservation3D],[NativeBLDirectionInvalidation3D],[NativeFeaturePublication3D],[NativeCoordinateStar3D],[NativeAtlasWorkStop3D],[NativeDeadline3D],Native tetrahedral MPI migrates to M workers and returns accepted records to N ranks,Native tetrahedral MPI rejected versions rank veto budgets and shared coordinates preserve both stores,Native MPI donor imports preserve original P1 sensors and actual-query composition,[NativeDonorReuse3D]')
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
original_save=checks.save
def save_pinned(path,value):
    if Path(path).name=='pins.json':value[str(Path(__file__).resolve())]=checks.sha(Path(__file__))
    original_save(path,value)
checks.save=save_pinned
sys.argv=['run_cases.py','build393',case]
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
