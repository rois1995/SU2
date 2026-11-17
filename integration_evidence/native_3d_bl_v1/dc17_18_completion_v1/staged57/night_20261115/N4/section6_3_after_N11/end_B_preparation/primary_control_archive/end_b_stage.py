from pathlib import Path
import json,gzip,hashlib,time,shutil,subprocess
N=Path('integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57/night_20261115');P=N/'N4/section6_3_after_N11';E=P/'end_B_preparation';assert json.loads((E/'acceptance_summary.json').read_text())['phase_B_complete'];assert not subprocess.check_output(['git','diff','--cached','--name-only']).strip();archive=E/'primary_control_archive';archive.mkdir(exist_ok=True)
for f in Path('/tmp').glob('end_b_*.py'):shutil.copy2(f,archive/f.name)
roots=[E,P/'end_B_readability_preparation',P/'B8/validation'];roots += [Path(r['case']) for r in json.loads((E/'g4_case_paths.json').read_text())['new_cases']];paths=[];records=[];sha=lambda data:hashlib.sha256(data).hexdigest()
for directory in roots:
 for f in list(directory.rglob('*')):
  if '__pycache__' in str(f):continue
  if f.is_symlink():paths.append(str(f));continue
  if not f.is_file():continue
  if f.suffix in {'.log','.jsonl','.csv','.txt','.xml','.before','.proposed','.inc'}:
   raw=f.read_bytes();z=f.with_name(f.name+'.gz');z.write_bytes(gzip.compress(raw,mtime=0));assert gzip.decompress(z.read_bytes())==raw;paths.append(str(z));records.append(dict(raw=str(f),archive=str(z),raw_sha256=sha(raw),archive_sha256=sha(z.read_bytes()),raw_bytes=len(raw),round_trip_equal=True))
  elif f.suffix in {'.json','.md','.py','.gz','.sha256','.cfg'}:paths.append(str(f))
m=E/'exact_archive_manifest.json';m.write_text(json.dumps(dict(unix=time.time(),raw_preserved=True,all_numerical_handles_terminal=True,files=records),indent=2)+'\n');paths += [str(m),str(N/'progress.md'),str(N/'progress_history_through_phase_B_qualification_20261117.md'),'HANDOFF_Codex.md','HowToProceed.md','Handbook/ADAPTATION_TIMINGS.md','Handbook/ADAPTATION_CUT_PHASE_B_20261117.md',str(P/'build558/cut_build_plan.json')];paths += [str(P/f'B{b}/R1_qualification_amendment.json') for b in [6,7,8]];out=Path('/tmp/end_b_checkpoint_paths.json');out.write_text(json.dumps(sorted(set(paths)),indent=2)+'\n');print(json.dumps(dict(paths=len(set(paths)),archives=len(records),raw_bytes=sum(r['raw_bytes'] for r in records))))
