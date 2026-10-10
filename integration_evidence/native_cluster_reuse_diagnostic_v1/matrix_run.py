"""Strict same-executable OFF/BOTH frozen matrix, with both original workload directions."""
import importlib.util
import sys
import prepare
import analyze
from run import runner

spec = importlib.util.spec_from_file_location('matrix_strict_analysis', prepare.PREVIOUS_PACKAGE / 'analyze.py')
strict = importlib.util.module_from_spec(spec);spec.loader.exec_module(strict)


def details(case, row):
    return analyze.details(case, dict(row,role='OFF' if row['role']=='control' else 'BOTH'))


def configure():
    runner.CHECKPOINT = prepare.ROOT / 'build-native/reuse_matrix_checkpoint.json'
    runner.RESULT_PREFIX = 'reuse_matrix'
    runner.KINDS = ('frozen_euler_to_bl','frozen_bl_to_euler')
    runner.VARIANTS = ((4,'NO'),(4,'YES'),(3,'YES'),(2,'YES'))
    runner.ROLES = ('control','profile')
    runner.ROLE_BINARIES = {'control':'profile','profile':'profile'}
    runner.ROLE_ENVIRONMENTS = {role:dict(SU2_NATIVE_REUSE=mode,SU2_NATIVE_REUSE_AUDIT='NO')
                                for role,mode in (('control','OFF'),('profile','BOTH'))}
    runner.CASE_DETAILS = details
    runner.compare = strict.compare
    runner.REQUIRE_IDENTITY = True
    runner.SCOPE = 'Same tested job582344 executable: control=OFF, profile=BOTH. Strict accepted mesh/tensor byte identity and operation counts plus all numerical audits required. Two frozen workload directions and four working-partition modes. Historical executables retained but not used as numerical references. No CFD timesteps.'


if __name__=='__main__':
    configure();sys.exit(runner.main())
