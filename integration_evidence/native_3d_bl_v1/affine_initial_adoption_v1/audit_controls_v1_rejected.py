#!/usr/bin/env python3
"""Independent exact geometry/source/reference audit; no remeshing/viscous claim."""
import collections,hashlib,itertools,json,shlex,sys
from fractions import Fraction as F
from pathlib import Path
import numpy as np
ROOT=Path(__file__).resolve().parents[3];HERE=Path(__file__).resolve().parent
sys.path.insert(0,str(ROOT/'integration_evidence/native_3d_core_v1'));import audit_coupled as base
sys.path.insert(0,str(ROOT/'TestCases/adaptation/capability'));from capcheck import read_su2
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
def reference(path):
    lines=path.read_text().splitlines();assert lines[0]=='SU2_NATIVE_REFERENCE_3D 4'
    nm,nf,na,np=map(int,lines[1].split());names=[shlex.split(l)[0] for l in lines[2:2+nm]];at=2+nm;facets={};planes={}
    for line in lines[at:at+nf]:
        row=line.split();assert len(row)==14;id,marker=map(int,row[:2]);assert id not in facets and 0<=marker<nm
        facets[id]=dict(id=id,marker=marker,vertices=[[int(row[2+4*k])]+list(map(float,row[3+4*k:6+4*k])) for k in range(3)])
    at+=nf+na
    for line in lines[at:at+np]:
        row=line.split();assert len(row)==11;id,identity,version,dimension=map(int,row[:4]);assert id in facets and id not in planes
        planes[id]=dict(identity=identity,version=version,dimension=dimension,origin=list(map(float,row[4:7])),normal=list(map(float,row[7:10])),error=float(row[10]))
    assert at+np==len(lines);return names,facets,planes,na

