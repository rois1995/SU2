#!/usr/bin/env python3
"""Measure final dimensional MMS_NS_UNIT_QUAD errors from an ASCII restart."""
import argparse
import csv
import json
import math
from pathlib import Path

from run_convergence import read_options


def exact_solution(x, y, gamma):
    # Common/src/toolboxes/MMS/CMMSNSUnitQuadSolution.cpp, GetSolution (L=1).
    pi = math.pi
    rho = 1 + .1*math.sin(.75*pi*x) + .15*math.cos(pi*y) + .08*math.cos(1.25*pi*x*y)
    u = 70 + 4*math.sin(1.6666666667*pi*x) - 12*math.cos(1.5*pi*y) + 7*math.cos(.6*pi*x*y)
    v = 90 - 20*math.cos(1.5*pi*x) + 4*math.sin(pi*y) - 11*math.cos(.9*pi*x*y)
    p = 100000 - 30000*math.cos(pi*x) + 20000*math.sin(1.25*pi*y) - 25000*math.sin(.75*pi*x*y)
    return rho, rho*u, rho*v, p/(gamma-1) + .5*rho*(u*u+v*v)


def assess(config, restart):
    options = {key: value.strip() for key, value in read_options(config.read_text()).items()}
    if (options.get('SOLVER') != 'NAVIER_STOKES' or
            options.get('KIND_VERIFICATION_SOLUTION') != 'MMS_NS_UNIT_QUAD' or
            options.get('REF_DIMENSIONALIZATION') != 'DIMENSIONAL' or options.get('MGLEVEL') != '0'):
        raise ValueError('Require dimensional MMS_NS_UNIT_QUAD and MGLEVEL=0')
    gamma = float(options.get('GAMMA_VALUE', '1.4'))
    if not math.isfinite(gamma) or gamma <= 1:
        raise ValueError('Invalid ideal-gas gamma')
    columns = ('Density', 'Momentum_x', 'Momentum_y', 'Energy')
    errors = [[], [], [], []]
    min_density, min_internal = math.inf, math.inf
    with restart.open() as stream:
        reader = csv.DictReader(stream)
        if reader.fieldnames and ('z' in reader.fieldnames or 'Momentum_z' in reader.fieldnames):
            raise ValueError('This diagnostic supports 2D restarts only')
        for row in reader:
            x, y = float(row['x']), float(row['y'])
            state = tuple(float(row[key]) for key in columns)
            if not all(math.isfinite(value) for value in (x, y, *state)) or state[0] <= 0:
                raise ValueError('Nonfinite/nonpositive restart state')
            internal = state[3] - (state[1]**2+state[2]**2)/(2*state[0])
            if not math.isfinite(internal) or internal <= 0:
                raise ValueError('Nonpositive internal energy density')
            min_density, min_internal = min(min_density, state[0]), min(min_internal, internal)
            for error, value, exact in zip(errors, state, exact_solution(x, y, gamma)):
                error.append(value-exact)
    if not errors[0]:
        raise ValueError('Empty restart')
    return dict(points=len(errors[0]), min_density=min_density, min_internal_energy_density=min_internal,
                rms_error={name: math.sqrt(math.fsum(e*e for e in error)/len(error))
                           for name, error in zip(columns, errors)},
                max_error={name: max(map(abs, error)) for name, error in zip(columns, errors)})


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('config', type=Path)
    parser.add_argument('restart', type=Path)
    args = parser.parse_args()
    print(json.dumps(assess(args.config, args.restart), indent=2, allow_nan=False))
