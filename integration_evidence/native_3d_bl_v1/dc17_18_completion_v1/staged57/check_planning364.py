"""Pinned focused serial / one MPI milestone for sole-destination routing."""
import json,os,sys,tempfile,time
from pathlib import Path
import run_cases as checks
R=Path(__file__).resolve().parent
mode=sys.argv[1];assert mode in ('serial','mpi6')
assert json.loads((R/'build364_terminal_handles.json').read_text())['all_owned_handles_terminal']
case='curved364_height_callers_'+mode
filters={'serial':'[NativeSurfacePlanning3D],[NativeSurfacePreparationEngine3D],[NativeFirstHeightPreservation3D],[NativeBLDirectionInvalidation3D]', 'mpi6':'[NativeSurfacePlanning3D],[NativeSurfacePreparationEngine3D],[NativeCurvedMappedGuards3D],[NativeFirstHeightPreservation3D],[NativeBLDirectionInvalidation3D],[NativeCurvedFirstHeightBudget3D],[NativeCurvedFirstChild3D],[NativeFeaturePublication3D],[NativeCoordinateStar3D]'}
checks.CASES[case]=(6 if mode=="mpi6" else 1,filters[mode])
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
    if Path(path).name=='pins.json':
        value[str(Path(__file__).resolve())]=checks.sha(Path(__file__))
        parser=R.parents[3]/'TestCases/adaptation/capability/capcheck.py'
        value[str(parser.resolve())]=checks.sha(parser)
    original_save(path,value)
checks.save=save_pinned
sys.argv=['run_cases.py','build364',case]
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
