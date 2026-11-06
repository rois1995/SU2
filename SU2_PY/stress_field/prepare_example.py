#!/usr/bin/env python3
"""Write a tiny planar input set and run plan. Never runs a flow solver."""
import argparse
import json
import itertools
from pathlib import Path
import numpy as np
from field_io import write_field


def prepare_cube(output, base_config):
    """A 27-node tetrahedral wiring fixture with one wall; never runs CFD."""
    root=Path(output);root.mkdir(parents=True,exist_ok=False)
    n=3;nodes=np.array([(x,y,z) for z in np.linspace(0,1,n) for y in np.linspace(0,1,n) for x in np.linspace(0,1,n)])
    def index(v):return int(v[0]+n*v[1]+n*n*v[2])
    cells=[]
    for base in itertools.product(range(n-1),repeat=3):
        for order in itertools.permutations(range(3)):
            point=np.array(base);tet=[index(point)]
            for axis in order:point=point.copy();point[axis]+=1;tet.append(index(point))
            if np.linalg.det(nodes[tet[1:]]-nodes[tet[0]])<0:tet[1],tet[2]=tet[2],tet[1]
            cells.append(tet)
    cells=np.array(cells);faces={}
    for cell in cells:
        for omitted in range(4):
            face=[int(cell[i]) for i in range(4) if i!=omitted];key=tuple(sorted(face))
            faces.setdefault(key,[]).append((face,int(cell[omitted])))
    boundary={name:[] for name in ("wall","inlet","outlet","top")}
    for adjacent in faces.values():
        if len(adjacent)!=1:continue
        face,opposite=adjacent[0];xyz=nodes[face];center=xyz.mean(axis=0)
        if np.dot(np.cross(xyz[1]-xyz[0],xyz[2]-xyz[0]),nodes[opposite]-center)>0:face[1],face[2]=face[2],face[1]
        name="wall" if center[2]==0 else "inlet" if center[0]==0 else "outlet" if center[0]==1 else "top"
        boundary[name].append(face)
    with (root/"mesh.su2").open("w") as stream:
        stream.write(f"NDIME= 3\nNELEM= {len(cells)}\n")
        for i,c in enumerate(cells):stream.write("10 "+" ".join(map(str,c))+f" {i}\n")
        stream.write(f"NPOIN= {len(nodes)}\n")
        for i,point in enumerate(nodes):stream.write(" ".join(map(str,point))+f" {i}\n")
        stream.write("NMARK= 4\n")
        for name,triangles in boundary.items():
            stream.write(f"MARKER_TAG= {name}\nMARKER_ELEMS= {len(triangles)}\n")
            for face in triangles:stream.write("5 "+" ".join(map(str,face))+"\n")
    definition={"scenario":"tiny_cube","U_ref":1,"L_ref":1,"epsilon":.01,"ell":1,"ell_w":.02,
                "geometry":{"type":"PLANES","walls":[{"normal":[0,0,1],"offset":0}]},
                "support":{"type":"BOX","lower":[0,0,0],"upper":[1,1,1],"width":[.2,.2,.2]}}
    for name,amplitude in (("zero",0),("positive",.01),("negative",-.01)):
        raw=np.zeros((len(nodes),5));raw[:,3]=amplitude
        write_field(root/(name+".suf"),nodes,cells,raw,{**definition,"scenario":"tiny_cube_"+name})
    (root/"base.cfg").write_text(Path(base_config).read_text())
    return root


