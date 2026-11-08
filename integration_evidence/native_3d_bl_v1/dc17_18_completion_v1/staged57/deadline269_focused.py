"""Focused deterministic deadline risk checks using the existing pinned runner."""
from pathlib import Path
import os,sys
import run_cases
ROOT=Path(__file__).resolve().parent
for key in list(os.environ):
    if key.startswith('SU2_NATIVE_'):del os.environ[key]
os.environ.update(OMP_NUM_THREADS='1',OPENBLAS_NUM_THREADS='1',MKL_NUM_THREADS='1',OMPI_MCA_osc='pt2pt')
run_cases.CASES['deadline_serial']=(1,'[NativeDeadline3D]')
sys.argv=[str(ROOT/'run_cases.py'),'build269','deadline_serial']
run_cases.main()
