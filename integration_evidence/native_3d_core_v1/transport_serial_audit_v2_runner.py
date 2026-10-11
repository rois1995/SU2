from pathlib import Path
import hashlib,json,re,sys
root=Path('/media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated');where=root/'integration_evidence/native_3d_core_v1'
folder=where/'transport_serial_audit_v2';folder.mkdir(exist_ok=False)
sha=lambda p:hashlib.sha256(Path(p).read_bytes()).hexdigest()
sys.path.insert(0,str(where))
from audit_lifecycle import parse,transport_counters
original=where/'transport_serial_v1';receipt_path=original/'validation.json';v=json.loads(receipt_path.read_text())
assert v['status']=='FAIL' and v['reason']=="No module named 'audit_lifecycle'"
assert not any(v[k] for k in ('source_drift','object_drift','archive_drift','tool_drift'))
for name,h in v['artifact_sha256'].items():assert sha(original/name)==h,name
for s in v['stages']:assert s['exit_code']==0 and not s.get('timeout'),s
buildpath=where/'transport_build_v1/validation.json';b=json.loads(buildpath.read_text())
assert b['status']=='PASS' and sha(buildpath)==v['build_validation_sha256']
for name,h in b['source_sha256'].items():assert sha(root/name)==h,name
for name,h in b['actual_objects_sha256'].items():assert sha(root/'integration_evidence/build-integrated-v2'/name)==h
for name,h in {**b['library_sha256'],**b['thin_archive_member_sha256']}.items():assert sha(name)==h
assert sha(root/'integration_evidence/build-integrated-v2/UnitTests/test_driver')==v['binary_sha256']==b['binary_sha256']
native=original/'native_serial';out=(native/'native_serial.stdout').read_text()
assert re.search(r'All tests passed \(10189 assertions in 69 test cases\)',out)
assert 'All tests passed (48 assertions in 2 test cases)' in (original/'mmg_serial/mmg_serial.stdout').read_text()
a=json.loads((native/'independent_audit.json').read_text());assert a['status']=='PASS'
assert len(a['frames'])==12 and len(a['states'])==6 and a['actual_output_roundtrips']==6
rows=[parse(line) for line in out.splitlines() if line.startswith('NATIVE3D ')]
assert len(rows)==2 and all(transport_counters(r) for r in rows)
assert [int(r['complete']) for r in rows]==[0,1]
baseline=where/'reference_admission_serial_v1/native_serial'
identity={p.name:dict(baseline_sha256=sha(p),current_sha256=sha(native/p.name)) for p in baseline.iterdir() if p.is_file()}
changed=[name for name,h in identity.items() if h['baseline_sha256']!=h['current_sha256']]
assert all(name.endswith('.cgns') for name in changed),changed
record=dict(status='SERIAL_AUDIT_PASS',original_run_sha256=sha(receipt_path),build_validation_sha256=sha(buildpath),binary_sha256=v['binary_sha256'],
 original_wrapper_status=v['status'],original_wrapper_failure=v['reason'],native_cases=69,native_assertions=10189,mmg_cases=2,mmg_assertions=48,
 independent_audit_sha256=sha(native/'independent_audit.json'),native_rows=rows,raw_identity=identity,changed_raw_files=changed,
 audit_source_sha256=sha(where/'audit_lifecycle.py'),
 scope='Postprocessing-only recovery of a missing collector import path. Actual original native/MMG/independent bridge processes all exited0; original FAIL receipt retained unchanged. Reverified pinned logs/artifacts/full68-object binary/sources/774 archive members, new accepted/rejected transport closure and byte identity of non-CGNS fixtures versus prior serial source. CGNS checked independently. No numerical rerun, MPI2/4, n4/restart campaign, timing/scaling or fullGoal1 claim.')
(folder/'validation.json').write_text(json.dumps(record,indent=2)+'\n')
print(record['status'],'non-CGNS artifacts identical:',len(identity)-sum(n.endswith('.cgns') for n in identity),'changed CGNS containers:',len(changed))
for r in rows:print({k:r[k] for k in ('complete','tracked_bytes','tracked_calls','tracked_import_bytes','tracked_parent_bytes','tracked_worker_bytes')})
