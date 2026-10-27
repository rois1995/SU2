import hashlib,json,os,subprocess,time
from pathlib import Path
p=Path(__file__).resolve().parent
repo=Path.cwd()
def sha(f):return hashlib.sha256(Path(f).read_bytes()).hexdigest()
def save(n,v):(p/n).write_text(json.dumps(v,indent=2)+"\n")
files=[repo/x for x in ["Common/include/adaptation/CNativeAtlas3D.hpp","Common/src/adaptation/CNativeAtlas3D.cpp","Common/include/adaptation/CNativeTopology3D.hpp","Common/include/adaptation/CNativeMesh3D.hpp","UnitTests/Common/adaptation/CNativeAtlas3D_tests.cpp","externals/catch2/catch.hpp"]]+[p/"main.cpp",Path(__file__),Path("/usr/bin/c++").resolve()]
pins={str(f):sha(f) for f in files};save("pins.json",pins)
assert not (p/"status.json").exists()
env=os.environ.copy();env.update(TMPDIR=str(repo/"integration_evidence/build-integrated-v2/compile-tmp"),OMP_NUM_THREADS="1",OPENBLAS_NUM_THREADS="1",MKL_NUM_THREADS="1")
commands=[["nice","-n","19","/usr/bin/time","-v","-o",str(p/"compile.resources"),"c++","-std=c++17","-O2","-fno-fast-math","-Iexternals/catch2",str(p/"main.cpp"),"Common/src/adaptation/CNativeAtlas3D.cpp","UnitTests/Common/adaptation/CNativeAtlas3D_tests.cpp","-o",str(p/"tests")],["nice","-n","19","/usr/bin/time","-v","-o",str(p/"test.resources"),str(p/"tests"),"[NativeAtlas3D]"]]
save("command.json",{"commands":commands,"environment":{k:env[k] for k in ["TMPDIR","OMP_NUM_THREADS","OPENBLAS_NUM_THREADS","MKL_NUM_THREADS"]}})
start=time.time();code=0
for label,command in zip(["compile","test"],commands):
 with (p/(label+".log")).open("w") as log:
  job=subprocess.Popen(command,env=env,stdout=log,stderr=subprocess.STDOUT)
  save("status.json",{"state":"running","stage":label,"pid":job.pid});print(label,job.pid,flush=True);code=job.wait()
 if code:break
 if label=="compile":pins[str(p/"tests")]=sha(p/"tests");save("pins.json",pins)
changed=[f for f,h in pins.items() if sha(f)!=h]
result={"state":"terminal","exit_code":code,"seconds":time.time()-start,"pins_verified":not changed,"changed":changed};save("status.json",result);print(json.dumps(result),flush=True)
raise SystemExit(code or bool(changed))
