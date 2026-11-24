"""Build only the FP2-M2 lab test driver with the sealed build664 solver."""

from argparse import ArgumentParser
from pathlib import Path
import sys

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE.parent / 'FP2_M1'))
import build_m1

build_m1.HERE = HERE
build_m1.BASE = '0957977801'
build_m1.BASE_BUILD = HERE.parent / 'FP2_M1/build666'
LAB = 'UnitTests/SU2_CFD/adaptation'
build_m1.SOURCES = (
    f'{LAB}/CNativeLocalCoreLab_tests.cpp',
    *(f'{LAB}/fixtures/{name}' for name in (
        'native_volume_handover.inc', 'native_fast_path_stage2_setup.hpp',
        'native_m2_stages.inc', 'native_m2_tables.inc', 'native_m2_model.inc')),
)

if __name__ == '__main__':
    parser = ArgumentParser(description=__doc__)
    parser.add_argument('label', type=int)
    raise SystemExit(not build_m1.build(parser.parse_args().label))
