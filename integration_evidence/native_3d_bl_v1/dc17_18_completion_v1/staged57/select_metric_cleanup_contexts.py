"""Select sealed actual clean stars; no operators, solver, or production changes."""
from pathlib import Path
import hashlib,json,time
import meshio
import numpy as np
from prepare_boundary202 import FACES,reader
R=Path(__file__).resolve().parent
A=R/'m6_runs/euler_build262_compact_capture_mach_only_real_cfd50k'
OUT=A/'post_refinement_cleanup_selection_v1.json'
sha=lambda p:hashlib.file_digest(Path(p).open('rb'),'sha256').hexdigest()

def incidence(tets,points):
    order=np.argsort(tets.ravel(),kind='stable')
    offset=np.r_[0,np.cumsum(np.bincount(tets.ravel(),minlength=points))]
    return lambda n:np.unique(order[offset[n]:offset[n+1]]//4)

def main():
    started=time.monotonic();assert not OUT.exists()
    test=incidence(np.array([[0,1,2,3],[0,1,4,5]]),6)
    assert test(0).tolist()==[0,1] and test(3).tolist()==[0]
    checked=json.loads((A/'inspection_reader_status.json').read_text())
    assert checked['all_reader_checks_passed'] and checked['actual_cells']==734315
    assert json.loads((A/'terminal_handles.json').read_text())['all_owned_handles_terminal']
    ready=json.loads((A/'capture_ready.json').read_text())
    before={p:sha(p) for p in ready['files']}
    assert all(before[p]==x['sha256'] for p,x in ready['files'].items())
    vtu=A/'inspection/rejected_mesh.vtu';before[str(vtu.resolve())]=sha(vtu)
    assert sha(vtu)==checked['input_hashes_after'][str(vtu.resolve())]
    v=meshio.read(vtu);P=v.points;T=v.cells[0].data
    assert T.shape==(734315,4) and P.shape==(148608,3)
    ids=v.point_data['global_node_id'].ravel();cids=v.cell_data['global_cell_id'][0].ravel()
    M=v.point_data['captured_nodal_metric'].reshape(-1,3,3)
    q=v.cell_data['q'][0].ravel();J=v.cell_data['J'][0].ravel()
    g=v.cell_data['geometry_bad'][0].ravel();cache=v.cell_data['cache_valid'][0].ravel()
    clean=np.isfinite(q)&np.isfinite(J)&(q>=.2)&(J>=.05)&(g==0)&(cache==1)&(v.cell_data['metric_edge_max'][0].ravel()<=1.8)
    assert np.count_nonzero(~np.isfinite(q))==4
    manifests,missing=reader._load_manifests([str(A/'adapted_mesh_inspection_rank*_manifest.json')]);assert not missing and len(manifests)==6
    node=np.dtype([('id','<u8'),('xyz','<f8',(3,))])
    dtype=np.dtype([('format','<u8'),('id','<u8'),('nodes',node,(4,)),('version','<u8'),('physical','<u8',(4,2)),('sensor_present','<u8'),('sensor','<f8',(4,6))]);assert dtype.itemsize==416
    arrays=[np.memmap(m['_resolved_files']['owned'],dtype=dtype,offset=8,shape=(m['written_rows'],),mode='r') for m in manifests]
    raw=np.concatenate(arrays);assert np.array_equal(raw['id'],cids) and np.array_equal(raw['nodes']['id'],ids[T])
    facets={x['id']:x for x in reader._jsonl(manifests[0]['_resolved_files']['reference'],manifests[0]['_caps']['reference'],'reference') if x.get('kind')=='facet'}
    names=[x.split('=',1)[1].strip() for x in (A/'m6.su2').read_text().splitlines() if x.startswith('MARKER_TAG')];assert names[2]=='WING3D'
    faces=[]
    for slot,indices in enumerate(FACES):
        rows=np.flatnonzero(raw['physical'][:,slot,0])
        for ci in rows:
            facet=int(raw['physical'][ci,slot,1])
            if facets[facet]['marker']==2:faces.append((int(ci),slot,facet))
    wing=np.unique(np.concatenate([T[ci,list(FACES[s])] for ci,s,_ in faces]));star=incidence(T,len(P));selected=[];used=set()
    centers=np.array([P[T[ci,list(FACES[s])]].mean(axis=0) for ci,s,_ in faces])
    for role,y0,target_x,axis in [('leading_span_low',.4,.008,'span'),('leading_span_high',.8,.008,'span'),('leading_nose_control',.6,.008,'nose'),('smooth_wing_clean_control',.6,.15,'span')]:
        band=P[wing][abs(P[wing,1]-y0)<.025];xi=band[:,0]-band[:,1]/np.sqrt(3);chord=float(np.ptp(xi));xle=float(xi.min())
        xc=(centers[:,0]-centers[:,1]/np.sqrt(3)-xle)/chord
        eligible=np.flatnonzero((abs(centers[:,1]-y0)<.035)&(abs(xc-target_x)<(.02 if target_x<.03 else .05)))
        order=sorted(eligible,key=lambda k:abs(centers[k,1]-y0)/.035+abs(xc[k]-target_x)/.02)
        choice=None
        for k in order[:2000]:
            ci,slot,facet=faces[k]
            if ci in used or not clean[ci]:continue
            vertices=T[ci];stars=[star(int(n)) for n in vertices];support=np.unique(np.concatenate(stars))
            if len(support)>64 or not clean[support].all():continue
            tri=T[ci,list(FACES[slot])];xyz=P[tri];normal=np.cross(xyz[1]-xyz[0],xyz[2]-xyz[0]);normal/=np.linalg.norm(normal)
            span=np.array([1/np.sqrt(3),1.,0.]);span-=normal.dot(span)*normal;span/=np.linalg.norm(span)
            edges=[]
            for i,j in ((0,1),(0,2),(1,2)):
                e=xyz[j]-xyz[i];alignment=float(abs(e.dot(span))/np.linalg.norm(e));length=float(np.sqrt(e@((M[tri[i]]+M[tri[j]])/2)@e))
                if length<.9 and (alignment>=.8 if axis=='span' else alignment<=.35):edges.append((length,alignment,int(ids[tri[i]]),int(ids[tri[j]])))
            if not edges:continue
            edge=min(edges);choice=dict(id=int(cids[ci]),role=role,nodes=list(map(int,ids[vertices])),context_cell_ids=list(map(int,cids[support])),four_star_union_size=len(support),vertex_star_sizes=list(map(len,stars)),within_existing64=True,global_support_complete=True,geometry_bad=False,physical_faces=int(np.count_nonzero(raw['physical'][ci,:,0])),reference_facet=facet,reference={k:facets[facet].get(k) for k in ('marker','body','sheet')},center=centers[k].tolist(),section_x_over_chord=float(xc[k]),selected_edge=list(edge[2:]),edge_span_alignment=edge[1],endpoint_metric_length_estimate=edge[0],all_context_q_J_known_clean=True,all_context_geometry_cached_clean=True,minimum_q=float(q[support].min()),minimum_J=float(J[support].min()),context_indices=list(map(int,support)))
            selected.append(choice);used.add(ci);break
        assert choice is not None,role
    wanted={i for x in selected for i in x['context_indices']};metrics={};base=0
    for manifest,data in zip(manifests,arrays):
        local={i-base for i in wanted if base<=i<base+len(data)};path=manifest['_resolved_files']['metrics'];assert path.stat().st_size<=manifest['_caps']['metrics']
        with path.open() as stream:
            count=0
            for i,line in enumerate(stream):
                count+=1
                if i not in local:continue
                row=json.loads(line);row=reader._unpack_compact_metric(row) if isinstance(row,list) else row
                r=data[i];cell=dict(id=int(r['id']),version=int(r['version']),nodes=[dict(id=int(n['id']),xyz=n['xyz'].tolist()) for n in r['nodes']],physical=[dict(present=bool(f[0]),facet=int(f[1])) for f in r['physical']],sensor_present=bool(r['sensor_present']),sensor=r['sensor'].ravel().tolist())
                metric=reader._validate_metric(row,cell,path)
                assert metric['q_measured'] and metric['j_measured'] and metric['q']>=.2 and metric['J']>=.05 and metric['geometry_measured'] and not metric['geometry_bad']
                assert not any(metric[x] for x in ('q_bad','j_bad','size_bad','height_bad','height_unresolved'))
                metrics[int(r['id'])]=metric
        assert count==manifest['written_rows'];base+=len(data)
    assert base==734315
    for case in selected:
        edges={};bindings={}
        for cid in case['context_cell_ids']:
            m=metrics[cid]
            for a,b,l in m['edges']:
                key=tuple(sorted((a,b)));assert edges.setdefault(key,l)==l
            for binding in m['bindings']:
                if binding is not None:assert bindings.setdefault(binding['id'],binding)==binding
        key=tuple(sorted(case['selected_edge']));case['selected_edge_exact_cached_length']=edges[key];assert edges[key]<.9
        case['selected_edge_bindings']=[bindings[n] for n in case['selected_edge']];assert all(b['mobility_dimension']==2 for b in case['selected_edge_bindings'])
        case['exact_cached_context_edge_quantiles']=dict(zip(('min','p10','median','p90','max'),map(float,np.quantile(list(edges.values()),[0,.1,.5,.9,1]))))
        case['context_unique_edges']=len(edges);case['requested_heights']=False
        for n,size in zip(case['nodes'],case['vertex_star_sizes']):assert int(np.count_nonzero(np.any(raw['nodes']['id']==n,axis=1)))==size
        del case['context_indices']
    after={p:sha(p) for p in before};assert before==after
    result=dict(schema='actual262_clean_metric_cleanup_selection_v1',source_build='build262',expected_bounded_contexts=4,bounded_selected_contexts=selected,input_hashes_before=before,input_hashes_after=after,helper_sha256=sha(Path(__file__)),seconds=time.monotonic()-started,scope='Global complete <=64 four-star contexts selected from sealed full rejected262 mesh. Every selected context has known clean cached q/J/geometry/size; chosen-edge length is exact cached production quadrature. Direction selection/endpoint estimate is only diagnostic geometry, not a candidate certificate. No operator run or feasible replacement established; original262 still has four unknown q/J cells elsewhere.')
    OUT.write_text(json.dumps(result,indent=2,allow_nan=False)+'\n');print(json.dumps(dict(contexts=[{k:c[k] for k in ('id','role','four_star_union_size','selected_edge','selected_edge_exact_cached_length')} for c in selected],seconds=result['seconds'])))
if __name__=='__main__':main()
