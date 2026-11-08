"""Bounded source-faithful cleanup diagnosis using the existing pinned runner."""
from pathlib import Path
import json,os,sys,tempfile
import run_cases
R=Path(__file__).resolve().parent
assert json.loads((R/'build272_terminal_handles.json').read_text())['all_owned_handles_terminal']
for k in list(os.environ):
    if k.startswith('SU2_NATIVE_'):del os.environ[k]
os.environ.update(OMP_NUM_THREADS='1',OPENBLAS_NUM_THREADS='1',MKL_NUM_THREADS='1',OMPI_MCA_osc='pt2pt')
os.environ['SU2_NATIVE_FROZEN_CONFIG']=str(R/'m6_runs/euler_build262_compact_capture_mach_only_real_cfd50k/post_refinement_cleanup_inputs_v1/run.cfg')
run_cases.CASES['current_production']=(1,'[NativeMetricCleanup3D]')
sys.argv=[str(R/'run_cases.py'),'build272','current_production']
run_cases.main()
