#!/usr/bin/env python3
"""Independent saved-region geometry, tensor arithmetic and analytic pure-core edge audit."""
import csv,json,math,sys
from fractions import Fraction as F
from pathlib import Path
from construct import HERE,ROOT,sha,authorities,write_su2
from audit_runtime_mesh import mesh
sys.path.insert(0,str(ROOT/'integration_evidence/native_3d_core_v1'))
from audit_cavities import determinant,sub
from audit_coupled import spd,quad

def sensor_samples(case,data,nodes,old_ids,geometry):
    path=case/'core_sensors.csv'
    if not path.exists():return {'sensor_audit':'Legacy raw sensor export absent; pure-core checks only.'}
    donors={};primary=[]
    for donor in data['original']:
        v=donor['vertices'];key=tuple(sorted(x[0] for x in v));points=[tuple(map(F,x[1:4])) for x in v]
        assert key not in donors
        record=(points,[tuple(map(F,x[4])) for x in v],determinant(points))
        assert record[2]>0;donors[key]=record
        if all(x[0] in old_ids for x in v):primary.append((key,record))
    primary.sort();cells={c['id']:c for c in data['cells']};rows=list(csv.DictReader(path.open()))
    assert len(rows)==len(cells) and {int(r['cell']) for r in rows}==set(cells)
    def weights(record,point):
        points,_,volume=record;result=[]
        for i in range(4):
            trial=points[:];trial[i]=point;value=determinant(trial)/volume
            if value<0:return None
            result.append(value)
        assert sum(result)==1;return result
    maximum=0.;physical=collar=0
    for row in rows:
        point=tuple(F(float(row[k])) for k in ('x','y','z'));cell=cells[int(row['cell'])]
        center=tuple(sum(nodes[n][k] for n in cell['nodes'])/4 for k in range(3))
        assert max(abs(float(a-b)) for a,b in zip(point,center))<2e-15
        key=tuple(int(row['d'+str(i)]) for i in range(4));record=donors[key];w=weights(record,point);assert w is not None
        extension=int(row['extension'])
        if extension==2**64-1:assert any(k==key for k,_ in primary);physical+=1
        else:
            assert extension in geometry and not any(k==key for k,_ in primary);collar+=1
            # An exterior collar is legal only where no original physical donor contains the point.
            assert all(weights(r,point) is None for _,r in primary)
        expected=[sum(w[i]*record[1][i][k] for i in range(4)) for k in range(6)]
        actual=[F(float(row[k])) for k in ('xx','xy','xz','yy','yz','zz')]
        maximum=max(maximum,max(abs(float(a-b))/(1+abs(float(b))) for a,b in zip(actual,expected)))
    assert physical>0 and maximum<2e-12
    return {'sensor_audit':'Exact rational containment and original P1 interpolation, physical priority over declared collar, all exported centroid sensors.',
            'physical_sensor_samples':physical,'collar_sensor_samples':collar,'maximum_exact_original_sensor_residual':maximum}

