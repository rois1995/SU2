from pathlib import Path
import hashlib,json,re,time,subprocess
p=Path('integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57/night_20261115/N4/section6_3_after_N11');g=p/'B3_preparation/proposal/generated';m=json.loads((g/'binding_manifest.json').read_text());n=p.parents[1];v=n/'N14/section6_12/validation';assert subprocess.check_output(['git','rev-parse','HEAD'],text=True).strip()=='0c2cb5c460a5e01c5dde1564c96d209f457e8fa8';assert json.loads((v.parent/'acceptance_summary.json').read_text())['state']=='N14_N15_PASSED'
for name in ['full_primary_terminal_receipt.json','restart_primary_terminal_receipt.json']:
 for row in json.loads((v/name).read_text())['identities']:
  try:
   f=(Path('/proc')/str(row['pid'])/'stat').read_text().rsplit(')',1)[1].split();alive=int(f[19])==row['start_ticks'] and f[0] not in {'Z','X'}
  except FileNotFoundError:alive=False
  assert not alive,row
mapping={'CNativeEngine3D.cpp':Path('Common/src/adaptation/CNativeEngine3D.cpp'),'EngineCensus.hpp':Path('Common/src/adaptation/native_engine3d/EngineCensus.hpp'),'EngineCensus.cpp':Path('Common/src/adaptation/native_engine3d/EngineCensus.cpp')}
def sha(b):return hashlib.sha256(b).hexdigest()
for name,dest in mapping.items():
 assert sha(dest.read_bytes())==m['base_sha256'][name];assert sha((g/(name+'.proposed')).read_bytes())==m['proposed_sha256'][name]
old=mapping['EngineCensus.hpp'].read_text();new=(g/'EngineCensus.hpp.proposed').read_text();tokens=lambda text: [re.sub(r'\s+','',x) for x in re.findall(r'std::optional[^;]+;',text)];assert tokens(old)==tokens(new);assert m['main_cpp_byte_reversal_after_call_routing'] and len(m['actual_removed_lambda_body_token_proofs'])==6;assert all(x['formatted_lines']<=80 for x in m['formatted_member_methods']);out=p/'B3';out.mkdir(exist_ok=False)
for name,dest in mapping.items():dest.write_bytes((g/(name+'.proposed')).read_bytes())
receipt=dict(applied=True,unix=time.time(),base_commit='0c2cb5c460a5e01c5dde1564c96d209f457e8fa8',build_reference=552,base_sha256=m['base_sha256'],applied_sha256={name:sha(dest.read_bytes()) for name,dest in mapping.items()},proposal_manifest_sha256=sha((g/'binding_manifest.json').read_bytes()),generator_sha256=sha((p/'B3_preparation/proposal/generate_b3_proposal.py').read_bytes()),main_byte_reversal=True,actual_six_lambda_body_proofs=True,canonical_optional_fields_identical=True,collective_and_RAII_order_preserved=True,production_paths=[str(x) for x in mapping.values()],pending=['build553','G1serial','G1mpi2','G1mpi4','strictG2'])
(out/'actual_apply_receipt.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps(dict(applied=True,paths=len(mapping),max_new_method_lines=max(x['formatted_lines'] for x in m['formatted_member_methods']),all_optional_fields_identical=True)))
