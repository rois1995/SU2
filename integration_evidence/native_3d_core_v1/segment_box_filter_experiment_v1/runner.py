"""One-CPU correctness oracle for a conservative broad phase; no production code or timing claim."""
from pathlib import Path
from fractions import Fraction as F
import hashlib,json,math,os,random,struct,subprocess,sys,time
folder=Path(__file__).resolve().parent;root=folder.parents[2]
sys.path.insert(0,str(root/'integration_evidence/native_cluster_3d_core_v1'))
from run import command
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
rng=random.Random(20261012);cases=[]
def exact(a,b,box):
    if not all(math.isfinite(x) for x in a+b+box) or any(box[i]>box[i+3] for i in range(3)):return None
    lo,hi=F(0),F(1)
    for i in range(3):
        x,y,m,n=map(F,(a[i],b[i],box[i],box[i+3]));d=y-x
        if not d:
            if x<m or x>n:return False
        else:
            left,right=sorted(((m-x)/d,(n-x)/d));lo=max(lo,left);hi=min(hi,right)
            if lo>hi:return False
    return True
def add(a,b,box,label):
    cases.append(dict(a=list(a),b=list(b),box=list(box),family=label,exact=exact(list(a),list(b),list(box))))
for _ in range(1800):
    a=[rng.uniform(-2,2) for _ in range(3)];b=[rng.uniform(-2,2) for _ in range(3)]
    ends=[sorted((rng.uniform(-1,1),rng.uniform(-1,1))) for _ in range(3)]
    add(a,b,[v[0] for v in ends]+[v[1] for v in ends],'random')
for power in (-1070,-1000,-500,-20,0,20,500,1000):
    factor=math.ldexp(1.,power)
    for _ in range(150):
        a=[rng.uniform(-1,1)*factor for _ in range(3)];b=[rng.uniform(-1,1)*factor for _ in range(3)]
        ends=[sorted((rng.uniform(-1,1)*factor,rng.uniform(-1,1)*factor)) for _ in range(3)]
        add(a,b,[v[0] for v in ends]+[v[1] for v in ends],'scaled_'+str(power))
for value in (0.,.1,.25,1.,math.ldexp(1.,-1000),math.ldexp(1.,1000)):
    other=math.nextafter(value,math.inf)
    for step in (value,other,math.nextafter(value,-math.inf)):
        add([value,value,value],[other,other,other],[step,step,step,step,step,step],'ulp_point_box')
        add([value,value,value],[value,value,value],[step,step,step,step,step,step],'zero_segment')
for power in (-1000,-20,0,20,1000):
    h=math.ldexp(1.,power)
    for face in range(3):
        for sign in (-1.,1.):
            a=[.25,.25,.25];b=[.75,.75,.75];a[face]=b[face]=sign*.5
            box=[-.5,-.5,-.5,.5,.5,.5]
            add([x*h for x in a],[x*h for x in b],[x*h for x in box],'closed_face_touch')
            add([x*h for x in a],[x*h for x in a],[x*h for x in box],'closed_point_touch')
