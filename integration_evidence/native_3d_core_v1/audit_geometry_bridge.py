"""Independent small-mesh bridge, transported P1 sensor, BDF2 and actual SU2-output audit."""
from collections import Counter
from fractions import Fraction as F
from pathlib import Path
import copy,json,math,re,sys
from audit_coupled import frame,metric,sources
from audit_cavities import determinant

MARKERS={'x_minus','x_plus','y_minus','y_plus','z_plus','z_minus_a','z_minus_b'}

def signature(data,markers=MARKERS):
    points={n[0]:tuple(n[1:]) for n in data['nodes']}
    names=data['marker_names'];assert set(names)==markers and len(names)==len(markers)
    facets={f['id']:names[f['marker']] for f in data['facets']}
    cells=Counter(tuple(sorted(points[i] for i in c['nodes'])) for c in data['cells'])
    boundary=Counter((facets[f['facet']],tuple(sorted(points[i] for i in f['nodes']))) for f in data['surface'])
    return cells,boundary

def su2_signature(path,markers=MARKERS):
    rows=[s.strip() for s in path.read_text().splitlines() if s.strip() and not s.lstrip().startswith(('%','#'))]
    def header(key,start=0):
        i=next(i for i in range(start,len(rows)) if rows[i].split('=',1)[0].strip()==key)
        return i,rows[i].split('=',1)[1].strip()
    _,dimension=header('NDIME');assert int(dimension)==3
    i,count=header('NELEM');cells=[]
    for row in rows[i+1:i+1+int(count)]:
        r=list(map(int,row.split()));assert r[0]==10;cells.append(r[1:5])
    assert len(cells)==int(count)
    i,count=header('NPOIN');points={}
    for j,row in enumerate(rows[i+1:i+1+int(count)]):
        r=row.split();id=int(r[3]) if len(r)>3 else j
        assert id not in points;points[id]=tuple(map(float,r[:3]))
    assert len(points)==int(count) and all(math.isfinite(x) for p in points.values() for x in p)
    i,count=header('NMARK');boundary=Counter()
    for _ in range(int(count)):
        i,name=header('MARKER_TAG',i+1);i,n=header('MARKER_ELEMS',i+1)
        assert name in markers
        for row in rows[i+1:i+1+int(n)]:
            r=list(map(int,row.split()));assert r[0]==5
            boundary[name,tuple(sorted(points[k] for k in r[1:4]))]+=1
        i+=int(n)
    assert all(determinant([tuple(F(x) for x in points[i]) for i in c])>0 for c in cells),'Nonpositive actual SU2 tetrahedron'
    return Counter(tuple(sorted(points[i] for i in c)) for c in cells),boundary