def prepare(output):
    root=Path(output).resolve();root.mkdir(parents=True,exist_ok=False)
    n=9;nodes=np.array([(x,y) for y in np.linspace(0,1,n) for x in np.linspace(0,1,n)])
    cells=[]
    for j in range(n-1):
        for i in range(n-1):
            a=j*n+i;b=a+1;c=a+n+1;d=a+n;cells += [(a,b,c),(a,c,d)]
    cells=np.array(cells);np.savez(root/"field_mesh.npz",nodes=nodes,cells=cells)
    with (root/"mesh.su2").open("w") as mesh:
        mesh.write(f"NDIME= 2\nNELEM= {len(cells)}\n")
        for i,c in enumerate(cells):mesh.write("5 "+" ".join(map(str,c))+f" {i}\n")
        mesh.write(f"NPOIN= {len(nodes)}\n")
        for i,(x,y) in enumerate(nodes):mesh.write(f"{x:.17g} {y:.17g} {i}\n")
        boundaries={"wall":[(i,i+1) for i in range(n-1)],"outlet":[(j*n+n-1,(j+1)*n+n-1) for j in range(n-1)],
                    "top":[((n-1)*n+i+1,(n-1)*n+i) for i in range(n-1)],"inlet":[((j+1)*n,j*n) for j in range(n-1)]}
        mesh.write("NMARK= 4\n")
        for name,edges in boundaries.items():
            mesh.write(f"MARKER_TAG= {name}\nMARKER_ELEMS= {len(edges)}\n")
            for a,b in edges:mesh.write(f"3 {a} {b}\n")
    definition={"scenario":"tiny_planar_shear","U_ref":1,"L_ref":1,"epsilon":.01,"ell":.6,"ell_w":.02,
                "geometry":{"type":"PLANES","walls":[{"normal":[0,1,0],"offset":0}]},
                "support":{"type":"BOX","lower":[0,0,0],"upper":[1,1,0],"width":[.2,.2,0]},
                "boundary_classification":{"wall":"solid","inlet":"open","outlet":"open","top":"symmetry"},
                "geometry_authority":"analytic unit square; exact straight boundaries"}
    for name,amplitude in (("zero",0),("positive",.01),("negative",-.01)):
        raw=np.zeros((len(nodes),5));raw[:,2]=amplitude
        write_field(root/(name+".suf"),nodes,cells,raw,{**definition,"scenario":"tiny_"+name})
    (root/"base.cfg").write_text("""% Unexecuted 81-node wiring fixture, not a converged flow case.
SOLVER= INC_RANS
KIND_TURB_MODEL= SA
SA_OPTIONS= NONE
SST_OPTIONS= V2003m
MATH_PROBLEM= DIRECT
RESTART_SOL= NO
SYSTEM_MEASUREMENTS= SI
MGLEVEL= 0
MESH_FILENAME= mesh.su2
MESH_FORMAT= SU2
INC_DENSITY_MODEL= CONSTANT
INC_DENSITY_INIT= 1.0
INC_VELOCITY_INIT= (1.0, 0.0, 0.0)
VISCOSITY_MODEL= CONSTANT_VISCOSITY
MU_CONSTANT= 0.001
MARKER_HEATFLUX= (wall, 0.0)
MARKER_SYM= (top)
INC_INLET_TYPE= VELOCITY_INLET
INC_OUTLET_TYPE= PRESSURE_OUTLET
MARKER_INLET= (inlet, 300.0, 1.0, 1.0, 0.0, 0.0)
MARKER_OUTLET= (outlet, 0.0)
CONV_NUM_METHOD_FLOW= FDS
MUSCL_FLOW= NO
NUM_METHOD_GRAD= WEIGHTED_LEAST_SQUARES
CFL_NUMBER= 0.1
ITER= 2
OUTPUT_FILES= (RESTART, PARAVIEW)
VOLUME_OUTPUT= (COORDINATES, SOLUTION, PRIMITIVE, STRESS_DISCREPANCY)
HISTORY_OUTPUT= (ITER, RMS_RES)
OUTPUT_WRT_FREQ= 1
WRT_RESTART_COMPACT= NO
""")
    runs=[]
    for model in ("SA","SST"):
        for field in (None,"zero","positive","negative"):
            options={"KIND_TURB_MODEL":model,"STRESS_DISCREPANCY":"PRESCRIBED" if field else "NONE"}
            if field is None:options["VOLUME_OUTPUT"]="(COORDINATES, SOLUTION, PRIMITIVE)"
            run={"id":model+"_"+(field or "base"),"stage":"S2" if field in (None,"zero") else "S4",
                 "config":"base.cfg","mesh":"mesh.su2","options":options}
            if field:run["field"]=field+".suf"
            runs.append(run)
    for component,swap in ((1,False),(1,True),(2,False),(2,True),(3,False)):
        runs.append({"id":f"SST_EPM_{component}C_"+("PMIN" if swap else "PMAX"),"stage":"S6",
                     "config":"base.cfg","mesh":"mesh.su2","options":{"KIND_TURB_MODEL":"SST",
                     "STRESS_DISCREPANCY":"EPM_MATCHED","UQ_COMPONENT":component,"UQ_PERMUTE":"YES" if swap else "NO",
                     "UQ_DELTA_B":1,"UQ_URLX":.1}})
    (root/"plan.json").write_text(json.dumps({"schema":"SU2_STRESS_CAMPAIGN_1","runs":runs},indent=2)+"\n")
    return root/"plan.json"

if __name__=="__main__":
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument("--output",required=True)
    print(prepare(parser.parse_args().output))
