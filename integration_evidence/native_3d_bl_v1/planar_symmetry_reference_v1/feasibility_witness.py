#!/usr/bin/env python3
"""Construct and independently check one mesh for the SAME controlled frozen annular field; not native adaptation."""
import importlib.util,json,sys
from fractions import Fraction as F
from pathlib import Path
HERE=Path(__file__).resolve().parent
spec=importlib.util.spec_from_file_location('annular_audit',HERE/'audit_annulus.py');audit=importlib.util.module_from_spec(spec);spec.loader.exec_module(audit)

def main():
    folder=Path(sys.argv[1]).resolve();folder.mkdir(exist_ok=True)
    source=HERE/'annulus_v3_n1';stem='symmetry_annulus_bl_budget_20_m1'
    original=source/(stem+'_initial.json');data=json.loads(original.read_text());profile=json.loads((source/(stem+'_frozen_profile.json')).read_text())
    axes=([0.,.4,.6,1.],[0.,.5,1.],[0.,.1,.2,.3,.4,.6,1.]);points=[];ids={}
    for k,z in enumerate(axes[2]):
        for j,y in enumerate(axes[1]):
            for i,x in enumerate(axes[0]):ids[i,j,k]=len(points);points.append((x,y,z))
    cells=[];incidence={}
    for i in range(len(axes[0])-1):
        for j in range(len(axes[1])-1):
            for k in range(len(axes[2])-1):
                if axes[0][i]==.4 and axes[0][i+1]==.6 and axes[2][k]==.4 and axes[2][k+1]==.6:continue
                for order in audit.itertools.permutations(range(3)):
                    q=[i,j,k];tet=[ids[tuple(q)]]
                    for axis in order:q[axis]+=1;tet.append(ids[tuple(q)])
                    if audit.base.determinant([tuple(F(x) for x in points[n]) for n in tet])<0:tet[0],tet[1]=tet[1],tet[0]
                    cells.append({'id':len(cells),'nodes':tet})
                    a,b,c,d=tet
                    for f in ((b,c,d),(a,d,c),(a,b,d),(a,c,b)):incidence.setdefault(tuple(sorted(f)),[]).append(f)
    facets={f['id']:f for f in data['facets']};groups={}
    for g in data['facet_groups']:
        triangles=[[tuple(F(float(x)) for x in v[1:]) for v in facets[id]['vertices']] for id in g]
        a,b,c=triangles[0];normal=audit.base.cross(audit.base.sub(b,a),audit.base.sub(c,a));axis=next(i for i in range(3) if normal[i])
        groups[min(g)]=(a,normal,axis,[[tuple(p[k] for k in range(3) if k!=axis) for p in t] for t in triangles])
    surface=[]
    for row in incidence.values():
        assert len(row) in (1,2)
        if len(row)==2:continue
        f=row[0];t=[tuple(F(x) for x in points[n]) for n in f];chosen=[]
        for root,(origin,normal,axis,triangles) in groups.items():
            if any(audit.base.dot(normal,audit.base.sub(p,origin))!=0 for p in t):continue
            signed=audit.base.cross(audit.base.sub(t[1],t[0]),audit.base.sub(t[2],t[0]))[axis]
            if signed*normal[axis]<=0:continue
            flat=[tuple(p[k] for k in range(3) if k!=axis) for p in t]
            if sum(audit.intersection_area(flat,r) for r in triangles)==abs(audit.twice_area(flat)):chosen.append(root)
        assert len(chosen)==1,(f,chosen)
        surface.append({'facet':chosen[0],'nodes':f})
    data.update(nodes=[[n]+list(p) for n,p in enumerate(points)],cells=cells,surface=surface)
    path=folder/'annulus_feasibility_witness.json';path.write_text(json.dumps(data,separators=(',',':'))+'\n')
    report=audit.geometry(data,profile['core'],profile['parameter'])
    lines=['NDIME= 3','NELEM= '+str(len(cells))]
    lines += ['10 '+' '.join(map(str,c['nodes']))+' '+str(c['id']) for c in cells]
    lines += ['NPOIN= '+str(len(points))]+[' '.join(format(x,'.17g') for x in p)+' '+str(n) for n,p in enumerate(points)]
    markers=sorted({facets[f['facet']]['marker'] for f in surface});lines+=['NMARK= '+str(len(markers))]
    for marker in markers:
        faces=[f['nodes'] for f in surface if facets[f['facet']]['marker']==marker]
        lines+=['MARKER_TAG= side_'+str(marker),'MARKER_ELEMS= '+str(len(faces))]+['5 '+' '.join(map(str,f)) for f in faces]
    (folder/'annulus_feasibility_witness.su2').write_text('\n'.join(lines)+'\n')
    report.update(status='INDEPENDENT_SAME_FROZEN_FIELD_FEASIBILITY_WITNESS',native_adaptation=False,solution_transfer=False,
        original_source_sha256=audit.policy.digest(original),original_profile_sha256=audit.policy.digest(source/(stem+'_frozen_profile.json')),
        witness_sha256=audit.policy.digest(path),generator_sha256=audit.policy.digest(Path(__file__)),audit_sha256=audit.policy.digest(HERE/'audit_annulus.py'),
        limits=['Explicit Cartesian construction for this controlled domain, not a general native constructor or M6 feasibility proof',
            'Original donor field/profile unchanged; no adapted-node replacement field, state transfer or affordability claim'])
    (folder/'report.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report,indent=2))
if __name__=='__main__':main()
