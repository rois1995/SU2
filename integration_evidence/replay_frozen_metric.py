"""Replay a serial pre-BL tensor field/scale using frozen_metric_probe.patch.

Usage: python3 integration_evidence/replay_frozen_metric.py BUILD_DIRECTORY
Historical runner for pre-rebase source 3b320bfd6b, not the integrated BL policy.
Requires the v3 frozen RAE inputs/configuration and NumPy for the final audit.
The patch is investigation-only: restore CSolver.cpp and rebuild after use.
No remeshing or flow iterations run. Output is metric-isolation/noise-replay.
"""
from pathlib import Path
import hashlib, json, os, re, subprocess, sys, time
import numpy as np

build = Path(sys.argv[1]).resolve()
binary = build / 'SU2_CFD/src/SU2_CFD'
root = build / 'metric-isolation/noise-replay'
if '--capture-all' in sys.argv[2:]:
    root = build / 'metric-isolation/noise-upstream-construction'
root.mkdir(parents=True, exist_ok=True)
configuration = (build / 'rae-native-metric-v3/quadratic_least_squares_noise-mpi1/run.cfg').read_text()
frozen = root / 'capture/pre_bl.rank0'
receipt = {'scope': 'Investigation-only experimental nodal BL policy; fixed serial pre-BL tensors and scale, zero flow/remesh iterations.',
           'binary_sha256': hashlib.sha256(binary.read_bytes()).hexdigest(), 'runs': []}
probes = [('capture', 1, 80, True)]
probes += [(f'capture-mpi{rank}', rank, 80, True) for rank in (2, 4)]
for sweeps, ranks in [(0, (1, 2, 4)), (1, (1, 2, 4)), (10, (1, 4)), (80, (1, 2, 4))]:
    probes += [(f'fixed-{sweeps}', rank, sweeps, True) for rank in ranks]
probes += [('fixed-no-bl', rank, 80, False) for rank in (1, 4)]
if '--capture-mpi' in sys.argv[2:]:
    receipt = json.loads((root / 'runs.json').read_text())
    probes = [probe for probe in probes if probe[0].startswith('capture-mpi')]
if '--capture-all' in sys.argv[2:]:
    probes = [probe for probe in probes if probe[0].startswith('capture')]

for label, rank, sweeps, bl in probes:
    capture = label.startswith('capture')
    case = root / (label if capture else f'{label}-mpi{rank}')
    case.mkdir(exist_ok=True)
    cfg = configuration
    if not bl:
        cfg = '\n'.join(line for line in cfg.splitlines() if not line.startswith('ADAP_BL_')) + '\n'
    (case / 'run.cfg').write_text(cfg)
    environment = {**os.environ, 'OMP_NUM_THREADS': '1', 'SU2_METRIC_PROBE_SWEEPS': str(sweeps)}
    environment['SU2_METRIC_PROBE_DUMP' if capture else 'SU2_METRIC_PROBE_REPLAY'] = str(case / 'pre_bl' if capture else frozen)
    command = ['timeout', '--kill-after=5s', '300s', 'mpirun', '--host', 'localhost:4', '-np', str(rank), str(binary), 'run.cfg']
    started = time.monotonic()
    with (case / 'solver.log').open('w') as log:
        result = subprocess.run(command, cwd=case, env=environment, stdout=log, stderr=subprocess.STDOUT)
    entry = {'label': label, 'ranks': rank, 'sweeps_cap': sweeps, 'boundary_layer': bl,
             'elapsed_seconds': time.monotonic() - started, 'exit_code': result.returncode,
             'command': command, 'configuration': cfg,
             'probe_environment': {key: value for key, value in environment.items() if key.startswith('SU2_METRIC_PROBE_')}}
    log = (case / 'solver.log').read_text()
    for key, pattern in [('gradation', r'Native metric gradation: (.*)'), ('work', r'Native metric work: (.*)'),
                         ('blocked', r'Native BL gradation: (.*)'), ('scale', r'Frozen metric replay: (.*)')]:
        match = re.search(pattern, log)
        if match:
            entry[key] = match[1]
    for name in ['solver.log', 'fields.csv']:
        if (case / name).exists():
            entry[name + '_sha256'] = hashlib.sha256((case / name).read_bytes()).hexdigest()
    receipt['runs'].append(entry)
    (root / 'runs.json').write_text(json.dumps(receipt, indent=2) + '\n')
    print(json.dumps({key: value for key, value in entry.items() if key not in ('configuration', 'command', 'probe_environment')}), flush=True)
    if result.returncode:
        raise SystemExit(result.returncode)

