"""Report identity and operation differences without relaxing the ordinary comparison gate."""
import importlib.util
import math
from pathlib import Path
import re

SHARED = Path(__file__).resolve().parents[2] / 'integration_evidence/native_cluster_balance_profile_v1'
spec = importlib.util.spec_from_file_location('diagnostic_profile', SHARED / 'analyze.py')
base = importlib.util.module_from_spec(spec);spec.loader.exec_module(base)
profile = base.profile
ACTIONS = base.ACTIONS


def details(case, row):
    if row['role'] in ('control', 'archived'): return dict(archived=True)
    text = (case / 'solver.log').read_text()
    mode = 'BOTH' if row['role']=='AUDIT' else row['role']
    audit = 'YES' if row['role']=='AUDIT' else 'NO'
    assert text.count('Native reuse diagnostic mode: '+mode+'; audit='+audit)==1, 'Missing/wrong diagnostic mode'
    result = dict(mode=mode, audit=audit)
    if audit=='YES':
        records = re.findall(r'^Native reuse audit metric checks/mismatches score checks/mismatches: (\d+) (\d+) (\d+) (\d+); maximum relative: (\S+) (\S+)$', text, re.M)
        assert len(records)==1, 'Missing/repeated fresh-hit audit summary'
        counts = list(map(int, records[0][:4]));relative = list(map(float, records[0][4:]))
        assert counts[0]>0 and counts[2]>0 and counts[1]<=counts[0] and counts[3]<=counts[2]
        assert all(math.isfinite(x) and x>=0 for x in relative)
        result.update(counts=counts, maximum_relative=relative,
                      sampled_hits_bit_identical=counts[1]==counts[3]==0,
                      first_mismatch_records=[line for line in text.splitlines() if line.startswith('Native reuse first mismatch worker=')][:4],
                      scope='Eight dynamic metric hits per imported patch; eight cell/edge hits per score cache and eight star checks per joint configuration. Authoritative seeds excluded. Sampling is not an exhaustive proof; extra evaluations are not performance measurements.')
    return result


def compare(rows):
    result = []
    for rep in sorted({row['repeat'] for row in rows}):
        group = {row['role']:row for row in rows if row['repeat']==rep}
        assert set(group)=={'control','archived','OFF','SCORES','METRIC','BOTH','AUDIT'}, 'Incomplete diagnostic group'
        for left, right in (('control','OFF'), ('archived','BOTH'), ('OFF','SCORES'), ('OFF','METRIC'), ('OFF','BOTH'), ('BOTH','AUDIT')):
            old, new = group[left], group[right]
            assert old['status']==new['status']=='PASS'
            pair = base.compare([dict(old,role='control'), dict(new,role='profile')])[0]
            pair['comparison'] = left+' vs '+right
            pair['diagnostic_only'] = True
            pair['operation_counts_identical'] = all(a[k]==b[k]
                for ra,rb in zip(old['balance'],new['balance'])
                for a,b in zip(ra['operations'],rb['operations'])
                for k in ('rank','workers','action','selected','attempts','reconstructed','committed','cells','max_cells','selection_scans'))
            pair['audit_extra_work'] = right=='AUDIT'
            if right=='AUDIT': pair['fresh_hit_audit'] = new['diagnostic']
            result.append(pair)
    return result
