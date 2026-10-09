#!/usr/bin/env python3
"""Generate controlled mixed/triangular meshes and run the native serial operator probe."""
import argparse
import csv
import io
import json
import os
from pathlib import Path
import subprocess
import time

from run_convergence import affinity_cpu_activity
from run_fixes import configure, digest
from run_mms import mesh_text


def mixed_mesh_text(n, aspect, curved):
    lines = mesh_text(n, aspect, curved).splitlines()
    old_count = int(lines[1].split('=')[1])
    elements = []
    for j in range(n):
        for i in range(n):
            a = i + (n + 1) * j
            b, c, d = a + 1, a + n + 2, a + n + 1
            if (i + j) % 2 == 0:
                elements.append(f'9 {a} {b} {c} {d} {len(elements)}')
            else:
                elements += [f'5 {a} {b} {c} {len(elements)}', f'5 {a} {c} {d} {len(elements)+1}']
    return '\n'.join([lines[0], f'NELEM= {len(elements)}'] + elements + lines[2 + old_count:]) + '\n'


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('probe', type=Path)
    parser.add_argument('output', type=Path)
    args = parser.parse_args()
    binary = args.probe.resolve(strict=True)
    if not binary.is_file() or not os.access(binary, os.X_OK):
        parser.error('Probe must be executable')
    output = args.output.resolve()
    output.mkdir(parents=True, exist_ok=False)
    root = Path(__file__).resolve().parents[2]
    template = (root / 'TestCases/mms/fvm_navierstokes/lam_mms_roe.cfg').read_text()
    results = []
    for shape, aspect, curved in [('regular', 1, False), ('thin', 1e4, False), ('curved', 1e4, True)]:
        for n in (12, 24):
            for mixed in (True, False):
                while os.getloadavg()[0] > len(os.sched_getaffinity(0)) and affinity_cpu_activity() >= .75:
                    print('Waiting for assigned CPUs', flush=True)
                    time.sleep(30)
                name = f'{shape}{n}_{"mixed" if mixed else "tri"}'
                case = output / name
                case.mkdir()
                mesh = case / 'mesh.su2'
                mesh.write_text((mixed_mesh_text if mixed else mesh_text)(n, aspect, curved))
                config = case / 'case.cfg'
                config.write_text(configure(template, {'MESH_FILENAME': str(mesh), 'MGLEVEL': '0'}))
                env = dict(os.environ, OMP_NUM_THREADS='1', OPENBLAS_NUM_THREADS='1')
                process = subprocess.run(['nice', '-n', '19', str(binary), str(config)], cwd=case, env=env,
                                         text=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE, timeout=120)
                (case / 'probe.csv').write_text(process.stdout)
                (case / 'stderr.log').write_text(process.stderr)
                if process.returncode:
                    raise RuntimeError(f'{name}: native probe/self-check failed; inspect {case}')
                summary = dict(item.split('=') for item in process.stdout.split('# ', 1)[1].strip().split(', '))
                row = dict(name=name, n=n, aspect=aspect, mixed=mixed, binary_sha256=digest(binary),
                           mesh_sha256=digest(mesh), config_sha256=digest(config),
                           metrics={key: float(value) for key, value in summary.items()},
                           fields=list(csv.DictReader(io.StringIO(process.stdout.split('#', 1)[0]))))
                results.append(row)
                (output / 'results.json').write_text(json.dumps(results, indent=2) + '\n')
                print(name, summary, flush=True)


if __name__ == '__main__':
    main()
