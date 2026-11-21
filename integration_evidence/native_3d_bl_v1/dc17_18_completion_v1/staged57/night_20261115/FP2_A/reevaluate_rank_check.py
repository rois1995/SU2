"""Re-evaluate the sealed build627 rank receipts under FP2-A Correction 4."""

from argparse import ArgumentParser
from decimal import Decimal
from pathlib import Path

import run_a as a
import run_rank_check as prior

ITEM = Path(__file__).resolve().parent
ROOT = ITEM / 'rank_check/build627'
ORDER = ITEM / 'inputs/FP2_A_CORRECTION_4.md'
ORDER_SHA = '43b828333e7bcd4db85445de7ee94b660785a8aab485ac509140536637a2b3dd'
OLD_SUMMARY = ROOT / 'rank_check_summary.json'
OLD_SHA = 'a577ec4a17f28198dd19a75d72d43e54e34187eb8351a473f0df78dd1c197d3e'
EVALUATION = ROOT / 'correction4_evaluation.json'
FINGERPRINTS = ('build', 'binary_sha256', 'test_binary_sha256', 'source_pins_sha256')
FACE_LIMIT = 126_153


def alarms(surface, face_totals, pass_rows, check_time):
    found = []
    for row in surface:
        identity = {name: int(row[name]) for name in ('rank', 'pass')}
        for name, value, failed in (
            ('star_max', 1_000, int(row['star_max']) > 1_000),
            ('sweeps_distance', 40, int(row['sweeps_distance']) == 40),
        ):
            if failed:
                found.append({**identity, 'field': name, 'observed': int(row[name]), 'limit': value})
    for pass_number, count in face_totals.items():
        if count > FACE_LIMIT:
            found.append({'pass': int(pass_number), 'field': 'faces_out_sum', 'observed': count, 'limit': FACE_LIMIT})
    if check_time:
        for row in pass_rows:
            if Decimal(row['seconds_pass']) > prior.PASS_ALARM:
                found.append({'rank': int(row['rank']), 'pass': int(row['pass']), 'field': 'seconds_pass',
                              'observed': row['seconds_pass'], 'limit': int(prior.PASS_ALARM)})
    return found


