from pathlib import Path
import json,os,time,sys
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
 prev=before.get(pid); fraction=(r['ticks']-prev['ticks'])/os.sysconf('SC_CLK_TCK')/elapsed if prev and prev['start_ticks']==r['start_ticks'] else 0
 if r['comm'] in {'SU2_CFD','test_driver','ninja','cc1plus','mpirun','mpiexec'} and r['state'] not in {'Z','X'}:owned.append(r)
 elif fraction>.5:other.append({'pid':pid,'comm':r['comm'],'core_fraction':fraction})
mem=next(x for x in Path('/proc/meminfo').read_text().splitlines() if x.startswith('MemAvailable:'));assert int(mem.split()[1])*1024>19999490048;assert not owned,owned
receipt={'actual_host_proc':True,'unix':time.time(),'sample_seconds':elapsed,'active_OWNED_heavy_processes':owned,'other_busy_observations':other,'MemAvailable':mem,'loadavg':Path('/proc/loadavg').read_text().strip()};assert not output.exists();output.write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps(receipt))
