"""Exact Fraction source clipping and high-precision integrals for the actual native sensor-edge path."""
from collections import defaultdict
from decimal import Decimal as D, localcontext
from fractions import Fraction as F
import hashlib
import json
import math
import os
from pathlib import Path
import subprocess
import sys
import time
from audit_cavities import determinant, export, overlap

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
source_paths=['Common/include/adaptation/CNativeMesh3D.hpp','Common/src/adaptation/CNativePredicates3D.cpp',
 'Common/include/adaptation/CNativeField3D.hpp','Common/src/adaptation/CNativeField3D.cpp',
 'Common/src/adt/CADTBaseClass.cpp','Common/src/adt/CADTElemClass.cpp','Common/include/parallelization/mpi_structure.cpp',
 'integration_evidence/native_3d_core_v1/edge_oracle.cpp','integration_evidence/native_3d_core_v1/audit_cavities.py',
 str(Path(__file__).resolve().relative_to(root))]
linked=['predicates.o','field.o','CADTBaseClass.o','CADTElemClass.o','mpi_structure.o']
initial={p:hashlib.sha256((root/p).read_bytes()).hexdigest() for p in source_paths}
status='FAIL'

def run(command,name,text=None):
    start=time.monotonic()
    p=subprocess.run(command,cwd=root,env=env,text=True,input=text,capture_output=True)
    (folder/(name+'.stdout')).write_text(p.stdout);(folder/(name+'.stderr')).write_text(p.stderr)
    records.append(dict(stage=name,command=list(map(str,command)),exit_code=p.returncode,seconds=time.monotonic()-start))
    assert p.returncode==0,name+' failed; evidence retained in '+str(folder)
    return p.stdout

def donor(ids,points,sensors,cell_id=0):
    if determinant([list(map(F,p)) for p in points])<0:
        ids=ids[:];points=points[:];sensors=sensors[:]
        ids[0],ids[1]=ids[1],ids[0];points[0],points[1]=points[1],points[0];sensors[0],sensors[1]=sensors[1],sensors[0]
    assert determinant([list(map(F,p)) for p in points])>0
    return dict(id=cell_id,nodes=ids,points=points,sensors=sensors)

def reference(cells,a,b):
    """Clip affine barycentric halfspaces with exact rationals; enforce complete unambiguous coverage."""
    fa,fb=list(map(F,a)),list(map(F,b));chord=[y-x for x,y in zip(fa,fb)]
    result=[]
    for cell in sorted(cells,key=lambda c:sorted(c['nodes'])):
        tet=[list(map(F,p)) for p in cell['points']];volume=determinant(tet)
        va=[];vb=[]
        for i in range(4):
            ta=tet[:];tb=tet[:];ta[i]=fa;tb[i]=fb
            va.append(determinant(ta));vb.append(determinant(tb))
        lo,hi=F(0),F(1)
        for x,y in zip(va,vb):
            if x<0 and y<0:hi=F(-1);break
            if x<0:lo=max(lo,-x/(y-x))
            if y<0:hi=min(hi,x/(x-y))
        if lo>=hi:continue
        weights=[[ (x+t*(y-x))/volume for x,y in zip(va,vb)] for t in (lo,hi)]
        assert all(sum(w)==1 and min(w)>=0 for w in weights)
        support=sorted(cell['nodes'][i] for i in range(4) if weights[0][i] or weights[1][i])
        result.append(dict(key=sorted(cell['nodes']),begin=lo,end=hi,width=hi-lo,weights=weights,support=support,cell=cell))
    result.sort(key=lambda p:(p['begin'],p['end'],p['key']))
    unique=[];cursor=F(0)
    for p in result:
        if unique and p['begin']==unique[-1]['begin'] and p['end']==unique[-1]['end']:
            if len(p['support'])>=4 or p['support']!=unique[-1]['support']:return None
            continue
        if p['begin']!=cursor:return None
        unique.append(p);cursor=p['end']
    if cursor!=1:return None
    def quadratic(m):
        x,y,z=chord;xx,xy,xz,yy,yz,zz=m
        return xx*x*x+yy*y*y+zz*z*z+2*(xy*x*y+xz*x*z+yz*y*z)
    def dec(x):return D(x.numerator)/D(x.denominator)
    with localcontext() as context:
        context.prec=150
        total=D(0)
        for p in unique:
            tensors=[[sum(w[i]*F(p['cell']['sensors'][i][j]) for i in range(4)) for j in range(6)] for w in p['weights']]
            q0,q1=[dec(quadratic(m)) for m in tensors]
            assert min(q0,q1)>0
            # Independent antiderivative, rather than the production cancellation-safe rearrangement.
            integral=q0.sqrt() if q0==q1 else D(2)/3*(q1*q1.sqrt()-q0*q0.sqrt())/(q1-q0)
            total+=dec(p['width'])*integral
        return unique,str(total)