def audit(folder,name,components):
    data_path=folder/(name+'.json');ref_path=folder/(name+'.native_ref');initial=folder/(name+'.su2')
    data=json.loads(data_path.read_text());names,facets,planes,analytic=reference(ref_path)
    assert analytic==0 and names==data['marker_names'] and facets=={f['id']:f for f in data['facets']}
    assert len(planes)==2*components and {g['identity'] for g in planes.values()}==set(range(17,17+components))
    nodes={n[0]:tuple(map(float,n[1:])) for n in data['nodes']};span=max(nodes)+1
    assert span==len(nodes)==8*components and len(data['cells'])==6*components
    active=collections.defaultdict(list)
    for id,g in planes.items():
        assert g['version']==1 and g['dimension']==3 and g['error']==1e-13 and names[facets[id]['marker']]=='symmetry'
        n=list(map(F,g['normal']));o=list(map(F,g['origin']));limit=F(g['error'])
        for v in facets[id]['vertices']:
            q=list(map(F,v[1:]));residual=sum(n[k]*(q[k]-o[k]) for k in range(3))
            assert residual**2<=limit**2*sum(x*x for x in n);active[v[0]].append(g)
    original=read_su2(initial);matched={};used=set();maximum_shift=F(0);ratios=[]
    for id,p in nodes.items():
        distances=[sum((F(float(x))-F(float(y)))**2 for x,y in zip(p,q)) for q in original.P]
        index=min(range(len(distances)),key=distances.__getitem__);assert index not in used and distances[index]<F(1e-9)**2
        used.add(index);matched[id]=index;shift=sum(abs(F(float(x))-F(float(y))) for x,y in zip(p,original.P[index]));maximum_shift=max(maximum_shift,shift)
        if id not in active:assert shift==0,'Non-symmetry initial node moved'
        else:
            assert distances[index]<=(F(1e-10)+2*F(1e-13))**2
            for g in active[id]:
                n=list(map(F,g['normal']));o=list(map(F,g['origin']))
                residual=sum(n[k]*(F(float(original.P[index,k]))-o[k]) for k in range(3))
                assert residual**2<=F(1e-10)**2*sum(x*x for x in n)
    expected={tuple(sorted(map(int,c))) for c in original.E};seen=set()
    for c in data['cells']:
        t=[tuple(map(F,nodes[n])) for n in c['nodes']];value=base.determinant(t);assert value>0
        source=[matched[n] for n in c['nodes']];key=tuple(sorted(source));assert key in expected and key not in seen;seen.add(key)
        before=base.determinant([tuple(F(float(x)) for x in original.P[n]) for n in source]);assert before
        ratios.append(value/abs(before))
    assert seen==expected
    groups=collections.Counter();signature=[]
    for c in data['original']:
        vertices=c['vertices'];assert base.determinant([tuple(map(F,v[1:4])) for v in vertices])>0
        level=max(v[0]//span for v in vertices);groups[level]+=1
        for v in vertices:
            id=v[0]%span;assert id in nodes;p=nodes[id]
            expected_sensor=[float(F(2)+F(p[0])/8),0.,0.,float(F(3)+F(p[1])/8),0.,float(F(4)+F(p[2])/8)]
            assert v[4]==expected_sensor,'Original sensor trace changed'
            if v[0]>=span:
                assert v[1]==p[0] and v[3]==p[2] and v[2]<p[1]
                assert 2e-13<float(F(p[1])-F(v[2]))<=4e-13+2*float(F(float(np.nextafter(p[1],float('inf'))))-F(p[1]))
        signature.append(tuple(sorted((tuple(v[1:4]),tuple(v[4])) for v in vertices)))
    assert groups[0]==6*components and sum(groups.values())==12*components and sum(n for k,n in groups.items() if k)>0
    return dict(status='EXACT_INITIAL_MAPPING_ORIGINAL_SOURCE_V4_PASS',cells=len(data['cells']),points=len(nodes),components=components,
        mapping_error_contract=1e-13,maximum_L1_initial_displacement=float(maximum_shift),minimum_exact_volume_ratio=float(min(ratios)),source_levels=dict(groups),
        source_signature_sha256=hashlib.sha256(repr(sorted(signature)).encode()).hexdigest(),
        hashes={str(p.relative_to(ROOT)):sha(p) for p in (data_path,ref_path,initial)})

def narrow_gap(folder):
    before_path=folder/'affine_narrow_gap.su2';after_path=folder/'affine_narrow_gap_rejected.su2'
    before=read_su2(before_path);after=read_su2(after_path);assert (before.E==after.E).all() and before.M.keys()==after.M.keys()
    def cells(mesh):return [[tuple(F(float(x)) for x in mesh.P[n]) for n in row] for row in mesh.E]
    old,new=cells(before),cells(after);assert all(base.determinant(t)>0 for t in old+new)
    old_overlap=[];new_overlap=[]
    for i,j in itertools.combinations(range(len(old)),2):
        if base.overlap(old[i],old[j]):old_overlap.append((i,j))
        if base.overlap(new[i],new[j]):new_overlap.append((i,j))
    assert not old_overlap and new_overlap,'Counterexample must be globally invalid despite positive local volumes'
    receipt=json.loads((folder/'affine_narrow_gap_rejection.json').read_text());assert not receipt['accepted'] and receipt['boundary_pair_checks']>0
    return dict(status='POSITIVE_CELL_BOUNDARY_COLLISION_COUNTEREXAMPLE_CONFIRMED',source_cells=len(old),collision_pairs=new_overlap,
        all_source_and_proposed_cells_positive=True,source_has_no_positive_volume_overlap=True,native_rejection=receipt,
        hashes={str(p.relative_to(ROOT)):sha(p) for p in (before_path,after_path,folder/'affine_narrow_gap_rejection.json')})

def main():
    folder=HERE/(sys.argv[1] if len(sys.argv)>1 else 'controls_v1_n1');result=json.loads((folder/'result.json').read_text());assert result['exit_code']==0
    assert sha(folder/'provenance.json')==result['provenance_sha256'] and sha(folder/'run.log')==result['log_sha256']
    for p,digest in result['artifacts'].items():assert sha(folder/p)==digest
    reports={}
    for components in (1,2):
        for solver in ('euler','rans'):reports[f'{solver}_c{components}']=audit(folder,f'affine_initial_{solver}_c{components}',components)
        assert reports[f'euler_c{components}']['source_signature_sha256']==reports[f'rans_c{components}']['source_signature_sha256']
    output=dict(status='INDEPENDENT_AFFINE_INITIAL_CONTROLS_PASS',cases=reports,narrow_gap=narrow_gap(folder),auditor_sha256=sha(Path(__file__)),
        helpers={str(p.relative_to(ROOT)):sha(p) for p in (Path(base.__file__),ROOT/'TestCases/adaptation/capability/capcheck.py')},
        limits=['Initial geometry/original-source/codec and negative collision audit; no adapted mesh or physical viscous accuracy',
            'Native test assertions cover coordinate/state atomicity; this script has no independent solver-state snapshot',
            'One/two disconnected finite plane components, no general analytic intersection/multiple-plane collar or2D/exterior readiness'])
    (folder/'independent_controls.json').write_text(json.dumps(output,indent=2)+'\n');print(output['status'],flush=True)
if __name__=='__main__':main()
