#!/usr/bin/env python3
"""Bounded local wiring tests: 81/27-node meshes, at most four iterations per solve, no MPI launcher."""
import argparse
import csv
import json
import os
from pathlib import Path
import shutil
import subprocess
import time
import numpy as np
from campaign import config_options, digest, read_csv, save, stress_budget
from prepare_example import prepare, prepare_cube


def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument("--solver",required=True);p.add_argument("--output",required=True)
    a=p.parse_args();solver=Path(a.solver).resolve();root=Path(a.output).resolve()
    root.mkdir(parents=True,exist_ok=False);prepare(root/"inputs")
    prepare_cube(root/"cube",root/"inputs/base.cfg")
    base=config_options((root/"inputs/base.cfg").read_text())
    base.update(OUTPUT_FILES="(RESTART_ASCII)",RESTART_FILENAME="restart",
                READ_BINARY_RESTART="NO",CFL_ADAPT="NO",CONV_STARTITER="1000",USE_VECTORIZATION="NO",
                INC_ENERGY_EQUATION="YES",VOLUME_OUTPUT="(COORDINATES, SOLUTION, PRIMITIVE)",ITER="2")
    report={"schema":"SU2_STRESS_SOLVER_CHECKS_1","solver_sha256":digest(solver),"mesh_points":[81,27],
            "production_runs":0,"cluster_access":False,"runs":[],"comparisons":[]}
    def persist():save(root/"report.json",report)
    def run(name,options,initial=None,replay=None,expected_error=None):
        folder=root/name;folder.mkdir();cfg={k:v for k,v in {**base,**options}.items() if v is not None}
        fixture=Path(cfg.pop("_FIXTURE",root/"inputs"))
        shutil.copyfile(fixture/"mesh.su2",folder/"mesh.su2")
        if cfg.get("STRESS_DISCREPANCY")=="PRESCRIBED":
            field=cfg.pop("FIELD","positive")
            shutil.copyfile(fixture/(field+".suf"),folder/"field.suf")
            cfg["STRESS_DISCREPANCY_FILE"]="field.suf"
            cfg["VOLUME_OUTPUT"]="(COORDINATES, SOLUTION, PRIMITIVE, STRESS_DISCREPANCY)"
        if initial:
            shutil.copyfile(initial/"restart.csv",folder/"initial.csv")
            if (initial/"restart.stress").exists():shutil.copyfile(initial/"restart.stress",folder/"initial.stress")
            cfg.update(RESTART_SOL="YES",SOLUTION_FILENAME="initial")
        if replay:shutil.copyfile(replay/"control.sur",folder/"control.sur")
        if not 1<=int(cfg["ITER"])<=4:raise ValueError("local test iteration limit")
        (folder/"case.cfg").write_text(''.join(f'{k}= {v}\n' for k,v in cfg.items()))
        inputs={f.name:digest(f) for f in folder.iterdir() if f.is_file()}
        start=time.monotonic();command=[str(solver),"case.cfg"]
        env={**os.environ,"OMP_NUM_THREADS":"1","OPENBLAS_NUM_THREADS":"1"}
        with (folder/"solver.log").open("w") as log:
            try: result=subprocess.run(command,cwd=folder,env=env,stdout=log,stderr=subprocess.STDOUT,timeout=45);code=result.returncode
            except subprocess.TimeoutExpired:code=124
        text=(folder/"solver.log").read_text()
        passed=(code!=0 and expected_error in text) if expected_error else code==0
        if passed and not expected_error and cfg.get("SOLVER")=="RANS" and cfg.get("STRESS_DIAGNOSTIC","NONE")!="NONE":
            path="SIMD" if cfg.get("USE_VECTORIZATION")=="YES" else "SCALAR"
            passed="Stress diagnostic flux path: "+path in text
        entry={"id":name,"command":command,"returncode":code,"seconds":time.monotonic()-start,
               "status":"PASS" if passed else "FAIL","expected_error":expected_error,"inputs":inputs,
               "outputs":{f.name:digest(f) for f in folder.iterdir() if f.is_file() and f.name not in inputs}}
        save(folder/"execution.json",entry);report["runs"].append(entry);persist()
        if not passed:raise RuntimeError(f"{name} failed; inspect {folder/'solver.log'}")
        return folder
    def compare(name,left,right,filename,columns=None,tol=1e-10):
        x,y=read_csv(left/filename),read_csv(right/filename)
        ix,iy=np.argsort(x["PointID"]),np.argsort(y["PointID"])
        if not np.array_equal(x["PointID"][ix],y["PointID"][iy]):raise ValueError("point ids differ")
        columns=columns or [c for c in x if c in y and c!="PointID" and not c.startswith("Stress_")]
        if not columns:raise ValueError("empty comparison")
        errors={c:float(np.max(abs(x[c][ix]-y[c][iy]))) for c in columns}
        passed=max(errors.values())<=tol
        report["comparisons"].append({"id":name,"status":"PASS" if passed else "FAIL",
                                      "absolute_tolerance_internal_units":tol,"errors":errors});persist()
        if not passed:raise AssertionError(f"{name}: {errors}")
    for model in ("SA","SST"):
        common={"KIND_TURB_MODEL":model}
        baseline=run(model+"_base",common)
        zero=run(model+"_zero",{**common,"STRESS_DISCREPANCY":"PRESCRIBED","FIELD":"zero"})
        compare(model+"_zero_recovery",baseline,zero,"restart.csv")
        positive=run(model+"_positive",{**common,"STRESS_DISCREPANCY":"PRESCRIBED"})
        data=read_csv(positive/"restart.csv")
        report.setdefault("budgets",{})[model]=stress_budget(data,1e-12);persist()
        continuous=run(model+"_continuous",{**common,"STRESS_DISCREPANCY":"PRESCRIBED","ITER":"4"})
        resumed=run(model+"_resumed",{**common,"STRESS_DISCREPANCY":"PRESCRIBED"},initial=positive)
        compare(model+"_disk_restart",continuous,resumed,"restart.csv",tol=1e-9)
        run(model+"_changed_field",{**common,"STRESS_DISCREPANCY":"PRESCRIBED","FIELD":"negative"},
            initial=positive,expected_error="Missing or mismatched stress restart identity")
        diagnostic={**common,"ITER":"1","STRESS_DIAGNOSTIC_FILE":"control.sur","STRESS_DIAGNOSTIC":"CAPTURE"}
        if model=="SA":diagnostic["SA_OPTIONS"]="QCR2000"
        else:diagnostic.update(STRESS_DISCREPANCY="EPM_MATCHED",UQ_COMPONENT="1",UQ_DELTA_B="1",UQ_URLX="0.1")
        capture=run(model+"_capture",diagnostic,initial=baseline)
        replay_options={**common,"ITER":"1","STRESS_DIAGNOSTIC":"RESIDUAL","STRESS_DIAGNOSTIC_FILE":"control.sur",
                        "STRESS_DISCREPANCY":"REPLAY","STRESS_DISCREPANCY_PURPOSE":"COUPLING_CONTROL"}
        replay=run(model+"_replay",replay_options,initial=baseline,replay=capture)
        for block in ("flow","turb"):compare(model+"_exact_"+block,capture,replay,"control.sur."+block+".csv")
        run(model+"_changed_replay_state",replay_options,initial=positive,replay=capture,expected_error="fingerprint mismatch")
        off={**common,"INC_ENERGY_EQUATION":"NO","STRESS_DISCREPANCY":"PRESCRIBED"}
        part=run(model+"_no_energy_part",off)
        complete=run(model+"_no_energy_complete",{**off,"ITER":"4"})
        continuation=run(model+"_no_energy_restart",off,initial=part)
        compare(model+"_no_energy_disk_restart",complete,continuation,"restart.csv",tol=1e-9)
    compressible={"SOLVER":"RANS","KIND_TURB_MODEL":"SST","MACH_NUMBER":"0.1","REYNOLDS_NUMBER":"1000",
                  "REF_DIMENSIONALIZATION":"FREESTREAM_VEL_EQ_ONE","CONV_NUM_METHOD_FLOW":"ROE",
                  "MARKER_INLET":None,"MARKER_OUTLET":None,"MARKER_SYM":None,"MARKER_FAR":"(inlet, outlet, top)"}
    initial=run("COMP_base",compressible)
    uniform=initial
    # A deliberately nonuniform, physical state avoids testing eigenvector selection at an isotropic state.
    smooth=root/"COMP_smooth_state";smooth.mkdir()
    with (initial/"restart.csv").open(newline="") as stream:
        reader=csv.DictReader(stream);names=reader.fieldnames;rows=list(reader)
    for row in rows:
        rho=float(row["Density"]);old_u=float(row["Momentum_x"])/rho;old_v=float(row["Momentum_y"])/rho
        x,y=float(row["x"]),float(row["y"]);u=1+.1*x+.2*y;v=.05*x-.1*y
        row["Momentum_x"]=format(rho*u,".17g");row["Momentum_y"]=format(rho*v,".17g")
        row["Energy"]=format(float(row["Energy"])+.5*rho*(u*u+v*v-old_u*old_u-old_v*old_v),".17g")
    with (smooth/"restart.csv").open("w",newline="") as stream:
        writer=csv.DictWriter(stream,fieldnames=names,quoting=csv.QUOTE_NONNUMERIC,lineterminator="\n");writer.writeheader()
        writer.writerows({k:int(v) if k=="PointID" else float(v) for k,v in row.items()} for row in rows)
    initial=smooth
    for mode in ("PRESCRIBED","EPM_MATCHED"):
        options={**compressible,"STRESS_DISCREPANCY":mode,"ITER":"1","STRESS_DIAGNOSTIC":"RESIDUAL",
                 "STRESS_DIAGNOSTIC_FILE":"control.sur","UQ_COMPONENT":"1","UQ_DELTA_B":"1","UQ_URLX":"0.1"}
        scalar=run("COMP_"+mode+"_scalar",options,initial=initial)
        vector=run("COMP_"+mode+"_simd",{**options,"USE_VECTORIZATION":"YES"},initial=initial)
        for block in ("flow","turb"):
            compare("COMP_"+mode+"_"+block,scalar,vector,"control.sur."+block+".csv")
    for model in ("SA","SST"):
        common={"_FIXTURE":root/"cube","KIND_TURB_MODEL":model}
        initial=run("CUBE_"+model+"_base",common)
        diagnostic={**common,"ITER":"1","STRESS_DIAGNOSTIC":"CAPTURE","STRESS_DIAGNOSTIC_FILE":"control.sur"}
        if model=="SA":diagnostic["SA_OPTIONS"]="QCR2000"
        else:diagnostic.update(STRESS_DISCREPANCY="EPM_MATCHED",UQ_COMPONENT="1",UQ_DELTA_B="1",UQ_URLX="0.1")
        capture=run("CUBE_"+model+"_capture",diagnostic,initial=initial)
        replay=run("CUBE_"+model+"_replay",{**common,"ITER":"1","STRESS_DISCREPANCY":"REPLAY",
                   "STRESS_DISCREPANCY_PURPOSE":"COUPLING_CONTROL","STRESS_DIAGNOSTIC":"RESIDUAL",
                   "STRESS_DIAGNOSTIC_FILE":"control.sur"},initial=initial,replay=capture)
        for block in ("flow","turb"):compare("CUBE_"+model+"_"+block,capture,replay,"control.sur."+block+".csv")
    for component in (1,3):
        independent={**compressible,"STRESS_DISCREPANCY":"EPM_MATCHED","ITER":"1","STRESS_DIAGNOSTIC":"RESIDUAL",
                     "STRESS_DIAGNOSTIC_FILE":"control.sur","UQ_COMPONENT":str(component),"UQ_DELTA_B":"1","UQ_URLX":"0.1"}
        name=f"COMP_{component}C_uniform"
        scalar=run(name+"_scalar",independent,initial=uniform)
        vector=run(name+"_simd",{**independent,"USE_VECTORIZATION":"YES"},initial=uniform)
        for block in ("flow","turb"):compare(name+"_"+block,scalar,vector,"control.sur."+block+".csv")
    report["status"]="PASS";persist();print(json.dumps({"status":"PASS","runs":len(report["runs"]),"comparisons":len(report["comparisons"])}))

if __name__=="__main__":main()