def slab(x,peak,coarse=1.):
    tets=[[0,1,2,6],[0,2,3,6],[0,3,7,6],[0,7,4,6],[0,4,5,6],[0,5,1,6]]
    result=[]
    for plane in range(4):
        corners=[(plane,0,0),(plane+1,0,0),(plane+1,1,0),(plane,1,0),
                 (plane,0,1),(plane+1,0,1),(plane+1,1,1),(plane,1,1)]
        for indices in tets:
            ids=[4*corners[i][0]+corners[i][1]+2*corners[i][2] for i in indices]
            points=[[x[corners[i][0]],float(corners[i][1]),float(corners[i][2])] for i in indices]
            sensors=[[peak if p[0]==x[2] else coarse,0.,0.,1.,0.,1.] for p in points]
            result.append(donor(ids,points,sensors,len(result)))
    return result

def export_source(name,cells):
    nodes={i:p for c in cells for i,p in zip(c['nodes'],c['points'])};uses=defaultdict(list)
    geometry=[]
    for c in cells:
        a,b,c0,d=c['nodes']
        for face in [(b,c0,d),(a,d,c0),(a,b,d),(a,c0,b)]:
            parity=(-1)**sum(face[i]>face[j] for i in range(3) for j in range(i+1,3))
            uses[tuple(sorted(face))].append(parity)
        geometry.append([list(map(F,p)) for p in c['points']])
    assert all(len(v)==1 or len(v)==2 and sum(v)==0 for v in uses.values())
    for i,a in enumerate(geometry):
        for b in geometry[i+1:]:assert not overlap(a,b)
    assert sum(determinant(t)/6 for t in geometry)==F(cells[-1]['points'][1][0])-F(cells[0]['points'][0][0])
    data=dict(nodes=[[i]+p for i,p in sorted(nodes.items())],cells=[dict(id=c['id'],nodes=c['nodes']) for c in cells])
    export(folder/(name+'_source.su2'),data,{k:v[0] for k,v in uses.items() if len(v)==1})
    (folder/(name+'_sensor.json')).write_text(json.dumps(cells,indent=2)+'\n')

