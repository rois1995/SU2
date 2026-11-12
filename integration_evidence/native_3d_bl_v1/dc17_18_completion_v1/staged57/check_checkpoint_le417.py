"""Pinned existing-operator original-mesh current-target front experiment; no production acceptance claim."""
import json,os,sys,tempfile,time
from pathlib import Path
import run_cases as checks
R=Path(__file__).resolve().parent
mode='surface'
assert json.loads((R/'build417_terminal_handles.json').read_text())['all_owned_handles_terminal']
selected='checkpoint_target_front'
case='curved417_checkpoint_le'
checks.CASES[case]=(1,'[NativeActualBLSurfaceCavity3D]')
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
producer=R/'cases/curved396_saved_cfd_metric_stage_20261111T214613_3294100'
qualification=R/'target_checkpoint396_qualification.json'
assert json.loads(qualification.read_text())['actual_target_checkpoint_qualified']
allocation=producer/'core_allocation_assessment.json'
core_export=json.loads(allocation.read_text())['core_native_export']
core=Path(core_export['path']);assert checks.sha(core)==core_export['sha256']
metadata=json.loads((producer/'saved_cfd_metric_rank0.jsonl').read_text().splitlines()[0])
reference=R/'m6_bl_inputs/rans_metric_entry364_v1/m6.su2.native_ref'
contexts=R/'m6_bl_inputs/seed_measurement363_v1/contexts.dat'
checkpoint=producer/'target_rank0.bin'
saved_paths=[]
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
        inputs=R/'m6_bl_inputs/rans_metric_entry364_v1'
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
        receipt=json.loads((R/'target_checkpoint396_producer.json').read_text())
        for name,info in receipt['files'].items():
            p=producer/name;assert checks.sha(p)==info['sha256'];value[str(p)]=info['sha256']
    original_save(path,value)
checks.save=save_pinned
sys.argv=['run_cases.py','build417',case]
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
