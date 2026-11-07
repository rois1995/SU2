"""Use the existing validation runner for the connected quadratic-conditioning change."""
import sys
from pathlib import Path
import run_cases
checks={
 'condition254_serial':(1,'[HessianReliability],[MetricRobustness],[SensorMetricFloor]'),
 'condition254_mpi6':(6,'[HessianReliability],[SensorMetricFloor],[GoalMPI],Native full tensor gradation includes complexity and crosses MPI partitions in 2D and 3D'),
}
assert len(sys.argv)==2 and sys.argv[1] in checks
check=sys.argv[1];run_cases.CASES[check]=checks[check]
sys.argv=[str(Path(run_cases.__file__).resolve()),'build254',check]
run_cases.main()
