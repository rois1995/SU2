"""Compare native and Newton flow/flamelet solves using a synthetic affine LUT."""
from pathlib import Path

from compare_manufactured_flow import main
from compare_sa_flatplate import SA_CASE
from report_state import option, sha


table = Path(__file__).resolve().parents[2] / 'UnitTests/SU2_CFD/fluid/newton_flamelet.drg'
config = SA_CASE['config']
for key, value in {
        'SOLVER': 'INC_NAVIER_STOKES', 'KIND_TURB_MODEL': 'NONE', 'CONV_NUM_METHOD_FLOW': 'FDS',
        'INC_DENSITY_MODEL': 'FLAMELET', 'INC_DENSITY_INIT': '1', 'INC_TEMPERATURE_INIT': '340.5',
        'INC_VELOCITY_INIT': '(1,0.02,0)', 'INC_NONDIM': 'DIMENSIONAL',
        'FLUID_MODEL': 'FLUID_FLAMELET', 'KIND_SCALAR_MODEL': 'FLAMELET', 'INTERPOLATION_METHOD': 'LUT',
        'FILENAMES_INTERPOLATOR': str(table),
        'CONTROLLING_VARIABLE_NAMES': '(ProgressVariable,EnthalpyTot)',
        'CONTROLLING_VARIABLE_SOURCE_NAMES': '(PVSource,NULL)', 'USER_SCALAR_NAMES': '(Trace)',
        'USER_SOURCE_NAMES': '(TraceProd,TraceCons)', 'LOOKUP_NAMES': '(HeatRelease)',
        'SPECIES_INIT': '(0.4,0.1,0.1)', 'DIFFUSIVITY_MODEL': 'FLAMELET',
        'VISCOSITY_MODEL': 'FLAMELET', 'CONDUCTIVITY_MODEL': 'FLAMELET',
        'FLAME_INIT_METHOD': 'FLAME_FRONT', 'FLAME_INIT': '(0,0,0,1,0,0,0.005,1)',
        'THICKENED_FLAME_CORRECTION': 'YES',
        'PREFERENTIAL_DIFFUSION': 'NO', 'FLAME_INIT_IGNITION': '340.500001', 'SPECIES_CLIPPING': 'NO',
        'MESH_BOX_LENGTH': '(0.005,1,1)', 'MUSCL_SPECIES': 'NO', 'CFL_NUMBER': '20',
        'NEWTON_ADAPT_CFL': 'YES',
        'MARKER_HEATFLUX': 'NONE', 'MARKER_MONITORING': 'NONE',
        'MARKER_FAR': '(x_minus,x_plus,y_minus,y_plus)',
        'ITER': '20000', 'OUTPUT_WRT_FREQ': '20000',
        'CONV_RESIDUAL_MINVAL': '-13',
        'CONV_FIELD': '(RMS_PRESSURE,RMS_VELOCITY-X,RMS_VELOCITY-Y,'
                      'RMS_ProgressVariable,RMS_EnthalpyTot,RMS_Trace)',
        'HISTORY_OUTPUT': '(ITER,RMS_RES,LINSOL,AERO_COEFF,FLAME_THICKNESS)',
        'SCREEN_OUTPUT': '(INNER_ITER,RMS_PRESSURE,RMS_ProgressVariable,RMS_Trace,THICKNESS)'}.items():
    config = option(config, key, value)
config += '% Synthetic affine LUT SHA256: ' + sha(table) + '\n'

FLAMELET_CASE = dict(SA_CASE, config=config,
    name='81-point incompressible flow/flamelet throughflow with a synthetic affine LUT',
    max_updates=20000,
    raw_rms=1e-12,
    wall_timeout=600,
    flamelet_diffusivity=0.02,
    minimum_progress_range=0.005,
    fields={'flow': ['rms[P]', 'rms[U]', 'rms[V]'],
            'species': ['rms[ProgressVariable]', 'rms[EnthalpyTot]', 'rms[Trace]']},
    limits='Synthetic thermodynamics, sources and transport on a generated stretched mesh; '
           'no physical flame validation, discretization accuracy or production speedup claim. '
           'All six physical residuals must converge.')


if __name__ == '__main__':
    main(FLAMELET_CASE)
