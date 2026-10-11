"""Exact geometry/original-P1, actual BDF transfer/output and exclusive-phase audit of one envelope case."""
from pathlib import Path
from collections import Counter
from fractions import Fraction as F
import copy,json,math,sys
from audit_coupled import frame,sources,metric
from audit_geometry_bridge import MARKERS,states,signature,su2_signature,cgns_signature,reject
from audit_lifecycle import parse,stage,closes,STAGES
STAGES['envelope']=('metric','observation','remesh','replace')

def reference_tensors(data,height):
    if height==1:return data
    result=copy.deepcopy(data);factors=(1.,1.,height,1.,height,height*height)
    for cell in result['original']:
        for v in cell['vertices']:
            assert len(v[4])==6 and all(math.isfinite(value) for value in v[4])
            v[4]=[value*factor for value,factor in zip(v[4],factors)]
    return result

def reference_states(data,state,sensor_at,height):
    if height==1:return states(data,state,sensor_at)
    normalized=copy.deepcopy(state);factors=(1.,1.,height,1.,height,height*height)
    for row in normalized['accepted']:
        assert len(row['metric'])==6 and all(math.isfinite(value) for value in row['metric'])
        row['metric']=[value*factor for value,factor in zip(row['metric'],factors)]
    result=states(reference_tensors(data,height),normalized,sensor_at)
    source,_=sources(data);physical_error=0.
    for row in state['accepted']:
        expected=metric(source,tuple(F(float(x)) for x in row['point']))
        physical_error=max(physical_error,max(abs(float(a)-b) for a,b in zip(expected,row['metric'])))
    result['maximum_transported_tensor_physical_error']=physical_error
    return result

def reload_metric(nodal,target,height=1.):
    original,_=sources(reference_tensors(target,height));current,nodes=sources(reference_tensors(nodal,height))
    physical_source,_=sources(target);_,physical_nodes=sources(nodal)
    assert sorted(c['key'] for c in current)==sorted(tuple(sorted(c['nodes'])) for c in nodal['cells'])
    assert {p for p,m in nodes.values()}=={tuple(F(float(x)) for x in n[1:]) for n in nodal['nodes']}
    error=physical_error=0.
    for id,(point,tensor) in nodes.items():
        wanted=metric(original,point)
        error=max(error,max(abs(float(a)-float(b)) for a,b in zip(wanted,tensor)))
        physical_wanted=metric(physical_source,point)
        physical_error=max(physical_error,max(abs(float(a)-float(b)) for a,b in zip(physical_wanted,physical_nodes[id][1])))
    assert error<1e-13,'Reload lost transported original-P1 nodal metric'
    return dict(maximum_transported_tensor_absolute_error=error,maximum_transported_tensor_physical_error=physical_error)

WEDGE_MARKERS={'x_minus','y_minus','wedge_oblique','z_plus','z_minus_a','z_minus_b'}

def wedge_domain(data,height):
    assert set(data['marker_names'])==WEDGE_MARKERS and len(data['facet_groups'])==6
    for node in data['nodes']:
        x,y,z=map(lambda v:F(float(v)),node[1:])
        assert x>=0 and y>=0 and x+y<=1 and 0<=z<=height
    for facet in data['facets']:
        name=data['marker_names'][facet['marker']]
        for vertex in facet['vertices']:
            x,y,z=map(lambda v:F(float(v)),vertex[1:4])
            plane={'x_minus':x,'y_minus':y,'wedge_oblique':x+y-1,'z_plus':z-height,'z_minus_a':z,'z_minus_b':z}
            assert plane[name]==0