def cgns_signature(path,markers=MARKERS):
    import h5py
    with h5py.File(path,'r') as file:
        def labelled(parent,label):
            return [g for g in parent.values() if isinstance(g,h5py.Group) and g.attrs.get('label')==label.encode()]
        bases=labelled(file,'CGNSBase_t');assert len(bases)==1
        base=bases[0];assert list(base[' data'][...])==[3,3]
        zones=labelled(base,'Zone_t');assert len(zones)==1;zone=zones[0]
        count,volume_count,boundary_count=map(int,zone[' data'][...].reshape(-1));assert 0<count<=1024 and 0<volume_count<=4096 and boundary_count==0
        grids=labelled(zone,'GridCoordinates_t');assert len(grids)==1
        axes=[grids[0]['Coordinate'+axis][' data'][...].reshape(-1).tolist() for axis in 'XYZ'];assert all(len(a)==count for a in axes)
        points={i+1:tuple(p) for i,p in enumerate(zip(*axes))};assert all(math.isfinite(x) for p in points.values() for x in p)
        cells=Counter();boundary=Counter();ranges={};seen=set()
        for section in labelled(zone,'Elements_t'):
            kind=int(section[' data'][...].reshape(-1)[0]);assert kind in (5,10)
            first,last=map(int,section['ElementRange'][' data'][...].reshape(-1));assert first>0 and last>=first
            ids=set(range(first,last+1));assert not ids&seen;seen|=ids
            width=4 if kind==10 else 3;rows=section['ElementConnectivity'][' data'][...].reshape(-1).tolist();assert len(rows)==width*len(ids)
            name=section.name.rsplit('/',1)[-1]
            if kind==5:assert name in markers;ranges[name]=(first,last)
            for i in range(0,len(rows),width):
                nodes=rows[i:i+width];assert len(set(nodes))==width and all(n in points for n in nodes)
                if kind==10:
                    assert determinant([tuple(F(x) for x in points[n]) for n in nodes])>0,'Nonpositive actual CGNS tetrahedron'
                    cells[tuple(sorted(points[n] for n in nodes))]+=1
                else:boundary[name,tuple(sorted(points[n] for n in nodes))]+=1
        assert sum(cells.values())==volume_count
        skins=labelled(zone,'ZoneBC_t');assert len(skins)==1
        bcs=labelled(skins[0],'BC_t');assert {b.name.rsplit('/',1)[-1] for b in bcs}==set(ranges)==markers
        for bc in bcs:
            name=bc.name.rsplit('/',1)[-1];assert tuple(map(int,bc['PointRange'][' data'][...].reshape(-1)))==ranges[name]
        def closed(name,obj):
            if isinstance(obj,h5py.Group):
                assert all(isinstance(obj.get(k,getlink=True),h5py.HardLink) for k in obj),'External/soft HDF5 mesh dependency'
        closed('',file);file.visititems(closed)
        return cells,boundary

def affine(p):
    x,y,z=p
    return (1.2+.1*x-.05*y+.02*z,400+20*x-10*y+5*z,-30+5*x+8*y-3*z,12-4*x+2*y+6*z,2.5e5+1e3*x-2e3*y+500*z)

def states(data,state,sensor_at=None):
    source,_=sources(data)
    wanted={'accepted':{tuple(n[1:]) for n in data['nodes']},'donor':{tuple(v[1:4]) for c in data['original'] for v in c['vertices']}}
    maximum=metric_error=0.
    for label,points in wanted.items():
        rows=state[label]
        assert len({r['id'] for r in rows})==len(rows)
        assert {tuple(r['point']) for r in rows}==points and len(rows)==len(points)
        for row in rows:
            values=row['states'];assert len(values)==15 and all(math.isfinite(x) for x in values)
            assert values[:5]==values[5:10],'End-step current/time_n states differ'
            base=affine(row['point']);expected=[factor*v for factor in (1.,1.,.96875) for v in base]
            maximum=max(maximum,max(abs(a-b)/max(abs(a),abs(b),1e-8) for a,b in zip(values,expected)))
            if label=='accepted':
                tensors=row['metric'];assert len(tensors)==6 and all(math.isfinite(x) for x in tensors)
                original=metric(source,tuple(F(float(x)) for x in row['point']))
                metric_error=max(metric_error,max(abs(float(a)-b) for a,b in zip(original,tensors)))
    assert maximum<2e-10,'Current/BDF2 affine state residual'
    assert metric_error<1e-13,'Transported original-P1 sensor residual'
    scale=state['sensor_scale']
    for c in data['original']:
        for v in c['vertices']:
            x,y,z=v[1:4];expected=(scale+.125*x,.03125*x,0.,scale+.125*y,0.,scale+.125*z) if sensor_at is None else sensor_at((x,y,z),scale)
            assert max(abs(a-b) for a,b in zip(expected,v[4]))<1e-13
    return dict(maximum_state_relative_error=maximum,maximum_transported_tensor_absolute_error=metric_error,
                donor_points=len(wanted['donor']),accepted_points=len(wanted['accepted']))

def reject(check):
    try:check()
    except (AssertionError,KeyError,StopIteration):return
    raise AssertionError('Auditor accepted its deliberately corrupted evidence')

