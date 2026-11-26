"""Recover C5 trial two from the inherited isolated-restart sampler latch."""

from argparse import ArgumentParser
from pathlib import Path
import runpy
import subprocess
from unittest.mock import patch

import p3_full_receipts as receipts

full = receipts.full
WORKFLOW = full.workflow
SELF = Path(__file__).resolve()
PRIOR = full.NIGHT/'FP2_C3/resume_restart_observation.py'


def repair(label):
    build = full.verify_build(label)
    root = full.root(label,'C5')
    work = root/'trial2'
    case = work/f'full{label}_run1'
    config = full.config_for('OFF')
    payloads = runpy.run_path(str(WORKFLOW.PAYLOADS))
    gate = full.cut.read(case/'final_source_first_collapse_gate.json')
    assert gate['passed'] and gate['count_prediction_matches'] and gate['zero_upper_refinements']
    assert gate['upper_work_sum'] <= gate['work_limit'] == 108459426
    full.C3.terminal_wait(work,'full',0)
    full.BASE['status_ok'](case,build,config)
    full.C3.require_clean_contact_logs(work,case)
    WORKFLOW.verify_helpers(work)
    hashes = WORKFLOW.workflow_payloads(case,build['binary_sha256'],payloads)
    assert hashes == full.cut.read(case/'n8_payload_identity.json')['hashes']
    events = WORKFLOW.event_collector()(case)

    retry = runpy.run_path(str(PRIOR))
    target = retry['corrected_helper'](work,label)
    subprocess.run([full.cut.PYTHON,'-B',str(target),'--self-check'],check=True)
    restart_case = work/f'disk_restart_{case.name}_event2'
    archive = retry['archive_attempt'](work,restart_case)
    if not (work/'restart_wait.json').is_file():
        assert not (work/'restart_launch.json').exists()
        WORKFLOW.run_stage(work,'restart',[full.cut.PYTHON,'-B',str(target),'--source-case',str(case)])
    full.C3.terminal_wait(work,'restart',0)
    full.C3.require_clean_contact_logs(work,restart_case)
    verify = runpy.run_path(str(WORKFLOW.F_WORKFLOW))['verify_restart']
    verify.__globals__['WORKSPACE'] = work
    restart = verify(label,case)
    write = WORKFLOW.sealed_write

    def save_receipt(path, row):
        assert path == work/'run_receipt.json'
        evidence = (SELF,PRIOR,target,archive,work/'restart_host_preflight.json',
                    *restart_case.glob('runtime_sample_eio.jsonl'))
        row['artifacts'].update({str(file.relative_to(work) if file.is_relative_to(work) else file):
                                 full.sha(file) for file in evidence})
        write(path,row)

    with patch.object(full,'switch_receipt',receipts.corrected_switch_receipt()), patch.object(
            WORKFLOW,'sealed_write',save_receipt):
        full.envelope(work,label,'C5',build,config,events,payloads,hashes,restart)
    full.authenticate_run(work,label,'C5',build,payloads)
    for path in (root/'case_summary.json',full.HERE/f'build{label}/C5.json'):
        archived = path.with_name(f'{path.stem}_before_restart_observation_repair.json')
        if not archived.exists():
            assert full.cut.read(path)['status']=='FAILED'
            path.rename(archived)
    return receipts.run(label,'C5',False)


if __name__ == '__main__':
    parser = ArgumentParser(description=__doc__)
    parser.add_argument('build',type=int)
    raise SystemExit(repair(parser.parse_args().build))
