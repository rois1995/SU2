"""Copy independently auditable adaptation evidence, leaving intermediate raw output in place."""
import argparse
import hashlib
import json
import re
import shutil
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
TOOLS = ('integration_evidence/audit_rae_unsteady.py', 'integration_evidence/audit_native_unsteady.py',
         'integration_evidence/audit_native_composite_rae.py', 'integration_evidence/audit_native_frozen_case.py',
         'integration_evidence/airfoil_reference_audit.py', 'integration_evidence/audit_native_bl.py',
         'integration_evidence/frozen_field_audit.py', 'integration_evidence/audit_native_profile_accounting.py',
         'TestCases/adaptation/capability/capcheck.py', 'TestCases/adaptation/custom_sensors/run_cases.py')


def selected_files(case):
    cfg = (case / 'run.cfg').read_text()
    files = {case / 'run.cfg', case / 'input.su2', case / 'solver.log', case / 'run_evidence.json'}
    if (case / 'frozen_sensor.csv').exists():
        files.update(case / name for name in ('frozen_sensor.csv', 'native_frozen_adapted.su2', 'frozen_sensor_source_flow.vtu'))
        files.update(case.glob('native_frozen_*.csv'))
    else:
        def setting(name):return int(re.search(r'^' + name + r'\s*=\s*(\d+)', cfg, re.M)[1])
        freq, steps = setting('ADAP_FREQ'), setting('TIME_ITER')
        restart = re.search(r'^RESTART_SOL\s*=\s*YES\s*$', cfg, re.M)
        start = setting('RESTART_ITER') if restart else 0
        assert start % freq == 0, 'Only complete window-boundary restart campaigns supported'
        for first in range(start + freq, steps, freq):
            mesh = case / f'mesh_{first:05d}.su2'
            if not mesh.exists():mesh = case / f'mesh_{first:05d}.cgns'
            files.update((mesh, case / (mesh.name + '.native_ref')))
            for step in (first - 2, first - 1):
                files.update((case / f'flow_{step:05d}.vtu', case / f'solution_{step:05d}.dat'))
        files.update((case / f'solution_{steps-1:05d}.dat', case / f'flow_{steps-1:05d}.vtu'))
        if restart:
            files.update(case / f'solution_{start-offset:05d}.dat' for offset in (1, 2))
    files.update(p for p in case.iterdir() if p.is_file() and
                 (p.suffix in ('.json', '.txt') or (p.suffix == '.csv' and (p.name.startswith('history') or p.name.startswith('native_'))) or p.name.endswith('.native_ref') or 'reject' in p.name))
    missing = sorted(str(path.name) for path in files if not path.is_file())
    return sorted(files), missing


def collect(case, destination):
    case = case.resolve(); destination = destination.resolve()
    assert not destination.exists(), 'Preserve previous exported evidence'
    destination.mkdir(parents=True)
    paths, missing = selected_files(case)
    copied = {}
    for path in paths:
        if not path.is_file():continue
        assert not path.is_symlink(), path
        target = destination / path.name
        shutil.copy2(path, target)
        copied[target.name] = dict(bytes=target.stat().st_size, sha256=hashlib.sha256(target.read_bytes()).hexdigest())
    result = dict(status='COMPLETE_SELECTED_EVIDENCE' if not missing else 'INCOMPLETE_SELECTED_EVIDENCE',
                  source=str(case), collector_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(), missing=missing, files=copied,
                  scope='Original donor snapshots at each adaptation and both transported histories, all adapted/rejected meshes and references, final state, logs/config/provenance. Raw intermediate outputs remain at source; nothing deleted.')
    (destination / 'collection_manifest.json').write_text(json.dumps(result, indent=2) + '\n')
    tools = destination.parent.parent / 'tools'
    for name in TOOLS:
        target = tools / name; target.parent.mkdir(parents=True, exist_ok=True)
        source = ROOT / name
        if target.exists():assert target.read_bytes() == source.read_bytes(), 'Do not mix audit versions in one export'
        else:shutil.copy2(source, target)
    return result


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('case', type=Path); parser.add_argument('destination', type=Path)
    args = parser.parse_args()
    row = collect(args.case, args.destination)
    print(row['status'], len(row['files']), 'files,', sum(item['bytes'] for item in row['files'].values()), 'bytes; missing:', row['missing'])
    raise SystemExit(0 if not row['missing'] else 1)
