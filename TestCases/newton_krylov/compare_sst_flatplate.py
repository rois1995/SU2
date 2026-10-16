"""Compare converged flow/SST with native zero-k wall blending."""
from compare_manufactured_flow import main
from compare_sa_flatplate import SA_CASE
from report_state import option


config = SA_CASE['config']
for key, value in {'KIND_TURB_MODEL': 'SST',
        'CONV_FIELD': '(RMS_DENSITY,RMS_MOMENTUM-X,RMS_MOMENTUM-Y,RMS_ENERGY,RMS_TKE,RMS_DISSIPATION)',
        'SCREEN_OUTPUT': '(INNER_ITER,RMS_TKE,RMS_DISSIPATION)', 'CONV_RESIDUAL_MINVAL': '-11'}.items():
    config = option(config, key, value)

SST_CASE = dict(SA_CASE, config=config,
    name='81-point compressible flow/SST flat plate, Re=1e5, first-order Roe',
    fields={'flow': SA_CASE['fields']['flow'], 'turbulence': ['rms[k]', 'rms[w]']}, projection=1e-7,
    limits='Coarse generated mesh; no discretization-accuracy or production speedup claim. '
           'All six history residuals reach -11; independent final physical RMS must be below 1e-10.')


if __name__ == '__main__':
    main(SST_CASE)
