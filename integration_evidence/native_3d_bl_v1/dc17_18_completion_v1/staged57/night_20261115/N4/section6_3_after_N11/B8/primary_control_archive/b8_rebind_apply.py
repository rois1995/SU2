from pathlib import Path
import hashlib,json,subprocess,time,shutil
p=Path('integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57/night_20261115/N4/section6_3_after_N11');read=lambda f:json.loads(Path(f).read_text());sha=lambda f:hashlib.sha256(Path(f).read_bytes()).hexdigest();base=subprocess.check_output(['git','rev-parse','HEAD'],text=True).strip()
assert read(p/'B7/acceptance_summary.json')['state']=='B7_SCOPED_G1_G2_PASSED'
for tag in ['G1_serial','G1_mpi2','G1_mpi4','G2']:
 t=read(p/f'B7/validation/{tag}_primary_terminal_receipt.json');assert t['all_owned_handles_terminal']
 for row in t['identities']:
  try:
   f=(Path('/proc')/str(row['pid'])/'stat').read_text().rsplit(')',1)[1].split();alive=int(f[19])==row['start_ticks'] and f[0] not in {'Z','X'}
  except FileNotFoundError:alive=False
  assert not alive,row
candidate=p/'B8_preparation/proposal/integrated_candidate_after_B7';m=read(candidate/'CANDIDATE_SHA256.json');assert all(sha(candidate/f)==h for f,h in m['sha256'].items());dest=p/'B8_preparation/proposal/accepted_B7_rebind';assert not dest.exists();shutil.copytree(candidate,dest)
inputs={}
for f in sorted(dest.glob('input_*')):
 name=f.name.removeprefix('input_');path=Path('Common/src/adaptation')/(name if name=='CNativeEngine3D.cpp' else 'native_engine3d/'+name);data=subprocess.check_output(['git','show',f'{base}:{path}']);assert f.read_bytes()==data,(f,path);inputs[str(path)]=sha(f)
proof=read(dest/'INTEGRATED_PROOF.json');assert proof['whole_B7_main_reverse_byte_equal'] and proof['actual_final_formatted_method_tokens_equal_to_sequential_stage_methods'] and proof['original_setup_fields_and_special_members_token_equal'] and proof['all_optional_and_bool_workers_have_terminal_return_audit'];assert max(proof['formatted_method_lines'].values())<=80 and max(proof['formatted_lambda_lines'])<=15
identity={'base_commit':base,'all_input_copies_equal_accepted_commit':True,'input_sha256':inputs,'all_three_reviewed_outputs_byte_identical':True,'candidate_manifest_sha256':sha(candidate/'CANDIDATE_SHA256.json')};(dest/'ACCEPTED_BASE_REBIND.json').write_text(json.dumps(identity,indent=2)+'\n')
mapping={'CNativeEngine3D.integrated.cpp.proposed':Path('Common/src/adaptation/CNativeEngine3D.cpp'),'EngineSetup.integrated.hpp.proposed':Path('Common/src/adaptation/native_engine3d/EngineSetup.hpp'),'EngineSetup.integrated.cpp.proposed':Path('Common/src/adaptation/native_engine3d/EngineSetup.cpp')};old=read(p/'build557/source_pins.json');before={}
for name,path in mapping.items():
 before[str(path)]=sha(path);assert before[str(path)]==old[str(path.resolve())];assert sha(dest/name)==m['sha256'][name]
out=p/'B8';out.mkdir(exist_ok=False)
for name,path in mapping.items():path.write_bytes((dest/name).read_bytes())
(out/'actual_apply_receipt.json').write_text(json.dumps(dict(applied=True,unix=time.time(),base_commit=base,reference_build=552,base_sha256=before,applied_sha256={str(path):sha(path) for path in mapping.values()},all_three_reviewed_outputs_byte_identical=True,actual_body_proof_sha256=sha(dest/'INTEGRATED_PROOF.json'),primary_review_sha256=sha(candidate/'PRIMARY_REVIEW.md'),accepted_base_rebind_sha256=sha(dest/'ACCEPTED_BASE_REBIND.json'),canonical_fields_and_special_members_unchanged=True,pending=['build558','G1serial','G1mpi2','G1mpi4','strictG2']),indent=2)+'\n')
print('B8 input copies bound to accepted B7; connected three-path extraction applied; validation pending.')
