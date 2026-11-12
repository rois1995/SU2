from pathlib import Path
import hashlib,json,os,subprocess,sys,time
repo=Path('/media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated');r=repo/'integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57'
sys.path.insert(0,str(r));from run_cases import sample_tree
out=r/'reviews/cavity_operator_20261112';case=out/('checks_'+time.strftime('%Y%m%dT%H%M%S',time.gmtime())+'_'+str(os.getpid()));case.mkdir()
sha=lambda p:hashlib.file_digest(Path(p).open('rb'),'sha256').hexdigest()
build=json.loads((r/'build431_status.json').read_text());binary=repo/'integration_evidence/build-integrated-v3-currenthost/UnitTests/test_driver'
assert build['completed_build'] and sha(binary)==build['binaries'][str(binary)]
pins=json.loads((r/'build431_source_pins.json').read_text());changed=[p for p,h in pins.items() if sha(p)!=h]
expected=[str(repo/p) for p in ['Common/src/adaptation/CNativeEngine3D.cpp','Common/include/adaptation/CNativeEngine3D.hpp']];assert set(changed)==set(expected),changed
for p in changed:
 name=str(Path(p).relative_to(repo));data=subprocess.check_output(['git','show','e4d3ca7618:'+name],cwd=repo)
 assert hashlib.sha256(data).hexdigest()==pins[p];(case/Path(name).name).write_bytes(data)
frozen={p:sha(p) for p in pins}
filter='Native 3D cone*,Native 3D fixed interface*,Native 3D cavity rejects*,Native 3D orientation*,Native 3D metric admission*,[NativeGrowthLayer12]'
env=os.environ.copy();env.update(OMP_NUM_THREADS='1',OPENBLAS_NUM_THREADS='1',MKL_NUM_THREADS='1',TMPDIR=str(repo/'integration_evidence/build-integrated-v2/compile-tmp'),OMPI_MCA_osc='pt2pt')
argv=['nice','-n','19','/usr/bin/time','-v','timeout','120',str(binary),filter,'--durations','yes']
seen={};start=time.time();own={'pid':os.getpid(),'start_ticks':int(Path('/proc/self/stat').read_text().rsplit(')',1)[1].split()[19])}
(case/'command.json').write_text(json.dumps(dict(argv=argv,baseline='e4d3ca7618',build=431,binary_sha256=sha(binary),unbuilt_432_not_tested=changed,source_pins_sha256=sha(r/'build431_source_pins.json'),frozen_current_source_sha256={p:frozen[p] for p in changed}),indent=2)+'\n')
with (case/'run.log').open('w') as log,(case/'resources.jsonl').open('w') as resources:
 proc=subprocess.Popen(argv,cwd=case,env=env,stdout=log,stderr=subprocess.STDOUT,start_new_session=True)
 (case/'status.json').write_text(json.dumps(dict(state='running',pid=proc.pid,wrapper=own,started=start)))
 print(json.dumps(dict(case=str(case),pid=proc.pid,wrapper=own)),flush=True)
 while proc.poll() is None:
  rows=sample_tree(proc.pid)
  for row in rows:
   try:
    row['start_ticks']=int(Path('/proc',str(row['pid']),'stat').read_text().rsplit(')',1)[1].split()[19]);seen[(row['pid'],row['start_ticks'])]={'pid':row['pid'],'start_ticks':row['start_ticks']}
   except FileNotFoundError:pass
  resources.write(json.dumps(dict(elapsed=time.time()-start,processes=rows))+'\n');resources.flush();time.sleep(1)
 code=proc.wait()
assert all(sha(p)==h for p,h in frozen.items())
live=[]
for row in seen.values():
 p=Path('/proc',str(row['pid']),'stat')
 if p.exists() and int(p.read_text().rsplit(')',1)[1].split()[19])==row['start_ticks']:live.append(row)
status=dict(state='terminal',exit_code=code,seconds=time.time()-start,current_sources_unchanged=True,baseline_binary_verified=True,unbuilt432_tested=False)
(case/'status.json').write_text(json.dumps(status,indent=2)+'\n');(case/'terminal_handles.json').write_text(json.dumps(dict(all_owned_handles_terminal=not live,actual_host_proc=True,live=live,identities=list(seen.values()),wrapper_excluded=own),indent=2)+'\n');print(json.dumps(status),flush=True);assert not live
