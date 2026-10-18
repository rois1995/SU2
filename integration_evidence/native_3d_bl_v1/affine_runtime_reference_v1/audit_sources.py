#!/usr/bin/env python3
"""Independent exact source identity/positivity/codec audit; no adaptation claim."""
import hashlib,json,shlex,sys
from collections import Counter
from fractions import Fraction as F
from pathlib import Path
HERE=Path(__file__).resolve().parent;ROOT=HERE.parents[2]
HOUSE=ROOT/'integration_evidence/native_3d_core_v1/audit_cavities.py'
sys.path.insert(0,str(HOUSE.parent));from audit_cavities import determinant

def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def audit(folder,label):
    path=folder/('affine_import_'+label+'.json');data=json.loads(path.read_text())
    nodes={n[0]:n[1:] for n in data['nodes']};span=max(nodes)+1;assert span==27 and len(nodes)==27 and len(data['cells'])==48
    groups=Counter();signature=[]
    for cell in data['original']:
        vertices=cell['vertices'];points=[tuple(F(x) for x in v[1:4]) for v in vertices]
        assert determinant(points)>0,'Nonpositive physical/generated source'
        ids=[v[0]//span for v in vertices];group=max(ids);assert group in (0,1,2);groups[group]+=1
        for v in vertices:
            original=v[0]%span;assert original in nodes;old=nodes[original]
            expected=[F(2)+F(old[0])/8,0,0,F(3)+F(old[2])/8,0,4]
            assert list(map(F,v[4]))==expected,'Original family modulo or sensor trace changed'
            if v[0]>=span:
                axis=2 if group==1 else 1
                assert all(v[k+1]==old[k] for k in range(3) if k!=axis)
                assert v[axis+1]!=old[axis]
        signature.append(tuple(sorted((tuple(v[1:4]),tuple(v[4])) for v in vertices)))
    assert groups=={0:48,1:3,2:24};assert len(signature)==75
    reference=folder/('affine_import_'+label+'.native_ref');lines=reference.read_text().splitlines();assert lines[0]=='SU2_NATIVE_REFERENCE_3D 4'
    markers,facets,analytic,affine=map(int,lines[1].split());assert (markers,facets,analytic,affine)==(7,48,1,8)
    names=[shlex.split(line)[0] for line in lines[2:2+markers]];assert names==data['marker_names']
    saved={f['id']:f for f in data['facets']};at=2+markers
    for line in lines[at:at+facets]:
        row=line.split();id,marker=map(int,row[:2]);f=saved[id];assert marker==f['marker']
        assert [[int(row[2+4*i])]+list(map(float,row[3+4*i:6+4*i])) for i in range(3)]==f['vertices']
    at+=facets+analytic
    for line in lines[at:at+affine]:
        row=line.split();id,identity,version,dimension=map(int,row[:4]);o=list(map(F,map(float,row[4:7])));n=list(map(F,map(float,row[7:10])));bound=F(float(row[10]))
        assert (identity,version,dimension)==(17,3,3) and names[saved[id]['marker']]=='y_minus'
        for v in saved[id]['vertices']:
            residual=sum(n[k]*(F(v[k+1])-o[k]) for k in range(3))
            assert residual*residual<=bound*bound*sum(x*x for x in n)
    assert at+affine==len(lines)
    return dict(status='EXACT_ORIGINAL_FAMILY_SOURCE_AND_CODEC_PASS',physical_sources=48,analytic_sources=3,affine_sources=24,original_span=span,
        geometry_data_sha256=sha(path),reference_sha256=sha(reference),source_signature_sha256=hashlib.sha256(repr(sorted(signature)).encode()).hexdigest(),
        scope='Exact positive source determinants, original-node modulo/sensor trace, independent v4 fields and original affine vertex bounds; no accepted oblique mesh or transfer/restart claim')

def main():
    reports={}
    for folder in sys.argv[1:]:
        path=HERE/folder;reports[folder]={label:audit(path,label) for label in ('euler','rans')}
    for label in ('euler','rans'):
        assert len({r[label]['source_signature_sha256'] for r in reports.values()})==1,'Serial/MPI field/geometry differs'
    out=dict(status='INDEPENDENT_SERIAL_MPI_SOURCE_CONSISTENCY_PASS',cases=reports,audit_source_sha256=sha(Path(__file__)),house_predicate_sha256=sha(HOUSE))
    (HERE/'independent_sources.json').write_text(json.dumps(out,indent=2)+'\n');print(out['status'])
if __name__=='__main__':main()
