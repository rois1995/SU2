"""Compare converged conventional, segregated and coupled flow/SA on a generated mesh."""
from compare_manufactured_flow import main


CONFIG = '''SOLVER= RANS
KIND_TURB_MODEL= SA
MESH_FORMAT= RECTANGLE
MESH_BOX_SIZE= 9,9,5
MESH_BOX_LENGTH= 1,1,1
MARKER_HEATFLUX= (y_minus,0)
MARKER_FAR= (x_minus,x_plus,y_plus)
MARKER_MONITORING= (y_minus)
MACH_NUMBER= 0.2
REYNOLDS_NUMBER= 1e5
REF_DIMENSIONALIZATION= FREESTREAM_VEL_EQ_ONE
MUSCL_FLOW= NO
MUSCL_TURB= NO
CONV_NUM_METHOD_FLOW= ROE
MGLEVEL= 0
MG_PRE_SMOOTH= 1
CFL_NUMBER= 20
CFL_ADAPT= NO
NEWTON_ADAPT_CFL= NO
NEWTON_ADAPTIVE_FORCING= NO
NEWTON_STEP_TOL= 1e-14
LINEAR_SOLVER_PREC= ILU
LINEAR_SOLVER_ITER= 80
LINEAR_SOLVER_ERROR= 1e-6
ITER= 10000
CONV_STARTITER= 5
CONV_FIELD= (RMS_DENSITY,RMS_MOMENTUM-X,RMS_MOMENTUM-Y,RMS_ENERGY,RMS_NU_TILDE)
CONV_RESIDUAL_MINVAL= -10
OUTPUT_FILES= RESTART
OUTPUT_WRT_FREQ= 10000
RESTART_FILENAME= restart_flow
SCREEN_OUTPUT= (INNER_ITER,RMS_DENSITY,RMS_NU_TILDE)
HISTORY_OUTPUT= (ITER,RMS_RES,LINSOL,AERO_COEFF)
'''

SA_CASE = {'config': CONFIG, 'name': '81-point compressible flow/SA flat plate, Re=1e5, first-order Roe',
           'fields': {'flow': ['rms[Rho]', 'rms[RhoU]', 'rms[RhoV]', 'rms[RhoE]'], 'turbulence': ['rms[nu]']},
           'raw_rms': 1e-10, 'normalized_rms': 1e-9, 'projection': 1e-8, 'state_tolerance': 1e-6,
           'quantity_tolerance': 1e-6, 'max_updates': 10000,
           'limits': 'Coarse generated mesh; no discretization-accuracy or production speedup claim. '
                     'Single launch-inclusive timing samples; all five physical residuals must converge.'}


if __name__ == '__main__':
    main(SA_CASE)
