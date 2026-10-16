"""Compare flow/species with variable-density mixture properties and wall diffusion."""
from compare_manufactured_flow import main
from compare_sa_flatplate import SA_CASE
from report_state import option


config = SA_CASE['config']
for key, value in {
        'SOLVER': 'INC_NAVIER_STOKES', 'KIND_TURB_MODEL': 'NONE', 'CONV_NUM_METHOD_FLOW': 'FDS',
        'INC_DENSITY_MODEL': 'VARIABLE', 'INC_ENERGY_EQUATION': 'YES', 'INC_NONDIM': 'INITIAL_VALUES',
        'INC_DENSITY_INIT': '1.2', 'INC_TEMPERATURE_INIT': '300', 'INC_VELOCITY_INIT': '(100,0,0)',
        'FLUID_MODEL': 'FLUID_MIXTURE', 'KIND_SCALAR_MODEL': 'SPECIES_TRANSPORT', 'SPECIES_INIT': '0.4',
        'MOLECULAR_WEIGHT': '28.96,16.043', 'SPECIFIC_HEAT_CP': '1009.39,2225',
        'VISCOSITY_MODEL': 'CONSTANT_VISCOSITY', 'MU_CONSTANT': '1,1', 'PRANDTL_LAM': '0.72,0.72',
        'PRANDTL_TURB': '0.9,0.9', 'DIFFUSIVITY_CONSTANT': '2', 'MUSCL_SPECIES': 'NO',
        'MARKER_FAR': '(y_plus)', 'INC_INLET_TYPE': 'VELOCITY_INLET',
        'MARKER_INLET': '(x_minus,300,100,1,0,0)', 'MARKER_INLET_SPECIES': '(x_minus,0.25)',
        'INC_OUTLET_TYPE': 'PRESSURE_OUTLET', 'MARKER_OUTLET': '(x_plus,0)',
        'MARKER_WALL_SPECIES': '(y_minus,VALUE,0.75)',
        'CONV_FIELD': '(RMS_PRESSURE,RMS_VELOCITY-X,RMS_VELOCITY-Y,RMS_ENTHALPY,RMS_SPECIES_0)',
        'SCREEN_OUTPUT': '(INNER_ITER,RMS_PRESSURE,RMS_ENTHALPY,RMS_SPECIES_0)'}.items():
    config = option(config, key, value)

SPECIES_CASE = dict(SA_CASE, config=config,
    name='81-point variable-density mixture/species wall diffusion, first-order FDS',
    fields={'flow': ['rms[P]', 'rms[U]', 'rms[V]', 'rms[h]'], 'species': ['rms[rho*Y_0]']},
    minimum_species_range=0.1,
    limits='Generated variable-density two-component mixture with artificial transport coefficients; '
           'no physical validation or production speedup claim. All five physical residuals must converge.')


if __name__ == '__main__':
    main(SPECIES_CASE)