def main(folder):
    p=json.loads((folder/'parameters.json').read_text());ranks=p['ranks'];anisotropy=(1.,1.5,2.)[p['anisotropy']]
    variation=.125 if p['varying'] else 0.;direction=p.get('direction',0)
    assert direction in (0,1)
    height_power=p.get('height_power',0);assert height_power in (0,10,20)
    height=math.ldexp(1.,-height_power);keep_target=direction or height_power
    wedge=p.get('wedge',False);assert wedge in (False,True)
    markers=WEDGE_MARKERS if wedge else MARKERS
    def sensor_at(x,scale):
        q=scale*(1.+variation*x[0])
        if direction:
            phi=math.pi*(.25+.25*x[0]);theta=math.pi*(x[1]/6.)
            u,v,w=math.cos(phi)*math.cos(theta),math.sin(phi)*math.cos(theta),math.sin(theta)
            delta=anisotropy*anisotropy-1.
            return q*(1.+delta*u*u),q*delta*u*v,q*delta*u*w,q*(1.+delta*v*v),q*delta*v*w,q*(1.+delta*w*w)
        diag=.5*(anisotropy*anisotropy+1.);off=.5*(anisotropy*anisotropy-1.)
        return q*diag,q*off,0.,q*diag,0.,q
    records=[];transport=[];reload_metrics=[];previous=None
    for cycle in range(3):
        name=f'envelope_s{cycle}'
        initial,a=frame(folder/(name+'_initial.json'),False)
        accepted,b=frame(folder/(name+'.json'),True)
        assert initial['volume']==accepted['volume']==str(F(1,2**(height_power+bool(wedge))))
        assert a['marker_names']==b['marker_names']
        if wedge:wedge_domain(a,height);wedge_domain(b,height)
        if previous is not None:
            assert signature(a,markers)==signature(previous,markers),'Next event lost the last accepted mesh'
            assert a['facets']==previous['facets'] and a['facet_groups']==previous['facet_groups']
        if cycle==0 and p.get('aligned_markers',False):
            seam=F(float((p['size']//2)/p['size']))
            assert len(a['facet_groups'])==len(markers),'Aligned fixture must preserve every finite canonical marker patch'
            for facet in a['facets']:
                marker=a['marker_names'][facet['marker']]
                if marker not in ('z_minus_a','z_minus_b'):continue
                xs=[F(float(v[1])) for v in facet['vertices']]
                assert all(F(float(v[3]))==0 for v in facet['vertices'])
                assert max(xs)<=seam if marker=='z_minus_a' else min(xs)>=seam
        state=json.loads((folder/(name+'_states.json')).read_text())
        assert state['sensor_scale']==p['size']**2*(.25 if cycle==1 else 1.)
        transport.append(dict(file=name+'_states.json',**reference_states(b,state,sensor_at,height)))
        actual=folder/f'envelope_mesh_{cycle+1:05d}.su2'
        assert su2_signature(actual,markers)==signature(b,markers)
        sidecar=actual.with_suffix('.su2.native_ref')
        assert sidecar.is_file()
        if cycle:assert sidecar.read_bytes()==(folder/'envelope_mesh_00001.su2.native_ref').read_bytes()
        records.extend([initial,accepted]);previous=b
    assert records[0]['cells']==(3 if wedge else 6)*p['size']**3
    assert records[3]['cells']<records[1]['cells'] and records[5]['cells']>records[3]['cells']
    for fmt in ('SU2','CGNS'):
        actual=folder/('envelope_s2_written_'+fmt+('.cgns' if fmt=='CGNS' else '.su2'))
        assert (cgns_signature(actual,markers) if fmt=='CGNS' else su2_signature(actual,markers))==signature(previous,markers)
        stem=f'envelope_s2_written_{fmt}'
        record,loaded=frame(folder/(stem+('_reload_target.json' if keep_target else '_reload.json')),True)
        assert signature(loaded,markers)==signature(previous,markers);records.append(record)
        if keep_target:
            nodal=json.loads((folder/(stem+'_reload.json')).read_text())
            assert signature(nodal,markers)==signature(loaded,markers)
            reload_metrics.append(dict(format=fmt,**reload_metric(nodal,loaded,height)))
    rank_rows=[];stages=[];native=[]
    for line in (folder/'case.stdout').read_text().splitlines():
        if line.startswith(('ENVELOPE ','NATIVE3D ')):
            row={k:float(v) for k,v in (word.split('=') for word in line.split()[1:])}
            assert all(math.isfinite(v) for v in row.values())
            assert all(v>=0 for k,v in row.items() if k!='other')
            assert row['ranks']==ranks and 0<=row['rank']<ranks and row['workers']==p['workers']
            if line.startswith('ENVELOPE '):
                assert row['size']==p['size'] and row['anisotropy']==anisotropy and row['variation']==variation
                assert row.get('direction',0)==direction and row.get('height_power',0)==height_power and row.get('wedge',0)==int(wedge)
                assert row['repeat']==p['repeat'] and row['detailed']==p['detail'] and 0<=row['cycle']<3
                assert row['state_error']<2e-10;rank_rows.append(row)
            else:
                phases=('support','reference','import','source','weights','partition','donor_migration','communicator','refine','coarsen','wait','return')
                closes(row,phases)
                assert row['complete']==1 and row['detailed']==p['detail'];native.append(row)
        elif line.startswith('NATIVE3D_STAGE '):
            row=parse(line);stage(row)
            assert row['ranks']==ranks and 0<=row['rank']<ranks
            stages.append(row)
    assert len(native)==len(rank_rows)==3*ranks
    assert {(r['cycle'],r['rank']) for r in rank_rows}=={(c,r) for c in range(3) for r in range(ranks)}
    envelope=[r for r in stages if r['stage']=='envelope'];assert len(envelope)==(3*ranks if p['detail'] else 0)
    if p['detail']:
        for rank in range(ranks):
            for cycle,row in enumerate(r for r in envelope if r['rank']==rank):
                parent=next(r for r in rank_rows if r['rank']==rank and r['cycle']==cycle)
                remesh=[r for r in native if r['rank']==rank][cycle]
                assert row['total']<=parent['parent_wall']+1e-12 and row['remesh']>=remesh['total']-1e-12
    source=json.loads((folder/'envelope_s2.json').read_text());state=json.loads((folder/'envelope_s2_states.json').read_text())
    bad=copy.deepcopy(state);bad['accepted'][0]['metric'][0]+=.1;reject(lambda:reference_states(source,bad,sensor_at,height))
    bad=copy.deepcopy(state);bad['accepted'][0]['states'][10]+=.1;reject(lambda:reference_states(source,bad,sensor_at,height))
    if keep_target:
        bad=copy.deepcopy(nodal);bad['original']=[];reject(lambda:reload_metric(bad,loaded,height))
        bad=copy.deepcopy(nodal);bad['original'][0]['vertices'][0][4][0]+=.1;reject(lambda:reload_metric(bad,loaded,height))
    if height_power:
        bad=copy.deepcopy(state);bad['accepted'][0]['metric'][5]*=1.01;reject(lambda:reference_states(source,bad,sensor_at,height))
        bad=copy.deepcopy(state);bad['accepted'][0]['metric'].append(0.);reject(lambda:reference_states(source,bad,sensor_at,height))
        bad=copy.deepcopy(nodal);bad['original'][0]['vertices'][0][4].append(0.);reject(lambda:reload_metric(bad,loaded,height))
    if wedge:
        bad=copy.deepcopy(source);node=next(n for n in bad['nodes'] if F(float(n[1]))+F(float(n[2]))==1);node[1]+=1.;reject(lambda:wedge_domain(bad,height))
        bad=copy.deepcopy(source);bad['marker_names'][bad['marker_names'].index('wedge_oblique')]='invalid';reject(lambda:signature(bad,markers))
    summary=dict(status='PASS',parameters=p,frames=records,states=transport,reload_metrics=reload_metrics,rank_rows=rank_rows,rank_stages=stages,native_rows=native,adversaries_rejected=2+2*bool(keep_target)+3*bool(height_power)+2*bool(wedge),
                 metric_residual_units="reference-box tensor components; coordinates physical" if height_power else "physical tensor components",
                 scope='Three actual native3D backend/driver replacement events on a manufactured rotated anisotropic target. Exact embedding/incidence/finite-facet coverage/markers/volume, original-P1 metric and current/BDF2 audits; actual SU2/CGNS output and readback. Timed manual metric/observer/remesh/replace phases: observer is diagnostic work, not CFD. One repetition is a correctness/diagnostic pilot, not scaling, instrument-overhead, physical-solver or full-Goal1 qualification.')
    (folder/'independent_audit.json').write_text(json.dumps(summary,indent=2)+'\n')
    print('PASS',len(records),'strict/input/readback frames;',len(transport),'state frames;',len(stages),'closed rank-stage rows')
if __name__=='__main__':main(Path(sys.argv[1]).resolve())
