"""Verify completed receipts, then record the report-only NumPy scalar correction."""
import hashlib,json
from pathlib import Path
BASE=Path(__file__).resolve().parent;ROOT=BASE.parents[3]
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
path=BASE/'campaign.py';archive=BASE/'executed_campaign.py';before=archive.read_bytes();assert path.read_bytes()==before
qualified=[]
for result in sorted(BASE.rglob('result.json')):
 if 'rejected_initial_controls_np1' in result.parts:continue
 report=json.loads(result.read_text());assert report['exit_code']==0
 proof=result.with_name('provenance.json');assert sha(proof)==report['provenance_sha256'];assert sha(result.with_name('run.log'))==report['log_sha256']
 for name,digest in json.loads(proof.read_text())['pins'].items():assert sha(ROOT/name)==digest,name
 for name,digest in json.loads(result.with_name('inputs.json').read_text()).items():assert sha(ROOT/name)==digest,name
 qualified.append(str(result.parent.relative_to(BASE)))
assert len(qualified)==9,qualified
summary=json.loads((BASE/'summary.json').read_text());assert summary['MPI_cloud_and_field_rows_exact']
old=b"  mass=json.loads((folder/'reassessment_policy_m2_stage0.json').read_text())";assert before.count(old)==1
after=before.replace(old,b'  reference=float(reference)\n'+old);path.write_bytes(after)
transition={'scope':'Report serialization only. All completed runtime/source/binary/input/log hashes verified; no SU2 rerun.',
 'qualified_cases':qualified,'executed_helper_sha256':hashlib.sha256(before).hexdigest(),'corrected_helper_sha256':sha(path),
 'summary_sha256':sha(BASE/'summary.json'),'finalizer_sha256':sha(Path(__file__)),
 'correction':'Convert NumPy quadrature reference to Python float before Boolean comparison and JSON output.'}
(BASE/'report_helper_transition.json').write_text(json.dumps(transition,indent=2)+'\n');print(json.dumps(transition,indent=2))