def main():
    case=HERE/sys.argv[1];receiptPath=case.with_suffix('.json');receipt=json.loads(receiptPath.read_text())
    assert receipt['status'] in ('PASS','FAIL')
    assert all(sha(case/p)==h for p,h in receipt['artifacts'].items()),'Saved native evidence changed'
    exports={}
    for stem in ('core_accepted','core_construction_rejected','core_input_retained'):
        p=case/(stem+'.json')
        if not p.exists():continue
        d=json.loads(p.read_text());used={n for c in d['cells'] for n in c['nodes']}
        skin=[f for f in d['surface'] if set(f['nodes'])<=used]
        if receipt['status']=='PASS':assert len(skin)==len(d['surface'])
        out=p.with_suffix('.su2');write_su2(out,{n[0]:n[1:] for n in d['nodes']},[c['nodes'] for c in d['cells']],skin,{f['id']:f['marker'] for f in d['facets']},d['marker_names'])
        ref=Path(str(out)+'.native_ref');ref.write_bytes((case/'core_initial.su2.native_ref').read_bytes())
        exports[out.name]={'sha256':sha(out),'reference_sha256':sha(ref),'unmeshed_surface_faces':len(d['surface'])-len(skin)}
    if receipt['status']=='FAIL':
        result={'status':'REJECTED_INSPECTION_ONLY','scope':'No accepted certificate. Exported rejected private volume may be incomplete; physical faces without an admitted volume node are counted, not silently certified. Raw JSON and original failed receipt remain unchanged.','exports':exports,'native_receipt_sha256':sha(receiptPath),'helper_sha256':sha(Path(__file__).resolve())}
        (case/'core_independent.json').write_text(json.dumps(result,indent=2)+'\n');print(result['status']);return
    data=json.loads((case/'core_accepted.json').read_text());summary=json.loads((case/'core_summary.json').read_text())
    geometry,_=authorities(case/'core_initial.su2.native_ref');config=dict(line.split('=',1) for line in (case/'core.cfg').read_text().splitlines() if '=' in line)
    height=float(config['ADAP_BL_FIRST_HEIGHT'].strip(' ()'))
    report,_=mesh(data,geometry,True,height)
    assert summary['regions']>=1 and summary['work']<=4096 and summary['sensor_residual']<2e-12
    nodes={n[0]:tuple(map(F,n[1:])) for n in data['nodes']};samples={int(r['cell']):r for r in csv.DictReader((case/'core_samples.csv').open())};assert len(samples)==len(data['cells'])
    initial=json.loads((case/'core_initial.json').read_text());oldIds={n[0] for n in initial['nodes']}
    sensorReport=sensor_samples(case,data,nodes,oldIds,geometry)
    physical=[c for c in data['original'] if all(v[0] in oldIds for v in c['vertices'])];assert len(physical)==24
    constant=tuple(map(F,physical[0]['vertices'][0][4]));kind=summary['field']
    def sensor(p):return (F(1),F(0),F(0),F(1),F(0),F(4)+24*p[2]) if kind==1 else constant
    for c in physical:
        for v in c['vertices']:assert tuple(map(F,v[4]))==sensor(tuple(map(F,v[1:4])))
    cylinder=next(g for g in geometry.values() if g[0]==3);origin=tuple(map(F,cylinder[1]));axis=tuple(map(F,cylinder[4]));axis2=sum(x*x for x in axis)
    radius=F(float(cylinder[2][0]))-F(.2)
    def pure(p):
        d=sub(p,origin);return sum(x*x for x in d)-sum(x*y for x,y in zip(d,axis))**2/axis2<=radius*radius
    qmin=jmin=1.;difference=tensorError=0.;pureCells=0;allEdges=set()
    for c in data['cells']:
        points=[nodes[n] for n in c['nodes']];row=samples[c['id']];m=tuple(F(float(row[k])) for k in ('xx','xy','xz','yy','yz','zz'));dm=spd(m)
        lengths={(i,j):math.sqrt(float(quad(m,sub(points[j],points[i])))) for i in range(4) for j in range(i+1,4)}
        dv=float(determinant(points))*math.sqrt(float(dm));q=12*(dv/2)**(2/3)/sum(v*v for v in lengths.values())
        jac=min(1.,min(math.sqrt(2)*dv/math.prod(lengths[tuple(sorted((i,j)))] for j in range(4) if j!=i) for i in range(4)))
        assert q>=.2 and jac>=.05;qmin=min(qmin,q);jmin=min(jmin,jac)
        difference=max(difference,abs(q-float(row['q'])),abs(jac-float(row['j'])))
        if all(pure(p) for p in points):
            pureCells+=1;center=tuple(F(float(sum(p[k] for p in points)/4)) for k in range(3));expected=sensor(center)
            tensorError=max(tensorError,max(abs(float(a-b))/(1+abs(float(b))) for a,b in zip(m,expected)))
        for i in range(4):
            for j in range(i+1,4):allEdges.add(tuple(sorted((c['nodes'][i],c['nodes'][j]))))
    assert difference<2e-10 and pureCells>0 and tensorError<2e-12
    rows=list(csv.DictReader((case/'core_edges.csv').open()));assert {(int(r['a']),int(r['b'])) for r in rows}==allEdges
    coreEdges=0;edgeError=0.;maximum=0.
    for r in rows:
        a,b=nodes[int(r['a'])],nodes[int(r['b'])];native=float(r['length']);assert 0<native<=1.8;maximum=max(maximum,native)
        if pure(a) and pure(b):
            d=sub(b,a);s0=math.sqrt(float(quad(sensor(a),d)));s1=math.sqrt(float(quad(sensor(b),d)))
            # The radial norm is convex, so this entire edge stays outside the BL support.
            expected=2*(s0*s0+s0*s1+s1*s1)/(3*(s0+s1));assert expected<=1.8
            edgeError=max(edgeError,abs(native-expected)/(1+expected));coreEdges+=1
    assert coreEdges>0 and edgeError<1e-10
    report.update(sensorReport)
    report.update(status='PASS_COMPLETE_STRONGER_FIELD_CURVED_REGION_SCOPE_LIMITED',field=kind,regions=summary['regions'],work=summary['work'],minimum_q=qmin,minimum_j=jmin,maximum_native_tensor_shape_difference=difference,maximum_native_full_length=maximum,pure_core_cells=pureCells,pure_core_edges=coreEdges,maximum_pure_core_tensor_residual=tensorError,maximum_analytic_core_edge_residual=edgeError,exports=exports,native_receipt_sha256=sha(receiptPath),helper_sha256=sha(Path(__file__).resolve()),reused_geometry_helper_sha256=sha(HERE/'audit_runtime_mesh.py'),scope='Complete native region, independently exact positive/oriented connected manifold geometry/every vertex link, no interior overlap, normal feet/height and analytic authority. Independent q/J arithmetic on exported actual-point tensors. Pure-core original P1 tensor and full-edge analytic integral independently checked where exact radial bounds prove the whole segment lies outside BL support. Remaining BL full lengths are native measurements, not independent whole-field quadrature. Frozen imposed sensor, not computed/graded CFD demand; no CFD transfer/restart/accuracy/general geometry/performance qualification.')
    (case/'core_independent.json').write_text(json.dumps(report,indent=2)+'\n');print(report['status'],flush=True)
if __name__=='__main__':main()
