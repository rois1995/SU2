#!/usr/bin/env python3
"""Statistical LOWER mass in disjoint original wall-cell subsets; never normalizes a target."""
import csv,json,math,sys,time
from pathlib import Path
import numpy as np
from run import HERE,ROOT,FROZEN,OLD,sha,invoke
from support_bound import wall_geometry,inverse_height_moments
sys.path.insert(0,str(ROOT/'TestCases/adaptation/capability'))
from capcheck import read_su2,faces_of
DTYPE=np.dtype([('cell','<u8'),('data','<f8',(16,))])


def radius_inverse(u,h0,growth,radius):
    b=2*(growth-1)/(growth+1);split=min(radius,h0/b) if b else radius
    total=float(inverse_height_moments(h0,growth,radius)[0]);low=split/h0
    mass=np.asarray(u)*total;result=mass*h0;upper=mass>low
    if np.any(upper):result[upper]=split*np.exp(b*(mass[upper]-low))
    return result


def statistics(values,cap,delta):
    assert len(values)>0 and np.all(np.isfinite(values)) and np.all(values>=0)
    clipped=np.minimum(values,cap);half=cap*math.sqrt(math.log(1/delta)/(2*len(values)))
    return dict(samples=len(values),unclipped_mean=float(values.mean()),clipped_lower_mean=float(clipped.mean()),
        clipped_samples=int(np.count_nonzero(values>cap)),cap=cap,one_sided_delta=delta,
        ideal_IID_Hoeffding_halfwidth=half,statistical_lower=max(0.,float(clipped.mean())-half),
        empirical_standard_error=float(clipped.std(ddof=1)/np.sqrt(len(values))))


def selfcheck():
    for growth in (1.,1.+1e-12,1.2,2.):
        h0=2e-6;radius=.009;u=np.array([0.,.1,.5,.9,1.]);r=radius_inverse(u,h0,growth,radius)
        b=2*(growth-1)/(growth+1);split=min(radius,h0/b) if b else radius
        integral=np.minimum(r,split)/h0;high=r>split
        if np.any(high):integral[high]+=np.log(r[high]/split)/b
        assert np.allclose(integral/float(inverse_height_moments(h0,growth,radius)[0]),u,rtol=2e-14,atol=2e-14)
        assert np.all(np.diff(r)>0) and abs(r[-1]/radius-1)<2e-14
    value=statistics(np.array([0.,1.,2.,100.]),3.,.01)
    assert value['clipped_lower_mean']==1.5 and value['clipped_samples']==1
    owner=np.array([5,5,8]);proxy=np.array([2.,4.,3.]);order=np.argsort(-proxy,kind='stable')
    _,first=np.unique(owner[order],return_index=True);assert sorted(order[first])==[1,2]


