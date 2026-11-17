from pathlib import Path
import json,gzip,hashlib,time,shutil,subprocess,sys
b=int(sys.argv[1]);assert b in [7,8];tag=f'B{b}';build=550+b
n=Path('integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57/night_20261115');p=n/'N4/section6_3_after_N11';u=p/tag;assert json.loads((u/'acceptance_summary.json').read_text())['state']==f'{tag}_SCOPED_G1_G2_PASSED';assert not subprocess.check_output(['git','diff','--cached','--name-only']).strip();control=u/'primary_control_archive';control.mkdir(exist_ok=True)
for name in [f'b{b}_rebind_apply.py',f'b{b}_accept_scoped.py',f'b{b}_materialize_build{build}.py',f'b{b}_launch_build{build}.py',f'b{b}_seal_build{build}.py',f'b{b}_launch_g1_{build}.py',f'b{b}_seal_g1_{build}.py',f'b{b}_launch_prefix_{build}.py',f'b{b}_seal_prefix_{build}.py','cut_finish_scoped.py','cut_stage_scoped.py']:
 f=Path('/tmp')/name
 if f.exists():shutil.copy2(f,control/name)
paths=[];archives=[];sha=lambda data:hashlib.sha256(data).hexdigest()
for root in [u,p/f'build{build}',p/f'{tag}_preparation']:
 for f in list(root.rglob('*')):
  if f.is_symlink():paths.append(str(f));continue
  if not f.is_file() or '__pycache__' in str(f):continue
  if f.suffix in {'.log','.jsonl','.csv','.txt','.before','.proposed','.inc','.fragment'} or f.name in {'stdout','stderr'} or (f.name.startswith('input_') and f.suffix=='.cpp'):
   data=f.read_bytes();z=f.with_name(f.name+'.gz');z.write_bytes(gzip.compress(data,mtime=0));assert gzip.decompress(z.read_bytes())==data;paths.append(str(z));archives.append(dict(raw=str(f),gzip=str(z),raw_sha256=sha(data),gzip_sha256=sha(z.read_bytes()),raw_bytes=len(data),round_trip_equal=True))
  elif f.suffix in {'.gz','.json','.md','.py','.sha256','.diff','.hpp','.cpp'}:paths.append(str(f))
m=u/'exact_log_archive_manifest.json';m.write_text(json.dumps(dict(unix=time.time(),all_numerical_cases_terminal=True,raw_preserved=True,files=archives),indent=2)+'\n');paths += [str(m),str(p/f'build{build}_aliases.py'),str(n/'progress.md'),str(n/'progress_preparation_history_through_B6_20261117.md'),'Handbook/ADAPTATION_TIMINGS.md','HANDOFF_Codex.md','HowToProceed.md','Common/src/adaptation/CNativeEngine3D.cpp'];unit='EngineControl' if b==7 else 'EngineSetup';paths += [f'Common/src/adaptation/native_engine3d/{unit}.{suffix}' for suffix in ['hpp','cpp']];out=Path(f'/tmp/b{b}_checkpoint_paths.json');out.write_text(json.dumps(sorted(set(paths)),indent=2)+'\n');print(json.dumps(dict(paths=len(set(paths)),archives=len(archives),raw_bytes=sum(x['raw_bytes'] for x in archives))))
