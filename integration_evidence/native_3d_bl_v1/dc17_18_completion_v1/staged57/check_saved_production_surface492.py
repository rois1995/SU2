"""Run production PrepareSurface on the saved485 state with a newly qualified492 target."""
import json,os,sys,tempfile,time
from pathlib import Path
import run_cases as checks

R=Path(__file__).resolve().parent
if len(sys.argv) not in (3,4):
    raise SystemExit('usage: check_saved_production_surface492.py PRODUCER_DIR QUALIFICATION_JSON [serial]')
producer=Path(sys.argv[1]).resolve();qualification=Path(sys.argv[2]).resolve()
mode=sys.argv[3] if len(sys.argv)==4 else 'serial'
assert mode=='serial'
assert json.loads((R/'build494_terminal_handles.json').read_text())['all_owned_handles_terminal']
receipt=json.loads((R/'target_checkpoint492_producer.json').read_text())
qualified=json.loads(qualification.read_text())
assert qualified['actual_target_checkpoint_qualified'] and qualified['recomposed_target_checked_after_reload']
assert Path(receipt['producer']).resolve()==producer==Path(qualified['producer']).resolve()
assert checks.sha(R/'target_checkpoint492_qualification.json')==checks.sha(qualification)

case='curved492_saved_production_surface_'+mode
for key in list(os.environ):
    if key.startswith('SU2_NATIVE_'):del os.environ[key]
checks.CASES[case]=(1,'[NativeActualSavedSurfaceStage3D]')
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

allocation=producer/'core_allocation_assessment.json'
allocation_receipt=json.loads(allocation.read_text())['core_native_export']
core=Path(allocation_receipt['path']).resolve()
assert checks.sha(core)==allocation_receipt['sha256']
metadata=json.loads((producer/'saved_cfd_metric_rank0.jsonl').read_text().splitlines()[0])
checkpoint=producer/'target_rank0.bin'
reference=R/'prepared_reference441.native_ref'
capture=R/'surface486_current_state'
assert json.loads((capture/'terminal_handles_actual.json').read_text())['all_owned_handles_terminal']
status=json.loads((producer/'status.json').read_text())
assert status['state']=='terminal' and status['exit_code']==0 and status['pins_verified']
state=json.loads((R/'saved_surface485_checkpoint.json').read_text())
for name in ('owned','parameters'):
    item=state[name];path=Path(item['path'])
    assert checks.sha(path)==item['sha256']
    adapter=capture/f'trajectory_working_{name}.bin'
    assert checks.sha(adapter)==item['sha256']
assert checks.sha(reference)==state['prepared_reference']['sha256']
assert json.loads((capture/'terminal_handles_actual.json').read_text())['all_owned_handles_terminal']

os.environ['SU2_NATIVE_NOSE_FIXED_FACETS']=str(R/'nose492_fixed_facets.dat')
os.environ.update(SU2_NATIVE_BL_FRONT_PROBE='checkpoint_target_surface',
    SU2_NATIVE_BL_PLANNING_CAPTURE=str(capture),SU2_NATIVE_BL_REFERENCE=str(reference),
    SU2_NATIVE_TARGET_CHECKPOINT3D=str(checkpoint),SU2_NATIVE_FROZEN_SENSOR3D=str(core),
    SU2_NATIVE_FROZEN_CORE3D=repr(metadata['core']),
    SU2_NATIVE_FROZEN_TANGENT_SCALE3D=repr(metadata['tangential_scale']))
for key in ('SU2_NATIVE_BL_SAVED_PATCH','SU2_NATIVE_BL_SAVED_NEIGHBORS'):
    os.environ.pop(key,None)

seed=R/'cases/curved441_ramp5_seed_stage_20261112T123956_1290282'
provenance=json.loads((seed/'provenance.json').read_text())
assert checks.sha(seed/'run.cfg')==state['configuration']['sha256']
original_save=checks.save
def save_pinned(path,value):
    if Path(path).name=='pins.json':
        import shutil
        attempt=Path(path).parent
        for name,item in provenance['files'].items():
            source=seed/name;assert checks.sha(source)==item['sha256']
            destination=attempt/name
            if name=='run.cfg':shutil.copyfile(source,destination)
            else:destination.symlink_to(source)
            value[str(source)]=item['sha256'];value[str(destination)]=item['sha256']
        os.environ['SU2_NATIVE_FROZEN_CONFIG']=str(attempt/'run.cfg')
        pinned=[R/'assess_saved_production_surface492.py',R/'focused_curvature492_fidelity.json',R/'target_reference492_fidelity.json',R/'target492_metadata_summary.json',R/'nose492_fixed_faces.json',R/'nose492_fixed_facets.dat',R/'prepare_nose492_measurements.py',Path(__file__).resolve(),R/'check_metric_stage492.py',R/'qualify_target492.py',
            R/'build494_terminal_handles.json',R/'runtime_20261113_474/loaded_runtime_pins.json',
            R/'target_checkpoint492_producer.json',qualification,allocation,reference,checkpoint,core,
            producer/'status.json',producer/'terminal_handles_actual.json',producer/'pins.json',
            *(producer/f'saved_cfd_metric_rank{rank}.jsonl' for rank in range(6)),
            R/'saved_surface485_checkpoint.json',R/'saved_surface_corrections485_assessment.json',
            R/'surface486_current_state/terminal_handles_actual.json',
            capture/'trajectory_working_owned.bin',capture/'trajectory_working_parameters.bin',
            capture/'terminal_handles_actual.json',R/'prepared_reference441.native_ref',
            seed/'provenance.json',seed/'qualification.json']
        for item in pinned:
            value[str(item)]=checks.sha(item)
        for rank in range(6):
            for name in (f'target_rank{rank}.bin',f'target_witness_rank{rank}.txt'):
                item=producer/name
                assert checks.sha(item)==receipt['files'][name]['sha256']
                value[str(item)]=receipt['files'][name]['sha256']
        value[str(producer/'pins.json')]=checks.sha(producer/'pins.json')
    original_save(path,value)
checks.save=save_pinned
sys.argv=['run_cases.py','build494',case]
try:checks.main(R/'runtime_20261113_474/loaded_runtime_pins.json')
except SystemExit as e:code=e.code or 0
attempt=sorted((R/'cases').glob(case+'_*'),key=lambda p:p.stat().st_mtime)[-1]
live=[]
for row in seen.values():
    try:
        v=Path('/proc',str(row['pid']),'stat').read_text().rsplit(')',1)[1].split()
        if int(v[19])==row['start_ticks']:live.append(row)
    except (FileNotFoundError,ProcessLookupError):pass
actual=dict(actual_host_proc=True,all_owned_handles_terminal=not live,live_owned_handles=live,
    sampled_identities=list(seen.values()),wrapper_excluded=own,checked_unix=time.time())
(attempt/'terminal_handles.json').write_text(json.dumps(actual,indent=2)+'\n')
print(json.dumps({'attempt':str(attempt),'exit_code':code,'sampled_terminal':not live}),flush=True)
assert not live,live
raise SystemExit(code)
