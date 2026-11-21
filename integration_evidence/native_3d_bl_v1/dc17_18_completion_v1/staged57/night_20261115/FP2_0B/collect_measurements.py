"""Verify FP2-0b receipts and render the measurement section of the report."""

from argparse import ArgumentParser
import json
from pathlib import Path
import sys

sys.dont_write_bytecode = True
from run_measurements import (HERE, LEDGERS, MEMORY_MIB, checked_inputs, cut, row_values, run_b_reason, sha, verified)

FP2_0_RECEIPT = HERE.parent / 'FP2_0/verified_measurements.json'
FP2_0_SHA = '7d1fc06d1c32c90e308794f55a61707da91d9e3c72c5b8de8f9e8774bd71dec2'


def row(report, name):
    values = row_values(report['rows'], name)
    assert len(values) <= 1, (report['case'], name)
    return values[0] if values else {}


def compact(value):
    if value is None:
        return '—'
    if isinstance(value, (dict, list)):
        return json.dumps(value, separators=(',', ':'), ensure_ascii=False)
    if isinstance(value, bool):
        return 'yes' if value else 'no'
    return str(value)


def table(headers, rows):
    def line(values):
        return '| ' + ' | '.join(compact(value).replace('|', r'\|').replace('\n', ' ') for value in values) + ' |'

    print(line(headers))
    print(line(['---'] * len(headers)))
    for values in rows:
        print(line(values))
    print()


def baseline():
    assert sha(FP2_0_RECEIPT) == FP2_0_SHA, FP2_0_RECEIPT
    prior = {receipt['case']: receipt for receipt in cut.read(FP2_0_RECEIPT)['cases']}
    return row(prior['split'], 'P_entities'), row(prior['bounds'], 'OUT1_load')


def counts_by_entity(values):
    return {tuple(value['entity']): (value['faces'], value['points']) for value in values}


def size_pair(counts):
    return f'{counts[0]:,} / {counts[1]:,}' if counts is not None else None


def write_entities(current, feature, prior, output):
    if not current:
        return
    print('### Entity counts and remaining faces\n')
    keyed = [counts_by_entity(values) for values in
             (current['IN1'], prior['PD'], current['PD'], output['entities'])]
    left = {tuple(report['entity']): report for report in current['PD_above_0p7_tol']}
    entity_rows = []
    for entity in sorted(set().union(*keyed)):
        remaining = left.get(entity, {})
        entity_rows.append([entity, *(size_pair(counts.get(entity)) for counts in keyed),
                            remaining.get('faces'), remaining.get('reasons')])
    entity_rows.append(['All',
                        size_pair((current['IN1_faces'], current['IN1_points'])),
                        size_pair((prior['PD_faces'], prior['PD_points'])),
                        size_pair((current['PD_faces'], current['PD_points'])),
                        size_pair((output['faces'], output['points'])),
                        sum(part['faces'] for part in left.values()), None])
    table(['Entity', 'IN1 faces / points', 'FP2-0 PD faces / points', 'Current faces / points', 'OUT1 faces / points',
           'Above 0.7 tol', 'Reason counts'], entity_rows)
    print(f"Feature traces were {compact(feature.get('traces'))} in IN1, {current['traces']} after this run, "
          'and 733 in OUT1.\n')


def timing_entry(value, name):
    calls = value.get(f'{name}_calls')
    seconds = value.get(f'{name}_seconds')
    average = value.get(f'{name}_us_per_call')
    if calls is None or seconds is None:
        return None
    return f'{calls} / {seconds:.6g} / {average:.6g}' if average is not None else f'{calls} / {seconds:.6g} / —'


def split_row(value):
    return [
        value['step'], value.get('pass') or 'Total', value.get('marked'), value.get('walk_splits'),
        value.get('feature_splits'), value.get('blocked_by_pass_conflict'), value.get('refusals'),
        value.get('feature_dominated'), value.get('unique_feature_dominated'), value.get('fold_all'),
        value.get('all_feature_edges'), value.get('seconds'), timing_entry(value, 'propose'),
        timing_entry(value, 'rebuild'), timing_entry(value, 'strict_children'),
    ]


