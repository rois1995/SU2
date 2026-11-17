#!/usr/bin/env python3
"""Reuse the house serial C492 run and 6.9 physical guard; apply N14's exact >=689 purpose."""
import argparse
import importlib.util
import json
import os
import sys
from pathlib import Path

HERE=Path(__file__).resolve().parent
ROOT=HERE.parent
NIGHT=ROOT.parents[1]
STAGED=NIGHT.parent
N11=NIGHT/"N11"
REFERENCE=next((N11/"validation/cases").glob("section6_2_bl_*"))
PHYSICAL=N11/"validation/validate_n11.py"
IDENTITY=NIGHT/"N9/section6_6/drafts/check_g3_identity_66.py"
FOCUSED=HERE/"run_focused552.py"


def load(name,path):
    spec=importlib.util.spec_from_file_location(name,path)
    module=importlib.util.module_from_spec(spec)
    sys.modules[name]=module
    spec.loader.exec_module(module)
    return module


def input_pins():
    old=json.loads((REFERENCE/"pins.json").read_text())
    source=json.loads((N11/"build535/source_pins.json").read_text())
    binary=json.loads((N11/"build535/build_status.json").read_text())
    excluded=set(source)|{binary["binary"],binary["test_binary"]}
    pins={key:value for key,value in old.items() if key not in excluded}
    return pins


def configure_house(focused):
    source=focused.HOUSE.read_text()
    needle='timeout_seconds = 2400 if case == "m6_stage" else 600'
    assert source.count(needle)==1
    source=source.replace(needle,'timeout_seconds = 2400 if case == "m6_stage" else 900')
    def house():
        namespace={"__name__":"n14_g3_house","__file__":str(focused.HOUSE)}
        exec(compile(source,str(focused.HOUSE),"exec"),namespace)
        namespace.update(ROOT=HERE,REPO=focused.REPO,BINARY=focused.REPO/"integration_evidence/build-integrated-v3-p5-20261114/UnitTests/test_driver")
        namespace["CASES"][focused.CASE]=(1,"[NativeActualSavedSurfaceStage3D]")
        return namespace
    focused.load_house=house
    original=focused.add_case_provenance
    extras=input_pins()
    def provenance(house,paths):
        original(house,paths+[Path(__file__).resolve(),PHYSICAL,IDENTITY,REFERENCE/"command.json",REFERENCE/"pins.json"])
        save=house["save"]
        def prepared(path,value):
            if Path(path).name=="pins.json":
                assert all(house["sha"](item)==digest for item,digest in extras.items())
                value.update(extras)
                config=Path(path).parent/"run.cfg"
                config.write_bytes((REFERENCE/"run.cfg").read_bytes())
                value[str(config.resolve())]=house["sha"](config)
                os.environ["SU2_NATIVE_FROZEN_CONFIG"]=str(config.resolve())
            save(path,value)
        house["save"]=prepared
    focused.add_case_provenance=provenance


def gate(case):
    physical=load("n14_physical_guard",PHYSICAL).physical_case(case)
    identity=load("n14_edit_parser",IDENTITY)
    edits,_,stages,_,_=identity.records(case)
    count,_=identity.accepted_count(case,stages)
    assert len(edits)==count
    return {"passed":physical["passed"] and count>=689,"case":str(case),"physical":physical,
            "purpose":{"accepted_splits":count,"minimum":689,"passed":count>=689}},edits


def pair(first,second):
    a,left=gate(first);b,right=gate(second)
    common=min(len(left),len(right))
    mismatch=next((i for i in range(common) if left[i]!=right[i]),None)
    return {"passed":a["passed"] and b["passed"] and mismatch is None,"first":a,"second":b,
            "compared_edits":common,"first_mismatch_index":mismatch}


def launch(label,host):
    output=HERE/f"n14_g3_{label}_gate.json"
    assert not output.exists(),"do not repeat an action"
    if label=="second":assert json.loads((HERE/"n14_g3_first_gate.json").read_text())["passed"]
    focused=load("n14_g3_house_frontend",FOCUSED)
    focused.host_preflight(host)
    load("n14_g3_alias",ROOT/"build552_aliases.py").verify_and_publish()
    focused.CASE=f"n14_g3_{label}"
    focused.FILTER="[NativeActualSavedSurfaceStage3D]"
    command=json.loads((REFERENCE/"command.json").read_text())
    for key,value in command["environment"].items():
        if key.startswith("SU2_NATIVE_") and key!="SU2_NATIVE_FROZEN_CONFIG":os.environ[key]=value
    os.environ["OMPI_MCA_osc"]="pt2pt"
    configure_house(focused)
    terminal=focused.run_once(host)
    receipt,_=gate(Path(terminal["case"]))
    receipt["terminal"]=terminal
    output.write_text(json.dumps(receipt,indent=2)+"\n")
    return receipt


def self_check():
    def purpose(count):return count>=689
    assert not purpose(688) and purpose(689) and purpose(690)
    focused=load("n14_g3_no_launch",FOCUSED)
    focused.CASE="n14_g3_first"
    configure_house(focused)
    house=focused.load_house()
    assert house["CASES"][focused.CASE]==(1,"[NativeActualSavedSurfaceStage3D]")
    assert house["ROOT"]==HERE
    return {"passed":True,"launched":False,"reference_case":str(REFERENCE),"purpose_minimum":689}


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument("action",choices=("self-check","first","second","pair"))
    parser.add_argument("--host-preflight",type=Path)
    args=parser.parse_args()
    if args.action=="self-check":result=self_check()
    elif args.action=="pair":
        first=Path(json.loads((HERE/"n14_g3_first_gate.json").read_text())["case"])
        second=Path(json.loads((HERE/"n14_g3_second_gate.json").read_text())["case"])
        result=pair(first,second)
        output=HERE/"n14_g3_pair_comparison.json";assert not output.exists()
        output.write_text(json.dumps(result,indent=2)+"\n")
    else:
        assert args.host_preflight
        result=launch(args.action,args.host_preflight)
    print(json.dumps(result,indent=2))
    raise SystemExit(0 if result["passed"] else 1)


if __name__=="__main__":main()
