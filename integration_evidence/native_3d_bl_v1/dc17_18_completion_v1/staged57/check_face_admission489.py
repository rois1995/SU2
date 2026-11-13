"""Exact directed carrier-contraction comparison on preserved485 state; build490 test-only binding."""
import json,os,sys,tempfile,time
from pathlib import Path
import run_cases as checks
R=Path(__file__).resolve().parent
mode=sys.argv[1] if len(sys.argv)>1 else 'serial'
assert mode in ('serial','mpi6')
assert json.loads((R/'build490_terminal_handles.json').read_text())['all_owned_handles_terminal']
selected='checkpoint_target_surface'
case='curved489_face_admission_'+mode
for key in list(os.environ):
    if key.startswith('SU2_NATIVE_'):del os.environ[key]
tags='[NativeActualFaceAdmission3D]'
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
reference=R/'prepared_reference441.native_ref'
contexts=R/'m6_bl_inputs/seed_measurement363_v1/contexts.dat'
checkpoint=producer/'target_rank0.bin'
capture=R/'surface486_current_state'
assert json.loads((capture/'terminal_handles_actual.json').read_text())['all_owned_handles_terminal']
os.environ['SU2_NATIVE_BL_PLANNING_CAPTURE']=str(capture)
saved_paths=[R/'saved_surface485_checkpoint.json',R/'prepare_reverse_surface486.py',R/'reverse_surface486_inputs.json',R/'saved_surface_corrections485_assessment.json',capture/'trajectory_working_owned.bin',capture/'trajectory_working_parameters.bin',R/'prepare_surface_residual485.py',R/'surface_residual485_inputs.json',R/'saved_surface484_checkpoint.json',R/'saved_production_surface484_serial_assessment.json',R/'surface_residual484_inventory.json',R/'surface_delta484_assessment.json']
contexts=R/'face_admission489_contexts.dat'
original_contexts=R/'face_admission489_inputs.json'
receipt=json.loads(original_contexts.read_text())
for name in ('owned','parameters','target','prepared_reference','contexts'):
    item=receipt[name];assert checks.sha(Path(item['path']))==item['sha256'],name

os.environ.update(SU2_NATIVE_BL_FRONT_PROBE=selected,SU2_NATIVE_BL_REFERENCE=str(reference),SU2_NATIVE_BL_CAVITY_CONTEXTS=str(contexts),
    SU2_NATIVE_TARGET_CHECKPOINT3D=str(checkpoint),SU2_NATIVE_FROZEN_SENSOR3D=str(core),
    SU2_NATIVE_FROZEN_CORE3D=repr(metadata['core']),SU2_NATIVE_FROZEN_TANGENT_SCALE3D=repr(metadata['tangential_scale']))
for key in ('SU2_NATIVE_BL_SAVED_PATCH','SU2_NATIVE_BL_SAVED_NEIGHBORS'):os.environ.pop(key,None)
assert checks.sha(R/'cases/curved441_ramp5_seed_stage_20261112T123956_1290282/run.cfg')==receipt['configuration']['sha256']
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
        for p in (R.parents[3]/'Handbook/ADAPTATION_FACE_ADMISSION_DECISION_489_20261113.md',R/'face_admission489_fidelity.json',R/'geometry_evidence489_audit.json',R/'corrected_surface488_assessment.json',R/'face_admission489_inputs.json',R.parents[3]/'Handbook/ADAPTATION_CORRECTED_CONTRACTION_DECISION_488_20261113.md',R/'corrected488_fidelity.json',R/'directed_surface487_assessment.json',R.parents[3]/'Handbook/ADAPTATION_DIRECTED_CANDIDATE_OUTCOME_487_20261113.md',R.parents[3]/'Handbook/ADAPTATION_DIRECTED_CANDIDATE_DECISION_487_20261113.md',R/'directed487_fidelity.json',R/'directed487_preedit_verification.json',R/'directed487_static_support.json',R/'reverse_surface486_assessment.json',R.parents[3]/'Handbook/ADAPTATION_REVERSE_CARRIER_486_20261113.md',R/'reverse_surface486_fidelity.json',R.parents[3]/'Handbook/ADAPTATION_RESIDUAL_CANDIDATES_485_20261113.md',R/'surface_residual485_fidelity.json',R.parents[3]/'Handbook/ADAPTATION_GEOMETRY_MAGNITUDE_DECISION_20261113.md',R/'geometry_magnitude481_fidelity.json',R.parents[3]/'Handbook/ADAPTATION_SURFACE_CORRECTION_DECISION_20261113.md',R/'surface_comparison478_fidelity.json',R/'trajectory476_design.json',R/'current_trajectory474_assessment.json',R/'build474_postcompile_helper_binding.json',R/'runtime_20261113_474/provenance.json',R/'residual473_terminal_inventory.json',R/'trajectory474_entry.json',R/'trajectory474_design.json',R/'geometry_refusals477_assessment.json',R/'prepare_trajectory474_contexts.py',reference,contexts,original_contexts,checkpoint,core,qualification,allocation,*saved_paths,Path(__file__).resolve(),inputs/'provenance.json',inputs/'qualification.json'):
            value[str(p)]=checks.sha(p)
        # Policy is globally identical (qualified all-rank profile/reference agreement); sourcecore is all6owned partitions.
        receipt=json.loads((R/'target_checkpoint441_producer.json').read_text())
        for name,info in receipt['files'].items():
            p=producer/name;assert checks.sha(p)==info['sha256'];value[str(p)]=info['sha256']
    original_save(path,value)
checks.save=save_pinned
sys.argv=['run_cases.py','build490',case]
try:checks.main(R/'runtime_20261113_474/loaded_runtime_pins.json')
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
