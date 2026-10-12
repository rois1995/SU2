"""Verify diagnostic-only MPI qualification, exact numerical records and source versions."""
from itertools import product
from pathlib import Path
import hashlib
import importlib.util
import json
import re
import sys

BASE = Path(__file__).resolve().parent
ROOT = BASE.parents[1]
sha = lambda p: hashlib.sha256(p.read_bytes()).hexdigest()


def receipt(folder):
    record = json.loads((folder/'validation.json').read_text())
    assert record['status'] == 'PASS', folder.name
    for key in ('source_drift','object_drift','archive_drift','tool_drift'):
        assert not record.get(key, []), (folder.name, key)
    for name, expected in record['artifact_sha256'].items():
        assert sha(folder/name) == expected, (folder.name, name)
    return record


def main():
    combinations_checked = 0
    for workers in (1,2,4):
        for rows in product(list(product((False,True), repeat=3)), repeat=workers):
            for coarsening in (False,True):
                previous = not any(invalid or missing or (coarsening and pending)
                                   for invalid,missing,pending in rows)
                codes = [3 if missing else 2 if invalid else 1 if coarsening and pending else 0
                         for invalid,missing,pending in rows]
                assert previous == (max(codes) == 0)
                combinations_checked += 1
    build_path = BASE/'completion_diagnostic_build_v1/validation.json'
    build = json.loads(build_path.read_text())
    assert build['status'] == 'PASS' and not build['source_drift']
    assert all(sha(ROOT/name) == digest for name,digest in build['source_sha256'].items())
    cases = []
    for ranks in (1,2,4):
        folder = BASE/('completion_diagnostic_mpi%d_v1' % ranks)
        current = receipt(folder)
        prior_folder = BASE/('box_filter_mpi%d_v1' % ranks)
        prior = receipt(prior_folder)
        assert current['binary_sha256'] == build['binary_sha256']
        assert current['build_validation_sha256'] == sha(build_path)
        assert len(re.findall(r'All tests passed \(\d+ assertions in 97 test cases\)',(folder/'case.stdout').read_text())) == ranks
        assert json.loads((folder/'independent_audit.json').read_text())['status'] == 'PASS'
        assert json.loads((folder/'distributed_audit.json').read_text())['status'] == 'PASS'
        matched = {}
        for name in current['artifact_sha256']:
            path = folder/name
            if path.suffix != '.json':
                continue
            data = json.loads(path.read_text())
            if not isinstance(data,dict) or not ({'nodes','cells'} <= set(data) or {'donor','accepted'} <= set(data)):
                continue
            assert name in prior['artifact_sha256'], (ranks,name,'missing prior numerical record')
            expected = prior['artifact_sha256'][name]
            assert sha(prior_folder/name) == expected and sha(path) == expected, (ranks,name,'numerical drift')
            matched[name] = expected
        assert len(matched) >= 20, (ranks,'incomplete numerical comparison')
        cases.append(dict(ranks=ranks, validation_sha256=sha(folder/'validation.json'),
                          prior_validation_sha256=sha(prior_folder/'validation.json'),
                          exact_numerical_records_sha256=matched,
                          independent_audit_sha256=sha(folder/'independent_audit.json'),
                          distributed_audit_sha256=sha(folder/'distributed_audit.json')))
    package = BASE/'completion_diagnostic_package_checks_v1'
    assert 'Ran 32 tests' in (package/'core.stderr').read_text() and 'OK' in (package/'core.stderr').read_text()
    assert 'Ran 14 tests' in (package/'comparison.stderr').read_text() and 'OK' in (package/'comparison.stderr').read_text()
    comparison = ROOT/'integration_evidence/native_cluster_3d_query_comparison_v1'
    sys.path.insert(0,str(comparison))
    spec = importlib.util.spec_from_file_location('diagnostic_comparison_prepare',comparison/'prepare.py')
    prepare = importlib.util.module_from_spec(spec);spec.loader.exec_module(prepare)
    full = {side:dict(source_sha256=dict(record['source_sha256']),build_options=[dict(name='enable-tests',value=True)])
            for side,record in [('baseline',json.loads((BASE/'box_filter_build_v2/validation.json').read_text())),('candidate',build)]}
    assert prepare.matched_sources(full) == dict(baseline='box',candidate='box_diagnostics')
    result = dict(status='PASS', checker_sha256=sha(Path(__file__).resolve()),
                  build_validation_sha256=sha(build_path),binary_sha256=build['binary_sha256'],
                  boolean_completion_equivalence_combinations=combinations_checked,cases=cases,
                  package_tests=dict(core=32,comparison=14),
                  source_pair='box->box_diagnostics',
                  scope='Actual rebuilt complete driver and97-case MPI1/2/4 plus fresh exact distributed and actual bridge/state/SU2CGNS/MMG audits. '
                  'Saved numerical records exactly match the prior qualified binary. Boolean global completion equivalence is checked separately; '
                  'different cause messages do not change acceptance, criteria, defaults, operator selection or communication in the engine. '
                  'No new whole n8/strong/thin/restart campaign, performance/scaling qualification or fix of the512work limit.')
    (BASE/'COMPLETION_DIAGNOSTIC_VALIDATION.json').write_text(json.dumps(result,indent=2)+'\n')
    print('PASS:97cases at MPI1/2/4; exact numerical records', [len(c['exact_numerical_records_sha256']) for c in cases],
          ';8336boolean combinations;46package checks; full compiled-source pair')


if __name__ == '__main__':
    main()
