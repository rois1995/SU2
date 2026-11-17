from pathlib import Path
import json,hashlib,time,re,subprocess
p=Path('integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57/night_20261115/N4/section6_3_after_N11');v=p/'B3/validation';base=subprocess.check_output(['git','rev-parse','HEAD'],text=True).strip();assert base=='ce25219f04cb01285bfeb8b33dba1e6a3496782a';assert json.loads((p/'B3/acceptance_summary.json').read_text())['state']=='B3_SCOPED_G1_G2_PASSED'
for name in ['G1_serial','G1_mpi2','G1_mpi4','G2']:
 x=json.loads((v/f'{name}_primary_terminal_receipt.json').read_text());assert x['all_owned_handles_terminal']
 for row in x['identities']:
  try:
   f=(Path('/proc')/str(row['pid'])/'stat').read_text().rsplit(')',1)[1].split();alive=int(f[19])==row['start_ticks'] and f[0] not in {'Z','X'}
  except FileNotFoundError:alive=False
  assert not alive,row
prep=p/'B4_preparation/proposal';g=prep/'generated';m=json.loads((g/'binding_manifest.json').read_text());mapping={'CNativeEngine3D.cpp':Path('Common/src/adaptation/CNativeEngine3D.cpp'),'EngineQueues.hpp':Path('Common/src/adaptation/native_engine3d/EngineQueues.hpp'),'EngineQueues.cpp':Path('Common/src/adaptation/native_engine3d/EngineQueues.cpp')};sha=lambda b:hashlib.sha256(b).hexdigest()
for name,dest in mapping.items():assert sha(dest.read_bytes())==m['base'][name] and sha((g/(name+'.proposed')).read_bytes())==m['proposed_sha256'][name]
fields=lambda text:[re.sub(r'\s+','',x) for x in re.findall(r'std::optional[^;]+;',text)]
assert fields(mapping['EngineQueues.hpp'].read_text())==fields((g/'EngineQueues.hpp.proposed').read_text());assert m['whole_main_file_byte_reversal'] and all(r['actual_removed_lambda_body_token_equal_after_inlining'] for r in m['member_token_proofs']);assert all(r['formatted_lines']<=80 for r in m['capture_binding_audit'].values());out=p/'B4';out.mkdir(exist_ok=False)
for name,dest in mapping.items():dest.write_bytes((g/(name+'.proposed')).read_bytes())
(out/'actual_apply_receipt.json').write_text(json.dumps(dict(applied=True,unix=time.time(),base_commit=base,reference_build=552,base_sha256=m['base'],applied_sha256={name:sha(dest.read_bytes()) for name,dest in mapping.items()},proposal_manifest_sha256=sha((g/'binding_manifest.json').read_bytes()),generator_sha256=sha((prep/'generate_b4_proposal.py').read_bytes()),optional_fields_unchanged=True,whole_main_byte_reversal=True,actual_two_lambda_token_proofs=True,collective_container_sort_RAII_order_preserved=True,production_paths=[str(f) for f in mapping.values()],pending=['build554','G1serial','G1mpi2','G1mpi4','G2']),indent=2)+'\n');print('Applied connected B4 to three paths; qualification pending.')
