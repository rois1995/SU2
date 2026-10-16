"""Independent exact geometry and complete composed-field audit of the opposing-wall target."""
import collections,functools,hashlib,itertools,json,math,re,sys,time
from fractions import Fraction as F
from pathlib import Path
from scipy.integrate import quad
HERE=Path(__file__).resolve().parent;ROOT=HERE.parents[2]
sys.path.insert(0,str(ROOT/'integration_evidence/native_3d_core_v1'))
sys.path.insert(0,str(ROOT/'integration_evidence/native_3d_bl_v1/alignment_audit_v1'))
from audit_coupled import frame
from baseline import footprints

def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def records(path,label):
    result=[];nodes={}
    for line in path.read_text().splitlines():
        if not line.startswith('MESH '+label+' '):continue
        tokens=line.split();cell=int(tokens[2]);ids=[]
        for k in range(4):
            n=int(tokens[4+4*k]);point=list(map(float,tokens[5+4*k:8+4*k]));ids.append(n)
            assert n not in nodes or nodes[n]==point;nodes[n]=point
        a,b,c,d=ids;faces=((b,c,d),(a,d,c),(a,b,d),(a,c,b));surface=[]
        for k,f in enumerate(faces):
            if int(tokens[20+2*k]):surface.append({'nodes':list(f),'facet':int(tokens[21+2*k])})
        result.append({'id':cell,'nodes':ids,'surface':surface})
    assert result;return nodes,result

def rejected_su2(path,facets,groups,names):
    lines=path.read_text().splitlines();e=next(i for i,l in enumerate(lines) if l.startswith('NELEM='));count=int(lines[e].split('=')[1])
    cells=[{'id':i,'nodes':list(map(int,lines[e+1+i].split()[1:5])),'surface':[]} for i in range(count)]
    assert all(lines[e+1+i].split()[0]=='10' for i in range(count))
    n=next(i for i,l in enumerate(lines) if l.startswith('NPOIN='));nodes={i:list(map(float,lines[n+1+i].split()[:3])) for i in range(int(lines[n].split('=')[1]))}
    at=next(i for i,l in enumerate(lines) if l.startswith('NMARK='))+1;physical={}
    while at<len(lines):
        marker=names.index(lines[at].split('=')[1].strip());count=int(lines[at+1].split('=')[1]);at+=2
        for line in lines[at:at+count]:
            values=list(map(int,line.split()));assert values[0]==5 and len(values)==4
            face=values[1:];matches=[min(ids) for (axis,value),ids in groups.items() if facets[min(ids)]['marker']==marker and all(nodes[n][axis]==value for n in face)]
            assert len(matches)==1;key=tuple(sorted(face));assert key not in physical;physical[key]=matches[0]
        at+=count
    used=set()
    for c in cells:
        a,b,d,e=c['nodes']
        for f in ((b,d,e),(a,e,d),(a,b,e),(a,d,b)):
            key=tuple(sorted(f))
            if key in physical:c['surface'].append({'nodes':list(f),'facet':physical[key]});assert key not in used;used.add(key)
    assert used==set(physical);return nodes,cells

def metric(original,p):
    x,y,z=map(float,p);s=4+12*x;distance=min(z,1-z);hn=max(.02,2*(.02+.2*distance)/2.2)
    return tuple(map(F,(s,0.,0.,s,0.,max(s,1/(hn*hn)))))
@functools.lru_cache(None)
def edge(a,b):
    a=tuple(map(float,a));b=tuple(map(float,b));d=[y-x for x,y in zip(a,b)];cuts=[0.,1.]
    if d[2]:cuts += [(z-a[2])/d[2] for z in (.01,.5,.99) if 0<(z-a[2])/d[2]<1]
    def integrand(t):
        p=[x+t*y for x,y in zip(a,d)];m=metric(None,p);return math.sqrt(sum(float(m[k])*d[j]**2 for j,k in enumerate((0,3,5))))
    cuts=sorted(cuts);return sum(quad(integrand,l,r,epsabs=1e-12,epsrel=1e-12)[0] for l,r in zip(cuts,cuts[1:]))

