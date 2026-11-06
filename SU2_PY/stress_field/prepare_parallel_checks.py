#!/usr/bin/env python3
"""Prepare bounded localhost MPI comparisons and repartitioned restarts; never launches CFD."""
import argparse
import json
from pathlib import Path
from campaign import config_options, digest, prepare as prepare_campaign, save
from prepare_example import prepare as prepare_fixture


def prepare(output, solver, launcher="mpiexec", config_probe=None, field_probe=None):
    root=Path(output).resolve()
    root.mkdir(parents=True,exist_ok=False)
    prepare_fixture(root/"inputs")
    base=root/"inputs/base.cfg"
    options=config_options(base.read_text())
    options.update(OUTPUT_FILES="(RESTART_ASCII)",RESTART_FILENAME="restart",READ_BINARY_RESTART="NO",
                   INC_ENERGY_EQUATION="YES",CFL_ADAPT="NO",CONV_STARTITER="1000")
    base.write_text(''.join(f'{k}= {v}\n' for k,v in options.items()))
    runs=[];jobs=[];comparisons=[]
    for model in ("SA","SST"):
        columns=["Pressure","Velocity_x","Velocity_y","Enthalpy","Temperature"]
        columns += ["Nu_Tilde"] if model=="SA" else ["Turb_Kin_Energy","Omega"]
        for mode in ("base","zero","positive"):
            for ranks in (1,2):
                name=f"{model}_{mode}_{ranks}rank"
                opt={"KIND_TURB_MODEL":model,"STRESS_DISCREPANCY":"NONE" if mode=="base" else "PRESCRIBED"}
                run={"id":name,"stage":"LOCAL_MPI","config":"base.cfg","mesh":"mesh.su2","options":opt}
                if mode!="base":run["field"]=mode+".suf"
                runs.append(run)
                jobs.append({"id":name,"ranks":ranks,"depends_on":[],"checkpoint_copies":[]})
            comparisons.append({"left":f"{model}_{mode}_1rank","right":f"{model}_{mode}_2rank","columns":columns,"tolerance":1e-9})
        for ranks in (1,2):
            complete=f"{model}_complete_{ranks}rank"
            resumed=f"{model}_restart_{3-ranks}to{ranks}rank"
            for name,iterations,restart in ((complete,4,False),(resumed,2,True)):
                opt={"KIND_TURB_MODEL":model,"STRESS_DISCREPANCY":"PRESCRIBED","ITER":iterations}
                if restart:opt.update(RESTART_SOL="YES",SOLUTION_FILENAME="initial")
                runs.append({"id":name,"stage":"LOCAL_MPI_RESTART","config":"base.cfg","mesh":"mesh.su2","field":"positive.suf","options":opt})
                donor=f"{model}_positive_{3-ranks}rank"
                copies=[{"from":f"{donor}/restart.{suffix}","to":f"{name}/initial.{suffix}"} for suffix in ("csv","stress")] if restart else []
                jobs.append({"id":name,"ranks":ranks,"depends_on":[donor] if restart else [],"checkpoint_copies":copies})
            comparisons.append({"left":complete,"right":resumed,"columns":columns,"tolerance":1e-9})
    save(root/"inputs/parallel_plan.json",{"schema":"SU2_STRESS_CAMPAIGN_1","runs":runs})
    prepare_campaign(root/"inputs/parallel_plan.json",root/"runs",config_probe,field_probe)
    for job in jobs:
        job.update(command=[str(launcher),"--host","localhost","-n",str(job["ranks"]),str(Path(solver).resolve()),"resolved.cfg"],
                   cwd="runs/"+job["id"],timeout_seconds=45,environment={"OMP_NUM_THREADS":"1","OPENBLAS_NUM_THREADS":"1"},status="PREPARED_NOT_RUN")
    for pair in comparisons:
        pair.update(file="restart.csv",key="PointID",scales=[1]*len(pair["columns"]),status="NOT_RUN")
    manifest={"schema":"SU2_STRESS_PARALLEL_PLAN_1","status":"PREPARED_NOT_RUN","mesh_points":81,
              "maximum_iterations":4,"CFD_runs":0,"cluster_access":False,
              "solver":str(Path(solver).resolve()),"solver_sha256":digest(solver) if Path(solver).is_file() else None,
              "requires":"MPI-enabled build of this branch and a working localhost MPI launcher",
              "checkpoint_copy_root":"runs","jobs":jobs,"comparisons":comparisons}
    save(root/"parallel_checks.json",manifest)
    (root/"README.md").write_text("""# Prepared local MPI checks

No solver or launcher was executed by this preparation tool. All 20 jobs use
81 nodes, one thread, one or two localhost ranks, and at most four iterations.
The manifest lists exact argument arrays, working directories, dependencies,
45-second timeouts and ten comparisons at fixed 1e-9 internal-unit tolerance.

Use an MPI-enabled solver built from this branch. Verify the launcher really
starts the requested ranks; a serial executable under mpiexec is not a valid test.
Run jobs in manifest order in their working directories. Before restart jobs,
copy both listed checkpoint files relative to the runs directory. Stop on any
nonzero exit, missing output or mismatch; retain logs and hash actual outputs.
Compare restart.csv rows by PointID using campaign.py compare and the listed
columns/scales. This checks 1/2-rank agreement and both directions of partition
change. A successful exit alone does not establish either comparison.

These inputs are wiring fixtures, not physical validation or production cases.
The current sandbox cannot initialize MPI sockets; do not bypass that restriction.
""")
    return manifest


if __name__=="__main__":
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output",required=True);parser.add_argument("--solver",required=True)
    parser.add_argument("--launcher",default="mpiexec")
    parser.add_argument("--config-probe");parser.add_argument("--field-probe")
    args=parser.parse_args()
    result=prepare(args.output,args.solver,args.launcher,args.config_probe,args.field_probe)
    print(json.dumps({"status":result["status"],"jobs":len(result["jobs"]),"executed":0}))
