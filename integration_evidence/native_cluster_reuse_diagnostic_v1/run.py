"""Seven sequential frozen runs; numerical audits remain mandatory and identity differences are diagnostic evidence."""
import importlib.util
import sys
import prepare
import analyze

SHARED = prepare.ROOT / prepare.SHARED
sys.path.append(str(SHARED))
spec = importlib.util.spec_from_file_location('reuse_diagnostic_runner', SHARED / 'run.py')
runner = importlib.util.module_from_spec(spec);spec.loader.exec_module(runner)
runner.PROFILE_CONTROL = True
runner.RESULT_PREFIX = 'reuse_diagnostic'
runner.KINDS = ('frozen_euler_to_bl',)
runner.VARIANTS = ((4, 'NO'),)
runner.ROLES = ('control', 'archived', 'OFF', 'SCORES', 'METRIC', 'BOTH', 'AUDIT')
runner.ROLE_BINARIES = {role:'profile' for role in runner.ROLES[2:]}
runner.BASE_ENVIRONMENT = dict(SU2_NATIVE_REUSE='BOTH', SU2_NATIVE_REUSE_AUDIT='NO')
runner.ROLE_ENVIRONMENTS = {role:dict(SU2_NATIVE_REUSE='BOTH' if role=='AUDIT' else role,
                                    SU2_NATIVE_REUSE_AUDIT='YES' if role=='AUDIT' else 'NO')
                            for role in runner.ROLES[2:]}
runner.MPI_EXPORTS = ('SU2_NATIVE_REUSE', 'SU2_NATIVE_REUSE_AUDIT')
runner.REQUIRE_IDENTITY = False
runner.CASE_DETAILS = analyze.details
runner.UNITS += ',[NativeMesh2D]'
runner.SCOPE = 'Same executable OFF/SCORES/METRIC/BOTH and bounded fresh-hit AUDIT; archived baseline and previous candidate retained. Every numerical audit must pass. DIAGNOSTIC_COMPLETE is not byte identity or performance PASS. No CFD timesteps.'

if __name__ == '__main__': sys.exit(runner.main())
