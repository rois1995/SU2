#!/usr/bin/env python3
"""Independent finite-facet, hole/volume, complete metric and current/BDF/SA audit for the controlled annulus."""
import collections,importlib.util,itertools,json,math,sys
from fractions import Fraction as F
from pathlib import Path
import numpy as np
HERE=Path(__file__).resolve().parent;ROOT=HERE.parents[2]
spec=importlib.util.spec_from_file_location('policy',ROOT/'integration_evidence/native_3d_bl_v1/production_policy_v1/audit.py')
policy=importlib.util.module_from_spec(spec);spec.loader.exec_module(policy);base=policy.base

def cross2(a,b,p):return (b[0]-a[0])*(p[1]-a[1])-(b[1]-a[1])*(p[0]-a[0])
def twice_area(p):return sum(a[0]*b[1]-a[1]*b[0] for a,b in zip(p,p[1:]+p[:1])) if p else F(0)
def intersection_area(a,b):
    polygon=list(a);sign=1 if twice_area(b)>0 else -1
    for x,y in zip(b,b[1:]+b[:1]):
        result=[]
        for u,v in zip(polygon,polygon[1:]+polygon[:1]):
            s,t=sign*cross2(x,y,u),sign*cross2(x,y,v)
            if s>=0:result.append(u)
            if (s<0 and t>0) or (s>0 and t<0):
                q=s/(s-t);result.append(tuple(u[k]+q*(v[k]-u[k]) for k in range(2)))
        polygon=result
    return abs(twice_area(polygon))

