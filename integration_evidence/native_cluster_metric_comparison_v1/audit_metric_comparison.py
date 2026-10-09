"""Run version-matching saved-mesh audits serially; never launch CFD or MPI."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def run(baseline, candidate, assessment, output):
    if output.exists():
        raise FileExistsError('Preserve existing audit output: ' + str(output))
    evidence = json.loads(assessment.read_text())
    prepared = []
    names = set()
    for row in evidence['cases']:
        name = row['case']
        if Path(name).name != name or name in ('.', '..') or name in names or row['version'] not in ('baseline', 'candidate'):
            raise ValueError('Invalid/duplicate case name: ' + name)
        names.add(name)
        case = (baseline if row['version'] == 'baseline' else candidate) / 'cases' / name
        tools = case / 'tools' if row['version'] == 'candidate' else baseline / 'tools'
        expected = [(case / name, digest) for name, digest in row['data_sha256'].items()]
        expected += [(tools / name, digest) for name, digest in row['tool_sha256'].items()]
        expected.append((case / 'collection_manifest.json', row['manifest_sha256']))
        for path, digest in expected:
            if not path.is_file() or sha(path) != digest:
                raise ValueError('Evidence/tool hash mismatch: ' + str(path))
        prepared.append((row, case, tools))
    if not prepared:
        raise ValueError('No cases in assessment')
    output.mkdir(parents=True)
    shutil.copy2(assessment, output / 'assessment.json')
    results = dict(status='RUNNING', assessment_sha256=sha(assessment), checker_sha256=sha(Path(__file__)), cases=[],
                   scope='Independent saved mesh/metric/reference/height and available histories. No CFD run, scaling benchmark or aerodynamic convergence check.')
    environment = dict(os.environ, OMP_NUM_THREADS='1', OPENBLAS_NUM_THREADS='1',
                       MKL_NUM_THREADS='1', PYTHONDONTWRITEBYTECODE='1')
    for row, case, tools in prepared:
        print('Auditing saved case:', case.resolve(), flush=True)
        destination = output / 'cases' / case.name
        destination.mkdir(parents=True)
        frozen = row['kind'].startswith('frozen_')
        script = 'audit_native_frozen_case.py' if frozen else 'audit_rae_unsteady.py'
        report_name = 'independent_frozen_metric_audit.json' if frozen else 'independent_rae_unsteady_audit.json'
        record = dict(case=case.name, version=row['version'], folder=str(case.resolve()), status='FAIL')
        try:
            with tempfile.TemporaryDirectory(prefix='native-saved-audit-') as temporary:
                copied = Path(temporary) / case.name
                shutil.copytree(case, copied, ignore=shutil.ignore_patterns('tools', 'independent_*.json'))
                command = [sys.executable, str((tools / 'integration_evidence' / script).resolve()), str(copied)]
                if frozen:
                    command += ['rae_' + row['kind'][7:], '--self-contained']
                process = subprocess.run(command, env=environment, stdout=subprocess.PIPE,
                                         stderr=subprocess.PIPE, universal_newlines=True)
                (destination / 'stdout.txt').write_text(process.stdout)
                (destination / 'stderr.txt').write_text(process.stderr)
                record['exit_code'] = process.returncode
                if (copied / report_name).exists():
                    shutil.copy2(copied / report_name, destination / report_name)
                    record['audit'] = json.loads((copied / report_name).read_text())
                if process.returncode != 0 or record.get('audit', {}).get('status') != 'PASS':
                    raise RuntimeError('Independent audit failed; inspect case stdout/stderr')
                record['status'] = 'PASS'
        except Exception as error:
            record['error'] = str(error)
        results['cases'].append(record)
        (output / 'validation.json').write_text(json.dumps(results, indent=2) + '\n')
    results['status'] = 'PASS' if all(r['status'] == 'PASS' for r in results['cases']) else 'FAIL'
    (output / 'validation.json').write_text(json.dumps(results, indent=2) + '\n')
    print(results['status'], len(results['cases']), 'saved cases;', output.resolve(), flush=True)
    return 0 if results['status'] == 'PASS' else 1


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ('baseline_results', 'candidate_results', 'assessment', 'output'):
        parser.add_argument(name, type=Path)
    args = parser.parse_args()
    sys.exit(run(args.baseline_results.resolve(), args.candidate_results.resolve(),
                 args.assessment.resolve(), args.output.resolve()))