# Sample actual source boxes and requested edges from the already certified frozen n8 mesh.
reference=root/'integration_evidence/native_3d_core_v1/query_packing_n8_n1_m1_rounds1024_v1/envelope_s1_initial.json'
data=json.loads(reference.read_text());nodes={n[0]:n[1:] for n in data['nodes']}
source=data['original'][::17]
for cell in data['cells'][::max(1,len(data['cells'])//10)][:10]:
    points=[nodes[n] for n in cell['nodes']]
    for i in range(4):
        for j in range(i+1,4):
            a,b=points[i],points[j]
            for tet in source:
                verts=[n[1:4] for n in tet['vertices']]
                box=[min(v[k] for v in verts) for k in range(3)]+[max(v[k] for v in verts) for k in range(3)]
                if all(min(a[k],b[k])<=box[k+3] and max(a[k],b[k])>=box[k] for k in range(3)):
                    add(a,b,box,'actual_n8_box_candidates')
add([2.,0.,float('nan')],[3.,0.,0.],[0.,0.,0.,1.,1.,1.],'invalid_fail_open')
add([0.,0.,0.],[1.,1.,1.],[1.,0.,0.,0.,1.,1.],'invalid_fail_open')
(folder/'cases.json').write_text(json.dumps(cases,indent=2)+'\n')
lines=[]
for i,c in enumerate(cases):
    bits=[str(struct.unpack('<Q',struct.pack('<d',x))[0]) for x in c['a']+c['b']+c['box']]
    lines.append(str(i)+' '+' '.join(bits))
(folder/'input.txt').write_text('\n'.join(lines)+'\n')
env=dict(os.environ,OMP_NUM_THREADS='1',OPENBLAS_NUM_THREADS='1',MKL_NUM_THREADS='1',PYTHONDONTWRITEBYTECODE='1',TMPDIR=str(folder))
stages=[];summaries=[];status='FAIL'
try:
    for args,label in [(['uptime'],'host_load'),(['free','-m'],'host_memory')]:
        stages.append(command(args,folder,label,env,30));assert not stages[-1]['exit_code']
    if int(next(l.split()[1] for l in Path('/proc/meminfo').read_text().splitlines() if l.startswith('MemAvailable:')))<2*1024**2:raise RuntimeError('Low available memory')
    allowed=sorted(os.sched_getaffinity(0));cpu=allowed[-1];os.sched_setaffinity(0,{cpu});os.nice(19)
    (folder/'host_gate.json').write_text(json.dumps(dict(cpu=cpu,nice=19,threads=1,owned_compute_cpus=1,scope='Correctness experiment under contention; no timing/scaling claim'),indent=2)+'\n')
    for name,flags in [('strict',['-O2']),('native',['-O3','-march=native','-ffast-math','-fno-finite-math-only']),('ld64',['-O2','-mlong-double-64'])]:
        binary=folder/('filter_'+name)
        row=command(['c++','-std=c++17',*flags,str(folder/'filter.cpp'),'-o',str(binary)],folder,'compile_'+name,env,120);stages.append(row);assert not row['exit_code']
        with (folder/'input.txt').open('r') as inp,(folder/(name+'.stdout')).open('w') as out,(folder/(name+'.stderr')).open('w') as err:
            process=subprocess.run([str(binary)],stdin=inp,stdout=out,stderr=err,env=env,timeout=120)
        assert not process.returncode
        output=[list(map(int,l.split())) for l in (folder/(name+'.stdout')).read_text().splitlines()]
        assert len(output)==len(cases)
        rejected=uncertain=actual_rejected=0
        for i,(case,row) in enumerate(zip(cases,output)):
            assert row[0]==i and row[1] in (0,1)
            if case['exact'] is None or case['exact']:assert row[1]==1,(name,i,case)
            rejected+=not row[1];uncertain+=bool(row[1] and case['exact'] is False)
            actual_rejected+=case['family']=='actual_n8_box_candidates' and not row[1]
        summaries.append(dict(mode=name,cases=len(cases),false_rejections=0,certified_box_rejections=rejected,false_positives_retained=uncertain,actual_n8_box_candidates=sum(c['family']=='actual_n8_box_candidates' for c in cases),actual_n8_candidates_rejected=actual_rejected,binary_sha256=sha(binary)))
    status='PASS'
finally:
    (folder/'validation.json').write_text(json.dumps(dict(status=status,stages=stages,summary=summaries,source_sha256=sha(folder/'filter.cpp'),runner_sha256=sha(Path(__file__)),actual_mesh_sha256=sha(reference),scope='Standalone conservative projected-box prefilter correctness versus exact binary64 rational closed segment/box oracle. Three compiler modes including production fast-math and binary64 long double. Not a production integration, tetrahedral coverage, MPI or performance certificate.'),indent=2)+'\n')
    print(status,len(cases),summaries,flush=True)
raise SystemExit(status!='PASS')
