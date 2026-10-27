"""Audit compiled Catch selections against current source names; preserve unique receipts."""
import hashlib
import json
import os
import re
import subprocess
import sys
from pathlib import Path
from run_cases import CASES, BINARY, REPO

ROOT = Path(__file__).resolve().parent
COMMON = REPO / 'UnitTests/Common/adaptation'
CFD = REPO / 'UnitTests/SU2_CFD/adaptation'
NEW = [COMMON / ('CNative' + name + '_tests.cpp') for name in
       ('Atlas3D', 'CurvedAtlas3D', 'CanonicalTrace3D', 'TraceCoverage3D',
        'ReferenceTriangulation3D', 'GeometryBackend3D')]
OTHER = [COMMON / 'CNativeAffineReference_tests.cpp', COMMON / 'CNativeDistributed3D_tests.cpp',
         CFD / 'CNativeImport3D_tests.cpp', CFD / 'CNativeRemesher3D_tests.cpp', CFD / 'DistributedTransfer_tests.cpp',
         CFD / 'CConservativeTransfer_tests.cpp']
BROAD_TAGS = {'NativeCurvedTransaction3D', 'NativeRuntime3D', 'NativeCurvedRuntime3D',
              'NativePolicyVeto3D', 'NativeBLPolicyRuntime3D', 'NativeCurvedLifecycle3D',
              'NativeAtlasCoordinatedPublication3D', 'NativeMappedRuntime3D', 'NativeConstructionSweep3D'}
CASE = re.compile(r'TEST_CASE\s*\(\s*"([^"\n]+)"\s*,\s*"([^"\n]*)"', re.M)


def sha(path):
    digest = hashlib.sha256()
    with path.open('rb') as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b''): digest.update(chunk)
    return digest.hexdigest()


def main():
    if len(sys.argv) != 2 or not re.fullmatch(r'build[1-9][0-9]*', sys.argv[1]):
        raise SystemExit('usage: audit_compiled_filters.py buildN')
    build = sys.argv[1]
    paths = NEW + OTHER + [Path(__file__).resolve(), ROOT / 'run_cases.py', BINARY]
    pins = {str(p): sha(p) for p in paths}
    tests = {p: CASE.findall(p.read_text()) for p in NEW + OTHER}
    if any(not tests[p] for p in NEW): raise RuntimeError('No literal test cases found in a required file')
    common = {name for p in NEW for name, _ in tests[p]}
    common |= {name for p in OTHER for name, tags in tests[p] if '[NativeSingleChartCertificate3D]' in tags or '[NativeAcceptedFaceCertificate3D]' in tags}
    prefixes = ('Distributed barycentric transfer:', 'Convex clipping:', 'Distributed conservative projection:',
                'Conservative kernels:', 'Distributed conservative kernels:', 'Transfer admissibility:',
                'Distributed conservative transfer:')
    broad = {name for p in OTHER for name, tags in tests[p]
             if any('[' + tag + ']' in tags for tag in BROAD_TAGS)
             or name.startswith(prefixes) or name == 'Single precision transfer contract'}
    serial_extra = {name for name, _ in tests[CFD / 'CConservativeTransfer_tests.cpp']
                    if name.startswith(('Conservative projection:', 'Conservative transfer:'))}
    if not broad: raise RuntimeError('No supplemental tests found')
    specific = {'single_certificate_checks': 'NativeSingleChartCertificate3D', 'mapped_runtime': 'NativeMappedRuntime3D', 'mapped_work_trace': 'NativeMappedRuntime3D', 'm6_stage': 'NativeFrozenVolume3D', 'cost_checks': 'NativeOverlappingAtlas3D',
                'workflow_solved6': 'NativeSolvedEuler3D', 'workflow_primitive6': 'NativePrimitiveRuntime3D',
                'workflow_resolved_m1': 'NativeResolvedEuler3D', 'workflow_resolved_m2': 'NativeResolvedEuler3D',
                'workflow_resolved_m4': 'NativeResolvedEuler3D'}
    required = {case: {name for p in NEW + OTHER for name, tags in tests[p] if '[' + tag + ']' in tags or (case=='single_certificate_checks' and '[NativeAcceptedFaceCertificate3D]' in tags)}
                for case, tag in specific.items()}
    if any(not names for names in required.values()): raise RuntimeError('A specific case has no matching source test')
    env = os.environ.copy(); env.update(OMP_NUM_THREADS='1', OPENBLAS_NUM_THREADS='1', MKL_NUM_THREADS='1')
    for case in ('serial_full', 'mpi6', 'atlas_checks', *specific):
        output = ROOT / (build + '_' + case + '_filter_audit.json')
        listing = ROOT / (build + '_' + case + '_selected_tests.log')
        if output.exists() or listing.exists(): raise RuntimeError('Audit receipt already exists: ' + str(output))
        argv = ['nice', '-n', '19', 'timeout', '30', str(BINARY), CASES[case][1], '--list-test-names-only']
        result = subprocess.run(argv, capture_output=True, text=True, env=env)
        names = result.stdout.splitlines(); expected = required[case] if case in specific else common | (broad if case != 'atlas_checks' else set())
        if case == 'serial_full': expected |= serial_extra
        changed = [p for p,h in pins.items() if sha(Path(p)) != h]
        missing = sorted(expected - set(names))
        receipt = {'argv':argv, 'exit_code':result.returncode, 'selected_lines':len(names),
                   'expected_cases':len(expected), 'missing':missing, 'source_pins':pins,
                   'binary_sha':pins[str(BINARY)], 'helper_sha':pins[str(Path(__file__).resolve())],
                   'changed':changed, 'stderr':result.stderr}
        listing.write_text(result.stdout); output.write_text(json.dumps(receipt, indent=2)+'\n')
        # Catch returns the number of listed tests, modulo the process exit-code range.
        if missing or changed or result.returncode != len(names) % 256:
            raise RuntimeError('Filter audit failed: ' + str(output))
        print(case, len(names), 'selected;', len(expected), 'required')


if __name__ == '__main__': main()
