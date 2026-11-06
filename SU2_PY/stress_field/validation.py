#!/usr/bin/env python3
"""Freeze a study definition and assess saved scalar ranges. Never runs CFD or fits parameters."""
import argparse
import csv
import json
from pathlib import Path
import numpy as np
from campaign import digest


def new_output(path, value):
    with Path(path).open("x") as stream:
        json.dump(value,stream,indent=2,allow_nan=False);stream.write("\n")


def freeze(study_path, output):
    source=Path(study_path).resolve();study=json.loads(source.read_text())
    if study.get("schema")!="SU2_STRESS_STUDY_1" or not study.get("family"):
        raise ValueError("study needs schema and declared flow family")
    lock={"schema":"SU2_STRESS_STUDY_LOCK_1","family":study["family"],"study_sha256":digest(source),"cases":[]}
    parameter=(source.parent/study["parameters"]).resolve()
    lock["parameters"]={"path":str(parameter),"sha256":digest(parameter)}
    ids=set();groups={};splits=set()
    for case in study["cases"]:
        name,group,split=case["id"],case["group"],case["split"]
        if not name or not group or name in ids or split not in ("CALIBRATION","VALIDATION"):
            raise ValueError("invalid/duplicate case or split")
        if group in groups and groups[group]!=split:raise ValueError("case family leaks across calibration/validation split")
        ids.add(name);groups[group]=split;splits.add(split)
        path=(source.parent/case["reference"]).resolve()
        with path.open(newline="") as stream: rows=list(csv.DictReader(stream))
        outputs=set()
        for row in rows:
            if not row["Output"] or row["Output"] in outputs:raise ValueError("duplicate reference output")
            outputs.add(row["Output"])
            low,high,scale=(float(row[k]) for k in ("Lower","Upper","Scale"))
            if not np.isfinite([low,high,scale]).all() or low>high or scale<=0:raise ValueError("invalid reference interval/scale")
        if not outputs:raise ValueError("empty reference table")
        lock["cases"].append({**case,"reference":str(path),"reference_sha256":digest(path)})
    if splits!={"CALIBRATION","VALIDATION"}:raise ValueError("both calibration and independent validation cases required")
    new_output(output,lock);return lock


def assess(lock_path, results_path, output):
    lock=json.loads(Path(lock_path).read_text())
    if lock.get("schema")!="SU2_STRESS_STUDY_LOCK_1":raise ValueError("invalid study lock")
    parameter=lock["parameters"]
    if digest(parameter["path"])!=parameter["sha256"]:raise ValueError("parameters changed after study was frozen")
    with Path(results_path).open(newline="") as stream: rows=list(csv.DictReader(stream))
    results={}
    for row in rows:
        key=(row["Case"],row["Output"])
        if key in results:raise ValueError("duplicate result")
        results[key]=row
    report={"schema":"SU2_STRESS_ASSESSMENT_1","family":lock["family"],"lock_sha256":digest(lock_path),
            "results_sha256":digest(results_path),"claim":"descriptive scenario comparison; no probability or coverage guarantee","outputs":[]}
    for case in lock["cases"]:
        if digest(case["reference"])!=case["reference_sha256"]:raise ValueError("reference data changed")
        with Path(case["reference"]).open(newline="") as stream: references=list(csv.DictReader(stream))
        for reference in references:
            key=(case["id"],reference["Output"]);row=results.pop(key,None)
            metric={"case":key[0],"output":key[1],"split":case["split"],"status":"MISSING"}
            if row:
                baseline,cost=(float(row[k]) for k in ("Baseline","CostSeconds"))
                requested,completed=int(row["Requested"]),int(row["Completed"])
                if not np.isfinite([baseline,cost]).all() or cost<0 or requested<=0 or not 0<=completed<=requested:
                    raise ValueError("invalid scenario result/counts")
                rlo,rhi,scale=(float(reference[k]) for k in ("Lower","Upper","Scale"))
                if completed==0:
                    if row["Minimum"].strip() or row["Maximum"].strip():raise ValueError("all-failed cases must have empty range")
                    metric.update(status="ALL_SCENARIOS_FAILED",requested=requested,completed=0,cost_seconds=cost,
                                  baseline_distance=max(rlo-baseline,baseline-rhi,0))
                    report["outputs"].append(metric);continue
                lo,hi=(float(row[k]) for k in ("Minimum","Maximum"))
                if not np.isfinite([lo,hi]).all() or lo>hi:raise ValueError("invalid scenario range")
                metric.update(status="COMPLETE" if completed==requested else "INCOMPLETE_SCENARIO_RANGE",
                              miss_distance=max(rlo-hi,lo-rhi,0),scaled_miss_distance=max(rlo-hi,lo-rhi,0)/scale,
                              width=hi-lo,scaled_width=(hi-lo)/scale,baseline_distance=max(rlo-baseline,baseline-rhi,0),
                              requested=requested,completed=completed,cost_seconds=cost)
            report["outputs"].append(metric)
    if results:raise ValueError("results contain cases/outputs outside frozen study")
    new_output(output,report);return report


def main():
    p=argparse.ArgumentParser(description=__doc__);sub=p.add_subparsers(dest="action",required=True)
    f=sub.add_parser("freeze");f.add_argument("--study",required=True);f.add_argument("--output",required=True)
    a=sub.add_parser("assess");a.add_argument("--lock",required=True);a.add_argument("--results",required=True);a.add_argument("--output",required=True)
    args=p.parse_args()
    result=freeze(args.study,args.output) if args.action=="freeze" else assess(args.lock,args.results,args.output)
    print(json.dumps(result,indent=2))

if __name__=="__main__":main()
