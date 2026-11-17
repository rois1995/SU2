from pathlib import Path
import hashlib,json,subprocess,time,shutil
p=Path('integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57/night_20261115/N4/section6_3_after_N11')
read=lambda f:json.loads(Path(f).read_text())
sha=lambda f:hashlib.sha256(Path(f).read_bytes()).hexdigest()
base=subprocess.check_output(['git','rev-parse','HEAD'],text=True).strip()
assert read(p/'B6/acceptance_summary.json')['state']=='B6_SCOPED_G1_G2_PASSED'
for tag in ['G1_serial','G1_mpi2','G1_mpi4','G2']:
 t=read(p/f'B6/validation/{tag}_primary_terminal_receipt.json');assert t['all_owned_handles_terminal']
 for row in t['identities']:
  try:
   f=(Path('/proc')/str(row['pid'])/'stat').read_text().rsplit(')',1)[1].split();alive=int(f[19])==row['start_ticks'] and f[0] not in {'Z','X'}
  except FileNotFoundError:alive=False
  assert not alive,row
candidate=p/'B7_preparation/proposal/candidate_after_B6_build555';dest=p/'B7_preparation/proposal/accepted_B6_rebind';assert not dest.exists();shutil.copytree(candidate,dest)
gen=dest/'generate_b7_proposal.py';s=gen.read_text().replace('B7_preparation/proposal/candidate_after_B6_build555','B7_preparation/proposal/accepted_B6_rebind').replace('CANDIDATE_NOT_ACCEPTED_AFTER_B6_BUILD555',base);gen.write_text(s)
manifest=read(candidate/'generated/binding_manifest.json')
paths={'CNativeEngine3D.cpp':Path('Common/src/adaptation/CNativeEngine3D.cpp')}
paths.update({f'EngineControl.{ext}':Path(f'Common/src/adaptation/native_engine3d/EngineControl.{ext}') for ext in ['hpp','cpp']})
for name,path in paths.items():
 assert path.read_bytes()==subprocess.check_output(['git','show',f'{base}:{path}'])
 assert sha(path)==manifest['base_sha256'][name]
 assert (dest/f'input_{name}').read_bytes()==path.read_bytes()
subprocess.run(['python3',str(gen)],check=True,stdout=(dest/'regeneration_stdout.json').open('w'))
fresh=read(dest/'generated/binding_manifest.json');assert fresh['base_commit']==base
for name,h in manifest['proposed_sha256'].items():assert sha(dest/'generated'/f'{name}.proposed')==h==fresh['proposed_sha256'][name]
assert fresh['main_call_lambda_reversal_byte_equal'] and fresh['all_six_old_names_absent_from_main_and_generated_members']
assert all(x['token_equal'] for x in fresh['helper_inlining_actual_removed_lambda_token_proofs'])
assert fresh['actual_alias_prefixes_match_canonical_bindings'] and fresh['header_original_tokens_equal_after_removing_added_declarations'] and fresh['B5_cpp_prefix_token_identical_after_full_format_and_comment']
out=p/'B7';out.mkdir(exist_ok=False)
for name,path in paths.items():path.write_bytes((dest/'generated'/f'{name}.proposed').read_bytes())
(out/'actual_apply_receipt.json').write_text(json.dumps(dict(applied=True,unix=time.time(),base_commit=base,reference_build=552,base_sha256=manifest['base_sha256'],applied_sha256={str(path):sha(path) for path in paths.values()},all_three_reviewed_outputs_byte_identical=True,actual_body_proof_sha256=sha(dest/'generated/binding_manifest.json'),primary_review_sha256=sha(candidate/'PRIMARY_REVIEW.md'),canonical_fields_and_special_members_unchanged=True,pending=['build557','G1serial','G1mpi2','G1mpi4','strictG2']),indent=2)+'\n')
print('B7 accepted-base identity verified and connected three-path extraction applied; validation pending.')
