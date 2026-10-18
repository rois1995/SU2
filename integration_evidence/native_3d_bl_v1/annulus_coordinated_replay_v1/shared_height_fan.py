#!/usr/bin/env python3
"""Exact analytic lower bound for the retained fan; independent per-face seeds are not a completed volume."""
import hashlib,itertools,json,math,sys
from fractions import Fraction as F
from pathlib import Path
import numpy as np
HERE=Path(__file__).resolve().parent;ROOT=HERE.parents[2]
sys.path.insert(0,str(ROOT/'integration_evidence/native_3d_core_v1'))
from audit_cavities import determinant,cross,sub
BASE=HERE.with_name('planar_symmetry_reference_v1')/'annulus_v7_n1';STEM='symmetry_annulus_bl_budget_20_m1'
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
def main():
    path=BASE/(STEM+'_rejected_rank0.json');profile_path=BASE/(STEM+'_frozen_profile.json')
    data=json.loads(path.read_text());profile=json.loads(profile_path.read_text());theta=profile['core'];assert profile['parameter']==0 and profile['original_field']
    nodes={n[0]:tuple(F(float(x)) for x in n[1:]) for n in data['nodes']};height=F(.1);growth=F(1.2);groups={}
    for face in data['surface']:
        if not all(nodes[n][2]==0 for n in face['nodes']):continue
        cells=[c for c in data['cells'] if set(face['nodes']).issubset(c['nodes'])];assert len(cells)==1
        apex=next(n for n in cells[0]['nodes'] if n not in face['nodes']);groups.setdefault(apex,[]).append(face['nodes'])
    terminal=json.loads((HERE/'terminal_residuals.json').read_text());bad=terminal['reports']['v7']['bad_edges']
    common=set(bad[0]['edge'])
    for edge in bad:common&=set(edge['edge'])
    assert len(common)==1;apex=common.pop();faces=groups[apex];assert len(faces)==8 and nodes[apex][2]==height
    corners=sorted({n for e in bad for n in e['edge'] if n!=apex});assert len(corners)==4
    diam2=max(sum((nodes[a][k]-nodes[b][k])**2 for k in range(2)) for a,b in itertools.combinations(corners,2))
    radius2=diam2/4
    # On0<=z<=h0 the unfaded normal spacing is at most2*g*h0/(g+1), and intersection preserves finer demands.
    normal_spacing_max=2*growth*height/(growth+1)
    normal_eigen_min=1/normal_spacing_max**2
    lower_squared=F(theta)*radius2+height**2*normal_eigen_min
    cap_squared=F(1.8)**2;assert lower_squared>cap_squared
    gx,gw=np.polynomial.legendre.leggauss(48)
    def length(a,b):
        a=np.array(list(map(float,a)));b=np.array(list(map(float,b)));d=b-a;cuts=[0.,1.]
        if d[2]:cuts += [(z-a[2])/d[2] for z in (.05,.27,.3) if 0<(z-a[2])/d[2]<1]
        cuts.sort();result=0.
        for low,high in zip(cuts,cuts[1:]):
            t=(low+high)/2+(high-low)*gx/2;z=a[2]+t*d[2];h=np.maximum(.1,(.1+.2*z)/1.1);u=np.clip((z-.27)/.03,0,1);w=1-u*u*(3-2*u)
            normal=np.exp(np.maximum(np.log(theta),w*(-2*np.log(h))+(1-w)*np.log(theta)))
            result+=(high-low)/2*float(gw@np.sqrt(theta*(d[0]**2+d[1]**2)+normal*d[2]**2))
        return float(result)
    seeds=[]
    for face in faces:
        base=[nodes[n] for n in face];tip=tuple(F(float(sum(p[k] for p in base)/3)) for k in range(2))+(height,)
        # The represented foot must remain in the exact finite base triangle.
        normal=cross(sub(base[1],base[0]),sub(base[2],base[0]))[2]
        for a,b in zip(base,base[1:]+base[:1]):assert normal*((b[0]-a[0])*(tip[1]-a[1])-(b[1]-a[1])*(tip[0]-a[0]))>=0
        tet=[tip]+base;det=determinant(tet);assert det>0
        matrix=np.diag([theta,theta,100.]);p=np.array([[float(x) for x in v] for v in tet]);dv=float(det)*theta*10
        squared={(a,b):float((p[b]-p[a])@matrix@(p[b]-p[a])) for a,b in itertools.combinations(range(4),2)}
        q=12*(dv/2)**(2/3)/sum(squared.values());jac=min(math.sqrt(2)*dv/math.prod(math.sqrt(squared[tuple(sorted((a,b)))]) for b in range(4) if a!=b) for a in range(4))
        longest=max(length(tet[a],tet[b]) for a,b in itertools.combinations(range(4),2));assert q>=.2 and jac>=.05 and longest<=1.8
        seeds.append(dict(face=face,apex=list(map(float,tip)),q=q,J=jac,maximum_full_edge=longest))
    report=dict(status='RETAINED_COMMON_HEIGHT_APEX_CONFLICT_WITH_FEASIBLE_INDIVIDUAL_ROW_SEEDS',common_apex=apex,point=list(map(float,nodes[apex])),wall_faces=faces,corners=corners,
        diametral_radius_squared=str(radius2),exact_analytic_length_lower_squared=str(lower_squared),cap_squared=str(cap_squared),analytic_length_lower_bound=math.sqrt(float(lower_squared)),actual_common_corner_length=bad[0]['length'],
        independently_placed_first_row_seeds=seeds,minimum_seed_q=min(s['q'] for s in seeds),minimum_seed_J=min(s['J'] for s in seeds),maximum_seed_edge=max(s['maximum_full_edge'] for s in seeds),
        implication='Tangential motion of one common height apex cannot satisfy all four corner edges under this frozen field. Decouple first-height ownership through coordinated wall-child/transition reconstruction. Existing wall triangles already admit individual height apices; merely refining the wall is not established as necessary.',
        limits=['Exact rational lower bound is for the analytic saved unfaded profile and its finer sensor composition, not an interval proof of every floating backend query','Independent row seeds are not a complete volume replacement; their transition gaps, original point retention and fixed interface still need construction/validation','No proof of whole-target infeasibility, and no geometry/height/metric threshold is relaxed'],
        source_sha256={str(p.relative_to(ROOT)):sha(p) for p in [path,profile_path,HERE/'terminal_residuals.json',Path(__file__).resolve()]})
    (HERE/'shared_height_fan.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps({k:v for k,v in report.items() if k not in ['independently_placed_first_row_seeds','source_sha256','wall_faces']},indent=2))
if __name__=='__main__':main()
