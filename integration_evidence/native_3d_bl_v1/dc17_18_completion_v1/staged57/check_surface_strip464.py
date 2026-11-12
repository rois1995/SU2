"""Probe recorded M6 surface edges and conservative clearance under the same saved target."""
import json,os,sys,tempfile,time
from pathlib import Path
import run_cases as checks
R=Path(__file__).resolve().parent
mode=sys.argv[1] if len(sys.argv)>1 else 'serial'
assert mode in ('serial','mpi6')
assert json.loads((R/'build464_terminal_handles.json').read_text())['all_owned_handles_terminal']
selected='checkpoint_target_surface'
case='curved464_surface_strip_'+mode
tags='[NativeActualSurfaceStripCavity3D]'
assert mode=='serial'
checks.CASES[case]=(1 if mode=='serial' else 6,tags)
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
producer=R/'cases/curved441_ramp5_metric_stage_20261112T124340_1300487'
qualification=R/'target_checkpoint441_qualification.json'
assert json.loads(qualification.read_text())['actual_target_checkpoint_qualified']
allocation=producer/'core_allocation_assessment.json'
core_export=json.loads(allocation.read_text())['core_native_export']
core=Path(core_export['path']);assert checks.sha(core)==core_export['sha256']
metadata=json.loads((producer/'saved_cfd_metric_rank0.jsonl').read_text().splitlines()[0])
reference=R/'m6_bl_inputs/rans_metric_entry364_v1/m6.su2.native_ref'
contexts=R/'m6_bl_inputs/seed_measurement363_v1/contexts.dat'
checkpoint=producer/'target_rank0.bin'
capture=R/'cases/curved451_strip_reuse_saved_target_surface_mpi6_20261112T185700_120424'
assert json.loads((capture/'terminal_handles_actual.json').read_text())['all_owned_handles_terminal']
os.environ['SU2_NATIVE_BL_PLANNING_CAPTURE']=str(capture)
saved_paths=[capture/('stage_surface_rank'+str(rank)+'_'+suffix) for rank in range(6) for suffix in ('owned.bin','donors.bin','parameters.bin','manifest.json')]
original_contexts=contexts
lines=contexts.read_text().splitlines(); cursor=1; selected_context=None
for _ in range(int(lines[0])):
    n=int(lines[cursor].split()[-1]); block=lines[cursor:cursor+n+1]
    if block[0].startswith('leading_edge '):selected_context=block
    cursor+=n+1
assert selected_context is not None
contexts=R/'m6_bl_inputs/checkpoint_front416_le_context.dat'
expected_context='1\n'+'\n'.join(selected_context)+'\n'
assert contexts.read_text()==expected_context
os.environ.update(SU2_NATIVE_BL_FRONT_PROBE=selected,SU2_NATIVE_BL_REFERENCE=str(reference),SU2_NATIVE_BL_CAVITY_CONTEXTS=str(contexts),
    SU2_NATIVE_TARGET_CHECKPOINT3D=str(checkpoint),SU2_NATIVE_FROZEN_SENSOR3D=str(core),
    SU2_NATIVE_FROZEN_CORE3D=repr(metadata['core']),SU2_NATIVE_FROZEN_TANGENT_SCALE3D=repr(metadata['tangential_scale']))
for key in ('SU2_NATIVE_BL_SAVED_PATCH','SU2_NATIVE_BL_SAVED_NEIGHBORS'):os.environ.pop(key,None)
original_save=checks.save
def save_pinned(path,value):
    if Path(path).name=='pins.json':
        import shutil
        attempt=Path(path).parent
        inputs=R/'cases/curved441_ramp5_seed_stage_20261112T123956_1290282'
        provenance=json.loads((inputs/'provenance.json').read_text())
        for name,receipt in provenance['files'].items():
            seed_file=inputs/name;assert checks.sha(seed_file)==receipt['sha256']
            destination=attempt/name
            if name=='run.cfg':shutil.copyfile(seed_file,destination)
            else:destination.symlink_to(seed_file)
            value[str(seed_file)]=receipt['sha256'];value[str(destination)]=receipt['sha256']
        os.environ['SU2_NATIVE_FROZEN_CONFIG']=str(attempt/'run.cfg')
        for p in (reference,contexts,original_contexts,checkpoint,core,qualification,allocation,*saved_paths,Path(__file__).resolve(),inputs/'provenance.json',inputs/'qualification.json'):
            value[str(p)]=checks.sha(p)
        # Policy is globally identical (qualified all-rank profile/reference agreement); sourcecore is all6owned partitions.
        receipt=json.loads((R/'target_checkpoint441_producer.json').read_text())
        for name,info in receipt['files'].items():
            p=producer/name;assert checks.sha(p)==info['sha256'];value[str(p)]=info['sha256']
    original_save(path,value)
checks.save=save_pinned
sys.argv=['run_cases.py','build464',case]
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
