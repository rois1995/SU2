from pathlib import Path
import json,gzip,hashlib,time,shutil,subprocess
n=Path('integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57/night_20261115');p=n/'N4/section6_3_after_N11';u=p/'B6';assert json.loads((u/'acceptance_summary.json').read_text())['state']=='B6_SCOPED_G1_G2_PASSED';assert not subprocess.check_output(['git','diff','--cached','--name-only']).strip();control=u/'primary_control_archive';control.mkdir(exist_ok=True)
for name in ['b6_launch_g1.py','b6_seal_g1.py','b6_launch_prefix.py','b6_seal_prefix.py','b6_accept_scoped.py','b6_apply_connected.py','b6_launch_build556.py','b6_seal_build556.py','b6_stage_passed.py']:
 shutil.copy2(Path('/tmp')/name,control/name)
paths=[];archives=[];sha=lambda data:hashlib.sha256(data).hexdigest()
for root in [u,p/'build556',p/'B6_preparation/proposal']:
 for f in list(root.rglob('*')):
  if f.is_symlink():
   paths.append(str(f));continue
  if not f.is_file() or '__pycache__' in str(f):continue
  if f.suffix in {'.log','.jsonl','.csv','.txt','.before','.proposed','.inc'} or f.name in {'stdout','stderr'}:
   data=f.read_bytes();z=f.with_name(f.name+'.gz');z.write_bytes(gzip.compress(data,mtime=0));assert gzip.decompress(z.read_bytes())==data;paths.append(str(z));archives.append(dict(raw=str(f),gzip=str(z),raw_sha256=sha(data),gzip_sha256=sha(z.read_bytes()),raw_bytes=len(data),round_trip_equal=True))
  elif f.suffix in {'.gz','.json','.md','.py','.sha256','.diff'}:paths.append(str(f))
m=u/'exact_log_archive_manifest.json';m.write_text(json.dumps(dict(unix=time.time(),all_numerical_cases_terminal=True,raw_preserved=True,files=archives),indent=2)+'\n');paths+=[str(m),str(p/'build556_aliases.py'),str(n/'progress.md'),'Handbook/ADAPTATION_TIMINGS.md','HANDOFF_Codex.md','HowToProceed.md','Common/src/adaptation/CNativeEngine3D.cpp','Common/src/adaptation/native_engine3d/EngineOwnership.cpp','Common/src/adaptation/native_engine3d/EngineOwnership.hpp'];out=Path('/tmp/b6_checkpoint_paths.json');out.write_text(json.dumps(sorted(set(paths)),indent=2)+'\n');print(json.dumps({'paths':len(set(paths)),'archives':len(archives),'raw_bytes':sum(x['raw_bytes'] for x in archives)}))
