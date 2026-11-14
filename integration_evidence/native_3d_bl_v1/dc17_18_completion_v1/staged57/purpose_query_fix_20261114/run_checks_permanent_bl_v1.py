"""Retarget existing bounded checks to P5; preserve their fixtures, gates and runner."""
import ast,hashlib,importlib.util,json,os,re,sys,tempfile,time
from pathlib import Path
HERE=Path(__file__).resolve().parent;R=HERE.parent;REPO=R.parents[3]
sys.path.insert(0,str(R));import run_cases as checks
sha=checks.sha
mode=sys.argv[1];assert mode in ('unit','merit481','saved481','saved_current','bl','2d1','2d2','2d4')
status=json.loads((HERE/'build_status.json').read_text())
assert status['state']=='terminal' and status['exit_code']==0 and status['pins_verified']
assert sha(status['test_binary'])==status['test_binary_sha256']
checks.BINARY=Path(status['test_binary'])
for p in Path('/proc').glob('[0-9]*'):
 try:assert (p/'comm').read_text().strip() not in ('SU2_CFD','test_driver','cc1plus','ninja','lto1'),str(p)
 except OSError:pass
for key in list(os.environ):
 if key.startswith('SU2_NATIVE_'):del os.environ[key]
os.environ.update(OMP_NUM_THREADS='1',OPENBLAS_NUM_THREADS='1',MKL_NUM_THREADS='1',OMPI_MCA_osc='pt2pt')
scratch=REPO/'integration_evidence/build-integrated-v2/compile-tmp'
os.environ['OMPI_MCA_orte_tmpdir_base']=tempfile.mkdtemp(prefix='p5-'+mode+'-',dir=scratch)
metadata=dict(status,completed_build=True)
source_pins=json.loads((HERE/'source_pins.json').read_text())
extra=json.loads((HERE/'unchanged_build494_source_pins.json').read_text())
assert not (source_pins.keys()&extra.keys())
source_pins.update(extra)
assert all(sha(path)==digest for path,digest in source_pins.items())
for path,value in ((R/'build495_status.json',metadata),(R/'build495_source_pins.json',source_pins),(R/'build495_terminal_handles.json',{'actual_host_proc':True,'all_owned_handles_terminal':True,'identities':[],'verified_no_heavy_process_unix':time.time()})):
 if path.exists():
  prior=json.loads(path.read_text())
  if 'verified_no_heavy_process_unix' not in prior:assert prior==value,str(path)
 else:path.write_text(json.dumps(value,indent=2)+'\n')
original_save=checks.save
extra_pins={str(Path(__file__).resolve()):sha(__file__)}
def save_pinned(path,value):
 if Path(path).name=='pins.json':value.update(extra_pins)
 original_save(path,value)
checks.save=save_pinned
if mode=='merit481':
 spec=importlib.util.spec_from_file_location('merit481',R/'check_geometry_magnitude481.py');module=importlib.util.module_from_spec(spec);spec.loader.exec_module(module)
 module.BUILD='build495';module.CASE='geometry_merit495_serial';module.STATUS=R/'build495_status.json';module.HANDLES=R/'build495_terminal_handles.json';module.SOURCE_PINS=R/'build495_source_pins.json';module.RESULT=HERE/'geometry_merit495_serial_assessment.json'
 module.main()
elif mode in ('saved481','saved_current'):
 path=R/'check_saved_surface_corrections481.py';text=path.read_text()
 assert text.count("'build481'")==1 and text.count('build481_terminal_handles.json')==1 and text.count('curved481_saved_surface_corrections_')==1
 text=text.replace("'build481'","'build495'").replace('build481_terminal_handles.json','build495_terminal_handles.json').replace('curved481_saved_surface_corrections_','curved495_saved_surface_corrections_')
 extra_pins[str(path)]=sha(path);sys.argv=[str(path),'serial']
 if mode=='saved_current':
  producer_receipt=R/'target_checkpoint492_producer.json';qualification=R/'target_checkpoint492_qualification.json'
  receipt=json.loads(producer_receipt.read_text());qualified=json.loads(qualification.read_text())
  assert qualified['actual_target_checkpoint_qualified'] and qualified['recomposed_target_checked_after_reload']
  assert qualified['producer_receipt_sha256']==sha(producer_receipt)
  producer=Path(receipt['producer']);assert producer.resolve()==Path(qualified['producer']).resolve()
  for name,item in receipt['files'].items():
   file=producer/name;assert sha(file)==item['sha256'];extra_pins[str(file)]=item['sha256']
  exact=json.loads((R/'cases/curved492_saved_production_surface_serial_20261113T163756_3724702/command.json').read_text())['environment']
  current={key:exact[key] for key in ('SU2_NATIVE_TARGET_CHECKPOINT3D','SU2_NATIVE_FROZEN_SENSOR3D','SU2_NATIVE_FROZEN_CORE3D','SU2_NATIVE_FROZEN_TANGENT_SCALE3D')}
  for file in (producer_receipt,qualification,Path(current['SU2_NATIVE_TARGET_CHECKPOINT3D']),Path(current['SU2_NATIVE_FROZEN_SENSOR3D'])):extra_pins[str(file)]=sha(file)
  original_main=checks.main
  def current_main(runtime_manifest=None):
   os.environ.update(current)
   return original_main(runtime_manifest)
  checks.main=current_main
  text=text.replace('curved495_saved_surface_corrections_','curved495_current_target_surface_corrections_')
 exec(compile(text,str(path),'exec'),{'__name__':'__main__','__file__':str(path)})
