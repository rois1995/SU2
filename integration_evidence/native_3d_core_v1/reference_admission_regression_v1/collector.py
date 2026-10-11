from pathlib import Path
import hashlib,json,re,shutil
root=Path(__file__).resolve().parents[3];where=Path(__file__).resolve().parent.parent;folder=Path(__file__).resolve().parent
sha=lambda p:hashlib.sha256(Path(p).read_bytes()).hexdigest()
build=root/'integration_evidence/build-integrated-v2';buildpath=where/'transport_build_v1/validation.json';b=json.loads(buildpath.read_text());assert b['status']=='PASS'
serial=where/'transport_serial_v1';original=json.loads((serial/'validation.json').read_text());recoverypath=where/'transport_serial_audit_v2/validation.json';recovery=json.loads(recoverypath.read_text())
assert original['status']=='FAIL' and original['reason']=="No module named 'audit_lifecycle'"
assert recovery['status']=='SERIAL_AUDIT_PASS' and recovery['original_run_sha256']==sha(serial/'validation.json') and recovery['build_validation_sha256']==sha(buildpath)
provenance=[];stages=[]
for ranks in (1,2,4):
 source=serial if ranks==1 else where/f'transport_mpi{ranks}_correctness_v2';v=json.loads((source/'validation.json').read_text())
 assert not any(v[k] for k in ('source_drift','object_drift','archive_drift','tool_drift'))
 assert v['build_validation_sha256']==sha(buildpath) and v['binary_sha256']==b['binary_sha256']
 if ranks!=1:assert v['status']=='PASS'
 assert all(s['exit_code']==0 and not s.get('timeout') for s in v['stages'])
 assert all(sha(source/n)==h for n,h in v['artifact_sha256'].items())
 actual=source/'native_serial' if ranks==1 else source;log=actual/('native_serial.stdout' if ranks==1 else 'case.stdout')
 assert len(re.findall(r'All tests passed \(\d+ assertions in 69 test cases\)',log.read_text()))==ranks
 audit=json.loads((actual/'independent_audit.json').read_text());assert audit['status']=='PASS' and audit['adversaries_rejected']==5
 for label in ('native_serial','independent_bridge') if ranks==1 else ('case','independent_bridge'):
  s=next(s for s in v['stages'] if s['label']==label);stages.append(dict(s,stage=f'mpi{ranks}_{label}'))
 for suffix in ('stdout','stderr'):shutil.copyfile(actual/(('native_serial.' if ranks==1 else 'case.')+suffix),folder/f'mpi{ranks}.{suffix}')
 provenance.append(dict(folder=str(source.relative_to(root)),validation_sha256=sha(source/'validation.json'),original_status=v['status'],independent_audit_sha256=sha(actual/'independent_audit.json'),ranks=ranks))
mmg=serial/'mmg_serial/mmg_serial.stdout';assert 'All tests passed (48 assertions in 2 test cases)' in mmg.read_text()
for suffix in ('stdout','stderr'):shutil.copyfile(serial/('mmg_serial/mmg_serial.'+suffix),folder/('mmg_serial.'+suffix))
s=next(s for s in original['stages'] if s['label']=='mmg_serial');stages.append(dict(s,stage='mmg_serial'))
source_drift=[n for n,h in b['source_sha256'].items() if sha(root/n)!=h];object_drift=[n for n,h in b['actual_objects_sha256'].items() if sha(build/n)!=h];archive_drift=[n for n,h in {**b['library_sha256'],**b['thin_archive_member_sha256']}.items() if sha(Path(n))!=h]
assert not source_drift and not object_drift and not archive_drift and sha(build/'UnitTests/test_driver')==b['binary_sha256']
v=dict(status='PASS',stages=stages,source_drift=source_drift,object_drift=object_drift,archive_drift=archive_drift,source_sha256=b['source_sha256'],actual_objects_sha256=b['actual_objects_sha256'],library_sha256=b['library_sha256'],thin_archive_member_sha256=b['thin_archive_member_sha256'],binary_sha256=b['binary_sha256'],build_ninja_sha256=b['build_ninja_sha256'],build_validation_sha256=sha(buildpath),serial_recovery_sha256=sha(recoverypath),provenance=provenance,scope='Postprocessing-only aggregation of actual complete68-object MPI1/2/4 selected69-case native/reference/2D+3Dbridge runs, their independent audits, and serialMMG48/2cases. All source/object/774archive-member/binary pins reverified. Original serial collector FAIL remains intact with independently checked recovery; no numerical rerun, new build or performance claim.')
v['artifact_sha256']={str(p.relative_to(folder)):sha(p) for p in folder.iterdir() if p.is_file()};(folder/'validation.json').write_text(json.dumps(v,indent=2)+'\n');print('PASS aggregated configured MPI1/2/4 and serialMMG; no rerun',flush=True)
