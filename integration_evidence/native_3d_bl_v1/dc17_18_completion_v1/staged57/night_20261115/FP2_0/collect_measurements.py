"""Verify sealed FP2-0 reports and print a table of the raw measurement rows."""

from argparse import ArgumentParser
import json
from pathlib import Path
import sys

sys.dont_write_bytecode = True
from run_measurements import EXPECTED, HERE, ORDER_SHA, SELECTORS, cut, inputs, measurements, missing_rows, peak_kib, sha


def verified(case, pinned_inputs):
    receipt_file = case / 'measurement.json'
    provenance_file = case / 'provenance.json'
    receipt, provenance = map(cut.read, (receipt_file, provenance_file))
    assert sha(provenance_file) == receipt['provenance_sha256'], case
    assert provenance['case'] == receipt['case'] and provenance['case'] in SELECTORS, case
    assert provenance['build'] == receipt['build'] and provenance['order_sha256'] == ORDER_SHA, case
    assert receipt['test_binary_sha256'] == provenance['test_binary_sha256'], case
    assert provenance['inputs'] == pinned_inputs, case
    for path, digest in (('build_status', 'build_status_sha256'), ('source_pins', 'source_pins_sha256'),
                         ('fp0_manifest', 'fp0_manifest_sha256')):
        assert sha(Path(provenance[path])) == provenance[digest], (case, path)
    build = cut.read(Path(provenance['build_status']))
    assert build['passed'] and (build['test_binary_sha256'], build['source_pins_sha256']) == (
        receipt['test_binary_sha256'], provenance['source_pins_sha256']), case
    for name, digest in receipt['artifacts'].items():
        assert sha(case / name) == digest, (case, name)
    wait = cut.read(case / 'test_wait.json')
    assert wait['terminal'] and wait['controller'] == receipt['controller'] and cut.terminal(wait['controller']), case
    assert wait['returncode'] == receipt['returncode'] and wait['elapsed_seconds'] == receipt['elapsed_seconds'], case
    command = cut.read(case / 'command.json')
    assert command['ranks'] == provenance['ranks'] == 1 and command['seconds_limit'] == provenance['seconds_limit'] == 5400
    assert command['selector'] == provenance['selector'], case
    assert f"[{SELECTORS[provenance['case']]}]" in command['selector'], case
    assert command['argv'][-2:] == [provenance['test_binary'], command['selector']], case
    assert all(f"{key}={value['path']}" in command['argv'] for key, value in pinned_inputs.items()), case
    with (case / 'test_outer.log').open(errors='replace') as stream:
        rows, malformed = measurements(stream)
    assert sha(case / 'test_outer.log') == receipt['log_sha256'], case
    assert rows == receipt['rows'] and malformed == receipt['malformed_rows'], case
    missing = missing_rows(provenance['case'], rows, provenance['splits_only'])
    assert missing == receipt['missing_rows'], case
    if (case / 'time.txt').is_file():
        assert peak_kib(case / 'time.txt') == receipt['process_peak_kib'], case
    return dict(receipt, case_path=str(case), provenance=provenance, receipt_sha256=sha(receipt_file))


def cell(value):
    return str(value).replace('|', r'\|').replace('\r', ' ').replace('\n', ' ')


def table(reports):
    print('| Case | Measurement | Seconds | Observation |')
    print('| --- | --- | ---: | --- |')
    for report in reports:
        for row in report['rows']:
            result = row['value']
            status = result.get('status', result.get('error', result.get('stop', 'reported')))
            if 'pass' in result:
                status = f"pass {result['pass']}, {status}"
            seconds = result.get('seconds', '—')
            print(f"| {report['case']} | {row['name']} | {seconds} | {cell(status)} |")
        if report['missing_rows'] or report['malformed_rows'] or report['returncode']:
            names = ', '.join(report['missing_rows']) or 'none'
            note = f"{report['status']}, missing rows {names}, malformed lines {len(report['malformed_rows'])}"
            print(f"| {report['case']} | Run termination | {report['elapsed_seconds']:.3f} | {cell(note)} |")
        rss = report['process_peak_kib']
        print(f"| {report['case']} | Process peak | — | {rss if rss is not None else 'unavailable'} KiB |")


def main():
    parser = ArgumentParser(description=__doc__)
    parser.add_argument('cases', nargs='*', type=Path)
    parser.add_argument('--json', type=Path, help='save verified rows and case provenance')
    args = parser.parse_args()
    pinned_inputs = inputs()
    cases = args.cases or [HERE / 'cases' / name for name in EXPECTED]
    reports = [verified(case.resolve(), pinned_inputs) for case in cases]
    if args.json:
        with args.json.open('x') as stream:
            json.dump({'cases': reports}, stream, indent=2, allow_nan=False)
            stream.write('\n')
    table(reports)


if __name__ == '__main__':
    main()
