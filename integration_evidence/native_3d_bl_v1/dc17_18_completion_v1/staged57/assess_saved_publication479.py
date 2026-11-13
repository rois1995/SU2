"""Complete the preserved478control proof without repeating construction comparisons."""
import json
from pathlib import Path
from assess_surface_transaction470 import sha,terminal_receipt,owned_rows
R=Path(__file__).resolve().parent

def main():
    case=max((R/'cases').glob('curved479_saved_publication_recheck_serial_*'),key=lambda p:p.stat().st_mtime)
    status=json.loads((case/'status.json').read_text());assert status['state']=='terminal' and status['exit_code']==0 and status['pins_verified']
    pins=json.loads((case/'pins.json').read_text());assert all(sha(p)==h for p,h in pins.items())
    receipt=terminal_receipt(case);(case/'terminal_handles_actual.json').write_text(json.dumps(receipt,indent=2)+'\n')
    failure=json.loads((R/'surface_comparison478_preserved_failure.json').read_text());source=Path(failure['case'])
    rows=[json.loads(l) for l in (case/'actual_bl_cavity_planning.jsonl').read_text().splitlines()];checks=[v for v in rows if v['stage']=='saved_publication_recheck']
    assert len(checks)==1 and not any(v['stage']=='saved_correction_trial' for v in rows)
    check=checks[0];assert check['constructor_calls']==0 and check['old_cells']==10 and check['new_cells']==8
    assert check['independent_parameter_epochs'] and check['existing_state_reused'] and check['new_geometry_excess']==0 and check['provisional_height'] and not check['mesh_accepted']
    assert sha(case/'rechecked_owned.bin')==failure['working_owned_sha256']
    assert sha(case/'rechecked_parameters.bin')==failure['working_parameters_sha256']
    capture=[v for v in failure['trials'] if v['context']=='clean_control' and v['operator']=='CollapseVertex'];assert len(capture)==1 and capture[0]['private_feasible']
    observation_match={k:check[k]==capture[0][k] for k in ('old_q_bad','new_q_bad','old_J_bad','new_J_bad','old_geometry_bad','new_geometry_bad','old_geometry_excess','new_geometry_excess')}
    report=dict(case=str(case),status=status,pins_verified=len(pins),all_actual_handles_terminal=True,preserved478_comparison=failure['case'],control_publication_semantically_rechecked=True,constructor_calls=0,repeated_comparison_slots=0,result=check,observations_match478=observation_match,whole_working_cells=len(dict(owned_rows(case/'rechecked_owned.bin'))),working_owned_sha256=sha(case/'rechecked_owned.bin'),working_parameters_sha256=sha(case/'rechecked_parameters.bin'),production_changed=False,goal_complete=False,limitations=['Qualifies the existing control publication/diagnostic reload, not active geometry closure or final mesh acceptance.','No new MPI capability from this serial proof; all BL/RANS workflow obligations remain.'])
    (R/'saved_publication479_assessment.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report,indent=2))
if __name__=='__main__':main()
