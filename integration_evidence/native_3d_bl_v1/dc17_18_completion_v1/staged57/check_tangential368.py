"""Pinned actual original-M6 cavity geometry experiment; no production metric acceptance claim."""
import json,os,sys,tempfile,time
from pathlib import Path
import run_cases as checks
R=Path(__file__).resolve().parent
mode=sys.argv[1] if len(sys.argv)>1 else 'serial'
assert mode in ('serial','actual')
assert json.loads((R/'build368_terminal_handles.json').read_text())['all_owned_handles_terminal']
selected='tangential_target'
case='curved368_tangential_'+mode
checks.CASES[case]=(1,'[NativeActualBLSurfaceCavity3D]' if mode=='actual' else '[NativeBLTangentialNormalization3D],[NativeBLOuterTransition],[NativeBLTargetNormalization],[NativeBLTangentialProgression3D]')
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
config=R/'m6_bl_inputs/surface_cavities335_v1/run.cfg'
reference=R/'m6_bl_inputs/rans_curved303_v1/m6.su2.native_ref'
chord=R/'m6_bl_inputs/seed_measurement363_v1/contexts.dat'
os.environ.update(SU2_NATIVE_BL_FRONT_PROBE="tangential_target",SU2_NATIVE_FROZEN_CONFIG=str(config),SU2_NATIVE_BL_REFERENCE=str(reference),SU2_NATIVE_BL_CAVITY_CONTEXTS=str(chord))
saved_source=json.loads((R/'front358_assessment.json').read_text())
saved_paths=[R/'cases/curved358_front_smooth_upper_20261111T045033_884933'/name for name in ('prepared_repair_before.bin','prepared_repair_neighbor_star.bin')]
for p in saved_paths:
    expected=saved_source['captures'][str(p.relative_to(R.parents[3]))]
    assert checks.sha(p)==expected,(p,expected)
os.environ.update(SU2_NATIVE_BL_SAVED_PATCH=str(saved_paths[0]),SU2_NATIVE_BL_SAVED_NEIGHBORS=str(saved_paths[1]))
original_save=checks.save
def save_pinned(path,value):
    if Path(path).name=='pins.json':
        for p in (config,reference,chord,Path(__file__),R/'m6_bl_inputs/seed_measurement363_v1/provenance.json',R/'front358_assessment.json',*saved_paths) : value[str(p.resolve())]=checks.sha(p)
        preparation=R/'m6_bl_inputs/surface_cavities335_v1/provenance.json'
        value[str(preparation.resolve())]=checks.sha(preparation)
        for p,h in json.loads(preparation.read_text())['pins'].items():
            assert checks.sha(Path(p))==h,p
            value[p]=h
    original_save(path,value)
checks.save=save_pinned
sys.argv=['run_cases.py','build368',case]
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
