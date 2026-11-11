"""Read-only localization of the existing nodal transport audit; not a continuous-field or acceptance certificate."""
from pathlib import Path
import hashlib,json,re,sys,time
import numpy as np
import assess_target_directions396 as checkpoint
R=Path(__file__).resolve().parent
sys.path.insert(0,str(R.parents[3]/'TestCases/adaptation/capability'))
import capcheck
sha=lambda p:hashlib.sha256(Path(p).read_bytes()).hexdigest()
def ratios(points,matrices,edges):
    eigen,vectors=np.linalg.eigh(matrices);assert (eigen>0).all()
    inverse=vectors/np.sqrt(eigen)[:,None,:];result=np.empty(len(edges))
    for start in range(0,len(edges),8192):
        stop=min(start+8192,len(edges));a,b=edges[start:stop].T;delta=points[b]-points[a];B=matrices[b]
        distance=np.sqrt(np.maximum(0,np.einsum('ni,nij,nj->n',delta,B,delta)))
        transported=B/(1+np.log(1.3)*distance)[:,None,None]**2
        inv=inverse[a];relative=np.einsum('nki,nkl,nlj->nij',inv,transported,inv)
        result[start:stop]=np.linalg.eigvalsh(relative)[:,-1]
    return result,eigen,vectors

def main():
    started=time.monotonic();cpu=time.process_time();receipt_path=R/'target_checkpoint396_producer.json';receipt=json.loads(receipt_path.read_text());directory=Path(receipt['producer'])
    mesh_path=R/'m6_bl_inputs/rans_metric_entry364_v1/m6.su2';mesh=capcheck.read_su2(mesh_path);n=len(mesh.P)
    flow=np.full((n,3,3),np.nan);full=np.full_like(flow,np.nan);seen=set();pins={str(mesh_path):sha(mesh_path),str(receipt_path):sha(receipt_path),str(Path(checkpoint.__file__)):sha(checkpoint.__file__)}
    for rank in range(6):
        path=directory/f'target_rank{rank}.bin';nodes,_=checkpoint.inspect(path,receipt['files'][path.name]['sha256']);pins[str(path)]=sha(path)
        for identity,(point,a,b) in nodes.items():
            assert identity not in seen and np.array_equal(point,mesh.P[identity]);seen.add(identity);flow[identity]=a;full[identity]=b
    assert len(seen)==n and np.isfinite(full).all() and np.isfinite(flow).all()
    undirected=np.concatenate([mesh.E[:,[a,b]] for a in range(4) for b in range(a+1,4)]);undirected.sort(axis=1);undirected=np.unique(undirected,axis=0)
    edges=np.concatenate([undirected,undirected[:,::-1]]);core_ratio,_,_=ratios(mesh.P,flow,edges);combined,eigen,vectors=ratios(mesh.P,full,edges)
    core_bad=core_ratio>1+1e-5;bad=combined>1+1e-5
    log=(directory/'run.log').read_text();match=re.search(r'Native composed nodal audit: maximum transported-metric ratio ([0-9.eE+-]+), directed edges above 1\+1e-5: (\d+)',log);assert match
    expected_count=int(match[2]);expected_max=float(match[1]);assert not core_bad.any()
    wall=np.zeros(n,dtype=bool);wall[np.unique(mesh.M['WING3D'])]=True
    boundary=np.zeros(n,dtype=bool)
    for faces in mesh.M.values():boundary[np.unique(faces)]=True
    a,b=edges.T;delta=mesh.P[b]-mesh.P[a];dominant=vectors[:,:,-1]
    angle=np.degrees(np.arccos(np.clip(np.abs(np.einsum('ni,ni->n',dominant[a],dominant[b])),0,1)))
    normal_dominated=(eigen[:,-1]/eigen[:,-2])>1e4
    categories={}
    for label,mask in [('wall_to_wall',wall[a]&wall[b]),('wall_to_nonwall',wall[a]&~wall[b]),('nonwall_to_wall',~wall[a]&wall[b]),('nonwall_to_nonwall',~wall[a]&~wall[b])]:
        selected=mask&bad;oriented=selected&normal_dominated[a]&normal_dominated[b]
        categories[label]=dict(directed_edges=int(mask.sum()),bad=int(selected.sum()),bad_both_axes_dominant=int(oriented.sum()),dominant_axis_angle_degrees_quantiles=np.quantile(angle[oriented],[0,.25,.5,.75,1]).tolist() if oriented.any() else [],maximum_ratio=float(combined[mask].max()))
    tags={identity:[] for identity in set(edges[np.argsort(combined)[-20:]].flat)}
    for name,faces in mesh.M.items():
        ids=set(np.unique(faces))
        for identity in tags:
            if identity in ids:tags[identity].append(name)
    worst=[]
    for k in np.argsort(combined)[-20:][::-1]:
        first,last=map(int,edges[k]);d=delta[k];normal_part=float((d@dominant[last])**2*eigen[last,-1]);total=float(d@full[last]@d)
        worst.append(dict(a=first,b=last,a_point=mesh.P[first].tolist(),b_point=mesh.P[last].tolist(),a_markers=tags[first],b_markers=tags[last],ratio=float(combined[k]),core_ratio=float(core_ratio[k]),physical_length=float(np.linalg.norm(d)),dominant_axis_angle_degrees=float(angle[k]),a_spectrum=eigen[first].tolist(),b_spectrum=eigen[last].tolist(),b_dominant_axis_fraction_of_chord_metric=normal_part/total))
    report=dict(scope=__doc__,nodes=n,undirected_edges=len(undirected),directed_edges=len(edges),core_bad=int(core_bad.sum()),composed_bad=int(bad.sum()),cpp_reported_bad=expected_count,cpp_count_reproduced=int(bad.sum())==expected_count,maximum_ratio=float(combined.max()),cpp_rounded_maximum_ratio=expected_max,cpp_maximum_reproduced_to_print_precision=bool(abs(combined.max()/expected_max-1)<1e-5),categories=categories,worst20=worst,input_sha256=pins,helper_sha256=sha(__file__),seconds=time.monotonic()-started,cpu_seconds=time.process_time()-cpu,limits=['Same original-point full tensors; independent NumPy eigensolver may differ near threshold from production arithmetic.','Nodal edge transport only, not continuous metric gradation or final mesh acceptance.','Dominant eigenvector is a diagnostic axis, not a certified surface normal;1e4spectral separation only classifies analysis.','Nonwall includes symmetry/farfield boundary nodes as well as interior. Genuine feature/seam status not inferred from coordinates or marker membership.','No new CFD/geometry queries, policy, heights, mesh or operator decisions.'])
    path=R/'composed_gradation399_assessment.json';assert not path.exists();path.write_text(json.dumps(report,indent=2,allow_nan=False)+'\n')
    print(json.dumps({k:report[k] for k in ('core_bad','composed_bad','cpp_reported_bad','cpp_count_reproduced','maximum_ratio','cpp_maximum_reproduced_to_print_precision','categories','seconds')}))
if __name__=='__main__':main()
