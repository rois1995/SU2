import hashlib,json,os,subprocess,time,pathlib,sys
p=pathlib.Path(__file__).resolve().parent;base=p.parent;out=p/'baseline142';out.mkdir(exist_ok=False)
def sha(f):
 h=hashlib.sha256()
 with open(f,'rb') as x:
  for b in iter(lambda:x.read(1024*1024),b''):h.update(b)
 return h.hexdigest()
cmd=json.loads((p/'command.json').read_text());sourcepins=json.loads((base/'build142_source_pins.json').read_text());inputs=json.loads((p/'provenance.json').read_text())['sha256'];build=pathlib.Path(cmd['cwd']); libs=[build/x for x in cmd['link'] if x.endswith('.a')];inputs.update({str(x):sha(x) for x in libs});inputs.update(sourcepins)
wrong=[x for x,h in inputs.items() if not pathlib.Path(x).exists() or sha(x)!=h]
if wrong:(out/'status.json').write_text(json.dumps({'state':'refused','mismatched':wrong},indent=2));sys.exit(1)
compile=[str(out/'probe.o') if x==str(p/'probe.o') else x for x in cmd['compile'] if x not in ['-O2','-ffast-math','-fno-finite-math-only','-fno-unsafe-math-optimizations']];compile.insert(compile.index('-std=c++17')+1,'-O3');link=[str(out/'probe.o') if x==str(p/'probe.o') else str(out/'probe') if x==str(p/'probe') else x for x in cmd['link']];run=[str(out/'probe') if x==str(p/'probe') else x for x in cmd['run']]; commands={'compile':compile,'link':link,'run':run}
env=os.environ.copy();env.update(cmd['environment']);(out/'pins.json').write_text(json.dumps(inputs,indent=2)+'\n');(out/'command.json').write_text(json.dumps({'cwd':str(build),'environment':cmd['environment'],'phases':commands},indent=2)+'\n')
sys.path.insert(0,str(base));from run_cases import sample_tree
status={};start=time.monotonic()
for phase,argv in commands.items():
 with (out/(phase+'.log')).open('w') as log,(out/'resources.jsonl').open('a') as resources:
  proc=subprocess.Popen(argv,cwd=build,env=env,stdout=log,stderr=subprocess.STDOUT);(out/'status.json').write_text(json.dumps({'state':'running','phase':phase,'pid':proc.pid,'completed':status},indent=2))
  while proc.poll() is None:
   rows=sample_tree(proc.pid);resources.write(json.dumps({'phase':phase,'elapsed_seconds':time.monotonic()-start,'processes':rows,'sampled_process_tree_rss_bytes':sum(r['rss_bytes'] for r in rows)})+'\n');resources.flush();time.sleep(2)
  status[phase]=proc.returncode
 if status[phase]:break
changed=[x for x,h in inputs.items() if not pathlib.Path(x).exists() or sha(x)!=h]
result={'state':'terminal','phase_exit_codes':status,'seconds':time.monotonic()-start,'pins_verified':not changed,'changed':changed,'binary_sha256':sha(out/'probe') if (out/'probe').exists() else None,'rss_note':'sampled process tree, not peak'};(out/'status.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result),flush=True)
