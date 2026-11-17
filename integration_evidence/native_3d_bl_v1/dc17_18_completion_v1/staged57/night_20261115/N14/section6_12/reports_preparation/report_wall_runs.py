#!/usr/bin/env python3
"""Report completed wall runs only; no gate, input or numerical result is modified."""
import argparse
import csv
import hashlib
import importlib.util
import json
import re
from pathlib import Path

ROOT=Path(__file__).resolve().parents[1]
NIGHT=ROOT.parents[1]
PARSER=NIGHT/"N9/section6_6/drafts/check_g3_identity_66.py"
spec=importlib.util.spec_from_file_location("n14_wall_fields",PARSER)
fields=importlib.util.module_from_spec(spec);spec.loader.exec_module(fields)
MARKER=re.compile(r"\b(NATIVE3D_WALL_[A-Z_]+|NATIVE3D_STAGE)\s")
METRICS=("accepted","rounds","measured","geometry_trials","geometry_rejected","guidance_trials",
         "guidance_rejected","guidance_seconds","upper_seconds","geometry_bad","size_bad",
         "geometry_debt","size_debt","maximum_work_bytes","upper_refinements")


def sha(path):
    return hashlib.file_digest(Path(path).open("rb"),"sha256").hexdigest()


def parse_text(text):
    result=[]
    for number,line in enumerate(text.splitlines(),1):
        found=MARKER.search(line)
        if not found:continue
        row=fields.key_values(line[found.end():])
        if found[1]=="NATIVE3D_STAGE" and row.get("stage")!="exit":continue
        result.append(dict(row,marker=found[1],line=number))
    return result


def self_check():
    rows=parse_text('NATIVE3D_WALL_REASON rank=0 text="facet # cannot see cell #" count=3\n'
                    'NATIVE3D_WALL_SPLITS rank=0 new_faces_unknown_parent=2\n'
                    'NATIVE3D_STAGE rank=0 stage=entry accepted=0\n'
                    'NATIVE3D_STAGE rank=0 stage=exit accepted=689\n')
    assert len(rows)==3 and rows[0]["text"]=="facet # cannot see cell #"
    assert rows[1]["new_faces_unknown_parent"]=="2" and rows[-1]["accepted"]=="689"
    assert not parse_text("NATIVE3D_WALL_SOURCELESS_BADPREFIX\n")
    return {"passed":True,"launched":False}


def write_report(cases,out):
    out.mkdir(parents=True,exist_ok=True)
    all_rows=[];inputs={};exits={}
    for label,case in cases:
        log=case/"run.log";rows=parse_text(log.read_text(errors="replace"))
        assert rows,"report helper found no wall rows"
        inputs[label]={"case":str(case),"run_sha256":sha(log)}
        all_rows.extend(dict(row,case=label) for row in rows)
        exits[label]=[row for row in rows if row["marker"]=="NATIVE3D_STAGE"]
    report={"scope":"report only; result gates remain in their separate receipts",
            "inputs":inputs,"rows":all_rows,"helper_sha256":sha(__file__),"parser_sha256":sha(PARSER)}
    (out/"wall_rows.json").write_text(json.dumps(report,indent=2)+"\n")
    columns=["case","marker","line"]+sorted(set().union(*(set(row) for row in all_rows))-{ "case","marker","line"})
    with (out/"wall_rows.csv").open("w",newline="") as stream:
        writer=csv.DictWriter(stream,fieldnames=columns,lineterminator="\n");writer.writeheader();writer.writerows(all_rows)
    lines=["# Wall-stage observations","","Report only; accepted mesh and result gates are separate.","",
           "Missing counter fields stay n/a. Parent coverage/remote attribution limits are explicit in WALL_SPLITS rows.","",
           "| case / rank | "+" | ".join(METRICS)+" |","|---|"+"---:|"*len(METRICS)]
    for label,_ in cases:
        for row in exits[label]:lines.append("| "+label+" / "+row.get("rank","n/a")+" | "+" | ".join(row.get(k,"n/a") for k in METRICS)+" |")
    lines += ["","## Wall counter rows","","| case | marker | fields |","|---|---|---|"]
    for row in all_rows:
        if row["marker"]=="NATIVE3D_STAGE":continue
        text=" ".join(k+"="+str(value) for k,value in row.items() if k not in {"case","marker","line"})
        lines.append("| "+row["case"]+" | "+row["marker"]+" | "+text.replace("|","\\|")+" |")
    (out/"wall_table.md").write_text("\n".join(lines)+"\n")
    return {"reported":True,"cases":len(cases),"rows":len(all_rows),"output":str(out)}


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--self-check",action="store_true")
    parser.add_argument("--case",action="append",default=[])
    parser.add_argument("--out",type=Path)
    args=parser.parse_args()
    if args.self_check:result=self_check()
    else:
        assert args.case and args.out
        cases=[(value.split("=",1)[0],Path(value.split("=",1)[1]).resolve(strict=True)) for value in args.case]
        result=write_report(cases,args.out)
    print(json.dumps(result,indent=2))


if __name__=="__main__":main()
