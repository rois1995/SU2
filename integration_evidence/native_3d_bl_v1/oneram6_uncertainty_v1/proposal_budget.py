#!/usr/bin/env python3
"""Geometry-only work estimate for a proposed strong-BL importance sampler; no sampled integral yet."""
import json,time
from pathlib import Path
import numpy as np
from run import HERE,ROOT,OLD,sha
from support_bound import wall_geometry,inverse_height_moments


def main():
    folder=HERE/'geometric_correction_sampler_v1';folder.mkdir(exist_ok=True)
    start=time.monotonic();fixture=OLD/'cell329/replay.dat';g=wall_geometry(fixture)
    edges=np.roll(g['x'],-1,axis=1)-g['x'];length=np.sqrt(np.sum(edges*edges,axis=2))
    cross=np.cross(g['x'][:,1]-g['x'][:,0],g['x'][:,2]-g['x'][:,0])
    trace_face=np.log(np.longdouble(4)/3*np.sum(cross*cross,axis=1))
    trace=np.zeros(len(g['p']),dtype=np.longdouble);weight=np.zeros_like(trace)
    for k in range(3):
        angle=np.arccos(np.clip(-np.sum(edges[:,(k+2)%3]*edges[:,k],axis=1)/(length[:,(k+2)%3]*length[:,k]),-1,1))
        np.add.at(trace,g['f'][:,k],angle*trace_face);np.add.at(weight,g['f'][:,k],angle)
    assert np.all(weight>0);trace/=weight
    tangent_density=np.exp(-.5*trace)
    face_density=np.max(tangent_density[g['f']],axis=1)
    edge_density=np.max(tangent_density[g['edges']],axis=1)
    radius=max(g['h0'],.9*g['thickness']);moments=inverse_height_moments(g['h0'],g['growth'],radius)
    # These are integrals of the chosen proposal profile 1/max(h0,b*r), not integrals of the physical metric.
    masses=[2*g['area']*moments[0],g['edge_angle_length']*moments[1],g['omega']*moments[2]]
    coefficients=[face_density,edge_density,tangent_density]
    weights=[a*b for a,b in zip(masses,coefficients)]
    parts=[float(np.sum(w,dtype=np.longdouble)) for w in weights];total=sum(parts)
    assert np.isfinite(total) and total>0 and all(p>=0 for p in parts)
    np.savez_compressed(folder/'primitive_weights.npz',face_density=np.asarray(face_density,float),
        edge_density=np.asarray(edge_density,float),vertex_density=np.asarray(tangent_density,float),
        face_mass=np.asarray(masses[0],float),edge_mass=np.asarray(masses[1],float),vertex_mass=np.asarray(masses[2],float))
    delta=.01
    planning=[{'samples':n,'ideal_IID_Hoeffding_halfwidth_99pct':float(total*np.sqrt(np.log(2/delta)/(2*n)))} for n in (4096,16384,65536,262144)]
    report={'scope':'Preparation only: faceted3D strong-band bare-wall density proposal, no original-domain integration, random samples, sensor correction or production normalization.',
        'wall_vertices':len(g['p']),'wall_faces':len(g['f']),'wall_edges':len(g['edges']),'radius':radius,
        'face_proxy_mass':parts[0],'edge_proxy_mass':parts[1],'vertex_proxy_mass':parts[2],
        'full_space_strong_band_upper_proxy':total,'planned_statistical_work':planning,
        'statistical_assumptions':'Ideal independent continuous samples, exact proposal density and exact field; planned bounds do not certify floating implementation or the unimplemented sampler.',
        'machine_bound_proven':False,'global_integral_measured':False,'CFD_steps':0,'seconds':time.monotonic()-start,
        'sha256':{str(p.relative_to(ROOT)):sha(p) for p in (Path(__file__).resolve(),Path(__file__).resolve().with_name('support_bound.py'),fixture,ROOT/'SU2_CFD/src/adaptation/CBoundaryLayerMetric.cpp')}}
    report['weights_sha256']=sha(folder/'primitive_weights.npz')
    (folder/'preparation.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report),flush=True)


if __name__=='__main__':main()
