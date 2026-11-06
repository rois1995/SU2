#!/usr/bin/env python3
"""Bounded donor-representation gate: 81 nodes, four iterations, no MPI or production cases."""
import argparse
import json
import os
from pathlib import Path
import shutil
import subprocess
import time
import numpy as np
from campaign import config_options, digest, read_csv, save
from export_target import fit
from prepare_example import prepare


def boundary_summary(data):
    """Boundary mismatch in the fixed unit-square fixture; six-component Euclidean norms."""
    point_id=data["PointID"]
    if len(point_id)!=81 or len(np.unique(point_id))!=81:
        raise ValueError("recovery fixture requires 81 unique points")
    x,y=data["x"],data["y"]
    stress=np.column_stack([data["Stress_E_"+name] for name in ("XX","YY","ZZ","XY","XZ","YZ")])
    if not np.isfinite(stress).all():raise ValueError("nonfinite donor stress")
    norm=np.linalg.norm(stress,axis=1)
    result={"norm":"Euclidean norm of six stored tensor components","max_all_nodes":float(norm.max())}
    for name,mask in {"wall":y==0,"inlet":x==0,"outlet":x==1,"top":y==1}.items():
        ids=np.flatnonzero(mask)
        if len(ids)!=9:raise ValueError("unexpected unit-square boundary")
        worst=ids[np.argmax(norm[ids])]
        result[name]={"maximum":float(norm[worst]),"worst_point_id":int(point_id[worst]),
                      "nonzero_nodes":int(np.count_nonzero(norm[ids]))}
    return result


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--solver",required=True);parser.add_argument("--probe",required=True)
    parser.add_argument("--output",required=True)
    args=parser.parse_args();root=Path(args.output).resolve();root.mkdir(parents=True,exist_ok=False)
    solver=Path(args.solver).resolve();probe=Path(args.probe).resolve();prepare(root/"inputs")
    base=config_options((root/"inputs/base.cfg").read_text())
    base.update(ITER="4",OUTPUT_FILES="(RESTART_ASCII)",RESTART_FILENAME="restart",READ_BINARY_RESTART="NO",
                CFL_ADAPT="NO",CONV_STARTITER="1000",INC_ENERGY_EQUATION="YES")
    definition=json.loads((root/"inputs/positive.suf.json").read_text())["definition"]
    save(root/"definition.json",definition)
    # Removing the map is an offline diagnostic only; the solver must reject its nonzero boundary values.
    save(root/"unrestricted_definition.json",{**definition,"geometry":{"type":"NONE"},"support":{"type":"NONE"}})
    report={"schema":"SU2_DONOR_REPRESENTATION_CHECKS_1","solver_sha256":digest(solver),"probe_sha256":digest(probe),
            "mesh_points":81,"maximum_iterations":4,"timeout_seconds":45,"production_runs":0,
            "cluster_access":False,"runs":[],"donors":{},"iterative_recovery":"NOT_RUN"}
    def persist():save(root/"report.json",report)
    def run(name,options,field=None,expected_error=None):
        folder=root/name;folder.mkdir();cfg={**base,**options}
        shutil.copyfile(root/"inputs/mesh.su2",folder/"mesh.su2")
        if field:shutil.copyfile(field,folder/"field.suf");cfg["STRESS_DISCREPANCY_FILE"]="field.suf"
        if int(cfg["ITER"]) not in (1,4):raise ValueError("bounded iteration count required")
        (folder/"case.cfg").write_text(''.join(f'{key}= {value}\n' for key,value in cfg.items()))
        inputs={path.name:digest(path) for path in folder.iterdir() if path.is_file()}
        command=[str(solver),"case.cfg"];start=time.monotonic()
        with (folder/"solver.log").open("w") as log:
            try:
                result=subprocess.run(command,cwd=folder,stdout=log,stderr=subprocess.STDOUT,
                                      env={**os.environ,"OMP_NUM_THREADS":"1","OPENBLAS_NUM_THREADS":"1"},timeout=45)
                code=result.returncode
            except subprocess.TimeoutExpired:code=124
        passed=code!=0 and expected_error in (folder/"solver.log").read_text() if expected_error else code==0
        record={"id":name,"command":command,"seconds":time.monotonic()-start,"returncode":code,
                "status":"PASS" if passed else "FAIL","expected_error":expected_error,"inputs":inputs,
                "outputs":{path.name:digest(path) for path in folder.iterdir() if path.is_file() and path.name not in inputs}}
        save(folder/"execution.json",record);report["runs"].append(record);persist()
        if not passed:raise RuntimeError("bounded recovery check failed: "+name)
        return folder
    for model,kind in (("SA","QCR"),("SST","EPM")):
        options={"KIND_TURB_MODEL":model,"SA_OPTIONS":"QCR2000" if model=="SA" else "NONE",
                 "STRESS_DISCREPANCY":"NONE" if model=="SA" else "EPM_MATCHED",
                 "UQ_COMPONENT":"1","UQ_DELTA_B":"1","UQ_URLX":"0.1"}
        donor=run(model+"_donor",options);summary=boundary_summary(read_csv(donor/"restart.csv"))
        entry={"boundary_stress":summary,"donor_convergence":"NOT_CLAIMED_FOUR_ITERATIONS",
               "nodal_csv_sha256":digest(donor/"restart.csv"),"physical_fit":"NOT_CHECKED"}
        report["donors"][model]=entry;persist()
        target=root/(model+"_physical_target.suf")
        try:
            result=fit(donor/"restart.csv",root/"inputs/field_mesh.npz",root/"definition.json",probe,target,1,1,kind,model+"_four_iteration_donor")
        except ValueError as error:
            if "incompatible with declared wall/support map" not in str(error):raise
            entry["physical_fit"]="REJECTED_INCOMPATIBLE_BOUNDARY"
            entry["physical_fit_report"]=json.loads(Path(str(target)+".fit.json").read_text())
            if target.exists():raise AssertionError("rejected target must not create a field")
        else:
            entry["physical_fit"]="COMPATIBLE";entry["physical_fit_report"]=result
            persist();raise RuntimeError("fixture now fits: design and verify iterative recovery before claiming it")
        offline=root/(model+"_offline_only_target.suf")
        entry["unrestricted_nodal_fit"]=fit(donor/"restart.csv",root/"inputs/field_mesh.npz",root/"unrestricted_definition.json",
                                          probe,offline,1,1,kind,model+"_OFFLINE_ONLY")
        if entry["unrestricted_nodal_fit"]["relative_max_nodal_error"]>1e-12:
            raise AssertionError("unrestricted nodal representation unexpectedly inaccurate")
        run(model+"_boundary_rejection",{"ITER":"1","KIND_TURB_MODEL":model,"SA_OPTIONS":"NONE",
             "STRESS_DISCREPANCY":"PRESCRIBED","STRESS_DISCREPANCY_PURPOSE":"COUPLING_CONTROL"},
             offline,"requires exact zero stress on every physical")
        entry["receiver"]="REJECTED_BEFORE_ITERATION";persist()
    report.update(status="PASS_GATE_CHECKS_RECOVERY_BLOCKED",
                  recovery_blocker="Donor corrections violate the prescribed-field zero-boundary contract; no taper or clipping applied.",
                  next_step="Choose a nontrivial compatible donor, or separately implement and verify prescribed boundary tractions. Do not claim recovery from these donors.")
    persist();print(json.dumps({"status":report["status"],"process_checks":len(report["runs"])}))


if __name__=="__main__":main()