def write_passes(report):
    passes = row_values(report['rows'], 'P_PASS')
    totals = [value for name in ('P_L', 'P_D') if (value := row(report, name))]
    if not passes and not totals:
        return
    print('### Split passes\n')
    print('The timing columns show calls, elapsed seconds, and microseconds per call. '
          'The dominated column lists all occurrences, followed by first occurrences.\n')
    table(['Step', 'Pass', 'Marked', 'Walk splits', 'Feature splits', 'Blocked', 'Refusal counts',
           'Dominated', 'First dominated', 'Fold all', 'All feature', 'Seconds', 'Feature proposal',
           'Trace rebuild', 'Strict children'], (split_row(value) for value in [*passes, *totals]))
    for value in totals:
        print(f"Step {value['step']} entered {value['passes']} nonempty passes across {value['scans']} scans. "
              f"First refusal reasons by category were `{compact(value['first_errors'])}`.\n")


STRICT_FIELDS = ('faces', 'contains_measured', 'contains_failed', 'float_sampled', 'float_above_half_tol',
                 'float_above_seven_tenths_tol', 'float_above_eight_tenths_tol', 'float_above_tol',
                 'classified', 'unclassified', 'classify_above_tol')
PAIRED_FIELDS = ('both_above', 'float_only_above', 'classify_only_above', 'both_within', 'unpaired')


def write_strict(strict, report, link):
    if not strict:
        return
    print('### Strict local comparisons\n')
    rows, paired = [], []
    for code, label in (('IN1', 'IN1'), ('PD_NEW', 'New faces'), ('PD', 'Final faces')):
        group = strict[code]
        stats = [('All', group['all']), *((value['entity'], value['counts']) for value in group['entities'])]
        for entity, counts in stats:
            rows.append((label, entity, *(counts[field] for field in STRICT_FIELDS)))
            for key, limit in (('paired_at_tol', '1.0 tol'), ('paired_at_float_limit', '0.7 tol')):
                if key in counts:
                    paired.append((label, entity, limit, *(counts[key][field] for field in PAIRED_FIELDS)))
    table(['Dataset', 'Entity', 'Faces', 'Entity checks', 'Entity failures', 'Float samples', 'Above 0.5 tol',
           'Above 0.7 tol', 'Above 0.8 tol', 'Above tol', 'Classified', 'Unclassified', 'Classify above tol'], rows)
    print('Paired comparisons use the listed float limit and a Classify cutoff of 1.0 tol.\n')
    table(['Dataset', 'Entity', 'Float limit', 'Both above', 'Float alone above', 'Classify alone above',
           'Both within', 'Unpaired'], paired)
    notes = {key: value for key, value in strict.items() if key not in ('IN1', 'PD_NEW', 'PD')}
    print(f"Strict timing, sampling coverage and provenance appear in `{compact(notes)}`.\n")
    failure_count = strict['PD']['all']['contains_failed']
    diagnoses = row_values(report['rows'], 'P_ENTITY_FAILURE')
    if report['case'] == 'A':
        print(f"Saved {len(diagnoses)} full per-corner diagnoses for {failure_count} final faces failing "
              f"`ContainsEntityFace`. Coordinates, admissibility, facet entity matches, orientation, and "
              f"float dot products are recorded in `P_ENTITY_FAILURE` rows. {link}\n")
    else:
        print(f"The strict-children run has {failure_count} final faces failing `ContainsEntityFace`.\n")


def write_proof(p3, p4):
    print('### Bulk constructor and two-sided proof\n')
    if p3:
        print(f"P3 accepted the surface **{compact(p3.get('accepted'))}**, took {compact(p3.get('seconds'))} s "
              f"and reached {compact(p3.get('modeled_peak_bytes'))} modeled bytes. Its input was "
              f"{compact(p3.get('points'))} points, {compact(p3.get('faces'))} faces and "
              f"{compact(p3.get('traces'))} traces.\n")
        if p3.get('accepted') is False:
            witness = {key: value for key, value in p3.items()
                       if key not in ('accepted', 'seconds', 'modeled_peak_bytes', 'points', 'faces', 'traces')}
            print(f"P3 refusal and first witness are `{compact(witness)}`.\n")
    if p4:
        notes = {key: value for key, value in p4.items() if key != 'entities'}
        print(f"P4 status, proof timings and budget observations are `{compact(notes)}`.\n")
    entities = p4.get('entities', ())
    if not entities:
        return
    table(['Entity', 'Mesh to model', 'Model to mesh', 'Accepted', 'Seconds', 'Classify above tol',
           'Classified / total'],
          ([value['entity'], value['mesh_to_model'], value['model_to_mesh'], value['accepted'], value.get('seconds'),
            value['classify_above_tol'], size_pair((value['classified'], value['total_faces']))]
           for value in entities))
    print('The per-entity timing cells are unavailable when a production interface times the direction as a whole.\n')
    witnesses = []
    for value in entities:
        for item in value['witnesses']:
            face = item['face']
            witnesses.append([value['entity'], face['anchor'], face['vertices'], face['float_sample'],
                              face['classify'], item['magnitude'], item.get('magnitude_error'), face['coordinates']])
    if witnesses:
        print('The first 20 `Classify` witnesses per entity are shown below.\n')
        table(['Entity', 'Anchor', 'Vertices', 'Float sample', 'Classify', 'Magnitude', 'Error', 'Coordinates'],
              witnesses)


