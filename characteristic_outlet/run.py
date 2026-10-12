"""Run the four fixtures: python3 run.py /path/to/SU2_CFD /path/to/output."""
import argparse
import csv
import json
import math
from pathlib import Path
import shutil
import subprocess

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('binary', type=Path)
parser.add_argument('output', type=Path)
args = parser.parse_args()
binary = args.binary.resolve(strict=True)
root = Path(__file__).resolve().parent
output = args.output.resolve()
output.mkdir(parents=True, exist_ok=True)
shutil.copy2(root / 'mesh.su2', output / 'mesh.su2')
results = []
for name in ('air_duct', 'MDM_duct', 'air_nozzle', 'MDM_nozzle'):
    case = output / name
    case.mkdir()  # Refuse to overwrite previous runs.
    shutil.copy2(root / name / 'case.cfg', case / 'case.cfg')
    with (case / 'solver.log').open('w') as log:
        run = subprocess.run(['mpirun', '-n', '2', str(binary), 'case.cfg'], cwd=case,
                             stdout=log, stderr=subprocess.STDOUT)
    rows = []
    if (case / 'history.csv').exists():
        with (case / 'history.csv').open() as f:
            rows = [{k.strip().strip('"'): v.strip() for k, v in r.items()} for r in csv.DictReader(f)]
    results.append(dict(case=name, exit_code=run.returncode, last=rows[-1] if rows else None,
                        finite_history=bool(rows) and all(math.isfinite(float(v)) for r in rows for v in r.values())))
    (output / 'results.json').write_text(json.dumps(results, indent=2) + '\n')
    print(results[-1], flush=True)
