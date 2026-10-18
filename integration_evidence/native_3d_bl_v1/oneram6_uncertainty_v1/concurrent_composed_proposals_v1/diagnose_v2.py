#!/usr/bin/env python3
"""Preserve all focused assertions and private diagnosis after a first-epoch fixture failure."""
import json,sys
from pathlib import Path
HERE=Path(__file__).resolve().parent;sys.path.insert(0,str(HERE.parent));import run
base=run.pins;paths=[Path(__file__).resolve(),run.ROOT/'Common/src/adaptation/CNativeEngine3D.cpp',run.ROOT/'Common/include/adaptation/CNativeEngine3D.hpp',run.ROOT/'UnitTests/Common/adaptation/CNativeDistributed3D_tests.cpp']
run.pins=lambda:dict(base(),**{str(p.relative_to(run.ROOT)):run.sha(p) for p in paths})
subprocess_run=run.subprocess.run
def all_assertions(command,**kwargs):
 assert command.count('--abort')==1;command.remove('--abort');return subprocess_run(command,**kwargs)
run.subprocess.run=all_assertions
run.invoke(HERE/'diagnostic_serial_v2',{},run.OLD/'cell329/replay.dat',1,test_filter='[NativeConcurrentComposed3D]')
