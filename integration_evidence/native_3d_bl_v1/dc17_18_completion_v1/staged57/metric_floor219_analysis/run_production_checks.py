"""Reuse the pinned case runner without modifying its production campaign definitions."""
import sys
from pathlib import Path
ROOT=Path('/media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated/integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57')
sys.dont_write_bytecode=True
sys.path.insert(0,str(ROOT))
import run_cases
run_cases.CASES.update({
    'metric_floor_serial':(1,'[SensorMetricFloor]'),
    'metric_floor_related':(1,'[SensorMetricFloor],[GoalMetric],[MetricRobustness],Sharp wall points*,Isotropic metric at sharp wall corners,Boundary-layer metric of*,Boundary-layer metric intersected with*'),
    'metric_floor_mpi6':(6,'[SensorMetricFloor],[GoalMPI],MPI adaptation: corner metric across the partitions,Native full tensor gradation includes complexity and crosses MPI partitions in 2D and 3D'),
})
run_cases.main()
