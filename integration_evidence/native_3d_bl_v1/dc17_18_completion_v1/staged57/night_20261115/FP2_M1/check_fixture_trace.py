"""Check the focused FP2-M1 observer against each production query entry."""

from argparse import ArgumentParser
import json
from pathlib import Path
import re


MINIMUM = 20
ANSI = re.compile(r'\x1b\[[0-?]*[ -/]*[@-~]')
INPUTS = (
    r'tolerance=(?P<tolerance>[0-9a-f]{16}) '
    r'support=(?P<support>(?:[0-9]+,)+) '
    r'triangles=(?P<triangles>(?:[0-9a-f]{16},){9};)'
)
EXPECT = re.compile(r'FP2_M1 EXPECT ordinal=(?P<ordinal>[0-9]+) ' + INPUTS)
ENTRY = re.compile(r'NATIVE_SURFACE_QUERY authority=(?P<authority>0x[0-9a-f]+) budget=\S+ start=[0-9]+ ' + INPUTS)
MARK = re.compile(r'FP2_M1 TRACE_(BEGIN|END) ordinal=([0-9]+)')
SUMMARY = {'SUPPORT', 'FALLBACK', 'REPLICA', 'OBSERVER', 'MEMO'}


def compare_log(lines, minimum=MINIMUM):
    expected = {}
    used = set()
    authorities = set()
    active = None
    observed = []
    matched = 0
    errors = []
    rows = {name: [] for name in sorted(SUMMARY)}
    for number, raw in enumerate(lines, 1):
        line = ANSI.sub('', raw).strip()
        if line.startswith('FP2_M1 EXPECT '):
            match = EXPECT.fullmatch(line)
            if match is None:
                errors.append({'line': number, 'error': 'malformed expected input'})
                continue
            values = match.groupdict()
            ordinal = int(values.pop('ordinal'))
            if ordinal in expected:
                errors.append({'line': number, 'ordinal': ordinal, 'error': 'duplicate expected input'})
            expected[ordinal] = values
            continue
        if line.startswith('FP2_M1 TRACE_'):
            match = MARK.fullmatch(line)
            if match is None:
                errors.append({'line': number, 'error': 'malformed trace marker'})
                continue
            marker, ordinal_text = match.groups()
            ordinal = int(ordinal_text)
            if marker == 'BEGIN':
                if active is not None or ordinal not in expected or ordinal in used:
                    errors.append({'line': number, 'ordinal': ordinal, 'error': 'unexpected begin marker'})
                active, observed = ordinal, []
                continue
            if active != ordinal or len(observed) != 1 or ordinal not in expected:
                errors.append({'line': number, 'ordinal': ordinal, 'active': active, 'entry_count': len(observed),
                               'error': 'end marker does not close exactly one production entry'})
            else:
                actual = observed[0]
                differences = {key: {'expected': value, 'actual': actual[key]}
                               for key, value in expected[ordinal].items() if value != actual[key]}
                if differences:
                    errors.append({'line': number, 'ordinal': ordinal, 'error': 'input mismatch',
                                   'differences': differences})
                else:
                    matched += 1
            active = None
            used.add(ordinal)
            continue
        if active is not None and line.startswith('NATIVE_SURFACE_QUERY authority='):
            match = ENTRY.fullmatch(line)
            if match is None:
                errors.append({'line': number, 'ordinal': active, 'error': 'malformed production entry'})
            else:
                values = match.groupdict()
                authorities.add(values.pop('authority'))
                observed.append(values)
            continue
        if line.startswith('FP2_M1 '):
            fields = line.split()
            if fields[1] in SUMMARY:
                rows[fields[1]].append(dict(field.split('=', 1) for field in fields[2:] if '=' in field))

    if active is not None or set(expected) != used:
        errors.append({'error': 'unterminated or unmatched marker', 'active': active,
                       'unused_expected': sorted(set(expected) - used)})
    if matched < minimum:
        errors.append({'error': 'too few identical inputs', 'required': minimum, 'matched': matched})
    if len(authorities) != 1:
        errors.append({'error': 'forwarded entries do not share one production authority',
                       'authority_count': len(authorities)})
    for kind, found in rows.items():
        if not found:
            errors.append({'error': 'required fixture row missing', 'kind': kind})
    if len(rows['OBSERVER']) != 1 or rows['OBSERVER'][0].get('status') != 'ok':
        errors.append({'error': 'fixture observer did not qualify'})
    stage1 = [row for row in rows['SUPPORT'] if row.get('stage') == '1']
    if len(stage1) != 1 or not positive(stage1[0].get('proved')):
        errors.append({'error': 'no stage 1 proof with reduced support'})
    if len(rows['REPLICA']) != 1 or not positive(rows['REPLICA'][0].get('attempted')):
        errors.append({'error': 'no fixture replica attempt'})
    return {'passed': not errors, 'matched': matched, 'required': minimum, 'errors': errors, 'rows': rows}


def positive(value):
    return value is not None and value.isascii() and value.isdecimal() and int(value) > 0


def self_check():
    inputs = f'tolerance={0:016x} support=7,2, triangles=' + f'{0:016x},' * 9 + ';'
    expect = 'FP2_M1 EXPECT ordinal=1 ' + inputs
    begin = 'FP2_M1 TRACE_BEGIN ordinal=1'
    entry = 'NATIVE_SURFACE_QUERY authority=0xabc budget=0xdef start=0 ' + inputs
    end = 'FP2_M1 TRACE_END ordinal=1'
    support = 'FP2_M1 SUPPORT rank=0 pass=1 stage=1 proved=1'
    logs = [entry, expect, begin, 'NATIVE_SURFACE_QUERY tolerance=0.1 model=0 mesh=1 facets=2', entry, end,
            support, 'FP2_M1 FALLBACK rank=0 pass=1 group=proved_1 calls=1',
            'FP2_M1 REPLICA attempted=1 agree=1 disagree=0 resource=0 threw=0',
            'FP2_M1 OBSERVER rank=0 pass=1 status=ok', 'FP2_M1 MEMO rank=0 pass=1 at=start authority=pass']
    assert compare_log(logs, 1)['passed']
    logs[4] = entry.replace('support=7,2,', 'support=2,7,')
    result = compare_log(logs, 1)
    assert not result['passed'] and result['errors'][0]['differences']['support']['actual'] == '2,7,'


def main():
    parser = ArgumentParser(description=__doc__)
    parser.add_argument('log', type=Path, nargs='?')
    parser.add_argument('--self-check', action='store_true')
    args = parser.parse_args()
    if args.self_check:
        self_check()
        return 0
    if args.log is None:
        parser.error('log is required')
    with args.log.open(encoding='utf-8', errors='replace') as stream:
        report = compare_log(stream)
    print(json.dumps(report, indent=2))
    return 0 if report['passed'] else 1


if __name__ == '__main__':
    raise SystemExit(main())