def main(folder,ranks,log):
    meshes={};records=[];state_records=[];roundtrips=0
    for path in sorted(folder.glob('bridge*.json')):
        if path.name.endswith('_states.json'):continue
        record,data=frame(path,True);records.append(record);meshes[path.stem]=data
    for method in ('barycentric','conservative'):
        for m in range(1,ranks+1):
            previous=None
            for cycle in range(3):
                stem='bridge_%s_m%d_s%d'%(method,m,cycle);data=meshes[stem]
                state=json.loads((folder/(stem+'_states.json')).read_text());assert state['sensor_scale']==(1. if cycle==1 else 4.)
                state_records.append(dict(file=stem+'_states.json',**states(data,state)))
                source=Counter(tuple(sorted(tuple(v[1:4]) for v in c['vertices'])) for c in data['original'])
                if previous is not None:assert source==signature(previous)[0],'Next event lost the previous accepted topology'
                previous=data
                if method=='barycentric':
                    expected=signature(data)
                    assert su2_signature(folder/(stem+'_written_SU2.su2'))==expected
                    assert cgns_signature(folder/(stem+'_written_CGNS.cgns'))==expected
                    for fmt in ('SU2','CGNS'):
                        reloaded=meshes[stem+'_written_'+fmt+'_reload'];assert signature(reloaded)==expected
                        extension='.cgns' if fmt=='CGNS' else '.su2';assert (folder/(stem+'_written_'+fmt+extension)).stat().st_size>0
                        roundtrips+=1
            assert len(meshes['bridge_%s_m%d_s1'%(method,m)]['cells'])<len(meshes['bridge_%s_m%d_s0'%(method,m)]['cells'])
            assert len(meshes['bridge_%s_m%d_s2'%(method,m)]['cells'])>len(meshes['bridge_%s_m%d_s1'%(method,m)]['cells'])
    phases=[]
    for line in log.read_text().splitlines():
        if not line.startswith('BRIDGE '):continue
        values={k:float(v) for k,v in (part.split('=') for part in line.split()[1:])};phases.append(values)
        assert values['ranks']==ranks and 0<=values['rank']<ranks and 1<=values['workers']<=ranks
        assert values['state_error']<2e-10
        assert min(values[k] for k in ('import_s','weights_s','reader_s'))>0
    assert len(phases)==ranks*ranks*6
    data=meshes['bridge_barycentric_m1_s0'];state=json.loads((folder/'bridge_barycentric_m1_s0_states.json').read_text())
    bad=copy.deepcopy(state);bad['accepted'][0]['states'][0]+=.01;reject(lambda:states(data,bad))
    bad=copy.deepcopy(state);bad['accepted'][0]['states'][5]+=.01;reject(lambda:states(data,bad))
    bad=copy.deepcopy(state);bad['accepted'][0]['metric'][0]+=1.;reject(lambda:states(data,bad))
    bad=copy.deepcopy(state);bad['donor'].pop();reject(lambda:states(data,bad))
    bad=copy.deepcopy(data);bad['facets'][0]['marker']=(bad['facets'][0]['marker']+1)%7
    reject(lambda:assert_same(signature(bad),signature(data)))
    result=dict(status='PASS',frames=records,states=state_records,actual_output_roundtrips=roundtrips,adversaries_rejected=5,rank_samples=phases,
      scope='Bounded actual-SU2 manufactured bridge: exact embedding/markers/volume and strict original-P1 shape/edge audits, transported sensors, current and BDF2 affine state checks, actual SU2 ASCII and independent HDF5 CGNS parse (positive tetrahedra, physical sections/BCs, closed links), plus actual SU2/CGNS reader round-trip signatures. No configured native3D driver, CFD iterations, curved geometry, 3D BL construction or performance scaling claim.')
    (folder/'independent_audit.json').write_text(json.dumps(result,indent=2)+'\n')
    print('PASS',len(records),'frames',len(state_records),'state pairs',roundtrips,'actual output round trips',flush=True)

def assert_same(a,b):assert a==b
if __name__=='__main__':main(Path(sys.argv[1]).resolve(),int(sys.argv[2]),Path(sys.argv[3]).resolve())
