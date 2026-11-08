"""Seal focused producer/reader qualification; never substitute it for M6 acceptance."""
from pathlib import Path
import base64,hashlib,json,math,re,struct,time,xml.etree.ElementTree as ET
import inspect_native_failure3d as reader
ROOT=Path(__file__).resolve().parent;REPO=ROOT.parents[3]
sha=lambda p:hashlib.file_digest(Path(p).open("rb"),"sha256").hexdigest()
def binary_values(array):
    assert array is not None and array.get("format")=="binary" and array.get("type")=="Float64"
    raw=base64.b64decode(array.text,validate=True);size=struct.unpack("<I",raw[:4])[0]
    assert size==len(raw)-4 and size%8==0
    return struct.unpack("<"+"d"*(size//8),raw[4:])
def main():
    output=ROOT/"capture262_qualification.json";assert not output.exists()
    build=json.loads((ROOT/"build262_status.json").read_text());assert build["completed_build"] and build["pins_verified"] and build["production_cfd_byte_identical"]
    assert json.loads((ROOT/"build262_terminal_handles.json").read_text())["all_owned_handles_terminal"]
    pins={**json.loads((ROOT/"build262_source_pins.json").read_text()),**json.loads((ROOT/"build262_helper_pins.json").read_text())}
    for p,h in pins.items():assert sha(p)==h,p
    unit=json.loads((ROOT/"capture261_reader_unit_status.json").read_text());assert unit["exit_code"]==0 and unit["tests"]==14
    for p,h in unit["source_hashes"].items():assert sha(p)==h
    oldrun=ROOT/"m6_runs/euler_build260_paired_geometry_priority_mach_only_real_cfd50k"
    replay=json.loads((oldrun/"capture_compact261_production.json").read_text());assert replay["rows"]==584311 and replay["input_hashes_before"]==replay["input_hashes_after"]
    replaypins=json.loads((oldrun/"capture_compact261_production_pins.json").read_text());assert replaypins["exit_code"]==0 and replaypins["input_hashes_before"]==replaypins["input_hashes_after"]
    for p,h in replaypins["input_hashes_after"].items():assert sha(p)==h
    le=json.loads((ROOT/"capture261_legacy_le_recheck_status.json").read_text());assert le["all_scientific_fields_identical"]
    for p,h in le["input_hashes_after"].items():assert sha(p)==h
    helpers=[Path(__file__),ROOT/"capture262_validate.py",ROOT/"launch_m6262.py"]
    pins.update({str(p):sha(p) for p in helpers});stages=[];exports=[];empty_ranks=0;legacy_limited=0
    for mode,ranks in (("serial",1),("mpi2",2)):
        wrapper_path=ROOT/("capture262_"+mode+"_wrapper.json");wrapper=json.loads(wrapper_path.read_text());assert wrapper["wrapper_sha256_before"]==wrapper["wrapper_sha256_after"]==sha(ROOT/"capture262_validate.py")
        case=Path(wrapper["case"]);status=json.loads((case/"status.json").read_text());assert status["exit_code"]==0 and status["pins_verified"] and not status["changed"]
        assert json.loads((case/"terminal_handles.json").read_text())["all_owned_handles_terminal"]
        casepins=json.loads((case/"pins.json").read_text())
        for p,h in casepins.items():assert sha(p)==h,p
        pins.update(casepins)
        for p in (wrapper_path,case/"status.json",case/"terminal_handles.json",case/"command.json",case/"pins.json"):pins[str(p)]=sha(p)
        clean=re.sub(r"\x1b\[[0-9;]*m","",(case/"run.log").read_text())
        summaries=re.findall(r"All tests passed \((\d+) assertions in (\d+) test case",clean)
        assert len(summaries)==ranks and all(int(c)==1 for _,c in summaries)
        roots=re.findall(r"NATIVE_FAILURE_CAPTURE_TEST construction=(\S+) mapped=(\S+) ranks=(\d+) root=(\S+)",clean)
        assert len(roots)==4 and len({(a,b) for a,b,_,_ in roots})==4
        folder=case/"codec_inspection";assert not folder.exists();folder.mkdir()
        for index,(construction,mapped,nrank,rootname) in enumerate(roots):
            assert int(nrank)==ranks;root=Path(rootname);assert root.is_relative_to(REPO/"integration_evidence/build-integrated-v2/compile-tmp")
            full,missing=reader._load_manifests([str(root/"full_rank*_manifest.json")]);assert not missing and len(full)==ranks
            tight,_=reader._load_manifests([str(root/"tight_rank*_manifest.json")]);tight_by_rank={m["rank"]:m for m in tight}
            expected={};legacy_controls=[]
            for m in full:
                assert m["schema"]==reader.COMPACT_SCHEMA and m["complete_shard"] and m["owned_complete"]
                empty_ranks+=int(mode=="mpi2" and m["working_rows"]==0)
                t=tight_by_rank[m["rank"]];assert t["complete_shard"] and t["written_rows"]==m["written_rows"]
                needed=max(p.stat().st_size for p in m["_resolved_files"].values());assert t["cap_bytes"]==needed
                for role,p in m["_resolved_files"].items():
                    assert p.stat().st_size<=m["_caps"][role] and sha(p)==sha(t["_resolved_files"][role])
                rows=list(reader._jsonl(m["_resolved_files"]["metrics"],m["_caps"]["metrics"],"metrics"));assert len(rows)==m["written_rows"]
                legacy=sum(len((json.dumps(row,separators=(",",":"))+"\n").encode()) for row in rows)
                legacy_limited+=int(legacy>needed);legacy_controls.append({"rank":m["rank"],"compact_bytes":m["_resolved_files"]["metrics"].stat().st_size,"tight_cap":needed,"legacy_json_bytes":legacy})
                for row in rows:expected[row["id"]]=row
                assert not (root/("refused_rank"+str(m["rank"])+"_manifest.json")).exists()
            for prefix in ("full","tight","unknown","capped"):
                pattern=str(root/(prefix+"_rank*_manifest.json"));manifests,_=reader._load_manifests([pattern])
                for m in manifests:
                    pins[str(m["_path"])]=sha(m["_path"])
                    for p in m["_resolved_files"].values():pins[str(p)]=sha(p)
                out=folder/(str(index)+"_"+prefix);result=reader.inspect([pattern],str(out));assert not result["final_mesh_accepted"]
                if prefix in ("full","tight","unknown"):
                    assert result["final_mesh_complete"] and result["actual_full_mesh_cells"]==len(expected)
                    piece=ET.parse(out/"rejected_mesh.vtu").getroot().find("./UnstructuredGrid/Piece")
                    count=int(piece.get("NumberOfCells"));points=int(piece.get("NumberOfPoints"));assert count==len(expected)
                    ids=list(map(int,piece.find("CellData/DataArray[@Name='global_cell_id']").text.split()))
                    for name in ("q","J","geometry_excess_l2"):
                        assert piece.find("PointData/DataArray[@Name='"+name+"']") is None
                        values=binary_values(piece.find("CellData/DataArray[@Name='"+name+"']"));assert len(values)==count
                        if prefix!="unknown":assert all(math.isnan(v) if expected[c][name] is None else v==expected[c][name] for c,v in zip(ids,values))
                    assert len(binary_values(piece.find("PointData/DataArray[@Name='captured_nodal_metric']")))==9*points
                if prefix in ("unknown","capped"):assert result["partial"]
                exports.append({"mode":mode,"construction":construction,"mapped":mapped,"prefix":prefix,"partial":result["partial"],"full_mesh_complete":result["final_mesh_complete"],"cells":result["actual_full_mesh_cells"],"inspection_manifest_sha256":sha(out/"inspect_manifest.json"),"legacy_cap_controls":legacy_controls if prefix=="tight" else []})
        stages.append({"mode":mode,"ranks":ranks,"case":str(case),"assertions":[int(a) for a,_ in summaries],"test_cases_each_rank":1})
    assert empty_ranks==4 and legacy_limited>=4
    for p,h in pins.items():assert sha(p)==h,p
    result={"all_required_checks_passed":True,"mpi_ranks":2,"stages":stages,"exports":exports,"actual_empty_rank_generations":empty_ranks,"legacy_metadata_would_exceed_tight_cap_controls":legacy_limited,"source_pins_sha256":sha(ROOT/"build262_source_pins.json"),"qualification_pins":pins,"production_cfd_byte_identical":True,"runtime_full_m6_capture_verified":False,"m6_workflow_accepted":False,"scope":"Capture-only qualification;no native gates,construction,ownership,metric,transfer orbudget change.No M6acceptance/fullcurrentmesh claim. Fullsame-seedtwo-eventM6 immediatelynext."}
    output.write_text(json.dumps(result,indent=2)+"\n");print(json.dumps({k:v for k,v in result.items() if k not in ("exports","qualification_pins")}))
if __name__=="__main__":main()
