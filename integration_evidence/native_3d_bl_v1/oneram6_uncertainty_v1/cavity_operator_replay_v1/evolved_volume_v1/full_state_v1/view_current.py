#!/usr/bin/env python3
"""Reuse the existing cavity viewer export, retaining original-ID sidecars."""
import sys
from pathlib import Path
HERE=Path(__file__).resolve().parent
sys.path.insert(0,str(HERE.parents[1]));from diagnose_contexts import patch,grid
sys.path.insert(0,str(HERE.parents[5]/'TestCases/adaptation/capability'));from capcheck import read_su2
for source in sorted((HERE/'current_stars').glob('*.dat')):
 cells,nodes,_=patch(source);out=source.with_name(source.stem+'_context.su2');grid(out,cells,nodes)
 mesh=read_su2(out);assert len(mesh.P)==len(nodes) and len(mesh.E)==len(cells)
print('PASS: current-star viewer grids/ID maps; diagnostic skin only, no CFD boundary/restart claim.')
