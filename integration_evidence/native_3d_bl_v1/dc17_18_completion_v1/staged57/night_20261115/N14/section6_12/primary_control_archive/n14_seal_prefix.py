from pathlib import Path
import json,time,hashlib,sys
v=Path('integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57/night_20261115/N14/section6_12/validation');gate=sys.argv[1];session=int(sys.argv[2]);assert gate in {'G2','GQ'};case=v/'cases'/f'N14_{gate}_552'
def read(p):return json.loads(Path(p).read_text())
def sha(p):return hashlib.file_digest(Path(p).open('rb'),'sha256').hexdigest()
s=read(case/'status.json');r=read(case/f'{gate.lower()}_build552_comparison.json');assert s['state']=='terminal' and s['stop_reason']==gate+'_prefix_passed' and s['all_owned_handles_terminal'] and s['inputs_unchanged'] and s['source_and_binary_pins_unchanged'];assert r['passed'] and r['strict_old_integer_rows'];rows=s['owned_identities']+[dict(read(v/f'{gate}_primary_launch_identity.json')['controller'],comm='controller')]
for row in rows:
 try:
  f=(Path('/proc')/str(row['pid'])/'stat').read_text().rsplit(')',1)[1].split();alive=int(f[19])==row['start_ticks'] and f[0] not in {'Z','X'}
 except FileNotFoundError:alive=False
 row['alive']=alive;assert not alive,row
inputs=read(case/'provenance.json')['input_hashes'];extra=read(v/f'{gate}_primary_validation_pins.json');pins=read(v.parent/'build552/source_pins.json');assert all(sha(p)==h for mapping in (inputs,extra,pins) for p,h in mapping.items());peak=max(json.loads(line)['sum_rank_process_peak_hwm_bytes'] for line in (case/'host_resources.jsonl').read_text().splitlines());assert peak<=19999490048
receipt={'actual_host_proc':True,'unix':time.time(),'identities':rows,'all_owned_handles_terminal':True,'exec_session':session,'exec_exit_code':0,'passed':True,'seconds_until_stop':s['seconds_until_stop'],'seconds_total':s['seconds_total'],'source_pins_verified':len(pins),'input_pins_verified':len(inputs),'extra_helper_pins_verified':len(extra),'observed_prefix_rank_hwm_sum_bytes':peak,'not_full_workflow_P8_peak':True,'strict_comparison_sha256':sha(case/f'{gate.lower()}_build552_comparison.json')};out=v/f'{gate}_primary_terminal_receipt.json';assert not out.exists();out.write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps({k:x for k,x in receipt.items() if k!='identities'}))
