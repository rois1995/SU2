"""Reuse the exact numerical reference only after matching every geometric sample and edge."""
import csv,json
from pathlib import Path
from construct import HERE,sha

def records(case):
    data=json.loads((case/'core_accepted.json').read_text());nodes={n[0]:tuple(n[1:]) for n in data['nodes']}
    cells={c['id']:tuple(sorted(nodes[i] for i in c['nodes'])) for c in data['cells']}
    samples={cells[int(r['cell'])]:tuple(float(r[k]) for k in ('q','j','xx','xy','xz','yy','yz','zz')) for r in csv.DictReader((case/'core_samples.csv').open())}
    edges={tuple(sorted((nodes[int(r['a'])],nodes[int(r['b'])]))):float(r['length']) for r in csv.DictReader((case/'core_edges.csv').open())}
    assert len(samples)==len(cells)
    return samples,edges

def main():
    old=HERE/'native_core_rotated_v10';new=HERE/'native_core_rotated_v12'
    receipts=[]
    for case in (old,new):
        receipt=case.with_suffix('.json');r=json.loads(receipt.read_text());assert r['status']=='PASS'
        assert all(sha(case/p)==h for p,h in r['artifacts'].items());receipts.append(r)
    pins=['Common/src/adaptation/CNativeField3D.cpp','Common/src/adaptation/CNativeBoundary3D.cpp','SU2_CFD/src/adaptation/CBoundaryLayerMetric.cpp','SU2_CFD/src/solvers/CSolver.cpp']
    assert all(receipts[0]['contract']['pins'][p]==receipts[1]['contract']['pins'][p] for p in pins)
    assert sha(old/'core_initial.json')==sha(new/'core_initial.json')
    assert sha(old/'core_initial.su2.native_ref')==sha(new/'core_initial.su2.native_ref')
    a,ae=records(old);b,be=records(new);assert a==b and ae==be
    reference=HERE/'full_metric_audit_v3/native_core_rotated_v10.json';r=json.loads(reference.read_text())
    coverage=HERE/'full_metric_audit_v3/coverage.json';c=json.loads(coverage.read_text())
    assert r['status']=='PASS_INDEPENDENT_COMPLETE_CYLINDER_METRIC_SCOPE_LIMITED'
    assert c['status']=='PASS_COMPLETE_EDGE_SET_AND_CONVERGENCE_CONTROL' and c['audit_sha256']==sha(reference)
    geometry=new/'core_independent.json';g=json.loads(geometry.read_text());assert g['status']=='PASS_COMPLETE_STRONGER_FIELD_CURVED_REGION_SCOPE_LIMITED'
    assert g['native_receipt_sha256']==sha(new.with_suffix('.json'))
    result={'status':'PASS_EXACT_GEOMETRIC_SAMPLE_AND_EDGE_REFERENCE_REUSE','cells':len(b),'full_edges':len(be),
            'old_native_receipt_sha256':sha(old.with_suffix('.json')),'new_native_receipt_sha256':sha(new.with_suffix('.json')),
            'new_independent_geometry_sha256':sha(geometry),'full_metric_reference_sha256':sha(reference),'coverage_sha256':sha(coverage),
            'helper_sha256':sha(Path(__file__)),'unchanged_composition_source_pins':{p:receipts[1]['contract']['pins'][p] for p in pins},
            'scope':'New constructor candidate has exactly equal geometric tetrahedra, actual centroid q/J/tensors, full geometric edge sets and native lengths, original frozen sensors and original reference. Reuse the separately executed complete independent numerical metric reference for this exact constant-sensor cylinder; independent new-source geometry/height/source audit also passes. Does not qualify high-ratio, MPI/CFD, general geometry or affordability of the changed constructor.'}
    (new/'full_metric_reference_reuse.json').write_text(json.dumps(result,indent=2)+'\n');print(result['status'])
if __name__=='__main__':main()
