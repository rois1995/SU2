"""Reuse the audited compact runner for profiled baseline/candidate reconstruction."""
import importlib.util
from pathlib import Path
import sys
import prepare

SHARED = prepare.ROOT / prepare.SHARED
sys.path.append(str(SHARED))
spec = importlib.util.spec_from_file_location('reuse_runner', SHARED / 'run.py')
runner = importlib.util.module_from_spec(spec)
spec.loader.exec_module(runner)
runner.PROFILE_CONTROL = True
runner.RESULT_PREFIX = 'reconstruction_reuse'
runner.UNITS += ',[NativeMesh2D]'
runner.SCOPE = 'Profiled job582199 control versus bounded score/metric reuse candidate. Identical numerical output and operation counts precede cost interpretation. Frozen-only; no CFD timesteps.'

if __name__ == '__main__':
    sys.exit(runner.main())