def evaluate():
    assert a.sha(ORDER) == ORDER_SHA
    checksum = ORDER.with_suffix('.sha256')
    assert checksum.read_text().split() == [ORDER_SHA, ORDER.name]
    assert a.sha(OLD_SUMMARY) == OLD_SHA
    history = a.cut.read(OLD_SUMMARY)
    assert history['build'] == 627 and history['correction'] == 3 and not history['proceed_11_3']
    build_path = ITEM / 'build627/build_status.json'
    build = a.cut.read(build_path)
    assert build['passed'] and all(build[key] == history[key] for key in FINGERPRINTS)
    seal_path = ITEM / 'scoped/build627/serial.json'
    seal = a.cut.read(seal_path)
    assert seal['passed'] and all(seal[key] == history[key] for key in FINGERPRINTS)
    lab_name = 'lab/LAB1_V2/fixture_status.json'
    lab_path = seal_path.parent / lab_name
    assert a.sha(lab_path) == seal['receipts'][lab_name]
    lab = a.cut.read(lab_path)
    assert lab['passed'] and lab['measurement']['passed']
    lab_rows = lab['measurement']['rows']
    lab_faces = int(a.expect_one(lab_rows, 'NATIVE3D_LOCAL_SURFACE')['faces_out'])
    input_cells = int(a.expect_one(lab_rows, 'FP2_A LAB_S')['cells_in'])
    assert lab_faces == 84_102 and FACE_LIMIT * 2 == lab_faces * 3

    bound = {str(path): a.sha(path) for path in (ORDER, checksum, OLD_SUMMARY, build_path, seal_path, lab_path,
                                                Path(prior.__file__).resolve())}
    results = {}
    for name, ranks in (('K1', 2), ('K2', 2), ('K3', 6)):
        old = history['results'][name]
        receipt_path = (ROOT / name / 'rank_check_receipt.json' if name != 'K2' else
                        ROOT.parent / 'build626/K2/rank_check_receipt.json')
        assert str(receipt_path) == old['receipt'] and a.sha(receipt_path) == old['receipt_sha256']
        receipt = a.cut.read(receipt_path)
        bound[str(receipt_path)] = old['receipt_sha256']
        for part, digest in receipt['artifacts'].items():
            assert a.sha(receipt_path.parent / part) == digest, part
        wait, owner = (receipt[field] for field in ('wait', 'owned_processes'))
        assert wait['terminal'] and owner['all_observed_terminal'] and receipt['ranks'] == ranks
        if name == 'K2':
            assert old['ended'] and receipt['ended'] and not receipt['surface']
            assert a.sha(prior.HISTORICAL) == old['history_sha256']
            bound[str(prior.HISTORICAL)] = old['history_sha256']
            results[name] = old
            continue

        data = receipt['by_rank']
        assert set(data) == {str(rank) for rank in range(ranks)} and receipt['surface']
        completion = prior.first_difference(data, ranks, True, input_cells, wait, owner['stop'])
        assert completion == receipt['completion_difference']
        surfaces = [row for rows in data.values() for row in rows['surface']]
        passes = [row for rows in data.values() for row in rows['passes']]
        totals = {n: sum(int(row['faces_out']) for row in surfaces if row['pass'] == n) for n in ('1', '2')}
        found = alarms(surfaces, totals, passes, name == 'K3')
        results[name] = {
            'ended': completion is None and not found, 'ranks': ranks, 'faces_out_sum': totals,
            'star_max': max(int(row['star_max']) for row in surfaces),
            'sweeps_distance_max': max(int(row['sweeps_distance']) for row in surfaces),
            'maximum_rank_pass_seconds': max(passes, key=lambda row: Decimal(row['seconds_pass']))['seconds_pass'],
            'wrapper_seconds': wait['elapsed_seconds'], 'completion_difference': completion, 'alarms': found,
            'receipt': str(receipt_path), 'receipt_sha256': old['receipt_sha256'],
            'correction3_state': old['state'], 'correction3_first_difference': old['first_difference'],
        }
    proceed = all(row['ended'] for row in results.values())
    return {**{name: history[name] for name in FINGERPRINTS}, 'correction': 4, 'lab_s_faces_out': lab_faces,
            'faces_out_sum_limit': FACE_LIMIT, 'default_gates_11_2_required': True, 'proceed_11_3': proceed,
            'status': 'proceed' if proceed else 'stopped_before_11_3', 'old_summary': str(OLD_SUMMARY),
            'bound_input_sha256': bound, 'results': results}


def verify():
    result = evaluate()
    assert a.cut.read(EVALUATION) == result, EVALUATION
    return result


def self_check():
    row = {'rank': '0', 'pass': '1', 'star_max': '1000', 'sweeps_distance': '39'}
    timing = {'rank': '0', 'pass': '1', 'seconds_pass': '600'}
    assert not alarms([row], {'1': FACE_LIMIT, '2': FACE_LIMIT}, [timing], True)
    assert [x['field'] for x in alarms([{**row, 'star_max': '1001', 'sweeps_distance': '40'}],
                                       {'1': FACE_LIMIT + 1, '2': FACE_LIMIT},
                                       [{**timing, 'seconds_pass': '600.001'}], True)] == [
                                           'star_max', 'sweeps_distance', 'faces_out_sum', 'seconds_pass']
    assert not alarms([row], {'1': FACE_LIMIT, '2': FACE_LIMIT}, [{**timing, 'seconds_pass': '601'}], False)


if __name__ == '__main__':
    parser = ArgumentParser(description=__doc__)
    parser.add_argument('--verify', action='store_true')
    args = parser.parse_args()
    self_check()
    if args.verify:
        result = verify()
    else:
        result = evaluate()
        a.sealed(EVALUATION, result)
    print(EVALUATION, result['status'])
