"""Exact binary64 geometry/tensor references for actual indexed native original-P1 queries."""
from fractions import Fraction as F
import hashlib
import json
import math
import os
from pathlib import Path
import subprocess
import sys
import time
root=Path(__file__).resolve().parents[2]
base=root/'integration_evidence/native_3d_core_v1'
objects=Path(sys.argv[1]).resolve()
folder=Path(sys.argv[2]).resolve()
print('Reused validated object folder:',objects,flush=True)
print('Case working and retained folder:',folder,flush=True)
folder.mkdir(exist_ok=False)
env=dict(os.environ,OMP_NUM_THREADS='1',OPENBLAS_NUM_THREADS='1',MKL_NUM_THREADS='1')
receipt=json.loads((objects/'validation.json').read_text())
records=[]
status='FAIL'
source_paths=['Common/include/adaptation/CNativeMesh3D.hpp','Common/src/adaptation/CNativePredicates3D.cpp',
 'Common/include/adaptation/CNativeField3D.hpp','Common/src/adaptation/CNativeField3D.cpp',
 'Common/src/adt/CADTBaseClass.cpp','Common/src/adt/CADTElemClass.cpp','Common/include/parallelization/mpi_structure.cpp',
 'integration_evidence/native_3d_core_v1/field_oracle.cpp',str(Path(__file__).resolve().relative_to(root))]
linked_objects=['predicates.o','field.o','CADTBaseClass.o','CADTElemClass.o','mpi_structure.o']
def run(command,name,text=None):
    start=time.monotonic()
    p=subprocess.run(command,cwd=root,env=env,text=True,input=text,capture_output=True)
    (folder/(name+'.stdout')).write_text(p.stdout);(folder/(name+'.stderr')).write_text(p.stderr)
    records.append(dict(stage=name,command=list(map(str,command)),exit_code=p.returncode,seconds=time.monotonic()-start))
    assert p.returncode==0,name+' failed; evidence retained in '+str(folder)
    return p.stdout

def sub(a,b):return [x-y for x,y in zip(a,b)]
def determinant(p):
    a,b,c=[sub(q,p[0]) for q in p[1:]]
    return a[0]*(b[1]*c[2]-b[2]*c[1])-a[1]*(b[0]*c[2]-b[2]*c[0])+a[2]*(b[0]*c[1]-b[1]*c[0])
def quadratic(m,d):
    x,y,z=d;xx,xy,xz,yy,yz,zz=m
    return xx*x*x+yy*y*y+zz*z*z+2*(xy*x*y+xz*x*z+yz*y*z)

try:
    assert receipt['status']=='PASS'
    for p in source_paths[:7]:assert hashlib.sha256((root/p).read_bytes()).hexdigest()==receipt['source_sha256'][p],p
    for p in linked_objects:assert hashlib.sha256((objects/p).read_bytes()).hexdigest()==receipt['artifact_sha256'][p],p
    (folder/'validated_objects_receipt.json').write_text(json.dumps(receipt,indent=2)+'\n')
    run(['nice','-n','19','g++','-std=c++17','-O2','-fno-fast-math','-ffp-contract=off','-Wall','-Wextra','-Werror',
         '-I.',str(base/'field_oracle.cpp')]+[str(objects/p) for p in linked_objects]+['-o',str(folder/'field_oracle')],'compile_oracle')
    cases=[]
    c,s=math.cos(.37),math.sin(.37)
    for aspect in [1.,1e4,1e6,1e8,1e10]:
        for scale in [1e-50,1.,1e50]:
            for offset in [0.,1000.]:
                def transform(p):
                    x,y,z=p
                    return [scale*(c*x+s*z/aspect+offset),scale*(y+offset),scale*(-s*x+c*z/aspect+offset)]
                tet=[transform(p) for p in [(0,0,0),(1,0,0),(0,1,0),(0,0,1)]]
                exact=list(map(lambda row:list(map(F,row)),tet))
                volume=determinant(exact)
                assert volume>0
                for p in [(.125,.25,.125),(.25,.25,.25),(.5,.125,.125),(.75,.125,.25),(0,0,0),(1,0,0),(1,1,1)]:
                    q=transform(p);fq=list(map(F,q));weights=[]
                    for i in range(4):
                        replacement=exact[:];replacement[i]=fq
                        weights.append(determinant(replacement)/volume)
                    assert sum(weights)==1
                    cases.append(dict(aspect=aspect,scale=scale,translation=offset,donor=tet,point=q,
                                      inside=all(w>=0 for w in weights),weights=[str(w) for w in weights]))
    text='\n'.join(' '.join(format(x,'.17g') for row in case['donor']+[case['point']] for x in row) for case in cases)+'\n'
    (folder/'samples.txt').write_text(text);(folder/'exact_cases.json').write_text(json.dumps(cases,indent=2)+'\n')
    output=run(['nice','-n','19',str(folder/'field_oracle')],'queries',text).splitlines()
    assert len(output)==len(cases)==210
    inside=outside=exact_calls=0;maximum_weight_error=maximum_tensor_defect=0.
    directions=[[F(1),F(0),F(0)],[F(0),F(1),F(0)],[F(0),F(0),F(1)],
                [F(1),F(1),F(1)],[F(1,10),F(1),F(1,100)]]
    nodal=[[F(1+i),F(1,8),F(0),F(2)+F(i,2),F(0),F(int(1e10)*(1+i))] for i in range(4)]
    for case,line in zip(cases,output):
        values=line.split();assert bool(int(values[0]))==case['inside']
        if not case['inside']:outside+=1;assert len(values)==1;continue
        inside+=1;assert len(values)==12
        weights=list(map(F,case['weights']))
        errors=[abs(F(v)-w) for v,w in zip(values[1:5],weights)]
        maximum_weight_error=max(maximum_weight_error,max(map(float,errors)))
        assert max(errors)<=F(1,10**12)
        tensor=[sum(weights[i]*nodal[i][j] for i in range(4)) for j in range(6)]
        actual=list(map(F,values[5:11]))
        for d in directions:
            defect=abs(quadratic(actual,d)-quadratic(tensor,d))/quadratic(tensor,d)
            maximum_tensor_defect=max(maximum_tensor_defect,float(defect))
            assert defect<=F(1,10**12)
        exact_calls+=int(values[11])
    result=dict(status='PASS',queries=len(cases),inside=inside,outside=outside,maximum_weight_error=maximum_weight_error,
                maximum_directional_tensor_defect=maximum_tensor_defect,exact_predicate_calls=exact_calls,
                scope='210 actual indexed immutable P1 queries on rotated, scaled, translated thin tetrahedra, checked against exact binary64 Fraction weights and directional tensor references. Single serial control; no geometric BL provider, metric-edge integration, solver or MPI certificate.')
    (folder/'oracle_audit.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result,indent=2),flush=True)
    status='PASS'
finally:
    result=dict(status=status,stages=records,source_sha256={p:hashlib.sha256((root/p).read_bytes()).hexdigest() for p in source_paths},
        reused_object_sha256={p:hashlib.sha256((objects/p).read_bytes()).hexdigest() for p in linked_objects if (objects/p).exists()})
    result['artifact_sha256']={p.name:hashlib.sha256(p.read_bytes()).hexdigest() for p in folder.iterdir() if p.is_file()}
    (folder/'validation.json').write_text(json.dumps(result,indent=2)+'\n');print(status,folder,flush=True)