def prepare(label,seed,count):
    assert 0<count<=262144;folder=HERE/label
    if folder.exists():raise RuntimeError('Use a fresh label; preserve existing evidence: '+str(folder))
    folder.mkdir();start=time.monotonic();selfcheck()
    capture=json.loads((FROZEN/'result.json').read_text());assert sha(FROZEN/'frozen_trial.npz')==capture['sha256']['frozen_trial.npz']
    fixture=OLD/'cell329/replay.dat';g=wall_geometry(fixture);mesh=read_su2(FROZEN/'Coarse.su2')
    weights=HERE/'geometric_correction_sampler_v1/primitive_weights.npz';preparation=json.loads((weights.parent/'preparation.json').read_text())
    assert sha(weights)==preparation['weights_sha256']
    with np.load(weights,allow_pickle=False) as a:coefficient=a['face_density'].copy()
    assert len(coefficient)==len(g['f'])
    lookup={tuple(point):i for i,point in enumerate(mesh.P)};global_ids=np.array([lookup[tuple(point)] for point in g['p']])
    boundary=np.sort(global_ids[g['f']],axis=1);wanted={tuple(face):i for i,face in enumerate(boundary)}
    assert len(wanted)==len(boundary) and set(wanted)=={tuple(sorted(face)) for face in mesh.M['WING3D']}
    all_faces,parents=faces_of(mesh.E,3);on_wall=np.zeros(len(mesh.P),dtype=bool);on_wall[global_ids]=True
    mask=np.all(on_wall[all_faces],axis=1);owners=np.full(len(boundary),-1,dtype=np.int64)
    for face,parent in zip(all_faces[mask],parents[mask]):
        index=wanted.get(tuple(face))
        if index is not None:
            assert owners[index]<0,'Duplicate original wall face parent.';owners[index]=int(parent)
    assert np.all(owners>=0)
    radius=max(g['h0'],.9*g['thickness']);moment=float(inverse_height_moments(g['h0'],g['growth'],radius)[0])
    mass=2*np.asarray(g['area'],float)*moment;proxy=mass*coefficient
    order=np.argsort(-proxy,kind='stable');_,first=np.unique(owners[order],return_index=True);selected=order[first];selected.sort()
    assert len(np.unique(owners[selected]))==len(selected)
    cap=float(np.sum(proxy[selected],dtype=np.longdouble));cdf=np.cumsum(proxy[selected]/cap,dtype=float);cdf[-1]=1.
    effective=np.diff(np.r_[0.,cdf]);assert np.all(effective>0)
    draws=np.random.default_rng(seed).random((count,5));slot=np.searchsorted(cdf,draws[:,0],side='right');face=selected[slot]
    sqrt=np.sqrt(draws[:,1]);bary=np.column_stack([1-sqrt,sqrt*(1-draws[:,2]),sqrt*draws[:,2]])
    foot=np.sum(g['x'][face]*bary[:,:,None],axis=1,dtype=np.longdouble)
    distance=radius_inverse(draws[:,3],g['h0'],g['growth'],radius);sign=np.where(draws[:,4]<.5,-1.,1.)
    point=np.asarray(foot+sign[:,None]*distance[:,None]*g['normal'][face],float)
    b=2*(g['growth']-1)/(g['growth']+1);profile=1/np.maximum(g['h0'],b*distance)
    inverse=mass[face]/(effective[slot]*profile);assert np.all(np.isfinite(inverse)) and np.all(inverse>0)
    rows=np.empty(count,dtype=DTYPE);rows['cell']=owners[face];rows['data'][:,:12]=mesh.P[mesh.E[owners[face]]].reshape(count,12)
    rows['data'][:,12:15]=point;rows['data'][:,15]=inverse;packet=folder/'queries.bin'
    with packet.open('xb') as stream:np.array([count],dtype='<u8').tofile(stream);rows.tofile(stream)
    np.savez_compressed(folder/'sample_map.npz',face=face,parent=owners[face],radius=distance,sign=sign,
        selected_faces=selected,selected_parents=owners[selected],effective_probability=effective)
    paths=[Path(__file__).resolve(),HERE/'support_bound.py',HERE/'proposal_budget.py',weights,weights.parent/'preparation.json',fixture,
        FROZEN/'Coarse.su2',FROZEN/'frozen_trial.npz',FROZEN/'result.json',ROOT/'TestCases/adaptation/capability/capcheck.py']
    proof=dict(scope='Disjoint original wall-cell prism subsets; full-strength bare geometric BL only, not the complete composed integral.',
        count=count,seed=seed,radius=radius,cap=cap,original_wall_faces=len(boundary),selected_faces=len(selected),
        original_full_face_proxy=float(np.sum(proxy,dtype=np.longdouble)),delta=.005,requested_complexity=100000,
        preparation_seconds=time.monotonic()-start,original_domain_embedding_is_precondition=True,machine_bound_proven=False,
        statistical_assumptions='Ideal independent continuous samples, exact prism Jacobian/PDF, metric and domain membership. Floating coordinates/PDF are numerical, not outward certificates.',
        input_sha256=sha(packet),map_sha256=sha(folder/'sample_map.npz'),sha256={str(p.relative_to(ROOT)):sha(p) for p in paths})
    (folder/'preparation.json').write_text(json.dumps(proof,indent=2)+'\n');print(json.dumps(proof),flush=True)


def run(label,ranks):
    assert ranks in (1,2,4);folder=HERE/label;proof=json.loads((folder/'preparation.json').read_text());packet=folder/'queries.bin'
    assert sha(packet)==proof['input_sha256']
    for name,digest in proof['sha256'].items():assert sha(ROOT/name)==digest,name
    dest=folder/('np%d'%ranks)
    invoke(dest,{'SU2_NATIVE_BL_WALL_PRISMS':str(packet)},OLD/'cell329/replay.dat',ranks)
    rows=[]
    for rank in range(ranks):rows.extend(csv.DictReader((dest/('wall_prism_rank%d.csv'%rank)).open()))
    rows.sort(key=lambda row:int(row['row']));assert [int(row['row']) for row in rows]==list(range(proof['count']))
    result=statistics(np.array([float(row['density'])*float(row['inverse_pdf']) for row in rows]),proof['cap'],proof['delta'])
    result.update(inside_samples=sum(int(row['inside']) for row in rows),strong_samples=sum(int(row['strong']) for row in rows),
        maximum_log_spectral_relative_difference=max(float(row['log_spectral_relative_difference']) for row in rows),
        wall_cell_subset_only=True,complete_field_integrated=False,normalization_changed=False,machine_bound_proven=False,
        statistical_assumptions=proof['statistical_assumptions'],CFD_steps=0,target=proof['requested_complexity'],
        lower_exceeds_target=result['statistical_lower']>proof['requested_complexity'],
        sha256={p.name:sha(p) for p in dest.glob('wall_prism_rank*.csv')})
    if ranks>1 and (folder/'np1/statistics.json').exists():
        saved=list(csv.DictReader((folder/'np1/wall_prism_rank0.csv').open()));assert rows==saved,'MPI queries differ from serial.'
        result['MPI_matches_all_serial_rows']=True
    (dest/'statistics.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result),flush=True)


if __name__=='__main__':
    if sys.argv[1]=='prepare':prepare(sys.argv[2],int(sys.argv[3]),int(sys.argv[4]))
    elif sys.argv[1]=='run':run(sys.argv[2],int(sys.argv[3]))
    else:raise ValueError('prepare LABEL SEED COUNT | run LABEL RANKS')
