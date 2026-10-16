"""Independent original-P1, planar BL/fade and full-mesh audit of regional one-wall stresses."""
import copy,functools,hashlib,itertools,json,math,sys,time
from fractions import Fraction as F
from pathlib import Path
from scipy.integrate import quad
from audit_native import HERE,ROOT,records
sys.path.insert(0,str(ROOT/'integration_evidence/native_3d_core_v1'))
sys.path.insert(0,str(ROOT/'integration_evidence/native_3d_bl_v1/alignment_audit_v1'))
from audit_coupled import frame
from baseline import footprints

def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def main():
    started=time.monotonic();case=HERE/sys.argv[1];rp=case.with_suffix('.json');r=json.loads(rp.read_text())
    assert r['status']=='PASS' and all(sha(case/p)==h for p,h in r['artifacts'].items())
    kind=r['contract']['environment']['SU2_NATIVE_PRISM_SENSOR'];assert kind in ('SCALAR','CROSSING','TANGENTIAL','ROTATING');crossing=kind=='CROSSING'
    prior=HERE/'native_opposing_v3';proof=json.loads((prior/'independent.json').read_text());reference=prior/proof['frames'][0]['file'];assert sha(reference)==proof['frames'][0]['input_sha256']
    template=copy.deepcopy(json.loads(reference.read_text()))
    for f in template['facets']:
        if f['marker']==2:f['marker']=0
    def sensor(p):
        x,y,z=map(float,p)
        if crossing:return 4.,0.,4.,4+96*(1-abs(x-z))
        def rotated(high,sign):return .75*high+1,sign*.25*math.sqrt(3)*(high-4),.25*high+3,4.
        if kind=='TANGENTIAL':return rotated(256,1)
        if kind=='ROTATING':
            a=rotated(64,-1);b=rotated(256,1);return tuple((1-x)*u+x*v for u,v in zip(a,b))
        s=4+12*x;return s,0.,s,s
    for c in template['original']:
        signs=[v[1]-v[3] for v in c['vertices']];assert min(signs)>=0 or max(signs)<=0
        for v in c['vertices']:
            xx,xy,yy,zz=sensor(v[1:4]);v[4]=[xx,xy,0,yy,0,zz]
    def values(p,layers):
        xx,xy,yy,zz=sensor(p)
        if layers:
            z=float(p[2]);hn=max(.1,2*(.1+.2*z)/2.2);t=min(1.,max(0.,(z-.27)/.03));w=1-t*t*(3-2*t)
            zz=max(zz,math.exp(w*math.log(1/(hn*hn))+(1-w)*math.log(4)))
        return xx,xy,yy,zz
    def metric(original,p,layers):
        xx,xy,yy,zz=values(p,layers);return tuple(map(F,(xx,xy,0,yy,0,zz)))
    @functools.lru_cache(None)
    def length(a,b,layers):
        a=tuple(map(float,a));b=tuple(map(float,b));d=tuple(y-x for x,y in zip(a,b));cuts=[0.,1.]
        if crossing and d[0]!=d[2]:
            u=(a[2]-a[0])/(d[0]-d[2])
            if 0<u<1:cuts.append(u)
        if layers and d[2]:cuts += [(z-a[2])/d[2] for z in (.05,.27,.3) if 0<(z-a[2])/d[2]<1]
        cuts=sorted(set(cuts))
        def squared(t):
            xx,xy,yy,zz=values(tuple(x+t*y for x,y in zip(a,d)),layers)
            return xx*d[0]*d[0]+2*xy*d[0]*d[1]+yy*d[1]*d[1]+zz*d[2]*d[2]
        total=0.
        for left,right in zip(cuts,cuts[1:]):
            if not layers:
                s0=math.sqrt(squared(left));s1=math.sqrt(squared(right));total+=(right-left)*2*(s0*s0+s0*s1+s1*s1)/(3*(s0+s1));continue
            for k in range(8):
                lo=left+(right-left)*k/8;hi=left+(right-left)*(k+1)/8
                value,error=quad(lambda t:math.sqrt(squared(t)),lo,hi,epsabs=2e-12,epsrel=2e-12,limit=100)
                assert error<2e-8;total+=value
        return total
    prefix='regional_'+{'SCALAR':'','CROSSING':'crossing_','TANGENTIAL':'tangential_','ROTATING':'rotating_'}[kind]+'finer_grouped_';reports=[]
    for stage,layers in [('coupled_BL',True),('released_Euler',False)]:
        nodes,cells=records(case/'native.log',prefix+stage);data=dict(template,nodes=[[n]+p for n,p in sorted(nodes.items())],cells=[{'id':c['id'],'nodes':c['nodes']} for c in cells],surface=[f for c in cells for f in c['surface']]);path=case/(stage+'_independent.json');path.write_text(json.dumps(data,separators=(',',':'))+'\n')
        report,_=frame(path,True,True,lambda original,p:metric(original,p,layers),lambda original,a,b:length(tuple(a),tuple(b),layers))
        assert F(report['volume'])==1
        if layers:
            feet=footprints(data,'marker_10');assert feet and all(not r['outside'] for r in feet)
            height=max(abs(nodes[r['apex']][2]/.1-1) for r in feet);assert height<1e-10
            pairs={tuple(sorted((c['nodes'][i],c['nodes'][j]))) for c in cells for i,j in itertools.combinations(range(4),2)}
            wallNodes={n for r in feet for n in r['face']};aligned=0
            for n in wallNodes:
                assert any(p[:2]==nodes[n][:2] and abs(p[2]-.1)<1e-12 and tuple(sorted((n,j))) in pairs for j,p in nodes.items());aligned+=1
            assert min(p[2] for p in nodes.values() if p[2]>0)>=.1-1e-12
            report.update(first_height_faces=len(feet),maximum_height_relative_error=height,wall_normal_first_edges=aligned,full_composed_edges=len(pairs))
        report.update(layers=layers,input_sha256=sha(path));reports.append(report)
    assert reports[1]['cells']<reports[0]['cells']
    out={'status':'PASS_COMPLETE_REGIONAL_SINGLE_WALL_ORIGINAL_FIELD_GEOMETRY_AND_FULL_EDGES','field':kind,'frames':reports,'native_receipt_sha256':sha(rp),'reference_input_sha256':sha(reference),'helper_sha256':sha(Path(__file__)),'geometry_helper_sha256':sha(ROOT/'integration_evidence/native_3d_core_v1/audit_coupled.py'),'elapsed_seconds':time.monotonic()-started,
        'scope':'Same finite unit-box geometry with only the lower wall constrained h.1/g1.2/T.3. Independent analytic planar BL/fade plus the declared exact six-tet original-P1 scalar, crossing-normal, constant rotated tangent256:4, or affine varying/rotating tangent tensor; each original tet stays on one side of x=z, proving the crossing formula affine there. Tangent inverse-covariance eigenvalues<=1.5 and fade core4 are dominated by the sensor. Exact whole embedding/finite authority/links/height/normal columns, all q/J and every full numerical BL edge, analytic piecewise original-sensor edges after removal. Numerical error indicators are not enclosures. No actual CFD/transport/restart, arbitrary tangential-field alignment, general geometry or affordability qualification.'}
    (case/'independent.json').write_text(json.dumps(out,indent=2)+'\n');print(out['status'],flush=True)
if __name__=='__main__':main()
