"""Run the remaining ordered regression checks once; stop on the first failure."""
from pathlib import Path
import hashlib,json,os,re,subprocess,sys,time
HERE=Path(__file__).resolve().parent;R=HERE.parent
MODES=('merit481','saved_current','2d1','2d2','2d4')
def sha(p):
 with Path(p).open('rb') as f:return hashlib.file_digest(f,'sha256').hexdigest()
def save(v):(HERE/'final_checks_sequence.json').write_text(json.dumps(v,indent=2)+'\n')
assert not (HERE/'final_checks_sequence.json').exists()
rows=[];helper=HERE/'run_remaining_checks.py';frozen=sha(helper)
for mode in MODES:
 before=set((R/'cases').iterdir());started=time.time()
 with (HERE/('final_'+mode+'.log')).open('x') as log:
  code=subprocess.call([sys.executable,str(helper),mode],stdout=log,stderr=subprocess.STDOUT)
 new=set((R/'cases').iterdir())-before;assert len(new)==1,(mode,new)
 case=new.pop();status=json.loads((case/'status.json').read_text())
 text=(case/'run.log').read_text(errors='replace');maxima=[int(v)*1024 for v in re.findall(r'Maximum resident set size \(kbytes\):\s*(\d+)',text)]
 ranks=int(mode[-1]) if mode.startswith('2d') else 1
 # GNU time's largest descendant peak bounds each rank; multiplying is conservative, not a measured sum.
 bound=ranks*max(maxima) if maxima else None
 row={'mode':mode,'exit_code':code,'seconds':time.time()-started,'case':str(case),'status':status,'ranks':ranks,'largest_descendant_peak_RSS_bytes':max(maxima) if maxima else None,'sum_rank_peak_RSS_upper_bound_bytes':bound,'whole_job_20GB_conservative_gate':bound is not None and bound<=20_000_000_000,'test_summaries':re.findall(r'All tests passed[^\n]*',re.sub(r'\x1b\[[0-9;]*m','',text))}
 row['passed']=code==0 and status.get('exit_code')==0 and status.get('pins_verified') and not status.get('changed') and sha(helper)==frozen and row['whole_job_20GB_conservative_gate']
 rows.append(row);save({'completed':rows,'all_planned_checks_passed':len(rows)==len(MODES) and all(r['passed'] for r in rows),'next':None if row['passed'] and mode==MODES[-1] else 'stop first failure' if not row['passed'] else MODES[len(rows)],'controller_pid':os.getpid(),'helper_sha256':frozen})
 print(json.dumps(row),flush=True)
 if not row['passed']:raise SystemExit(1)