def load(case):
    with (case / 'fields.csv').open() as stream:
        names = next(stream).strip().replace('"', '').split(',')
    data = np.loadtxt(case / 'fields.csv', delimiter=',', skiprows=1)
    data = data[np.argsort(data[:, 0])]
    index = {name.strip(): k for k, name in enumerate(names)}
    metric = np.zeros((len(data), 2, 2))
    metric[:, 0, 0] = data[:, index['Metric_XX']]
    metric[:, 0, 1] = metric[:, 1, 0] = data[:, index['Metric_XY']]
    metric[:, 1, 1] = data[:, index['Metric_YY']]
    return metric, data[:, [k for name, k in index.items() if name.startswith('Hessian_')]]

receipt['frozen_input_sha256'] = hashlib.sha256(frozen.read_bytes()).hexdigest()
receipt['mpi_comparisons'] = {}
for label in ['fixed-0', 'fixed-1', 'fixed-10', 'fixed-80', 'fixed-no-bl']:
    if not (root / f'{label}-mpi1/fields.csv').exists():
        continue
    base, hessian = load(root / f'{label}-mpi1')
    comparisons = {}
    for rank in (2, 4):
        if not (root / f'{label}-mpi{rank}').exists():
            continue
        other, other_hessian = load(root / f'{label}-mpi{rank}')
        relative = np.linalg.norm(base - other, axis=(1, 2)) / np.linalg.norm(base, axis=(1, 2))
        comparisons[str(rank)] = {'max_relative_metric_difference': float(relative.max()),
                                 'worst_global_point': int(relative.argmax()),
                                 'max_absolute_hessian_difference': float(np.max(np.abs(hessian - other_hessian)))}
    receipt['mpi_comparisons'][label] = comparisons
receipt['construction_comparisons'] = {}
for prefix in ['pre_bl', 'pre_bl.initial']:
    serial = root / f'capture/{prefix}.rank0'
    if not serial.exists():
        continue
    base = np.loadtxt(serial, skiprows=1)
    base = base[np.argsort(base[:, 0])]
    reference_parameters = [float(x) for x in serial.read_text().splitlines()[0].split()[1:]]
    comparisons = {}
    for rank in (2, 4):
        files = list((root / f'capture-mpi{rank}').glob(prefix + '.rank*'))
        if not files:
            continue
        other = np.concatenate([np.loadtxt(path, skiprows=1) for path in files])
        other = other[np.argsort(other[:, 0])]
        assert np.array_equal(base[:, 0], other[:, 0])
        parameters = [float(x) for x in files[0].read_text().splitlines()[0].split()[1:]]
        weights = np.array([1, 2 ** .5, 1])
        error = np.linalg.norm((base[:, 1:] - other[:, 1:]) * weights, axis=1) / np.linalg.norm(base[:, 1:] * weights, axis=1)
        comparisons[str(rank)] = {'max_relative_tensor_difference': float(error.max()), 'worst_global_point': int(error.argmax()),
                                 'serial_parameters': reference_parameters, 'mpi_parameters': parameters}
    receipt['construction_comparisons'][prefix] = comparisons
(root / 'runs.json').write_text(json.dumps(receipt, indent=2) + '\n')
print(json.dumps(receipt['mpi_comparisons'], indent=2), flush=True)
