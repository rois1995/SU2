import json,os,shlex,subprocess,time,sys,hashlib,shutil
from pathlib import Path
root=Path('/media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated');base=root/'integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57';build=root/'integration_evidence/build-integrated-v2';sys.path.insert(0,str(base));from run_cases import sample_tree,save
out=base/'syntax86';out.mkdir(exist_ok=False)
files=subprocess.check_output(['git','diff','--name-only','HEAD','--','Common/src/adaptation','UnitTests/Common/adaptation'],cwd=root,text=True).splitlines();files=[f for f in files if f.endswith('.cpp')]
db=json.loads((build/'compile_commands.json').read_text());commands={}
for f in files:
 matches=[x for x in db if (Path(x['directory'])/x['file']).resolve()==(root/f).resolve()]
 if len(matches)!=1:raise RuntimeError('compile command missing '+f)
 args=shlex.split(matches[0]['command']);clean=[];i=0
 while i<len(args):
  if args[i] in ('-o','-MF','-MQ','-MT'):i+=2;continue
  if args[i] in ('-c','-MD','-MMD'):i+=1;continue
  clean.append(args[i]);i+=1
 commands[f]=clean+['-fsyntax-only']
paths=set(json.loads((base/'build79_source_pins.json').read_text()))|{str((root/f).resolve()) for f in files}|{str(build/'compile_commands.json'),str(Path(__file__).resolve()),str((base/'run_cases.py').resolve()),str(Path(shutil.which('c++')).resolve())}
def sha(p):
 h=hashlib.sha256()
 with open(p,'rb') as f:
  for b in iter(lambda:f.read(1024*1024),b''):h.update(b)
 return h.hexdigest()
pins={p:sha(p) for p in paths};save(out/'pins.json',pins);save(out/'command.json',{'cwd':str(build),'commands':commands,'scope':'syntax only, not a build or validation'})
env=os.environ.copy();env.update({'OMP_NUM_THREADS':'1','OPENBLAS_NUM_THREADS':'1','MKL_NUM_THREADS':'1','TMPDIR':str(build/'compile-tmp')});results={};start=time.monotonic()
for i,(f,args) in enumerate(commands.items()):
 with (out/(str(i)+'.log')).open('w') as log,(out/'resources.jsonl').open('a') as resources:
  proc=subprocess.Popen(['nice','-n','19','timeout','180',*args],cwd=build,env=env,stdout=log,stderr=subprocess.STDOUT)
  save(out/'status.json',{'state':'running','pid':proc.pid,'file':f,'completed':results})
  while proc.poll() is None:
   rows=sample_tree(proc.pid);resources.write(json.dumps({'file':f,'elapsed_seconds':time.monotonic()-start,'processes':rows,'sampled_process_tree_rss_bytes':sum(r['rss_bytes'] for r in rows)})+'\n');resources.flush();time.sleep(2)
  results[f]={'exit_code':proc.returncode,'log':str(i)+'.log'}
unchanged=all(sha(p)==h for p,h in pins.items());save(out/'status.json',{'state':'terminal','results':results,'pins_verified':unchanged,'seconds':time.monotonic()-start,'rss_note':'sampled process tree, not peak allocation'});sys.exit(0 if unchanged and all(x['exit_code']==0 for x in results.values()) else 1)
