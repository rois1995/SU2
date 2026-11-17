from pathlib import Path
import json,os,time,sys,hashlib
def is_owned_helper(script):return bool(script and ('/integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57/night_20261115/' in script or script.startswith('/tmp/b6_') or script.startswith('/tmp/b7_') or script.startswith('/tmp/b8_') or script.startswith('/tmp/end_b_')))
assert is_owned_helper('/repo/integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57/night_20261115/N4/draft.py')
assert not is_owned_helper('/tmp/claude-1000/scratchpad/analysis/user.py')
output=Path(sys.argv[1]); started=time.time()
def identities():
 rows={}
 for q in Path('/proc').iterdir():
  if not q.name.isdigit():continue
  try:
   f=(q/'stat').read_text().rsplit(')',1)[1].split();rows[int(q.name)]={'pid':int(q.name),'comm':(q/'comm').read_text().strip(),'ticks':int(f[11])+int(f[12]),'start_ticks':int(f[19]),'state':f[0]}
  except (OSError,ValueError,IndexError):pass
 return rows
before=identities();time.sleep(3);after=identities();elapsed=time.time()-started;owned=[];other=[]
for pid,r in after.items():
 if pid==os.getpid():continue
 prev=before.get(pid); fraction=(r['ticks']-prev['ticks'])/os.sysconf('SC_CLK_TCK')/elapsed if prev and prev['start_ticks']==r['start_ticks'] else 0
 script=None
 try:
  argv=[x.decode(errors='replace') for x in (Path('/proc')/str(pid)/'cmdline').read_bytes().split(b'\0') if x]
  if len(argv)>1 and argv[1].endswith('.py'):
   q=Path(argv[1]);q=q if q.is_absolute() else (Path('/proc')/str(pid)/'cwd').resolve()/q
   script=str(q.resolve())
 except OSError:pass
 owned_python=is_owned_helper(script)
 if owned_python and fraction>.5:r['owned_script']=script;r['core_fraction']=fraction;owned.append(r)
 elif r['comm'] in {'SU2_CFD','test_driver','ninja','cc1plus','mpirun','mpiexec'} and r['state'] not in {'Z','X'}:owned.append(r)
 elif fraction>.5:other.append({'pid':pid,'comm':r['comm'],'core_fraction':fraction})
mem=next(x for x in Path('/proc/meminfo').read_text().splitlines() if x.startswith('MemAvailable:'));assert int(mem.split()[1])*1024>19999490048;assert not owned,owned
receipt={'actual_host_proc':True,'owned_python_helpers_accounted':True,'preflight_script_sha256':hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),'unix':time.time(),'sample_seconds':elapsed,'active_OWNED_heavy_processes':owned,'other_busy_observations':other,'MemAvailable':mem,'loadavg':Path('/proc/loadavg').read_text().strip()};assert not output.exists();output.write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps(receipt))
