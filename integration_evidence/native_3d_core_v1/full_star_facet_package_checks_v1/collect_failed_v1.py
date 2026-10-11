"""Audit new full-star facet regression counters and current reusable package checks."""
from pathlib import Path
import hashlib,json,sys
folder=Path(__file__).resolve().parent;where=folder.parent;root=where.parents[1]
sys.path.insert(0,str(where))
from audit_lifecycle import parse,coarsening_counters
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
assert 'Ran 31 tests' in (folder/'package_checks.stderr').read_text()
assert (folder/'package_checks.stderr').read_text().rstrip().endswith('OK')
records=[]
for name in ('full_star_facet_mpi1_v1','full_star_facet_mpi2_v1','full_star_facet_mpi4_v1','full_star_facet_n8_n1_m1_v1',
             'full_star_facet_lifecycle_mpi1_v1','full_star_facet_n4_n4_m2_v1',
             'full_star_facet_restart_su2_n2_n1_m1_v1','full_star_facet_restart_cgns_n2_n4_m2_v1'):
    path=where/name/'case.stdout';rows=[]
    for line in path.read_text().splitlines():
        if line.startswith('ENGINE '):
            row={k:v if k=='case' else float(v) for k,v in (item.split('=',1) for item in line.split()[1:])}
            row.update(publication=row['publication_s'],selection=row['selection_s'],
                       seed=row['seed_s'],update=row['update_s'],nested_collapse_geometry=row['collapse_geometry_nested_s'])
            rows.append(row)
        elif line.startswith('NATIVE3D_DETAIL ') and any('stage='+s+' ' in line for s in ('refine','coarsen')):
            rows.append(parse(line))
    assert rows and all(coarsening_counters(row) for row in rows),name
    for row in rows:
        for key in ('collapse_geometry_checks','collapse_filtered'):
            assert row[key]>=0 and int(row[key])==row[key],(name,key)
        assert row['collapse_filtered']<=row['collapse_geometry_checks'],name
        # Cached facet checks run during seed/update; the new full-star check runs during selection.
        parent=sum(row[k] for k in ('seed','selection','update'))
        assert 0<=row['nested_collapse_geometry']<=parent+1e-10*max(1,parent),name
    sent=sum(row['changed_vertices_sent'] for row in rows);received=sum(row['changed_vertices_received'] for row in rows)
    assert sent==received,(name,sent,received)
    records.append(dict(folder=name,log_sha256=sha(path),engine_rows=len(rows),change_sent=sent,change_received=received))
result=dict(status='PASS',lightweight_checks=31,accounting_audits=records,
            tools_sha256={str(p.relative_to(root)):sha(p) for p in (Path(__file__).resolve(),where/'audit_lifecycle.py',
                          root/'integration_evidence/native_cluster_3d_core_v1/check_package.py',
                          root/'integration_evidence/native_cluster_3d_core_v1/run.py')},
            scope='Actual saved 31 parser/mocked orchestration tests pass. Corrected collector audits actual ENGINE and detailed runtime rows for finite integral counters, nested parent costs and global notification record closure. Each case retains its own numerical verdict; this checks accounting, not n8 completion or performance/scaling.')
result['artifact_sha256']={p.name:sha(p) for p in folder.iterdir() if p.is_file() and p.name!='validation.json'}
(folder/'validation.json').write_text(json.dumps(result,indent=2)+'\n')
print('PASS31 checks and accounting',records)