else:
 case='p5_'+mode;ranks=int(mode[-1]) if mode.startswith('2d') else 1
 if mode.startswith('2d'):
  filters='[NativeEngine2D],[NativeMesh2D],[NativeBL2D]'
 elif mode=='unit':
  names=[]
  for file in ('CNativeAffineReference_tests.cpp','CNativeGeometryBackend3D_tests.cpp'):
   path=REPO/'UnitTests/Common/adaptation'/file
   names.extend(name for name,tags in re.findall(r'TEST_CASE\(\s*"([^"]+)"\s*,\s*"([^"]+)"',path.read_text()) if '[.]' not in tags)
  assert len(names)>30
  # Catch uses commas as separators even inside a test name.
  filters=','.join(name.replace(',',r'\,') for name in names)+',[NativeBoundedGuidance3D]'
  (HERE/'selected_unit_tests.json').write_text(json.dumps(names,indent=2)+'\n')
  extra_pins[str(HERE/'selected_unit_tests.json')]=sha(HERE/'selected_unit_tests.json')
 else:
  source=R/'cases/curved492_saved_production_surface_serial_20261113T163756_3724702'
  old=json.loads((source/'command.json').read_text());os.environ.update(old['environment'])
  os.environ['OMPI_MCA_orte_tmpdir_base']=tempfile.mkdtemp(prefix='p5-bl-',dir=scratch)
  for key,value in old['environment'].items():
   if key.startswith('SU2_NATIVE_') and Path(value).is_file():extra_pins[str(Path(value).resolve())]=sha(value)
  state_path=R/'saved_surface485_checkpoint.json';state=json.loads(state_path.read_text());extra_pins[str(state_path)]=sha(state_path)
  for key in ('owned','parameters','prepared_reference'):
   item=state[key];assert sha(item['path'])==item['sha256'];extra_pins[item['path']]=item['sha256']
  setup_save=checks.save;bl_case=[]
  gate_path=HERE/'check_bl_regression.py'
  extra_pins[str(gate_path.resolve())]=sha(gate_path)
  extra_pins[str((HERE/'bl_output_comparison.json').resolve())]=sha(HERE/'bl_output_comparison.json')
  def setup(path,value):
   if Path(path).name=='pins.json':
    import shutil
    attempt=Path(path).parent;bl_case.append(attempt)
    for name in ('m6.su2','m6.su2.native_ref','m6.su2.native_rows','baseline_sa_seed.dat'):
     src=(source/name).resolve();(attempt/name).symlink_to(src);value[str(src)]=sha(src)
    shutil.copyfile(source/'run.cfg',attempt/'run.cfg');value[str(attempt/'run.cfg')]=sha(attempt/'run.cfg')
    os.environ['SU2_NATIVE_FROZEN_CONFIG']=str(attempt/'run.cfg')
   setup_save(path,value)
  checks.save=setup;filters='[NativeActualSavedSurfaceStage3D]'
 checks.CASES[case]=(ranks,filters);sys.argv=['run_cases.py','build495',case]
 if mode=='bl':
  try:checks.main(R/'runtime_20261113_474/loaded_runtime_pins.json')
  except SystemExit as result:
   if result.code:raise
   assert len(bl_case)==1,bl_case
   spec=importlib.util.spec_from_file_location('bl_regression',gate_path)
   gate=importlib.util.module_from_spec(spec);spec.loader.exec_module(gate)
   gate.self_check();raise SystemExit(gate.main(bl_case[0]))
 else:checks.main(R/'runtime_20261113_474/loaded_runtime_pins.json')
