"""Compare flow/SA-BC with nonuniform native transition activity."""
from compare_manufactured_flow import main
from compare_sa_flatplate import SA_CASE
from report_state import option


BC_CASE = dict(SA_CASE, config=option(SA_CASE['config'], 'SA_OPTIONS', 'BCM'),
               name='81-point compressible flow/SA-BC flat plate, Re=1e5, first-order Roe',
               transition_model='SA_BC')


if __name__ == '__main__':
    main(BC_CASE)
