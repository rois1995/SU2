"""Replay stage3 with qualified bounded correction, same complete441 stage0 target and saved CFD seed."""
import json,os,sys,tempfile,time
from pathlib import Path
import run_cases as checks
R=Path(__file__).resolve().parent
assert json.loads((R/'build445_terminal_handles.json').read_text())['all_owned_handles_terminal']
assert json.loads((R/'cavity_correction445_qualification.json').read_text())['operator_mpi_qualified']
case='curved445_corrected_cavity_saved_target_surface_mpi6'
checks.CASES[case]=(6,'[NativeSavedTargetSurfaceStage3D]')
source=R/'cases/curved441_ramp5_metric_stage_20261112T124340_1300487'
assert json.loads((R/'target_checkpoint441_qualification.json').read_text())['actual_target_checkpoint_qualified']
state=json.loads((source/'status.json').read_text());assert state['state']=='terminal' and state['exit_code']==0 and state['pins_verified']
assert json.loads((source/'terminal_handles.json').read_text())['all_owned_handles_terminal']
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
# Preserve the existing600s stage-diagnostic outer envelope; no longer whole-workflow timeout.
import shutil
original_save=checks.save
inputs=R/'cases/curved441_ramp5_seed_stage_20261112T123956_1290282'
provenance=json.loads((inputs/'provenance.json').read_text())
def save_pinned(path,value):
    if Path(path).name=='pins.json':
        attempt=Path(path).parent
        for name,receipt in provenance['files'].items():
            seed_file=inputs/name
            assert checks.sha(seed_file)==receipt['sha256'],seed_file
            destination=attempt/name
            if name=='run.cfg':shutil.copyfile(seed_file,destination)
            else:destination.symlink_to(seed_file)
            value[str(seed_file)]=receipt['sha256'];value[str(destination)]=receipt['sha256']
        os.environ['SU2_NATIVE_FROZEN_CONFIG']=str(attempt/'run.cfg')
        # Existing actual M6 contexts, retained unchanged; no synthetic replacement.
        context=R/'m6_bl_inputs/target_chord373_v1/contexts.dat'
        if not context.exists():
            raise RuntimeError('Resolve the preserved actual context filename before launching.')
        os.environ['SU2_NATIVE_BL_CAVITY_CONTEXTS']=str(context)
        value[str(context)]=checks.sha(context)
        if source:
            receipt=json.loads((R/'target_checkpoint441_producer.json').read_text())
            for name,info in receipt['files'].items():
                file=source/name;assert checks.sha(file)==info['sha256'];value[str(file)]=info['sha256']
            value[str(R/'target_checkpoint441_qualification.json')]=checks.sha(R/'target_checkpoint441_qualification.json')
            os.environ['SU2_NATIVE_FROZEN_SENSOR3D']=str(source)
            for rank in range(6):
                for name in ('target_rank'+str(rank)+'.bin','target_witness_rank'+str(rank)+'.txt'):
                    file=source/name;value[str(file)]=checks.sha(file)
            value[str(source/'pins.json')]=checks.sha(source/'pins.json')

        for p in (Path(__file__).resolve(),inputs/'provenance.json',inputs/'qualification.json',R/'cavity_correction445_qualification.json'):
            value[str(p)]=checks.sha(p)
    original_save(path,value)
checks.save=save_pinned
sys.argv=['run_cases.py','build445',case]
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
