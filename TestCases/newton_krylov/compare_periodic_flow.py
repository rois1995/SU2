"""Compare converged native and Newton solutions on the existing 2D periodic sector."""
from pathlib import Path
import re

from compare_manufactured_flow import main
from report_state import option, sha


def sector_case():
    """Keep the sector physics and require every flow residual to converge in all three modes."""
    directory = Path(__file__).resolve().parents[1] / 'navierstokes/periodic2D'
    mesh, inlet = directory / 'sector.su2', directory / 'inlet.dat'
    config_path = directory / 'config.cfg'
    config = '\n'.join(line for line in config_path.read_text().splitlines()
                       if not re.match(r'^\s*INNER_ITER\s*=', line)) + '\n'

    # Retain the qualified native controls; Newton uses its existing bounded CFL adaptation.
    settings = {
        'MESH_FILENAME': str(mesh), 'INLET_FILENAME': str(inlet),
        'REF_DIMENSIONALIZATION': 'FREESTREAM_VEL_EQ_ONE', 'MGLEVEL': '0',
        'LINEAR_SOLVER_ITER': '50', 'LINEAR_SOLVER_ERROR': '1e-4',
        'ITER': '20000', 'CONV_STARTITER': '5',
        'CONV_FIELD': '(RMS_DENSITY,RMS_MOMENTUM-X,RMS_MOMENTUM-Y,RMS_ENERGY)',
        'CONV_RESIDUAL_MINVAL': '-10', 'OUTPUT_FILES': 'RESTART',
        'OUTPUT_WRT_FREQ': '20000', 'RESTART_FILENAME': 'restart_flow',
        'SCREEN_OUTPUT': '(INNER_ITER,RMS_RES)', 'HISTORY_OUTPUT': '(ITER,RMS_RES,LINSOL)',
        'NEWTON_ADAPT_CFL': 'YES', 'NEWTON_ADAPTIVE_FORCING': 'NO',
        'NEWTON_STEP_TOL': '1e-14',
    }
    for key, value in settings.items():
        config = option(config, key, value)
    points = int(re.search(r'^\s*NPOIN\s*=\s*(\d+)', mesh.read_text(), re.MULTILINE)[1])
    return {
        'name': '2D periodic sector with rotated momentum and second-order Roe',
        'config': config, 'points': points,
        'fields': {'flow': ['rms[Rho]', 'rms[RhoU]', 'rms[RhoV]', 'rms[RhoE]']},
        'raw_rms': 1e-10, 'normalized_rms': 1e-9, 'projection': 0,
        'state_tolerance': 1e-6, 'quantity_tolerance': 1e-6,
        'max_updates': 20000, 'wall_timeout': 600,
        'input_sha256': {str(path): sha(path) for path in [config_path, mesh, inlet]},
        'limits': 'FLOW alone has one group in both Newton modes. This sector does not qualify '
                  'periodic turbulence, transition, axis points or general performance gains.',
    }


if __name__ == '__main__':
    main(sector_case())
