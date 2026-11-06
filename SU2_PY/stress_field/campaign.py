#!/usr/bin/env python3
"""Prepare and inspect saved local experiments. This tool never launches CFD."""
import argparse
import csv
import hashlib
import json
from pathlib import Path
import re
import shutil
import subprocess
import numpy as np


def digest(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def save(path, data):
    Path(path).write_text(json.dumps(data,indent=2,allow_nan=False)+"\n")


def config_options(text):
    result={};key=None
    for raw in text.splitlines():
        line=raw.split("%",1)[0].strip()
        if not line:continue
        match=re.match(r"^([A-Z][A-Z0-9_]*)\s*=\s*(.*)$",line)
        if match:
            key,value=match.groups()
            if key in result:raise ValueError("duplicate configuration option: "+key)
            result[key]=value
        elif key and result[key].count("(")>result[key].count(")"):
            result[key]+=" "+line
        else:raise ValueError("unrecognized configuration line: "+line)
    if any(v.count("(")!=v.count(")") for v in result.values()):raise ValueError("unbalanced configuration list")
    return result


def prepare(plan_path, output, config_probe=None, field_probe=None):
    plan_path=Path(plan_path).resolve();plan=json.loads(plan_path.read_text());output=Path(output).resolve()
    if plan.get("schema")!="SU2_STRESS_CAMPAIGN_1":raise ValueError("unsupported plan schema")
    if output.exists():raise ValueError("output must be new; refusing to overwrite or reuse stale results")
    rows=[];ids=set()
    for run in plan["runs"]:
        name=run["id"]
        if not re.fullmatch(r"[A-Za-z0-9_-]+",name) or name in ids:raise ValueError("invalid/duplicate run id")
        ids.add(name)
        for key in ("config","mesh"):
            if not (plan_path.parent/run[key]).is_file():raise ValueError("missing input: "+run[key])
    output.mkdir(parents=True)
    save(output/"plan.json",plan)
    for run in plan["runs"]:
        folder=output/run["id"];folder.mkdir()
        config_path=(plan_path.parent/run["config"]).resolve();options=config_options(config_path.read_text())
        options.update({k:str(v) for k,v in run.get("options",{}).items()})
        files={"source.cfg":config_path,"mesh"+Path(run["mesh"]).suffix:(plan_path.parent/run["mesh"]).resolve()}
        options["MESH_FILENAME"]="mesh"+Path(run["mesh"]).suffix
        if run.get("field"):
            files["field.suf"]=(plan_path.parent/run["field"]).resolve();options["STRESS_DISCREPANCY_FILE"]="field.suf"
        if run.get("restart"):
            restart=(plan_path.parent/run["restart"]).resolve();files["initial"+restart.suffix]=restart
            if Path(str(restart.with_suffix(""))+".stress").exists():files["initial.stress"]=restart.with_suffix(".stress")
            options.update(SOLUTION_FILENAME="initial",RESTART_SOL="YES")
        inputs={}
        for name,source in files.items():
            shutil.copyfile(source,folder/name);inputs[name]={"source":str(source),"sha256":digest(folder/name)}
        (folder/"resolved.cfg").write_text("% Prepared only; no CFD run has been executed.\n"+"".join(f"{k}= {v}\n" for k,v in options.items()))
        validation={"config":"NOT_CHECKED","field":"NOT_APPLICABLE"}
        for kind,probe,command in (("config",config_probe,["resolved.cfg","PARSE","unused"]),
                                   ("field",field_probe,["field.suf"])):
            if kind=="field" and "field.suf" not in inputs:continue
            validation[kind]="NOT_CHECKED"
            if probe:
                result=subprocess.run([str(Path(probe).resolve()),*command],cwd=folder,input="",text=True,capture_output=True)
                (folder/(kind+"_validation.log")).write_text(result.stdout+result.stderr)
                if result.returncode:raise ValueError(f"{run['id']}: {kind} validation failed; inspect its log")
                validation[kind]="PASS"
        inputs["resolved.cfg"]={"sha256":digest(folder/"resolved.cfg")}
        record={**run,"status":"PREPARED_NOT_RUN","inputs":inputs,"validation":validation,
                "plan_sha256":digest(plan_path),"CFD_runs":0}
        save(folder/"run.json",record)
        rows.append({"id":run["id"],"stage":run.get("stage","UNASSIGNED"),"model":options.get("KIND_TURB_MODEL"),
                     "mode":options.get("STRESS_DISCREPANCY","NONE"),"purpose":options.get("STRESS_DISCREPANCY_PURPOSE","SCENARIO"),
                     "amplitude":options.get("STRESS_DISCREPANCY_SCALE","1"),"field_sha256":inputs.get("field.suf",{}).get("sha256",""),
                     "status":"PREPARED_NOT_RUN","reason":"execution excluded by user scope","evidence":str(folder/"run.json")})
    if not rows:raise ValueError("empty campaign")
    with (output/"run_matrix.csv").open("w",newline="") as stream:
        writer=csv.DictWriter(stream,fieldnames=list(rows[0]));writer.writeheader();writer.writerows(rows)
    return {"prepared":len(rows),"executed":0}


def read_csv(path):
    with Path(path).open(newline="") as stream:
        reader=csv.DictReader(stream,skipinitialspace=True)
        if not reader.fieldnames or len(set(reader.fieldnames))!=len(reader.fieldnames):raise ValueError("missing/duplicate CSV headers")
        rows=list(reader)
    if not rows:raise ValueError("empty CSV")
    data={key:np.array([float(row[key]) for row in rows]) for key in reader.fieldnames}
    if not all(np.isfinite(x).all() for x in data.values()):raise ValueError("nonfinite output data")
    return data


def stress_budget(data, covariance_tolerance=0.0):
    """Integrate nodal diagnostics over owned volumes; these are not face-flux balances."""
    if not np.isfinite(covariance_tolerance) or covariance_tolerance < 0:
        raise ValueError("covariance tolerance must be finite and nonnegative")
    volume=data["Stress_Volume"]
    if np.any(volume<0) or not np.any(volume>0):raise ValueError("invalid owned volumes")
    owned=volume>0;total=float(volume.sum())
    tensor=np.zeros((len(volume),3,3))
    for name,i,j in (("XX",0,0),("YY",1,1),("ZZ",2,2),("XY",0,1),("XZ",0,2),("YZ",1,2)):
        tensor[:,i,j]=tensor[:,j,i]=data["Stress_E_"+name]
    result={"scope":"nodal volume quadrature, not exact discrete energy balance","owned_volume":total,
            "max_absolute_trace":float(np.max(abs(np.trace(tensor[owned],axis1=1,axis2=2)))),
            "max_stress_norm":float(np.max(np.linalg.norm(tensor[owned],axis=(1,2)))),
            "stress_work_integral":float(volume@data["Stress_Work"])}
    if "Stress_PARENT" in data:
        for name in ("PARENT","RAW","LIMITED","OMEGA","CAP_GAP"):
            result[name.lower()+"_integral"]=float(volume@data["Stress_"+name])
        result["absolute_cap_gap_integral"]=float(volume@abs(data["Stress_CAP_GAP"]))
        for name in ("LOWER_CAP","UPPER_CAP","K_FLOOR","OMEGA_FLOOR"):
            mask=data["Stress_"+name]
            if not np.isin(mask,[0,1]).all():raise ValueError("invalid diagnostic flag: "+name)
            result[name.lower()+"_volume_fraction"]=float(volume@mask/total)
        covariance=data["Stress_COV_MIN"]
        result["minimum_covariance_eigenvalue"]=float(covariance[owned].min())
        result["negative_covariance_volume_fraction"]=float(volume@(covariance < -covariance_tolerance)/total)
        result["covariance_tolerance"]=covariance_tolerance
    return result


def extract(run_path,schema_path,output):
    root=Path(run_path).resolve();record=json.loads((root/"run.json").read_text());schema=json.loads(Path(schema_path).read_text())
    for name,info in record["inputs"].items():
        if digest(root/name)!=info["sha256"]:raise ValueError("input changed since preparation: "+name)
    execution=json.loads((root/"execution.json").read_text())
    if execution.get("returncode")!=0:raise ValueError("solver did not exit successfully")
    if execution.get("inputs")!={name:info["sha256"] for name,info in record["inputs"].items()}:
        raise ValueError("execution/input fingerprint mismatch")
    result={"run_id":record["id"],"status":"EXTRACTED_NOT_QUALIFIED","execution":execution,"files":{},"fields":{}}
    for kind,description in schema["files"].items():
        path=root/description["path"]
        if execution.get("outputs",{}).get(description["path"])!=digest(path):raise ValueError("output fingerprint missing or stale")
        data=read_csv(path);result["files"][kind]={"sha256":digest(path),"rows":len(next(iter(data.values())))}
        for name in description["required"]:
            if name not in data:raise ValueError(f"missing {kind} column: {name}")
        for name,spec in description.get("metrics",{}).items():
            values=data[spec["column"]];window=int(spec.get("window",min(10,len(values))))
            if not 1<=window<=len(values):raise ValueError("invalid history window")
            result["fields"][name]={"final":float(values[-1]),"window_range":float(np.ptp(values[-window:])),"units":spec["units"]}
        if description.get("volume_column"):
            volume=data[description["volume_column"]]
            if np.any(volume<=0):raise ValueError("nonpositive owned control volume")
            result["files"][kind]["integrals"]={name:float(volume@data[name]) for name in description.get("integrate",[])}
        if description.get("stress_budget"):
            result["files"][kind]["stress_budget"]=stress_budget(data,float(description.get("covariance_tolerance",0)))
    save(output,result);return result


def compare(left,right,columns,scales,tolerance,output,key):
    a,b=read_csv(left),read_csv(right)
    if key not in a or key not in b:raise ValueError("comparison requires a stable row id")
    if len(set(a[key]))!=len(a[key]) or len(set(b[key]))!=len(b[key]):raise ValueError("duplicate row id")
    ia,ib=np.argsort(a[key]),np.argsort(b[key])
    if not np.array_equal(a[key][ia],b[key][ib]):raise ValueError("different row ids; explicit resampling required")
    result={"left_sha256":digest(left),"right_sha256":digest(right),"tolerance":tolerance,"blocks":{}}
    for column,scale in zip(columns,scales):
        if not np.isfinite(scale) or scale<=0:raise ValueError("scales must be finite and positive, fixed before comparison")
        difference=a[column][ia]-b[column][ib];index=int(np.argmax(abs(difference)))
        linf=float(abs(difference[index]));l2=float(np.linalg.norm(difference)/np.sqrt(len(difference)))
        result["blocks"][column]={"absolute_Linf":linf,"absolute_RMS":l2,"scale":scale,"scaled_Linf":linf/scale,
                                  "scaled_RMS":l2/scale,"worst_id":float(a[key][ia[index]]),"pass":linf/scale<=tolerance}
    result["pass"]=all(x["pass"] for x in result["blocks"].values());save(output,result);return result


def main():
    parser=argparse.ArgumentParser(description=__doc__);sub=parser.add_subparsers(dest="action",required=True)
    p=sub.add_parser("prepare");p.add_argument("--plan",required=True);p.add_argument("--output",required=True)
    p.add_argument("--config-probe");p.add_argument("--field-probe")
    p=sub.add_parser("extract");p.add_argument("--run",required=True);p.add_argument("--schema",required=True);p.add_argument("--output",required=True)
    p=sub.add_parser("compare");p.add_argument("--left",required=True);p.add_argument("--right",required=True)
    p.add_argument("--columns",nargs="+",required=True);p.add_argument("--scales",nargs="+",type=float,required=True)
    p.add_argument("--key",default="PointID");p.add_argument("--tolerance",type=float,default=1e-10);p.add_argument("--output",required=True)
    a=parser.parse_args()
    if a.action=="prepare":result=prepare(a.plan,a.output,a.config_probe,a.field_probe)
    elif a.action=="extract":result=extract(a.run,a.schema,a.output)
    else:
        if len(a.columns)!=len(a.scales) or not np.isfinite(a.tolerance) or a.tolerance<0:parser.error("one positive scale per column and a nonnegative tolerance required")
        result=compare(a.left,a.right,a.columns,a.scales,a.tolerance,a.output,a.key)
    print(json.dumps(result,indent=2,allow_nan=False))
    if result.get("pass") is False:raise SystemExit(1)

if __name__=="__main__":main()
