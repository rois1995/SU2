#!/usr/bin/env python3
"""Validate the passive4-rank capture and pack exact owner AND halo matrices for offline replay."""
import hashlib
import json
from pathlib import Path
import sys
import numpy as np

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[2]
sys.path.insert(0, str(ROOT/'TestCases/adaptation/capability'))
from capcheck import read_su2

def digest(path):
    value=hashlib.sha256()
    with path.open('rb') as stream:
        for block in iter(lambda: stream.read(1024*1024), b''):value.update(block)
    return value.hexdigest()

def main():
    provenance=json.loads((HERE/'provenance.json').read_text())
    for name,expected in provenance['sha256'].items():
        # UnitTests were recorded as context; the capture executed only SU2_CFD.
        if name.startswith('UnitTests/') or '/UnitTests/' in name:continue
        assert digest(ROOT/name)==expected, name
    mesh=read_su2(HERE/'Coarse.su2');count=len(mesh.P)
    assert mesh.dim==3 and len(mesh.E)>0
    owner=np.full(count,-1,dtype=np.int8);sensor=np.empty((count,3,3));ranks=[];metadata=[];pins={}
    packed={};helper=digest(HERE/'capture.gdb');parent=digest(HERE/'provenance.json')
    for rank in range(4):
        path=HERE/('rank%d_capture.json'%rank);meta=json.loads(path.read_text())
        assert meta['state']=='COMPLETE_PRE_QUADRATURE_RAW_CAPTURE' and meta['rank']==rank and meta['size']==4
        assert meta['helper_sha256']==helper and meta['provenance_sha256']==parent
        assert meta['complexity_trial']==1 and meta['byteorder']=='little' and meta['tensor_bytes']==72
        for name,info in meta['files'].items():
            p=HERE/name;assert p.stat().st_size==info['bytes'] and digest(p)==info['sha256'],name
            pins[name]=info['sha256']
        ids=np.fromfile(HERE/('rank%d_global.u64'%rank),dtype='<u8')
        xyz=np.fromfile(HERE/('rank%d_coordinates.f64'%rank),dtype='<f8').reshape(-1,3)
        values=np.fromfile(HERE/('rank%d_sensor.f64'%rank),dtype='<f8').reshape(-1,3,3)
        domain=np.fromfile(HERE/('rank%d_domain.u8'%rank),dtype='u1')
        n=meta['points'];nowned=meta['owned_points']
        assert len(ids)==len(xyz)==len(values)==len(domain)==n and 0<nowned<=n
        assert len(np.unique(ids))==n and np.all(ids<count)
        assert np.all(domain[:nowned]==1) and np.all(domain[nowned:]==0)
        assert np.array_equal(xyz,mesh.P[ids]) and np.all(np.isfinite(values))
        assert np.all(owner[ids[:nowned]]==-1)
        owner[ids[:nowned]]=rank;sensor[ids[:nowned]]=values[:nowned]
        scale=np.max(np.abs(values),axis=(1,2));asym=np.max(abs(values-values.transpose(0,2,1)),axis=(1,2))
        assert np.all(asym<=64*np.finfo(float).eps*scale)
        eigenvalues=np.linalg.eigvalsh(values);assert np.all(eigenvalues>0)
        ranks.append((ids,values));metadata.append(meta)
        packed['rank%d_global'%rank]=ids;packed['rank%d_sensor'%rank]=values
        pins[path.name]=digest(path)
    assert np.all(owner>=0)
    core=metadata[0]['core'];log_scale=metadata[0]['log_scale']
    assert all(m['core']==core and m['log_scale']==log_scale for m in metadata)
    cells=np.sort(mesh.E,axis=1);cell_owner=owner[cells[:,0]]
    halo=[]
    for rank,(ids,values) in enumerate(ranks):
        order=np.argsort(ids);sorted_ids=ids[order]
        rank_cells=cells[cell_owner==rank];local=np.searchsorted(sorted_ids,rank_cells)
        assert np.all(local<len(ids)) and np.array_equal(sorted_ids[local],rank_cells)
        delta=np.max(abs(values-sensor[ids]),axis=(1,2));scale=np.max(abs(sensor[ids]),axis=(1,2))
        assert np.all(delta<=64*np.finfo(float).eps*scale)
        halo.append({'rank':rank,'points':len(ids),'owned':metadata[rank]['owned_points'],
                     'owned_cells':len(rank_cells),'maximum_absolute_owner_difference':float(delta.max()),
                     'maximum_relative_owner_difference':float((delta/scale).max()),
                     'bitwise_equal_to_owners':bool(np.array_equal(values,sensor[ids]))})
    failures=[]
    for cell in (88,157,329):
        p=HERE.parent/'oneram6_frozen_v2'/('captured_cell_%d.json'%cell);old=json.loads(p.read_text());nodes=np.array(old['nodes'])
        host=int(cell_owner[cell]);ids,values=ranks[host];order=np.argsort(ids);actual=values[order[np.searchsorted(ids[order],nodes)]]
        assert np.array_equal(nodes,cells[cell]) and np.array_equal(mesh.P[nodes],old['coordinates'])
        assert np.array_equal(actual,old['sensor']) and old['core']==core and old['log_scale']==log_scale
        failures.append({'cell':cell,'host_rank':host,'raw9_coefficients_match_previous_capture_exactly':True,'previous_capture_sha256':digest(p)})
    packed.update(sensor_owner=sensor,node_owner=owner,cell_owner=cell_owner,
                  core=np.array(core),log_scale=np.array(log_scale),complexity_trial=np.array(1))
    if not (HERE/'frozen_trial.npz').exists():np.savez_compressed(HERE/'frozen_trial.npz',**packed)
    with np.load(HERE/'frozen_trial.npz',allow_pickle=False) as saved:
        assert set(saved.files)==set(packed)
        for key,value in packed.items():assert np.array_equal(saved[key],value),key
    pins.update({'frozen_trial.npz':digest(HERE/'frozen_trial.npz'),'decode_capture.py':digest(Path(__file__)),
                 'capcheck.py':digest(ROOT/'TestCases/adaptation/capability/capcheck.py'),
                 'ownership_source':digest(ROOT/'Common/src/adaptation/CDistributedSearch.cpp'),
                 'halo_metric_storage_source':digest(ROOT/'SU2_CFD/include/variables/CVariable.hpp')})
    result={'status':'PASS_EXACT_COMPLETE_FIRST_TRIAL_FIELD_CAPTURE','points':count,'cells':len(cells),
            'complexity_trial':1,'core':core,'log_scale':log_scale,'ranks':halo,'previous_cells':failures,
            'original_coordinates_bitwise_equal':True,'complete_unique_owned_cover':True,
            'raw_owner_and_halo_9_coefficients_preserved':True,
            'ownership':'CSolver uses OwnedSimplices: owner of the smallest global vertex; sorted4 node IDs. Every cell has all4 nodes present on that rank. Replay must use that rank raw matrix, not substitute owner coefficients for halos.',
            'halo_difference':'Expected roundoff: SetMetricMat transmits upper6; GetMetricMat reconstructs symmetric halo matrices. Raw owned spectral recomposition can be slightly asymmetric. Both representations retained, no tensor repairs.',
            'scope':'Complete runtime first-trial sensor after gradation, before any complexity quadrature/remeshing/final normalization. One initial CFD step from saved120-step state. MPI consistency at one4-rank partition, not scaling, mesh acceptance, transfer, final metric or full-goal qualification.',
            'sha256':pins,'full_goal3_complete':False}
    (HERE/'result.json').write_text(json.dumps(result,indent=2)+'\n')
    print('EXACT_FIRST_TRIAL_CAPTURE_PASS',count,len(cells),'core',core,'scale',log_scale,
          'max_halo_roundoff',max(r['maximum_relative_owner_difference'] for r in halo),
          'packed_bytes',(HERE/'frozen_trial.npz').stat().st_size)

if __name__=='__main__':main()
