#!/usr/bin/env python3
"""Relink unchanged qualified coordinate tests against the new geometry objects; no CFD run."""
import hashlib,json,os,subprocess,time
from pathlib import Path
HERE=Path(__file__).resolve().parent;ROOT=HERE.parents[2];BUILD=ROOT/'integration_evidence/build-integrated-v2'
def sha(p):
 h=hashlib.sha256()
 with p.open('rb') as f:
  while b:=f.read(4*1024**2):h.update(b)
 return h.hexdigest()
def main():
 folder=HERE/'coordinate_compatibility_v1_n1';folder.mkdir(exist_ok=True);assert not (folder/'result.json').exists()
 core=json.loads((HERE/'core_v1_n1/result.json').read_text());assert core['status']=='PASS'
 for p,h in core['source_sha256'].items():assert sha(ROOT/p)==h
 pins={};objects=[]
 for name in ('CNativeMesh3D','CNativeField3D'):
  source='UnitTests/Common/adaptation/'+name+'_tests.cpp';data=subprocess.check_output(['git','show','b232f4acbf:'+source]);assert data==(ROOT/source).read_bytes()
  p=BUILD/('UnitTests/test_driver.p/Common_adaptation_'+name+'_tests.cpp.o');objects.append(str(p));pins[str(p)]=sha(p);pins[source]=sha(ROOT/source)
 link=next(s['command'] for s in core['steps'] if s['name']=='link');start=link.index('c++');link=link[start:];link[2]=str(folder/'coordinate_driver');position=link.index('-Wl,--as-needed');link[position:position]=objects
 env=os.environ.copy();env.update(OMP_NUM_THREADS='1',OPENBLAS_NUM_THREADS='1',TMPDIR=str(BUILD/'compile-tmp'));steps=[]
 for name,args,cwd in [('link',link,BUILD),('controls',[str(folder/'coordinate_driver'),'[NativeRepresentableSplit3D],[NativeSegmentFreedom3D]','--abort','--reporter','compact'],folder)]:
  t=time.monotonic()
  with (folder/(name+'.log')).open('w') as out:code=subprocess.run(['nice','-n','19','timeout','120s']+args,cwd=cwd,env=env,stdout=out,stderr=subprocess.STDOUT).returncode
  steps.append(dict(name=name,exit_code=code,seconds=time.monotonic()-t,command=args));assert code==0,name
 result=dict(status='PASS_UNCHANGED_COORDINATE_COMPATIBILITY',steps=steps,reused_objects_and_sources=pins,binary_sha256=sha(folder/'coordinate_driver'),script_sha256=sha(Path(__file__)),scope='Existing serial cancellation/subnormal/segment/saved-M6-midpoint controls only; no full CFD/integrated/MPI lifecycle qualification.')
 (folder/'result.json').write_text(json.dumps(result,indent=2)+'\n');print(result['status'])
if __name__=='__main__':main()
