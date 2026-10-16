"""Compare periodic wall-bounded flow with --model SA, SA_BC, SST or LM (default LM)."""
import argparse
import sys

from compare_manufactured_flow import main
from compare_sa_flatplate import SA_CASE
from compare_sa_bc_flatplate import BC_CASE
from compare_sst_flatplate import SST_CASE
from compare_lm_flatplate import LM_CASE
from report_state import option


def periodic_case(model):
    """Extrude the established flat plate, retaining its model and wall boundary conditions."""
    base = {'SA': SA_CASE, 'SA_BC': BC_CASE, 'SST': SST_CASE, 'LM': LM_CASE}[model]
    config = base['config'].replace('RMS_MOMENTUM-Y,', 'RMS_MOMENTUM-Y,RMS_MOMENTUM-Z,')
    settings = {
        'MESH_FORMAT': 'BOX', 'MESH_BOX_SIZE': '9,9,3', 'MESH_BOX_LENGTH': '1,1,0.2',
        'MARKER_PERIODIC': '(z_minus,z_plus,0,0,0,0,0,0,0,0,0.2)',
        # SU2's 3D AoA rotates velocity in x-z, parallel to the y=0 wall.
        'AOA': '5', 'SIDESLIP_ANGLE': '0', 'SCREEN_OUTPUT': '(INNER_ITER,RMS_RES)',
        # The weaker 2D stop leaves observable endpoint differences after extrusion.
        'CONV_RESIDUAL_MINVAL': '-12',
    }
    for key, value in settings.items():
        config = option(config, key, value)
    fields = dict(base['fields'], flow=['rms[Rho]', 'rms[RhoU]', 'rms[RhoV]', 'rms[RhoW]', 'rms[RhoE]'])
    return dict(base, config=config, dimensions=3, points=243, fields=fields, wall_timeout=600,
                raw_rms=1e-12, normalized_rms=1e-11,
                name=f'243-point spanwise-periodic flow/{model} flat plate with 5-degree crossflow',
                limits='Coarse generated crossflow boundary layer with spanwise translation. '
                       'All active residuals must reach -12; no axis, rotation, production accuracy or speedup claim. '
                       'Select a fixture with --model SA, SA_BC, SST or LM.')


if __name__ == '__main__':
    # The shared runner retains ownership of all launch, measurement and validation arguments.
    selector = argparse.ArgumentParser(add_help=False)
    selector.add_argument('--model', choices=['SA', 'SA_BC', 'SST', 'LM'], default='LM')
    selected, remaining = selector.parse_known_args()
    sys.argv[1:] = remaining
    main(periodic_case(selected.model))