def write_run(report, prior, output):
    run = report['case']
    print(f"## Run {run}\n")
    peak = report['process_peak_kib']
    rss = f'{peak:,} KiB ({peak / 1024:,.2f} MiB)' if peak is not None else 'unavailable'
    mode = 'on' if run == 'B' else 'off'
    print(f"`strict_children` was {mode}. The serial test ran for {report['elapsed_seconds']:.3f} s, returned "
          f"{report['returncode']} and recorded a peak process RSS of {rss} against the {MEMORY_MIB:,} MiB limit.\n")
    if report['process_peak_within_limit'] is False:
        print('The measured process peak exceeded the resource limit.\n')
    case = Path(report['case_path']).relative_to(HERE)
    link = f"[Sealed receipt](../{case}/measurement.json)"
    files = ['test_outer.log', *LEDGERS]
    links = [link, f"[Launch provenance](../{case}/provenance.json)"]
    links.extend(f'[{name}.gz](../{case}/{name}.gz)' for name in files if name in report['artifacts'])
    print(' · '.join(links) + '\n')
    missing_ledgers = [name for name in LEDGERS if name not in report['artifacts']]
    if missing_ledgers:
        print(f"Ledger files unavailable at termination were `{compact(missing_ledgers)}`.\n")

    exception_rows = [(item['name'], item['line'], item['value']) for item in report['rows']
                      if item['name'] == 'P_budget_stop' or 'error' in item['value']]
    if exception_rows:
        table(['Stopped or unavailable measurement', 'Log line', 'Message'], exception_rows)
    if report['missing_rows'] or report['malformed_rows'] or report['returncode']:
        print(f"The wrapper reported `{report['status']}`. Missing rows were "
              f"`{compact(report['missing_rows'])}`, with {len(report['malformed_rows'])} malformed lines.\n")
    feature = row(report, 'P_feature_start')
    startup = [(name, value) for name in ('IN1_load', 'IN1_setup', 'IN1_sensor', 'P_feature_start', 'P_initial_samples')
               if (value := row(report, name))]
    if startup:
        print('### Input and prototype initialization\n')
        table(['Measurement', 'Observation'], startup)
    write_passes(report)
    write_entities(row(report, 'P_entities'), feature, prior, output)
    write_strict(row(report, 'P_STRICT'), report, link)
    write_proof(row(report, 'P3'), row(report, 'P4'))


def main():
    parser = ArgumentParser(description=__doc__)
    parser.add_argument('cases', nargs='*', type=Path)
    parser.add_argument('--json', type=Path, help='save verified case receipts as a JSON document')
    args = parser.parse_args()
    inputs = checked_inputs()
    cases = args.cases or [HERE / 'cases/A']
    if not args.cases and (HERE / 'cases/B/measurement.json').is_file():
        cases.append(HERE / 'cases/B')
    cases = [case.resolve() for case in cases]
    assert all(HERE in case.parents for case in cases), cases
    reports = [verified(case, inputs) for case in cases]
    assert [report['case'] for report in reports] in (['A'], ['A', 'B']), reports
    first = reports[0]
    reason = run_b_reason(first)
    if len(reports) == 2:
        second = reports[1]
        assert not reason and second['test_binary_sha256'] == first['test_binary_sha256']
        assert second['provenance']['run_a'] == {
            'path': first['case_path'], 'receipt_sha256': first['receipt_sha256'],
        }
    if args.json:
        cut.save(args.json, {'cases': reports, 'prior_reference_sha256': FP2_0_SHA})
    prior, output = baseline()
    print(f"# FP2-0b measurement on build {first['build']}\n")
    print('The test-only fixture measured feature-edge splits, the strict validating constructor, '
          'and its conditional two-sided proof. [Whole-item source check](../source_check.md)\n')
    for report in reports:
        write_run(report, prior, output)
    if len(reports) == 1:
        print('## Run B activation\n')
        if reason:
            print(f"Run B was skipped. {reason}\n")
        else:
            print('Run A recorded the specified orientation refusal and saved its failing-face diagnostics. '
                  'Run B measurements are pending.\n')


if __name__ == '__main__':
    main()
