"""Compare flow/SST/LM with partially active transported intermittency."""
from compare_manufactured_flow import main
from compare_sst_flatplate import SST_CASE
from report_state import option


config = SST_CASE['config']
for key, value in {'KIND_TRANS_MODEL': 'LM', 'REYNOLDS_NUMBER': '500', 'CFL_REDUCTION_TURB': '0.5',
        'CONV_FIELD': '(RMS_DENSITY,RMS_MOMENTUM-X,RMS_MOMENTUM-Y,RMS_ENERGY,RMS_TKE,RMS_DISSIPATION,'
                      'RMS_INTERMITTENCY,RMS_RE_THETA_T)',
        'SCREEN_OUTPUT': '(INNER_ITER,RMS_INTERMITTENCY,RMS_RE_THETA_T)'}.items():
    config = option(config, key, value)

LM_CASE = dict(SST_CASE, config=config,
    name='81-point compressible flow/SST/LM flat plate, Re=500, first-order Roe',
    fields=dict(SST_CASE['fields'], transition=['rms[LM_1]', 'rms[LM_2]']), transition_model='LM',
    limits='Coarse low-Reynolds-number transition fixture; no physical validation or transition-front accuracy claim. '
           'All eight physical residuals must converge, with partial and high free-node intermittency.')


if __name__ == '__main__':
    main(LM_CASE)
