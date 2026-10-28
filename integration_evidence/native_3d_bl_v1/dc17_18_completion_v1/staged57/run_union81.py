"""Private geometry primitive build: no main archive or test-driver mutation."""
import json, os, shlex, shutil, subprocess, sys, time
from pathlib import Path
from run_cases import sha, sample_tree, save, loaded_libraries
ROOT=Path(__file__).resolve().parent
BUILD=ROOT.parents[2]/'build-integrated-v2'
OUT=ROOT/('union'+str(int(sys.argv[1]) if len(sys.argv)==2 else 81))
FILES=('CNativeAffineReference.cpp','CNativeGeometryBackend3D.cpp','CNativeGeometryBackend3D_tests.cpp')

def main():
    if OUT.exists(): raise RuntimeError('Receipt exists')
    db=json.loads((BUILD/'compile_commands.json').read_text())
    selected=[next(x for x in db if x['file'].endswith('/'+name)) for name in FILES]
    commands={};objects=[]
    for i,entry in enumerate(selected):
        args=shlex.split(entry['command']);obj=OUT/(str(i)+'.o');objects.append(obj)
        for flag in ('-o','-MF','-MQ'):
            index=args.index(flag)+1;args[index]=str(obj)+('.d' if flag=='-MF' else '')
        commands['compile_'+str(i)]=args
    ninja=BUILD/'tools/ninja'
    raw=subprocess.check_output([str(ninja),'-C',str(BUILD),'-t','commands','UnitTests/test_driver'],text=True).splitlines()[-1]
    link=shlex.split(raw);index=link.index('-o')+1;link[index]=str(OUT/'test_driver')
    original=selected[2]['output'];assert original in link;link[link.index(original)]=str(objects[2])
    index=link.index('-Wl,--start-group');link[index:index]=[str(objects[0]),str(objects[1])]
    commands['link']=link
    commands['backend_tests']=[str(OUT/'test_driver'),'[NativeGeometryBackend3D]']
    buildpins=json.loads((ROOT/'build79_source_pins.json').read_text());pins={str(p):sha(Path(p)) for p in buildpins}
    changed_since_build79=[p for p in pins if pins[p]!=buildpins[p]]
    paths=[Path(__file__).resolve(),ROOT/'run_cases.py',BUILD/'compile_commands.json',ninja,ROOT/'build79_source_pins.json',ROOT/'build79_status.json']
    paths += [BUILD/arg for arg in link if (arg.endswith('.o') and not arg.startswith(str(OUT))) or arg.endswith('.a')]
    for archive in [p for p in paths if p.suffix=='.a']:
        with archive.open('rb') as stream: magic=stream.read(8)
        if magic==b'!<thin>\n':
            members=subprocess.check_output(['ar','t',str(archive.resolve())],text=True).splitlines()
            for member in members:
                path=Path(member);paths.append(path if path.is_absolute() else archive.parent/path)
    paths += [Path(p) for p in loaded_libraries()]
    for program in ('c++','ar','nice','timeout'):
        path=shutil.which(program)
        if path:paths.append(Path(path).resolve())
    for p in paths:pins[str(p.resolve())]=sha(p)
    OUT.mkdir();save(OUT/'pins.json',pins);save(OUT/'command.json',{'cwd':str(BUILD),'commands':commands,'scope':'private primitive check; not integrated build','changed_since_build79':changed_since_build79})
    env=os.environ.copy();env.update({'OMP_NUM_THREADS':'1','OPENBLAS_NUM_THREADS':'1','MKL_NUM_THREADS':'1','TMPDIR':str(BUILD/'compile-tmp')})
    exits={};generated={};start=time.monotonic()
    for label,args in commands.items():
        with (OUT/(label+'.log')).open('w') as log,(OUT/'resources.jsonl').open('a') as res:
            proc=subprocess.Popen(['nice','-n','19','/usr/bin/time','-v','timeout','300',*args],cwd=BUILD,env=env,stdout=log,stderr=subprocess.STDOUT)
            save(OUT/'status.json',{'state':'running','pid':proc.pid,'phase':label,'completed':exits})
            while proc.poll() is None:
                rows=sample_tree(proc.pid);res.write(json.dumps({'phase':label,'elapsed_seconds':time.monotonic()-start,'processes':rows,'sampled_process_tree_rss_bytes':sum(r['rss_bytes'] for r in rows)})+'\n');res.flush();time.sleep(2)
            exits[label]=proc.returncode
            if proc.returncode==0 and label.startswith('compile_'):
                obj=objects[int(label.rsplit('_',1)[1])];generated[str(obj)]=sha(obj)
            if proc.returncode==0 and label=='link':generated[str(OUT/'test_driver')]=sha(OUT/'test_driver')
        if proc.returncode:break
    unchanged=all(sha(Path(p))==h for p,h in pins.items())
    save(OUT/'status.json',{'state':'terminal','exit_codes':exits,'pins_verified':unchanged,'seconds':time.monotonic()-start,'rss_note':'sampled process tree; not allocation peak','scope':'private primitive check only','generated_sha256':generated})
    if not unchanged or len(exits)!=len(commands) or any(exits.values()):sys.exit(1)

if __name__=='__main__':main()
