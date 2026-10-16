"""Compare converged native and Newton flow in the existing pipe sector with rotation-axis nodes."""
from pathlib import Path
import re

from compare_manufactured_flow import main
from report_state import option, sha


def axis_case():
    """Retain the pipe physics and require all five flow equations to converge."""
    directory = Path(__file__).resolve().parents[1] / 'navierstokes/periodic3D_axis'
    mesh, config_path = directory / 'wedge.su2', directory / 'config.cfg'
    config = config_path.read_text()
    settings = {
        'MESH_FILENAME': str(mesh), 'REF_DIMENSIONALIZATION': 'FREESTREAM_VEL_EQ_ONE', 'MGLEVEL': '0',
        'LINEAR_SOLVER_ITER': '80', 'LINEAR_SOLVER_ERROR': '1e-6',
        'ITER': '20000', 'CONV_STARTITER': '5', 'CONV_RESIDUAL_MINVAL': '-12',
        'CONV_FIELD': '(RMS_DENSITY,RMS_MOMENTUM-X,RMS_MOMENTUM-Y,RMS_MOMENTUM-Z,RMS_ENERGY)',
        'OUTPUT_FILES': 'RESTART', 'OUTPUT_WRT_FREQ': '20000', 'RESTART_FILENAME': 'restart_flow',
        'SCREEN_OUTPUT': '(INNER_ITER,RMS_RES)', 'HISTORY_OUTPUT': '(ITER,RMS_RES,LINSOL,AERO_COEFF)',
        'CFL_ADAPT': 'NO', 'NEWTON_ADAPT_CFL': 'YES', 'NEWTON_ADAPTIVE_FORCING': 'NO',
        'NEWTON_STEP_TOL': '1e-14',
    }
    for key, value in settings.items():
        config = option(config, key, value)
    return dict(name='3D pipe sector with 30-degree rotation and axis nodes', config=config, dimensions=3,
        points=int(re.search(r'^\s*NPOIN\s*=\s*(\d+)', mesh.read_text(), re.MULTILINE)[1]),
        fields={'flow': ['rms[Rho]', 'rms[RhoU]', 'rms[RhoV]', 'rms[RhoW]', 'rms[RhoE]']},
        raw_rms=1e-12, normalized_rms=1e-11, projection=0, state_tolerance=1e-6, quantity_tolerance=1e-6,
        # No imposed swirl means zero axial torque; compare its roundoff absolutely, not relative to zero.
        quantity_absolute_tolerance={'moment_z': 1e-12}, max_updates=20000, wall_timeout=600,
        input_sha256={str(path): sha(path) for path in [mesh, config_path]},
        limits='Coarse laminar sector with one circumferential cell. FLOW is one group in both Newton modes; '
               'this does not qualify turbulent axis endpoints, mesh accuracy or general speedup.')


if __name__ == '__main__':
    main(axis_case())
