from pathlib import Path
import json,hashlib,time,sys
v=Path('integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57/night_20261115/N4/section6_3_after_N11/B8/validation');cohort=sys.argv[1];session=int(sys.argv[2])
def read(p):return json.loads(Path(p).read_text())
def sha(p):return hashlib.file_digest(Path(p).open('rb'),'sha256').hexdigest()
r=read(v/f'G1_candidate_{cohort}_summary.json');assert r['identity_pass'] and not r.get('non_wall_differences');case=Path(r['candidate_attempt']);status=read(case/'status.json');assert status['state']=='terminal' and status['pins_verified'];terminal=read(case/'n11_terminal_receipt.json');assert terminal['all_owned_handles_terminal'];rows=terminal['owned_processes']+[dict(read(v/f'G1_{cohort}_primary_launch_identity.json')['controller'],name='controller')]
for row in rows:
 try:
  f=(Path('/proc')/str(row['pid'])/'stat').read_text().rsplit(')',1)[1].split();alive=int(f[19])==row['start_ticks'] and f[0] not in {'Z','X'}
 except FileNotFoundError:alive=False
 row['alive']=alive;assert not alive,row
casepins=read(case/'pins.json');extra=read(v/f'G1_{cohort}_primary_validation_pins.json');assert all(sha(p)==h for p,h in casepins.items());assert all(sha(p)==h for p,h in extra.items());out=v/f'G1_{cohort}_primary_terminal_receipt.json';assert not out.exists();out.write_text(json.dumps({'actual_host_proc':True,'unix':time.time(),'identities':rows,'all_owned_handles_terminal':True,'exec_session':session,'exec_exit_code':0,'case_pins_verified':len(casepins),'extra_validation_pins_verified':len(extra),'identity_pass':True,'wall_exception_differences':r.get('wall_exception_differences',[]),'reporter_totals_equal_observation':r['reporter_totals_equal'],'successfulasserttotals_are_report_only':True},indent=2)+'\n');print(json.dumps({'cohort':cohort,'passed':True,'terminal_identities':len(rows),'case_pins':len(casepins),'wall_changes':len(r.get('wall_exception_differences',[])),'seconds':status['seconds']}))
