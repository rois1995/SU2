#!/usr/bin/env python3
"""Audit an accepted difficult-target MPI publication with the existing exact geometry checker."""
import json,sys,time
from pathlib import Path
from construct import HERE,ROOT,sha,authorities
from audit_runtime_mesh import mesh

def main():
    case=HERE/sys.argv[1];receipt_path=case.with_suffix('.json');receipt=json.loads(receipt_path.read_text())
    assert receipt['status']=='PASS' and receipt['returncode']==0
    assert all(sha(case/p)==h for p,h in receipt['artifacts'].items()),'Saved native evidence changed'
    helpers=[Path(__file__).resolve(),HERE/'audit_runtime_mesh.py',HERE/'construct.py',
             ROOT/'integration_evidence/native_3d_core_v1/audit_cavities.py',
             ROOT/'integration_evidence/native_3d_core_v1/audit_coupled.py',
             HERE.parent/'alignment_audit_v1/baseline.py']
    contract={'native_receipt_sha256':sha(receipt_path),'helpers':{str(p.relative_to(ROOT)):sha(p) for p in helpers},
              'accepted_mesh_sha256':sha(case/'protocol_adapted.json')}
    output=case/'independent_geometry.json'
    if output.exists():
        old=json.loads(output.read_text())
        if old.get('status')=='PASS_EXACT_PUBLISHED_REGION_GEOMETRY_SCOPE_LIMITED' and old.get('contract')==contract:
            print('Reused',old['status'],flush=True);return
    started=time.monotonic();data=json.loads((case/'protocol_adapted.json').read_text());initial=json.loads((case/'audited_grid.json').read_text())
    assert len(initial['cells'])==1402
    assert all(data[k]==initial[k] for k in ['original','facets','facet_groups','marker_names'])
    native=json.loads((case/'region_summary.json').read_text())
    assert native['valid'] and native['cells']==len(data['cells']) and native['work']<=4096
    assert native['q']>=.2 and native['j']>=.05 and native['maximum_full_length']<=1.8
    reference=case/'curved_cylinder_initial_euler.su2.native_ref';geometry,_=authorities(reference)
    report,_=mesh(data,geometry,True)
    assert all(sha(ROOT/p)==h for p,h in contract['helpers'].items()),'Audit helpers changed during execution'
    report.update(status='PASS_EXACT_PUBLISHED_REGION_GEOMETRY_SCOPE_LIMITED',contract=contract,
                  native_metric_summary=native,unchanged_frozen_donors_and_reference=True,elapsed_seconds=time.monotonic()-started,
                  scope='Whole native MPI-published difficult target: exact positive volumes, oriented connected manifold skin/every vertex link, no interior overlap, exact wall feet/height, analytic nodes/chords; frozen donors/reference identical. q/J and full composed edges were strictly remeasured natively, not independently re-integrated here. No actual CFD/transfer/restart/accuracy/performance qualification.')
    output.write_text(json.dumps(report,indent=2)+'\n');print(report['status'],flush=True)
if __name__=='__main__':main()