def geometry(data,theta,parameter):
    nodes={n[0]:tuple(F(float(x)) for x in n[1:]) for n in data['nodes']}
    assert len(nodes)==len(data['nodes'])==len(set(nodes.values()))
    cells=[c['nodes'] for c in data['cells']];tets=[[nodes[n] for n in c] for c in cells]
    volumes=[base.determinant(t)/6 for t in tets];assert min(volumes)>0
    expected=1-(F(.6)-F(.4))**2;assert sum(volumes)==expected
    assert all(all(0<=x<=1 for x in p) for p in nodes.values())
    pairs=0
    for a,b in itertools.combinations(tets,2):
        if any(max(p[k] for p in a)<=min(p[k] for p in b) or max(p[k] for p in b)<=min(p[k] for p in a) for k in range(3)):continue
        assert not base.overlap(a,b),'Positive-volume overlap';pairs+=1
    uses=collections.defaultdict(list);face_cells=collections.defaultdict(list);links=collections.defaultdict(list)
    for index,c in enumerate(cells):
        a,b,d,e=c
        for face in ((b,d,e),(a,e,d),(a,b,e),(a,d,b)):
            key=tuple(sorted(face));uses[key].append(base.parity(face));face_cells[key].append(index)
        for n in c:links[n].append([k for k in c if k!=n])
    assert all(len(v)==1 or len(v)==2 and sum(v)==0 for v in uses.values())
    adjacency={i:set() for i in range(len(cells))}
    for row in face_cells.values():
        if len(row)==2:a,b=row;adjacency[a].add(b);adjacency[b].add(a)
    assert base.connected(adjacency)

    boundary={f:v[0] for f,v in uses.items() if len(v)==1};physical={tuple(sorted(f['nodes'])):f for f in data['surface']}
    assert len(physical)==len(data['surface']) and physical.keys()==boundary.keys()
    border_nodes={n for f in boundary for n in f}
    # The full boundary is a torus, while each volume vertex link remains a sphere/disk.
    boundary_edges=collections.Counter(tuple(sorted((f[k],f[(k+1)%3]))) for f in boundary for k in range(3))
    assert all(n==2 for n in boundary_edges.values())
    assert len(border_nodes)-len(boundary_edges)+len(boundary)==0
    for n,triangles in links.items():base.surface(triangles,n not in border_nodes)
    facets={f['id']:f for f in data['facets']};membership={};groups={};areas=collections.defaultdict(F)
    for group in data['facet_groups']:
        root=min(group);triangles=[];signed_area=F(0)
        for id in group:
            facet=facets[id];t=[tuple(F(float(x)) for x in v[1:]) for v in facet['vertices']]
            normal=base.cross(base.sub(t[1],t[0]),base.sub(t[2],t[0]));axis=next(i for i in range(3) if normal[i])
            if not triangles:origin=t[0];reference_normal=normal;reference_axis=axis
            assert axis==reference_axis and normal[axis]*reference_normal[axis]>0
            assert all(base.dot(reference_normal,base.sub(p,origin))==0 for p in t)
            flat=[tuple(p[k] for k in range(3) if k!=axis) for p in t];triangles.append(flat);membership[id]=root;signed_area+=normal[axis]
        groups[root]=(origin,reference_normal,axis,triangles,signed_area)
    minimum_q=minimum_j=1.;edges=set();heights=[]
    for key,face in physical.items():
        assert base.parity(face['nodes'])==boundary[key]
        root=membership[face['facet']];origin,normal,axis,reference,_=groups[root]
        t=[nodes[n] for n in face['nodes']];assert all(base.dot(normal,base.sub(p,origin))==0 for p in t)
        flat=[tuple(p[k] for k in range(3) if k!=axis) for p in t]
        signed=base.cross(base.sub(t[1],t[0]),base.sub(t[2],t[0]))[axis];assert signed*normal[axis]>0
        assert sum(intersection_area(flat,r) for r in reference)==abs(twice_area(flat)),'Face bridges a finite patch or hole'
        areas[root]+=signed
        if all(p[2]==0 for p in t):
            incident=[c for c in cells if set(face['nodes']).issubset(c)];assert len(incident)==1
            apex=nodes[next(n for n in incident[0] if n not in face['nodes'])];heights.append(abs(float(apex[2])/0.1-1))
            foot=apex[:2];wall=[p[:2] for p in t];sign=1 if twice_area(wall)>0 else -1
            assert all(sign*cross2(a,b,foot)>=0 for a,b in zip(wall,wall[1:]+wall[:1]))
    for root,(_,_,_,_,original_area) in groups.items():
        assert areas[root]==original_area,'Changed finite reference area'
    assert heights and max(heights)<1e-8
    def eigen(z):
        z=np.asarray(z);h=np.maximum(.1,(.1+.2*z)/1.1)
        slope=2*.2/2.2+parameter*(2*9/11-2*.2/2.2)
        if parameter:h=np.where(z<=.1,h,np.maximum(h,(.1+.2*.1)/1.1+slope*(z-.1)))
        u=np.clip((z-.27)/.03,0,1);w=1-u*u*(3-2*u);core=math.log(theta)
        return tuple(np.exp(np.maximum(core,w*v+(1-w)*core)) for v in (math.log(.5),math.log(1.5),-2*np.log(h)))
    for ids,t in zip(cells,tets):
        center=[float(sum(p[k] for p in t)/4) for k in range(3)];lo,hi,no=(float(x) for x in eigen(center[2]))
        m=policy.packed(np.array([[(lo+hi)/2,(lo-hi)/2,0],[(lo-hi)/2,(lo+hi)/2,0],[0,0,no]]))
        value=float(base.determinant(t))*math.sqrt(float(base.spd(m)));squared={pair:float(base.quad(m,base.sub(t[pair[1]],t[pair[0]]))) for pair in itertools.combinations(range(4),2)}
        q=12*(value/2)**(2/3)/sum(squared.values());j=min(math.sqrt(2)*value/math.prod(math.sqrt(squared[tuple(sorted((i,k)))]) for k in range(4) if i!=k) for i in range(4))
        assert q>=.2 and j>=.05;minimum_q=min(minimum_q,q);minimum_j=min(minimum_j,j)
        edges.update(tuple(sorted(e)) for e in itertools.combinations(ids,2))
    maximum=0;order_spread=0
    for a,b in edges:
        a,b=np.array(list(map(float,nodes[a]))),np.array(list(map(float,nodes[b])));delta=b-a;cuts=[0.,1.]
        if delta[2]:cuts += [(z-a[2])/delta[2] for z in (.05,.1,.27,.3) if 0<(z-a[2])/delta[2]<1]
        values=[]
        for order in (32,64):
            x,w=np.polynomial.legendre.leggauss(order);total=0.
            for low,high in zip(sorted(cuts),sorted(cuts)[1:]):
                t=(low+high)/2+(high-low)*x/2;lo,hi,no=eigen(a[2]+t*delta[2]);speed=np.sqrt((lo+hi)/2*(delta[0]**2+delta[1]**2)+(lo-hi)*delta[0]*delta[1]+no*delta[2]**2)
                total+=(high-low)/2*float(w@speed)
            values.append(total)
        order_spread=max(order_spread,abs(values[1]-values[0]));assert abs(values[1]-values[0])<1e-8
        maximum=max(maximum,values[-1]);assert values[-1]<=1.8+1e-12
    masses=[]
    for order in (32,64,96):
        x,w=np.polynomial.legendre.leggauss(order);mass=0.
        cuts=(0.,.05,.1,.27,.3,.4,.6,1.)
        for a,b in zip(cuts,cuts[1:]):
            z=(a+b)/2+(b-a)*x/2;lo,hi,no=eigen(z);area=.8 if a>=.4 and b<=.6 else 1.
            mass+=(b-a)/2*area*float(w@np.sqrt(lo*hi*no))
        masses.append(mass)
    assert max(masses)-min(masses)<1e-8
    return {'cells':len(cells),'volume':str(expected),'exact_overlap_pairs':pairs,'vertex_links':len(links),'finite_patch_area_coverage':True,
        'boundary_euler_characteristic':0,'minimum_quality':minimum_q,'minimum_scaled_jacobian':minimum_j,'maximum_full_edge':maximum,
        'height_relative_error':max(heights),'edge_order_spread':order_spread,'reference_complexity':masses[-1],'reference_order_spread':max(masses)-min(masses)}

