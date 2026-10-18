#!/usr/bin/env python3
"""Bounded saved-state MPI residual diagnosis; no mesh acceptance certificate."""
import hashlib,importlib.util,itertools,json,math,re,time
from pathlib import Path
import numpy as np
HERE=Path(__file__).resolve().parent;ROOT=HERE.parents[2]
BASE=HERE.with_name('planar_symmetry_reference_v1')/'annulus_v9_n2'
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
def main():
    start=time.monotonic();stem='symmetry_annulus_bl_budget_20_m2';sources=[BASE/(stem+f'_rejected_rank{rank}.json') for rank in range(2)]
    shards=[json.loads(p.read_text()) for p in sources];nodes={};cells={};skin={}
    for shard in shards:
        for row in shard['nodes']:
            assert row[0] not in nodes or nodes[row[0]]==row[1:],'Conflicting shared coordinates'
            nodes[row[0]]=row[1:]
        for cell in shard['cells']:
            assert cell['id'] not in cells,'Owned cells repeated across ranks'
            cells[cell['id']]=cell
        for f in shard['surface']:
            key=tuple(sorted(f['nodes']));assert key not in skin or skin[key]==f
            skin[key]=f
    mesh=dict(shards[0],nodes=[[n]+p for n,p in sorted(nodes.items())],cells=list(cells.values()),surface=list(skin.values()))
    profile=BASE/(stem+'_frozen_profile.json');parameters=json.loads(profile.read_text())
    assert parameters['original_field'] and parameters['parameter']==0;theta=parameters['core']
    probe=HERE.with_name('annulus_coordinated_replay_v1')/'fan_split_v1/probe_v4.py'
    spec=importlib.util.spec_from_file_location('probe',probe);module=importlib.util.module_from_spec(spec);spec.loader.exec_module(module)
    metrics=module.measures(mesh,theta)
    points={n:np.array(p) for n,p in nodes.items()};edges={tuple(sorted(e)) for c in cells.values() for e in itertools.combinations(c['nodes'],2)}
    gx,gw=np.polynomial.legendre.leggauss(48)
    def normal(z):
        h=np.maximum(.1,(.1+.2*z)/1.1);u=np.clip((z-.27)/.03,0,1);w=1-u*u*(3-2*u)
        return np.exp(np.maximum(np.log(theta),w*(-2*np.log(h))+(1-w)*np.log(theta)))
    bad=[]
    for e in sorted(edges):
        p=points[e[0]];d=points[e[1]]-p;cuts=sorted([0.,1.]+([(z-p[2])/d[2] for z in (.05,.27,.3) if 0<(z-p[2])/d[2]<1] if d[2] else []))
        value=0.
        for lo,hi in zip(cuts,cuts[1:]):
            t=(lo+hi)/2+(hi-lo)*gx/2
            value+=(hi-lo)/2*float(gw@np.sqrt(theta*sum(d[:2]**2)+normal(p[2]+t*d[2])*d[2]**2))
        if value>1.8:bad.append(dict(edge=e,length=value,points=[nodes[n] for n in e]))
    assert len(bad)==metrics['oversized_edges'] and abs(max(b['length'] for b in bad)-metrics['maximum_edge'])<1e-13
    walls=[f for f in skin.values() if all(nodes[n][2]==0 for n in f['nodes'])];owners={}
    for f in walls:
        incident=[c for c in cells.values() if set(f['nodes']).issubset(c['nodes'])];assert len(incident)==1
        apex=next(n for n in incident[0]['nodes'] if n not in f['nodes'])
        owners.setdefault(apex,[]).append(f['nodes'])
    ownership=[]
    for n,faces in owners.items():
        star=[c for c in cells.values() if n in c['nodes']]
        ownership.append(dict(apex=n,point=nodes[n],wall_faces=faces,complete_star_cells=len(star),
            incident_bad_edges=[b for b in bad if n in b['edge']]))
    logs=[]
    for line in (BASE/'run.log').read_text().splitlines():
        if 'NATIVE3D_DETAIL stage=refine' in line:
            fields=dict(re.findall(r'(\w+)=([^ ]+)',line));logs.append({k:fields[k] for k in ('rank','stop','global_work','accepted','placement_trials','placement_validated','wall_topology_trials','wall_topology_accepted','cached_invalid_cells','height_rejected','footprint_rejected')})
    serial=HERE.with_name('planar_symmetry_reference_v1')/'annulus_v9_n1/symmetry_annulus_bl_budget_20_m1_frozen_profile.json'
    serial_theta=json.loads(serial.read_text())['core']
    result=dict(status='SAVED_MPI2_METRIC_RESIDUAL_DIAGNOSIS',metrics=metrics,bad_edges=sorted(bad,key=lambda x:-x['length']),wall_ownership=ownership,
        rank_statistics=logs,sensor_eigenvalue=theta,serial_eigenvalue=serial_theta,sensor_relative_difference=theta/serial_theta-1,
        seconds=time.monotonic()-start,source_sha256={str(p.resolve().relative_to(ROOT)):sha(p) for p in sources+[profile,serial,probe,Path(__file__)]},
        limits=['One planar wall and constant original sensor; numerical reference, no rigorous quadrature bound',
            'Rejected private distributed trajectory; no accepted MPI mesh or transfer',
            'Different partitions can produce different valid edit sequences; serial profile differs within integration uncertainty'])
    (HERE/'mpi2_rejected_merged.json').write_text(json.dumps(mesh,separators=(',',':'))+'\n')
    (HERE/'mpi2_residuals.json').write_text(json.dumps(result,indent=2)+'\n')
    print(json.dumps(result,indent=2),flush=True)
if __name__=='__main__':main()
