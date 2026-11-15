#!/usr/bin/env python3
"""Read-only final gate with the solver's unpadded cycle label; filenames remain padded."""
import importlib.util,inspect,json,sys
from pathlib import Path
HERE=Path(__file__).resolve().parent
spec=importlib.util.spec_from_file_location('frozen_controller',HERE/'run_full_workflow.py')
controller=importlib.util.module_from_spec(spec);spec.loader.exec_module(controller)

def validator():
    source=inspect.getsource(controller.validate_full_workflow)
    old='Writing the adapted mesh of cycle {event:05d}: adapted_mesh_adap_{event:05d}.su2.'
    new='Writing the adapted mesh of cycle {event}: adapted_mesh_adap_{event:05d}.su2.'
    assert source.count(old)==1
    # CSinglezoneDriver.cpp:1815 streams iCycle, while its filename uses five-digit formatting.
    source=source.replace(old,new)
    namespace=dict(vars(controller));exec(compile(source,__file__,'exec'),namespace)
    return namespace['validate_full_workflow']

def self_check():
    assert f'Writing the adapted mesh of cycle {1}: adapted_mesh_adap_{1:05d}.su2.'=='Writing the adapted mesh of cycle 1: adapted_mesh_adap_00001.su2.'
    assert f'Writing the adapted mesh of cycle {2}: adapted_mesh_adap_{2:05d}.su2.'=='Writing the adapted mesh of cycle 2: adapted_mesh_adap_00002.su2.'
    validator()

if __name__=='__main__':
    self_check()
    if sys.argv[1:]==['--self-check']:raise SystemExit(0)
    case=Path(sys.argv[1]).resolve();status=json.loads((case/'status.json').read_text())
    assert status['state']=='terminal' and status['all_owned_handles_terminal']
    assert status['inputs_unchanged'] and status['source_and_binary_pins_unchanged']
    result=validator()(case)
    result['parser_correction']={'source':'SU2_CFD/src/drivers/CSinglezoneDriver.cpp:1815','only_change':'unpadded cycle number in publication log marker; filename and every physical/resource/transfer/CFD gate unchanged','frozen_controller_sha256':controller.sha(HERE/'run_full_workflow.py'),'assessment_helper_sha256':controller.sha(__file__),'raw_receipt_preserved':str(case/'full_workflow_receipt.json')}
    (case/'full_workflow_corrected_marker_receipt.json').write_text(json.dumps(result,indent=2)+'\n')
    print(json.dumps({'workflow_accepted':result['workflow_accepted'],'errors':result['errors'],'events':[{'event':e['event'],'seconds':e['whole_event_seconds_max_rank'],'counts':e['actual_mesh_counts']} for e in result['events']]}),flush=True)
    raise SystemExit(not result['workflow_accepted'])
