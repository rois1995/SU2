"""Read-only metric-frame sensitivity of an existing actual M6 row, not a target policy or mesh certificate."""
from pathlib import Path
import hashlib,json,math,struct,time
import numpy as np
R=Path(__file__).resolve().parent
CASE=R/'cases/curved358_front_smooth_upper_20261111T045033_884933'
LD=np.longdouble
sha=lambda p:hashlib.file_digest(Path(p).open('rb'),'sha256').hexdigest()

def read_patch(path):
    data=path.read_bytes();assert len(data)<128<<20
    pos=0
    def unpack(fmt):
        nonlocal pos
        result=struct.unpack_from('<'+fmt,data,pos);pos+=struct.calcsize('<'+fmt);return result
    n,=unpack('Q');assert 0<n<=128
    cells={}
    for _ in range(n):
        cid,=unpack('Q');nodes=[];xyz=[]
        for _ in range(4):
            node,x,y,z=unpack('Qddd');nodes.append(node);xyz.append((x,y,z))
        assert cid not in cells;cells[cid]=(nodes,np.array(xyz,dtype=LD))
    n,=unpack('Q');assert n<=512
    faces=[unpack('QQQQ') for _ in range(n)]
    n,=unpack('Q');assert n<=512;pos+=n*72;assert pos<=len(data)
    return cells,faces

def quality(x,m):
    # Independent extended-precision implementation of the documented metric mean-ratio/scaled-J definitions.
    m=np.asarray(m,dtype=LD);m=m/max(abs(m).flat)
    l00=np.sqrt(m[0,0]);l10=m[1,0]/l00;l20=m[2,0]/l00
    l11=np.sqrt(m[1,1]-l10*l10);l21=(m[2,1]-l20*l10)/l11;l22=np.sqrt(m[2,2]-l20*l20-l21*l21)
    assert np.isfinite([l00,l11,l22]).all() and min(l00,l11,l22)>0
    l=np.array([[l00,0,0],[l10,l11,0],[l20,l21,l22]],dtype=LD)
    dx=x-x[0];size=max(abs(dx).flat);dx/=size;y=dx@l
    vol=np.dot(dx[1],np.cross(dx[2],dx[3]))*l00*l11*l22;assert vol>0
    edges=[np.dot(y[j]-y[i],y[j]-y[i]) for i in range(4) for j in range(i+1,4)]
    q=12*np.cbrt(vol/2)**2/sum(edges)
    jac=min(np.sqrt(LD(2))*vol/np.prod([np.sqrt(np.dot(y[j]-y[i],y[j]-y[i])) for j in range(4) if j!=i]) for i in range(4))
    return float(q),float(jac)

def rotate(a,b):
    a=a/np.sqrt(a@a);b=b/np.sqrt(b@b)
    if a@b<0:b=-b
    v=np.cross(a,b);c=a@b
    k=np.array([[0,-v[2],v[1]],[v[2],0,-v[0]],[-v[1],v[0],0]],dtype=LD)
    return np.eye(3,dtype=LD)+k+(k@k)/(1+c)

def main():
    start=time.perf_counter();cpu=time.process_time()
    evidence=json.loads((R/'front358_assessment.json').read_text())
    paths=[CASE/'prepared_repair_before.bin',CASE/'prepared_repair_before_frames.jsonl',CASE/'actual_bl_front.jsonl']
    for p in paths[:2]:assert sha(p)==evidence['captures'][str(p.relative_to(R.parents[3]))]
    cells,faces=read_patch(paths[0]);frames=[json.loads(line) for line in paths[1].read_text().splitlines()]
    assert set(cells)=={r['cell'] for r in frames}
    results=[];baseline_error=0
    for r in frames:
        ids,x=cells[r['cell']];xx,xy,xz,yy,yz,zz=r['metric'];m=np.array([[xx,xy,xz],[xy,yy,yz],[xz,yz,zz]],dtype=LD)
        q,j=quality(x,m);baseline_error=max(baseline_error,abs(q-r['q']),abs(j-r['J']))
        assert abs(q-r['q'])<2e-7 and abs(j-r['J'])<2e-7
        wall=[f for f in faces if set(f[:3]).issubset(ids)]
        assert bool(wall)==r['wall_child']
        n=np.array(r['wall0_normal'],dtype=LD);n/=np.sqrt(n@n);p=np.eye(3,dtype=LD)-np.outer(n,n)
        variants={}
        for factor in [1,2,4,8,16]:
            trial=m+(factor-1)*(p@m@p)
            variants[str(factor)]=dict(zip(['q','J'],quality(x,trial)))
            assert abs((n@trial@n)/(n@m@n)-1)<1e-12
        row={'cell':r['cell'],'wall_child':bool(wall),'baseline_q':q,'baseline_J':j,'tangent_refinement_controls':variants}
        if wall:
            face=wall[0];points=np.array([x[ids.index(k)] for k in face[:3]],dtype=LD)
            nf=np.cross(points[1]-points[0],points[2]-points[0]);nf/=np.sqrt(nf@nf)
            rr=rotate(n,nf);aligned=rr@m@rr.T
            row['face_aligned_control']=dict(zip(['q','J'],quality(x,aligned)))
            row['normal_vs_face_degrees']=float(np.arccos(np.clip(abs(n@nf),-1,1))*180/np.pi)
        results.append(row)
    counts=[]
    for wall in [True,False]:
        selected=[r for r in results if r['wall_child']==wall]
        for f in ['1','2','4','8','16']:
            counts.append(dict(wall_children=wall,factor=int(f),cells=len(selected),q_bad=sum(r['tangent_refinement_controls'][f]['q']<.2 for r in selected),J_bad=sum(r['tangent_refinement_controls'][f]['J']<.05 for r in selected)))
    old=[json.loads(s) for s in paths[2].read_text().splitlines()]
    windows=[r for r in old if r.get('stage')=='surface_height_window']
    directions=[r for r in old if r.get('stage')=='physical_target_directions']
    geometry=[]
    for r in directions:
        for k,density in zip(r['curvatures'],r['control_planning_directional_density']):
            geometry.append({'facet':r['facet'],'vertex':r['vertex'],'principal_curvature':k,'requested_tangent_size':1/density,
                'local_quadratic_spacing_at_geometry_tolerance':math.sqrt(8e-4/abs(k)) if k else None,
                'local_quadratic_spacing_at_first_height_error':math.sqrt(8*.1*2e-6/abs(k)) if k else None})
    out=dict(scope='Independent read-only sensitivity on existing saved M6 cells/centroid tensors. Finer-tangent and face-aligned controls are NOT production target policies, admissible geometry changes, exact edge quadrature or acceptance proofs.',inputs={str(p.relative_to(R.parents[3])):sha(p) for p in paths},helper_sha256=sha(__file__),baseline_max_abs_error=baseline_error,results=results,counts=counts,
        existing_height_windows=windows,local_curvature_scale_illustrations=geometry,
        limitations=['Local quadratic curvature spacings are illustrations, NOT certified support bounds or required resolution.','Changing target to fit the current cells does not justify a metric policy. Physical/flow/height demands and feasible placement must justify it independently.','Centroid tensor controls do not certify metric-edge quadrature or a complete BL.'],wall_seconds=time.perf_counter()-start,cpu_seconds=time.process_time()-cpu)
    (R/'saved_row369_sensitivity.json').write_text(json.dumps(out,indent=2)+'\n');print(json.dumps({'baseline_error':baseline_error,'counts':counts,'wall_seconds':out['wall_seconds']}))
if __name__=='__main__':main()
