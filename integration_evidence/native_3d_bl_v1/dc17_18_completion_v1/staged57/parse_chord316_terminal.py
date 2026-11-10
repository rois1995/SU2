"""Extract terminal-only cubic chord evidence; never launches computation."""
import hashlib
import json
import math
import re
import sys
from decimal import Decimal
from pathlib import Path


def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def extract(text):
    clean = re.sub(r'\x1b\[[0-?]*[ -/]*[@-~]', '', text)
    marker = 'Cubic BL chord replay:'
    if marker not in clean:
        return None
    tail = clean.split(marker, 1)[1]
    match = re.search(r'\{[^{}]*\}', tail)
    if not match:
        raise ValueError('Incomplete chord JSON')
    raw = match.group()
    compact = re.sub(r'\s+', '', raw)
    value = json.loads(compact, parse_float=Decimal, parse_int=Decimal)
    return raw, value


def chord_values(value):
    values = [*value['a'], *value['b'], value['band_ratio'], value['fade_bands'], value['transition_core']]
    assert len(values)==9 and all(math.isfinite(float(v)) for v in values)
    assert value['fade_bands']==int(value['fade_bands'])
    if 'metric_padding' in value:
        padding=value['metric_padding']
        assert len(padding)==3
        padding=[sys.float_info.max if v is None else float(v) for v in padding]
        assert all(math.isfinite(v) and v>=0 for v in padding)
        values.extend(padding)
    return values


def self_check():
    raw, value = extract('\x1b[31mCubic BL chord replay: {"fresh_total": 9e-9,\n "adjacent_total": 2e-9}')
    assert value['fresh_total'] == Decimal('9e-9')
    assert value['fresh_total'] < Decimal.from_float(1e-8)
    assert extract('passed') is None
    base={'a':[0,0,0],'b':[1,1,1],'band_ratio':2,'fade_bands':4,'transition_core':.0001}
    assert len(chord_values(base))==9
    assert len(chord_values(dict(base,metric_padding=[0,1e-16,2e-16])))==12
    assert chord_values(dict(base,metric_padding=[None,0,0]))[-3]==sys.float_info.max
    try: chord_values(dict(base,metric_padding=[-1,0,0]))
    except AssertionError: pass
    else: raise AssertionError('negative padding accepted')


def main():
    if sys.argv[1:] == ['--self-check']:
        self_check()
        return
    assert len(sys.argv) in (3,4), 'usage: parser ATTEMPT NEW_OUTPUT_DIRECTORY [REFERENCE]'
    attempt, output = map(lambda p: Path(p).resolve(), sys.argv[1:3])
    status_path = attempt / 'status.json'
    terminal_path = attempt / 'terminal_handles_actual.json'
    status = json.loads(status_path.read_text())
    assert status['state'] == 'terminal' and status['pins_verified'] and status['mapped_runtime_verified']
    assert json.loads(terminal_path.read_text())['all_owned_handles_terminal']
    command_path = attempt / 'command.json'
    command = json.loads(command_path.read_text())
    environment = command['environment']
    config = Path(environment['SU2_NATIVE_FROZEN_CONFIG'])
    reference = Path(sys.argv[3]).resolve() if len(sys.argv)==4 else Path(environment['SU2_NATIVE_BL_REFERENCE'])
    pins_path = attempt / 'pins.json'
    pins = json.loads(pins_path.read_text())
    for path in (config, reference):
        assert sha(path) == pins[str(path.resolve())], path
    log = attempt / 'run.log'
    result = extract(log.read_text(errors='strict'))
    provenance = {'source_attempt': str(attempt), 'exit_code': status['exit_code'],
                  'pins': {str(p): sha(p) for p in (log, status_path, terminal_path, command_path, pins_path, config, reference, Path(__file__).resolve())},
                  'config': str(config), 'reference': str(reference),
                  'scope': 'Terminal diagnostic extraction only; totals do not uniquely establish cause.'}
    output.mkdir(exist_ok=False)
    if result is None:
        provenance['result'] = 'pass_no_chord_error' if status['exit_code'] == 0 else 'failure_without_chord_error'
    else:
        raw, value = result
        tolerance = Decimal.from_float(1e-8)
        provenance.update(result='chord_error_extracted', binary64_tolerance=str(tolerance),
                          diagnostic_values={k: v if isinstance(v, list) else str(v) for k, v in value.items()},
                          comparisons={k: {'le_tolerance': value[k] <= tolerance, 'decimal': str(value[k])}
                                       for k in ('fresh_total', 'adjacent_total')})
        values = chord_values(value)
        (output / 'raw_chord.json').write_text(raw + '\n')
        (output / 'chord.txt').write_text(' '.join(format(float(v), '.17g') for v in values) + '\n')
        provenance['output_sha256'] = {p.name: sha(p) for p in output.iterdir()}
    (output / 'provenance.json').write_text(json.dumps(provenance, indent=2, default=str) + '\n')


if __name__ == '__main__':
    main()
