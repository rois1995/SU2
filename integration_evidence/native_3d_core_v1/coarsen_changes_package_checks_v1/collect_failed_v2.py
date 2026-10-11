"""Audit actual emitted rows after the saved lightweight package self-check; preserve the first collector failure."""
from pathlib import Path
import hashlib,json,sys
folder=Path(__file__).resolve().parent;where=folder.parent;root=where.parents[1]
sys.path.insert(0,str(where))
from audit_lifecycle import parse,coarsening_counters
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
assert 'Ran 31 tests' in (folder/'package_checks.stderr').read_text()
assert (folder/'package_checks.stderr').read_text().rstrip().endswith('OK')
assert json.loads((folder/'collector_failure.json').read_text())['status']=='FAIL'
records=[]
for name in ('coarsen_changes_mpi1_v1','coarsen_changes_mpi2_v1','coarsen_changes_mpi4_v1',
             'coarsen_changes_lifecycle_mpi1_v1','coarsen_changes_n8_n1_m1_v1'):
    path=where/name/'case.stdout';rows=[]
    for line in path.read_text().splitlines():
        if line.startswith('ENGINE '):
            row={k:v if k=='case' else float(v) for k,v in (item.split('=',1) for item in line.split()[1:])}
            row.update(publication=row['publication_s'],selection=row['selection_s'])
            rows.append(row)
        elif line.startswith('NATIVE3D_DETAIL ') and any('stage='+s+' ' in line for s in ('refine','coarsen')):
            rows.append(parse(line))
    assert rows and all(coarsening_counters(row) for row in rows),name
    sent=sum(row['changed_vertices_sent'] for row in rows);received=sum(row['changed_vertices_received'] for row in rows)
    assert sent==received,(name,sent,received)
    records.append(dict(folder=name,log_sha256=sha(path),engine_rows=len(rows),change_sent=sent,change_received=received))
result=dict(status='PASS',lightweight_checks=31,accounting_audits=records,
            tools_sha256={str(p.relative_to(root)):sha(p) for p in (Path(__file__),where/'audit_lifecycle.py',
                          root/'integration_evidence/native_cluster_3d_core_v1/check_package.py',
                          root/'integration_evidence/native_cluster_3d_core_v1/run.py')},
            scope='Actual saved 31 parser/mocked orchestration tests pass. Corrected collector audits actual ENGINE and detailed runtime rows for finite integral counters, nested parent costs and global notification record closure. The original collector-only failure is preserved; no numerical rerun, n8 campaign PASS or performance/scaling claim.')
result['artifact_sha256']={p.name:sha(p) for p in folder.iterdir() if p.is_file() and p.name!='validation.json'}
(folder/'validation.json').write_text(json.dumps(result,indent=2)+'\n')
print('PASS31 checks and accounting',records)
