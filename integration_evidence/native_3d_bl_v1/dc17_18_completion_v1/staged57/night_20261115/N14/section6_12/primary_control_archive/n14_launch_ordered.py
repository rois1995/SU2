from pathlib import Path
import os,sys,json,time,hashlib
root=Path('integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57/night_20261115/N14/section6_12').resolve();v=root/'validation';mode=sys.argv[1];host=Path(sys.argv[2]).resolve(strict=True);tag=sys.argv[3] if len(sys.argv)>3 else mode
if mode=='mpi6':script=root/'mpi6_report_preparation/run_mpi6_wall_report.py';args=['/usr/bin/python3',str(script),'--launch','0']
elif mode=='full':script=v/'run_full552.py';args=['/usr/bin/python3',str(script)]
else:
 assert mode in {'merit481','saved481','saved_current','2d1','2d2','2d4'}
 script=root/'safeguard_preparation/run_build552_safeguards.py';args=['/usr/bin/python3',str(script),mode]
assert json.loads((v/'n14_g3_pair_comparison.json').read_text())['passed']
paths=[script,host,root/'build552_aliases.py',root/'build552/source_pins.json',root/'build552/build_status.json']
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
if mode=='full':
 import importlib.util
 spec=importlib.util.spec_from_file_location('full_launch_dependencies',script);m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m);paths+=list(m.controller_dependencies())
closure={str(p):sha(p) for p in paths};out=v/f'{tag}_primary_validation_pins.json';assert not out.exists();out.write_text(json.dumps(closure,indent=2)+'\n');f=Path('/proc/self/stat').read_text().rsplit(')',1)[1].split();out=v/f'{tag}_primary_launch_identity.json';assert not out.exists();out.write_text(json.dumps(dict(actual_host_proc=True,unix=time.time(),controller=dict(pid=os.getpid(),start_ticks=int(f[19])),argv=args,validation_pins_sha256=sha(v/f'{tag}_primary_validation_pins.json')),indent=2)+'\n');os.environ.update(OMP_NUM_THREADS='1',OPENBLAS_NUM_THREADS='1',MKL_NUM_THREADS='1',OMPI_MCA_osc='pt2pt');os.nice(19);os.execv(args[0],args)