def main():
    folder=Path(sys.argv[1]).resolve();reports=[]
    for p in sorted(folder.glob('symmetry_annulus_bl_budget_*_profile.json')):
        if p.name.endswith('_frozen_profile.json'):continue
        meta=json.loads(p.read_text());stem=p.name[:-len('_profile.json')];data=json.loads((folder/(stem+'.json')).read_text())
        theta=meta['final_sensor_eigenvalue'];source,_=base.sources(data)
        assert all(abs(float(m[k])-[theta,0,0,theta,0,theta][k])<2e-12 for c in source for m in c['metrics'] for k in range(6))
        report=geometry(data,theta,meta['parameter'])
        for suffix in ('','_turb'):
            states=json.loads((folder/(stem+suffix+'_states.json')).read_text());old=policy.integrals(data,states['donor'],True);new=policy.integrals(data,states['accepted'])
            defect=max(abs(a-b)/max(1e-30,abs(a)) if a else abs(b) for a,b in zip(old,new));assert defect<1e-10
            restart=json.loads((folder/(stem+suffix+'_restart_states.json')).read_text());expected={tuple(r['point']):r['states'] for r in restart['donor']}
            assert len(expected)==len(restart['accepted'])
            scale=max(abs(x) for row in expected.values() for x in row);residual=max(abs(x-y)/max(1e-30,scale) for row in restart['accepted'] for x,y in zip(expected[tuple(row['point'])],row['states']));assert residual<1e-12
            report[('SA' if suffix else 'flow')+'_integral_defect']=defect;report[('SA' if suffix else 'flow')+'_restart_residual']=residual
        reloads=list(folder.glob(stem+'_written_*_reload_target.json'));assert len(reloads)==2
        assert all(policy.signature(json.loads(p.read_text()))==policy.signature(data) for p in reloads)
        report.update(meta);report['independent_mass_error']=abs(meta['mass']-report['reference_complexity']);report['indicator_covers_observed_error']=report['independent_mass_error']<=meta['indicator'];reports.append(report)
    assert len(reports)==1 and reports[0]['budget']==20,'Complete declared annular workflow required'
    out={'status':'INDEPENDENT_ANNULAR_SYMMETRY_LIFECYCLE_PASS','cases':reports,
        'audit_source_sha256':policy.digest(Path(__file__)),'house_auditor_sha256':policy.digest(ROOT/'integration_evidence/native_3d_core_v1/audit_coupled.py'),'limitations':['Controlled faceted annular domain/one wall/constant original sensor, not M6 or interacting BL accuracy','Numerical quadrature reference/heuristic complexity indicator, not rigorous integral bounds','Native contact/field gates supplemented by independent volume/topology/overlap/finite-face/height and complete metric checks']}
    (folder/'independent_annulus.json').write_text(json.dumps(out,indent=2)+'\n');print(out['status'])
if __name__=='__main__':main()
