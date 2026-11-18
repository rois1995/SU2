#!/usr/bin/env python3
"""Verify a partial M28 example and reproduce the saved N16 measurements."""
import csv
import gzip
from pathlib import Path
import tempfile

from report_m28 import LEGACY_CSV, read_case, stuck_table


def synthetic_case(directory):
    log0 = (
        'NATIVE3D_PARTITION rank=0 parts=2\n'
        'NATIVE3D_PHASE_OWNERSHIP rank=0 quality=0 evaluate=1 migrate=1\n'
        'NATIVE3D_PHASE_OWNERSHIP rank=0 quality=1 evaluate=1 migrate=1\n'
        'NATIVE3D_PHASE_OWNERSHIP rank=0 quality=1 evaluate=1 migrate=1\n'
        'NATIVE3D_PHASE_OWNERSHIP rank=0 quality=1 committed=1\n'
        'NATIVE3D_PRIVATE_WORK rank=0 round=6 private_work_used=2 busy_seconds=1 choices=2 lost_own=1 lost_other=0\n'
        'NATIVE3D_PRIVATE_WORK rank=0 round=8 private_work_used=3 busy_seconds=2 choices=4 lost_own=0 lost_other=2\n'
        'NATIVE3D_QUALITY rank=0 stage=window census=0 complete=1 round=7 stuck_shape=1\n'
        'Wed Nov 18 18:10:37 2026<stdout>:NATIVE3D_M28_STUCK rank=0 stage=window round=7 cell=42 f=1:5:0:0:1:4:0:0\n'
        'NATIVE3D_QUALITY rank=0 stage=window census=1 complete=1 round=7 stuck_shape=0\n'
    )
    timer = (
        'NATIVE3D_TIMER root=remesh boundary=exit rank=0 phase=quality operation=scheduling exclusive_seconds=4\n'
        'NATIVE3D_TIMER_SUMMARY root=remesh boundary=exit rank=0 root_seconds=10\n'
    )
    # An incomplete output line must not advance the last census.
    log0 += 2 * timer + 'NATIVE3D_QUALITY rank=0 stage=exit census=2 complete=1 round=8'
    log1 = (
        'NATIVE3D_PARTITION rank=1 parts=2\n'
        'NATIVE3D_QUALITY rank=1 stage=exit census=2 complete=1 round=12 stuck_shape=3\n'
        'NATIVE3D_M28_STUCK rank=1 stage=exit round=12 cell=42 f=1:5:0:0:1:4:0:0\n'
        'NATIVE3D_M28_STUCK rank=1 stage=exit round=12 cell=51 f=1:6:0:0:1:0:1:0\n'
        'NATIVE3D_M28_STUCK_MORE rank=1 stage=exit round=12 not_printed=1\n'
    )
    for rank, log in enumerate((log0, log1)):
        path = directory / f'rank_output/1/rank.{rank}/stdout'
        path.parent.mkdir(parents=True)
        if rank == 0:
            path.write_text(log)
        else:
            path.with_suffix('.gz').write_bytes(gzip.compress(log.encode()))
    data, audit = read_case('partial', directory)
    first_rank, second_rank, total = data
    assert first_rank['quality_ownership_evaluations'] == 2 and first_rank['quality_ownership_migrations'] == 1
    assert first_rank['lost_own'] == 1 and first_rank['lost_other'] == 2
    assert second_rank['lost_own'] is None and 'lost_own' not in total
    assert first_rank['quality_scheduling_seconds'] == 4 and first_rank['root_seconds'] == 10
    assert [row['line'] for row in audit['rank_events'][0]['timer_snapshot']] == [14, 13]
    last0, last1 = audit['last_censuses']
    assert last0['round'] == 7 and last0['cells'] == [] and last0['last_census']['line'] == 10
    assert [cell['cell'] for cell in last1['cells']] == ['42', '51']
    migrated = last1['cells'][0]
    assert (migrated['first_rank'], migrated['first_stage'], migrated['first_round']) == (0, 'window', 7)
    assert migrated['first_line']['line'] == 9 and 'Wed Nov 18' in migrated['first_line']['raw']
    assert last1['overflow'][0]['not_printed'] == 1
    table = stuck_table([audit])
    assert '| 42 | 0/window/7' in table and 'cell=51' in table and 'not_printed=1' in table
    assert 'Ranks without a printed quality census' not in table


def compare_saved():
    night = LEGACY_CSV.parents[2]
    source = night / 'N4/section6_3_after_N11'
    with LEGACY_CSV.open(newline='') as file:
        expected = list(csv.DictReader(file))
    cases = [('T0', 'round_8_5r_B/G5/full574', [(120, 12), (78, 6)]),
             ('run1 failed event 1', 'round_8_5r_C/G5/full575_run1', [(540, 6)])]
    for label, relative, expected_counts in cases:
        records, audit = read_case(label, source / relative)
        reference = [row for row in expected if row['run'] == label]
        assert len(records) == len(reference)
        observed_counts = [(row['quality_ownership_evaluations'], row['quality_ownership_migrations'])
                           for row in records if row['rank'] == 'sum']
        assert observed_counts == expected_counts
        for row, original in zip(records, reference):
            legacy = {key: '' if row.get(key) is None else str(row[key]) for key in original}
            assert legacy == original, (label, row['event'], row['rank'])
        assert 'no stuck cell' in stuck_table([audit])
        assert all(row.get('lost_own') is None and row.get('lost_other') is None for row in records)
        print(f'{label}: {len(records)} CSV rows matched all N16 values, quality ownership {observed_counts}')


if __name__ == '__main__':
    with tempfile.TemporaryDirectory() as directory:
        synthetic_case(Path(directory))
    print('Partial example passed: latest census, migrated cell, overflow, archived log, truncated line and timer source')
    compare_saved()
