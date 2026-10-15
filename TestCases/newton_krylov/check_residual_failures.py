"""Check bounded Newton failures and native divergence policy without an external mesh."""
import argparse
import csv
import hashlib
import json
import math
import os
from pathlib import Path
import struct
import subprocess


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--binary', required=True, type=Path)
    parser.add_argument('--before', type=Path, help='Optional executable predating bounded residual reporting')
    parser.add_argument('--output', required=True, type=Path)
    parser.add_argument('--ranks', type=int, default=2)
    parser.add_argument('--threads', type=int, default=2)
    args = parser.parse_args()
    if args.ranks < 1 or args.threads < 1:
        parser.error('ranks and threads must be positive')
    binary = args.binary.resolve()
    before = args.before.resolve() if args.before else None
    if not binary.is_file() or (before and not before.is_file()):
        parser.error('executable does not exist')
    output = args.output.resolve()
    output.mkdir(parents=True, exist_ok=False)
    common = '''MESH_FORMAT= RECTANGLE
MESH_BOX_SIZE= 5,5,5
MESH_BOX_LENGTH= 1,1,1
MARKER_HEATFLUX= (y_minus,0)
MARKER_FAR= (x_minus,x_plus,y_plus)
MACH_NUMBER= 0.2
REYNOLDS_NUMBER= 1e6
MUSCL_FLOW= NO
MGLEVEL= 0
CFL_NUMBER= 0.05
BODY_FORCE= YES
BODY_FORCE_VECTOR= 1e30,0,0
NEWTON_COUPLED= YES
NEWTON_RETRIES= 0
NEWTON_ADAPT_CFL= NO
NEWTON_ADAPTIVE_FORCING= NO
ITER= 3
CONV_STARTITER= 1000
OUTPUT_FILES= NONE
SCREEN_OUTPUT= (INNER_ITER)
HISTORY_OUTPUT= (ITER,RMS_RES)
RESTART_FILENAME= restart_flow
'''
    compressible = '''SOLVER= RANS
KIND_TURB_MODEL= SST
CONV_NUM_METHOD_FLOW= ROE
NEWTON_EQUATIONS= (FLOW,TURB)
CONV_FIELD= REL_RMS_DENSITY
'''
    mixture = '''SOLVER= INC_NAVIER_STOKES
CONV_NUM_METHOD_FLOW= FDS
INC_DENSITY_MODEL= VARIABLE
INC_ENERGY_EQUATION= YES
FLUID_MODEL= FLUID_MIXTURE
KIND_SCALAR_MODEL= SPECIES_TRANSPORT
SPECIES_INIT= 0.4
MOLECULAR_WEIGHT= 28.96,16.043
SPECIFIC_HEAT_CP= 1009.39,2225
VISCOSITY_MODEL= CONSTANT_VISCOSITY
MU_CONSTANT= 1.8551e-5,1.1e-5
PRANDTL_LAM= 0.72,0.72
PRANDTL_TURB= 0.9,0.9
DIFFUSIVITY_CONSTANT= 1e-5
NEWTON_EQUATIONS= (FLOW,SPECIES)
'''
    incompressible = '''SOLVER= INC_NAVIER_STOKES
CONV_NUM_METHOD_FLOW= FDS
INC_DENSITY_MODEL= CONSTANT
INC_ENERGY_EQUATION= YES
FLUID_MODEL= CONSTANT_DENSITY
VISCOSITY_MODEL= CONSTANT_VISCOSITY
MU_CONSTANT= 1.8551e-5
NEWTON_EQUATIONS= (FLOW)
'''
    # Mixture body force subtracts operating density: its freestream residual cancels while its derivative is stiff.
    profiles = [('compressible_sst', compressible, 8, 'RESIDUAL_FAILURE'),
                ('incompressible', incompressible, 6, 'RESIDUAL_FAILURE'),
                ('mixture_species', mixture, 7, 'LINEAR_FAILURE'),
                ('explicit_flow_sst', compressible + 'TIME_DISCRE_FLOW= EULER_EXPLICIT\n', 8, 'RESIDUAL_FAILURE'),
                ('native_default', compressible.replace('(FLOW,TURB)', 'NONE'), 8, None)]
    results = []
    for name, physics, columns, expected_status in profiles:
        for label, executable in ([('before', before)] if before else []) + [('current', binary)]:
            directory = output / name / label
            directory.mkdir(parents=True)
            config = directory / 'run.cfg'
            config.write_text((common + physics).replace('NEWTON_COUPLED= YES', 'NEWTON_COUPLED= NO')
                              if name == 'native_default' else common + physics)
            command = ['mpirun', '--bind-to', 'none', '-n', str(args.ranks), str(executable), config.name]
            with (directory / 'log.txt').open('w') as log:
                process = subprocess.run(command, cwd=directory,
                    env=dict(os.environ, OMP_NUM_THREADS=str(args.threads)),
                    stdout=log, stderr=subprocess.STDOUT, timeout=120)
            text = (directory / 'log.txt').read_text()
            row = {'profile': name, 'variant': label, 'command': command, 'threads': args.threads,
                   'exit_code': process.returncode, 'binary_sha256': digest(executable),
                   'config_sha256': digest(config), 'numerical_mpi_abort': 'MPI_ABORT' in text}
            if label == 'current' and name == 'native_default':
                assert process.returncode != 0 and row['numerical_mpi_abort'], directory
            elif label == 'current':
                assert process.returncode != 0 and 'Numerical Failure (SU2_CFD)' in text, directory
                assert not row['numerical_mpi_abort'], directory
                with (directory / 'newton_coupled.csv').open() as stream:
                    attempts = list(csv.DictReader(stream))
                assert len(attempts) == 1 and attempts[0]['status'] == expected_status, directory
                assert float(attempts[0]['rollback_state_error']) == 0
                assert float(attempts[0]['rollback_merit_error']) == 0
                restart = directory / 'restart_flow_newton_failure.dat'
                with restart.open('rb') as stream:
                    header = struct.unpack('5i', stream.read(20))
                    assert header[:3] == (535532, columns, 25), header
                    fields = [stream.read(33).split(b'\0')[0].decode() for _ in range(columns)]
                    values = struct.unpack(str(columns * 25) + 'd', stream.read(columns * 25 * 8))
                assert all(math.isfinite(value) for value in values), directory
                row.update(restart_fields=fields, restart_sha256=digest(restart),
                           rollback_state_error=0, rollback_merit_error=0)
            results.append(row)
            (output / 'results.json').write_text(json.dumps(results, indent=2) + '\n')
            print(name, label, process.returncode, 'MPI abort:', row['numerical_mpi_abort'], flush=True)
    if before:
        assert any(row['numerical_mpi_abort'] for row in results if row['variant'] == 'before')


if __name__ == '__main__':
    main()
