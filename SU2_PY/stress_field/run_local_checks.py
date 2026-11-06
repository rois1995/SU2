#!/usr/bin/env python3
"""Build and run small serial operator/file tests. Never launches SU2_CFD or MPI."""
import argparse
import os
from pathlib import Path
import subprocess
import sys


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--build-dir",required=True,help="dedicated output directory")
    parser.add_argument("--cxx",default=os.environ.get("CXX","c++"))
    parser.add_argument("--v07-root")
    args=parser.parse_args()
    root=Path(__file__).resolve().parents[2];build=Path(args.build_dir).resolve();build.mkdir(parents=True,exist_ok=True)
    flags=[args.cxx,"-std=c++17","-O0","-ffunction-sections","-fdata-sections","-Iexternals/catch2"]
    def run(command):
        print("+",*map(str,command),flush=True)
        subprocess.run(list(map(str,command)),cwd=root,check=True)
    sources=["Common/src/CConfig.cpp","SU2_CFD/src/numerics/CNumerics.cpp",
             "SU2_CFD/src/numerics/flow/flow_diffusion.cpp","Common/include/parallelization/mpi_structure.cpp",
             "Common/src/toolboxes/SwapBytes.cpp","Common/src/toolboxes/printing_toolbox.cpp",
             "Common/src/fem/fem_gauss_jacobi_quadrature.cpp"]
    objects=[]
    for i,source in enumerate(sources):
        obj=build/f"support{i}.o";run(flags+["-c",source,"-o",obj]);objects.append(obj)
    main=build/"catch_main.cpp";main.write_text('#define CATCH_CONFIG_MAIN\n#include "catch.hpp"\n')
    tests=[main]+[root/"UnitTests/SU2_CFD/numerics"/(name+".cpp") for name in ("epm_tests","epm_source_tests","stress_coupling_tests","stress_replay_tests")]
    test_objects=[]
    for i,source in enumerate(tests):
        obj=build/f"test{i}.o";run(flags+["-c",source,"-o",obj]);test_objects.append(obj)
    executable=build/"test_stress";run([args.cxx,"-Wl,--gc-sections",*objects,*test_objects,"-o",executable]);run([executable])
    config_probe=build/"stress_config_probe"
    run(flags+["UnitTests/SU2_CFD/numerics/stress_config_probe.cpp",*objects,"-Wl,--gc-sections","-o",config_probe])
    run([sys.executable,"SU2_PY/stress_field/test_config.py","--probe",config_probe])
    probe=build/"stress_field_probe"
    run([args.cxx,"-std=c++17","-O2","-Wall","-Wextra","UnitTests/SU2_CFD/numerics/stress_field_probe.cpp","-o",probe])
    command=[sys.executable,"SU2_PY/stress_field/test_field.py","--probe",probe]
    if args.v07_root:command += ["--v07-root",args.v07_root]
    run(command)
    run([sys.executable,"SU2_PY/stress_field/test_campaign.py"])
    run([sys.executable,"SU2_PY/stress_field/test_target.py","--probe",probe])
    run([sys.executable,"SU2_PY/stress_field/test_validation.py"])

if __name__=="__main__":main()