try:
    assert receipt['status']=='PASS' and not receipt.get('changed_during_run')
    for p in source_paths[:7]:assert initial[p]==receipt['source_sha256'][p],p
    for p in linked:assert hashlib.sha256((objects/p).read_bytes()).hexdigest()==receipt['artifact_sha256'][p],p
    (folder/'validated_objects_receipt.json').write_text(json.dumps(receipt,indent=2)+'\n')
    run(['nice','-n','19','g++','-std=c++17','-O2','-fno-fast-math','-fno-unsafe-math-optimizations','-ffp-contract=off',
         '-Wall','-Wextra','-Werror','-I.',str(base/'edge_oracle.cpp')]+[str(objects/p) for p in linked]+['-o',str(folder/'edge_oracle')],'compile_oracle')
    groups=[];c,s=math.cos(.37),math.sin(.37)
    for aspect in [1.,1e4,1e6,1e8,1e10]:
        for scale in [1e-50,1.,1e50]:
            for offset in [0.,1000.]:
                def transform(p):
                    x,y,z=p
                    return [scale*(c*x+s*z/aspect+offset),scale*(y+offset),scale*(-s*x+c*z/aspect+offset)]
                points=[transform(p) for p in [(0,0,0),(1,0,0),(0,1,0),(0,0,1)]]
                nodal=[[float(1+i),.125,0.,2+.5*i,0.,1e10*(1+i)] for i in range(4)]
                a=transform((.125,.25,.125));b=transform((.5,.125,.125));outside=transform((1,1,1))
                groups.append(dict(name='rotated_'+str(len(groups)),cells=[donor(list(range(4)),points,nodal)],edges=[[a,b],[b,a],[a,outside]]))
    for name,x,peak,coarse in [
        ('micron',[0.,.3,.3000005,.300001,1.],4e12,1.),
        ('two_spacings',[0.,.3,.3+math.ldexp(1.,-54),.3+math.ldexp(1.,-53),1.],4e32,1.),
        ('coincident_cuts',[-1e100,0.,.5,1.,1e100],1e100,1e-100)]:
        cells=slab(x,peak,coarse);a=[x[0],.25,.25];b=[x[-1],.25,.25]
        export_source(name,cells)
        groups.append(dict(name=name,cells=cells,edges=[[a,b],[b,a],[[x[0],.125,.3],[x[-1],.6,.7]]]))
    p=[[0.,0.,0.],[1.,0.,0.],[0.,1.,0.],[0.,0.,1.]];m=[[1.,0.,0.,1.,0.,1.]]*4
    up=donor([0,1,2,3],p,m);down=donor([0,1,2,4],p[:3]+[[0.,0.,-1.]],m,1)
    groups.append(dict(name='face_and_vertex',cells=[up,down],edges=[[[.125,.125,0.],[.5,.125,0.]],[[0.,0.,-1.],[0.,0.,1.]]]))
    shifted=donor([4,5,6,7],[[v[0]+3,v[1],v[2]] for v in p],m,1)
    groups.append(dict(name='gap',cells=[up,shifted],edges=[[[.125,.125,.125],[3.125,.125,.125]]]))
    duplicate=donor([4,5,6,7],p,m,1)
    groups.append(dict(name='overlap',cells=[up,duplicate],edges=[[[.1,.1,.1],[.2,.1,.1]]]))
    groups.append(dict(name='empty',cells=[],edges=[[[0.,0.,0.],[1.,0.,0.]]]))
    lines=[];cases=[]
    for group in groups:
        lines.append(str(len(group['cells'])))
        for cell in group['cells']:
            record=[str(cell['id'])]
            for i in range(4):record.extend([str(cell['nodes'][i])]+[format(x,'.17g') for x in cell['points'][i]+cell['sensors'][i]])
            lines.append(' '.join(record))
        lines.append(str(len(group['edges'])))
        for a,b in group['edges']:
            lines.append(' '.join(format(x,'.17g') for x in a+b))
            expected=reference(group['cells'],a,b)
            cases.append(dict(name=group['name'],a=a,b=b,expected=expected))
    text='\n'.join(lines)+'\n';(folder/'samples.txt').write_text(text)
    (folder/'source_groups.json').write_text(json.dumps(groups,indent=2)+'\n')
    serial=[]
    for case in cases:
        p=dict(name=case['name'],a=case['a'],b=case['b'],covered=case['expected'] is not None)
        if p['covered']:
            pieces,length=case['expected'];p['length']=length
            p['intervals']=[dict(key=q['key'],begin=str(q['begin']),end=str(q['end']),width=str(q['width']),
                                weights=[[str(w) for w in row] for row in q['weights']]) for q in pieces]
        serial.append(p)
    (folder/'exact_cases.json').write_text(json.dumps(serial,indent=2)+'\n')
    output=run(['nice','-n','19',str(folder/'edge_oracle')],'edges',text).splitlines()
    assert len(output)==len(cases)
    accepted=rejected=pieces_total=0;length_error=weight_error=width_error=0.;defects=[]
    for case,line in zip(cases,output):
        values=line.split();expected=case['expected'];assert bool(int(values[0]))==(expected is not None),case['name']
        if expected is None:rejected+=1;assert len(values)==1;continue
        accepted+=1;pieces,length=expected;n=int(values[2]);assert n==len(pieces) and len(values)==3+15*n
        err=float(abs(D(values[1])-D(length))/D(length));length_error=max(length_error,err);assert err<=1e-12
        for i,p in enumerate(pieces):
            v=values[3+15*i:3+15*(i+1)];assert list(map(int,v[:4]))==p['key']
            assert abs(F(v[4])-p['begin'])<=F(1,10**12) and abs(F(v[5])-p['end'])<=F(1,10**12)
            err=float(abs(F(v[6])-p['width'])/p['width']);width_error=max(width_error,err);assert err<=1e-12
            for observed,w in zip(v[7:],p['weights'][0]+p['weights'][1]):
                err=float(abs(F(observed)-w));weight_error=max(weight_error,err);assert err<=1e-12
        pieces_total+=n
        if case['name'] in ('micron','two_spacings','coincident_cuts'):
            defects.append(dict(source=case['name']+'_source.su2',edge=[case['a'],case['b']],sensor_length=values[1],
                                exceeds_initial_edge_limit=D(values[1])>D('1.8'),scope='Sensor residual only; no adapted proposal or composed BL acceptance'))
    profiles=[json.loads(line[6:]) for line in (folder/'edges.stderr').read_text().splitlines() if line.startswith('STATS ')]
    assert len(profiles)==len(groups)
    for p in profiles:
        assert p['edge_seconds']>=p['search_seconds']+p['trace_seconds']+p['integral_seconds']
        assert p['maximum_candidates']<=256 and p['maximum_pieces']<=256
    (folder/'profiles.json').write_text(json.dumps(profiles,indent=2)+'\n')
    (folder/'edge_defects.json').write_text(json.dumps(defects,indent=2)+'\n')
    audit=dict(status='PASS',source_groups=len(groups),edges=len(cases),covered=accepted,rejected=rejected,intervals=pieces_total,
               maximum_relative_length_error=length_error,maximum_absolute_weight_error=weight_error,maximum_relative_width_error=width_error,
               scope='Original sensor-only P1 edge integration; exact binary64 Fraction clipping/coverage/weights and 150-digit Decimal integral references. Rotated aspect1..1e10, scales1e-50..1e50, thin and coincident cuts; faceted slab exports independently audited for positive volumes, closed skin, no overlaps and exact unit-cross-section volume. No composed BL, adapted grid, CFD/MPI or speedup certificate.')
    (folder/'oracle_audit.json').write_text(json.dumps(audit,indent=2)+'\n');print(json.dumps(audit,indent=2),flush=True)
    status='PASS'
finally:
    final={p:hashlib.sha256((root/p).read_bytes()).hexdigest() for p in source_paths}
    changed=[p for p in source_paths if final[p]!=initial[p]]
    if changed:status='FAIL'
    result=dict(status=status,stages=records,source_sha256=initial,final_source_sha256=final,changed_during_run=changed,
                reused_object_sha256={p:hashlib.sha256((objects/p).read_bytes()).hexdigest() for p in linked if (objects/p).exists()})
    result['artifact_sha256']={p.name:hashlib.sha256(p.read_bytes()).hexdigest() for p in folder.iterdir() if p.is_file()}
    (folder/'validation.json').write_text(json.dumps(result,indent=2)+'\n');print(status,folder,flush=True)
    if changed:raise RuntimeError('Source changed during validation: '+', '.join(changed))
