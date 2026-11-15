#!/usr/bin/env python3
"""Apply reviewer P6 check4, including its explicit allowance for different edits."""
import hashlib,importlib.util,json,re,sys
from pathlib import Path
HERE=Path(__file__).resolve().parent
R=HERE.parent.parent
spec=importlib.util.spec_from_file_location('prior_bl_guard',HERE.parent/'check_bl_regression.py')
prior=importlib.util.module_from_spec(spec);spec.loader.exec_module(prior)

def first_difference(reference,actual):
    for i in range(max(len(reference),len(actual))):
        a=reference[i] if i<len(reference) else None
        b=actual[i] if i<len(actual) else None
        if a!=b:return {'edit_index_zero_based':i,'reference':a,'actual':b}
    return None

def refinements_pass(rows):
    return bool(rows) and all('upper_refinements' in row and int(row['upper_refinements'])==0 for row in rows)

def self_check():
    prior.self_check()
    assert first_difference([1,2],[1,3])['edit_index_zero_based']==1
    assert first_difference([1],[1,2])['reference'] is None
    assert first_difference([1],[1]) is None
    assert refinements_pass([{'upper_refinements':'0'}])
    assert not refinements_pass([]) and not refinements_pass([{}])
    assert not refinements_pass([{'upper_refinements':'1'}])

def main(case):
    case=Path(case).resolve()
    status=json.loads((case/'status.json').read_text());command=json.loads((case/'command.json').read_text())
    row=json.loads((case/'actual_saved_surface_stage_rank0.json').read_text())
    gates=prior.conditions(status,command,row,dict.fromkeys(prior.FILES,True))
    del gates['same_bounded_output']  # P6 section1.6 check4 explicitly supersedes byte identity.
    reference_case=R/'measurement_20261114_v2/run_A'
    source_case=R/'cases/curved492_saved_production_surface_serial_20261113T163756_3724702'
    original=json.loads((source_case/'command.json').read_text())
    env=lambda c:{k:v for k,v in c['environment'].items() if k.startswith('SU2_NATIVE_') and k!='SU2_NATIVE_FROZEN_CONFIG'}
    gates['same_frozen_input_contract']=env(command)==env(original)
    gates['same_frozen_configuration']=(case/'run.cfg').read_bytes()==(source_case/'run.cfg').read_bytes()
    stages=[]
    for line in (case/'run.log').read_text(errors='replace').splitlines():
        if 'NATIVE3D_STAGE ' in line:
            record=dict(re.findall(r'(\w+)=([^\s]+)',line))
            if record.get('stage')=='exit' and record.get('rank')=='0':stages.append(record)
    gates['zero_upper_refinements']=refinements_pass(stages)
    def edits(folder):
        records=[json.loads(line) for line in (folder/'nose_target_observation.jsonl').read_text().splitlines()]
        return [{'edge':r['edge'],'point':r['point']} for r in records if r.get('stage')=='accepted_split']
    reference_edits=edits(reference_case);actual_edits=edits(case)
    hashes={}
    for name in prior.FILES:
        with (case/name).open('rb') as f:hashes[name]=hashlib.file_digest(f,'sha256').hexdigest()
    result={'passed':all(gates.values()),'gates':gates,'case':str(case),'actual_splits':row['splits'],
            'first_differing_edit':first_difference(reference_edits,actual_edits),
            'reference_edits':len(reference_edits),'actual_edits':len(actual_edits),'reference':str(reference_case),
            'stage_exit_rows':stages,'actual_output_sha256':hashes,
            'scope':'P6 bounded BL preservation; output identity is not required; no final accepted mesh claimed.'}
    (case/'p6_bl_receipt.json').write_text(json.dumps(result,indent=2)+'\n')
    print(json.dumps(result),flush=True)
    return not result['passed']

if __name__=='__main__':
    self_check()
    raise SystemExit(0 if sys.argv[1:]==['--self-check'] else main(sys.argv[1]))