def main():
    start=time.monotonic();case=HERE/sys.argv[1];receiptPath=case.with_suffix('.json');receipt=json.loads(receiptPath.read_text())
    assert receipt['status'] in ('PASS','FAIL') and all(sha(case/p)==v for p,v in receipt['artifacts'].items())
    baseline=HERE/'native_opposing_v1';oldReceipt=baseline.with_suffix('.json');old=json.loads(oldReceipt.read_text());assert old['status']=='FAIL'
    assert all(sha(baseline/p)==v for p,v in old['artifacts'].items())
    initial='opposing_finer_grouped_initial_euler_n'+str(receipt['contract']['MPI_ranks'])+'.su2'
    # Rank-local source ownership may reorder input records; serial input bytes must be unchanged.
    if receipt['contract']['MPI_ranks']==1:assert sha(case/initial)==sha(baseline/'opposing_finer_grouped_initial_euler_n1.su2')
    oldNodes,oldCells=records(baseline/'native.log','opposing_finer_grouped_coupled_BL_rejected');facets={};names=['marker_11','marker_10','marker_12']
    nextFacet=1+max(f['facet'] for c in oldCells for f in c['surface'])
    for c in oldCells:
        for f in c['surface']:
            points=[oldNodes[n] for n in f['nodes']];marker=1 if all(p[2]==0 for p in points) else 2 if all(p[2]==1 for p in points) else 0
            # Completed local patches canonicalize coplanar aliases; recover their same finite union.
            identifier=f['facet']
            if identifier in facets:identifier=nextFacet;nextFacet+=1
            record={'id':identifier,'marker':marker,'vertices':[[n]+oldNodes[n] for n in f['nodes']]}
            facets[identifier]=record
    assert len(facets)==12
    source={tuple(v[1:]):v[0] for f in facets.values() for v in f['vertices']};assert len(source)==8
    lines=(baseline/'opposing_finer_grouped_initial_euler_n1.su2').read_text().splitlines();p=next(i for i,l in enumerate(lines) if l.startswith('NPOIN='));n=int(lines[p].split('=')[1]);coords=[tuple(map(float,lines[p+1+i].split()[:3])) for i in range(n)]
    e=next(i for i,l in enumerate(lines) if l.startswith('NELEM='));n=int(lines[e].split('=')[1]);original=[]
    for i in range(n):
        ids=list(map(int,lines[e+1+i].split()[1:5]));vertices=[]
        for n in ids:
            point=coords[n];s=4+12*point[0];vertices.append([source[point]]+list(point)+[[s,0,0,s,0,s]])
        original.append({'id':i,'vertices':vertices})
    groups=collections.defaultdict(list)
    for f in facets.values():
        points=[v[1:] for v in f['vertices']];axis=next(k for k in range(3) if len({p[k] for p in points})==1);groups[axis,points[0][axis]].append(f['id'])
    engines=[dict(x.split('=',1) for x in l.split()[1:]) for l in (case/'native.log').read_text().splitlines() if l.startswith('ENGINE ')]
    constructed=[e for e in engines if e['case']=='opposing_finer_grouped_coupled_BL']
    assert len(constructed)==receipt['contract']['MPI_ranks'] and all(e['valid']=='1' for e in constructed)
    removed=[e for e in engines if e['case']=='opposing_finer_grouped_released_Euler']
    complete=len(removed)==receipt['contract']['MPI_ranks'] and all(e['valid']=='1' and e['stop']=='complete' for e in removed)
    assert receipt['status']!='PASS' or complete
    reports=[]
    for name,layers in [('opposing_finer_grouped_coupled_BL',True),('opposing_finer_grouped_released_Euler'+('' if complete else '_rejected'),False)]:
        if not complete and not layers:
            nodes,cells=rejected_su2(case/('opposing_finer_grouped_released_euler_rejected_n'+str(receipt['contract']['MPI_ranks'])+'.su2'),facets,groups,names)
        else:nodes,cells=records(case/'native.log',name)
        surface=[f for c in cells for f in c['surface']]
        data={'nodes':[[n]+p for n,p in sorted(nodes.items())],'cells':[{'id':c['id'],'nodes':c['nodes']} for c in cells],
            'surface':surface,'facets':list(facets.values()),'facet_groups':list(groups.values()),'marker_names':names,'original':original}
        path=case/(name+'_independent.json');path.write_text(json.dumps(data,separators=(',',':'))+'\n')
        reuse=case/'construction_reference_reuse.json'
        if layers and reuse.exists():
            saved=json.loads(reuse.read_text());serial=HERE/'native_opposing_v3'
            assert saved['status'] in ('PASS_EXACT_CONSTRUCTION_REFERENCE_REUSE_REMOVAL_REJECTED','PASS_EXACT_CONSTRUCTION_REFERENCE_REUSE')
            assert saved['native_workflow_status']==receipt['status']
            assert saved['native_receipt_sha256']==sha(receiptPath) and saved['equivalent_complete_input_sha256']==sha(path)
            assert saved['old_independent_audit_sha256']==sha(serial/'independent.json') and saved['old_front_coverage_sha256']==sha(serial/'front_coverage.json')
            executed=(serial/'audit_native_executed.py').read_text();current=Path(__file__).read_text()
            assert executed[executed.index('def metric('):executed.index('def main(')]==current[current.index('def metric('):current.index('def main(')]
            report=dict(saved['construction_frame']);report['exact_reference_reuse_sha256']=sha(reuse)
        else:
            report,checked=frame(path,layers or complete,layers or complete,metric if layers else None,(lambda original,a,b:edge(tuple(a),tuple(b))) if layers else None)
        assert F(report['volume'])==1
        if layers:
            allEdges={tuple(sorted((c['nodes'][i],c['nodes'][j]))) for c in cells for i,j in itertools.combinations(range(4),2)}
            heightError=tilt=0.;aligned=0;wallFaces=0
            for marker,z,direction in [('marker_10',0.,1),('marker_12',1.,-1)]:
                feet=footprints(data,marker);assert feet and all(not r['outside'] for r in feet);wallFaces+=len(feet)
                wallNodes={n for r in feet for n in r['face']}
                for n in wallNodes:
                    matches=[j for j,p in nodes.items() if p[:2]==nodes[n][:2] and abs((p[2]-z)*direction-.02)<1e-12 and tuple(sorted((n,j))) in allEdges]
                    assert matches,'Missing wall-normal first-row edge';aligned+=1
                for r in feet:
                    apex=nodes[r['apex']];heightError=max(heightError,abs(abs(apex[2]-z)/.02-1))
                    center=[sum(nodes[n][k] for n in r['face'])/3 for k in (0,1)]
                    tilt=max(tilt,math.hypot(apex[0]-center[0],apex[1]-center[1])/.02)
            minimumClearance=min(min(p[2],1-p[2]) for p in nodes.values() if 0<p[2]<1)
            assert heightError<1e-10 and minimumClearance>=.02-1e-12
            report.update(first_height_faces=wallFaces,maximum_height_relative_error=heightError,maximum_centroid_tilt=tilt,
                wall_normal_first_row_edges=aligned,minimum_nonwall_node_clearance=minimumClearance,
                full_metric_edges=len(allEdges),coverage='Complete positive prismatic first bands, normal edges at every wall node, no nonwall node below h0; centroid tilt is reported separately.')
        report['layers']=layers;report['input_sha256']=sha(path);reports.append(report)
    assert reports[1]['cells']<reports[0]['cells']
    result={'status':('PASS_COMPLETE_OPPOSING_CONSTRUCTION_REMOVAL_GEOMETRY_AND_FULL_FIELD_SCOPE_LIMITED' if receipt['status']=='PASS' else 'PASS_COMPLETE_MESHES_NATIVE_CONTROL_RECEIPT_FAILED') if complete else 'PASS_CONSTRUCTION_ONLY_NATIVE_REMOVAL_REJECTED','native_workflow_status':receipt['status'],'native_engine_stages_complete':complete,'frames':reports,
        'native_receipt_sha256':sha(receiptPath),'baseline_native_receipt_sha256':sha(oldReceipt),'helper_sha256':sha(Path(__file__)),
        'geometry_helper_sha256':sha(ROOT/'integration_evidence/native_3d_core_v1/audit_coupled.py'),
        'feet_helper_sha256':sha(ROOT/'integration_evidence/native_3d_bl_v1/alignment_audit_v1/baseline.py'),'elapsed_seconds':time.monotonic()-start,
        'scope':'Whole exact binary64 geometry/finite facets/manifold/links/positive-volume/no-overlap/volume1 and height/normal-column/coverage diagnostics. Independent analytic two-planar-wall BL plus exact original affine P1 and full-edge numerical quadrature for every constructed edge; independent original P1 arithmetic/analytic full edges after removal. Native actual-query tensor/edge validation is separately required by the fixture. Rejected removal frames have geometry and measured residuals only; their metric gates are not asserted or promoted. Phase-specific4096 work controls, not proof of shared backend event budget. No CFD/transport/restart/general geometry or affordability qualification.'}
    (case/'independent.json').write_text(json.dumps(result,indent=2)+'\n');print(result['status'],flush=True)
if __name__=='__main__':main()
