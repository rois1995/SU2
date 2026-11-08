"""Lossless, streaming size check on actual sealed260 metadata; no mesh decisions."""
from pathlib import Path
import hashlib,json,sys,time
ROOT=Path(__file__).resolve().parent
RUN=ROOT/"m6_runs/euler_build260_paired_geometry_priority_mach_only_real_cfd50k"
FLAGS=("cache_valid","q_measured","j_measured","geometry_measured","q_bad","j_bad","geometry_bad","size_bad","height_bad","height_unresolved","surface","near_feature","feature_known")
BINDINGS=("id","feature","version","dimension","u","v","mobility_dimension")
def pack(row):
    return [row["id"],row["version"],sum(int(row[k])<<i for i,k in enumerate(FLAGS)),row["q"],row["J"],row["geometry_excess_l2"],row["edges"],[[r["vertices"],r["blocked"],r["epoch"]] for r in row["repairs"]],[None if b is None else [b[k] for k in BINDINGS] for b in row["bindings"]]]
def unpack(row):
    assert len(row)==9 and 0<=row[2]<(1<<len(FLAGS))
    out={k:bool(row[2]&(1<<i)) for i,k in enumerate(FLAGS)}
    out.update(id=row[0],version=row[1],q=row[3],J=row[4],geometry_excess_l2=row[5],edges=row[6],repairs=[dict(vertices=r[0],blocked=r[1],epoch=r[2]) for r in row[7]],bindings=[None if b is None else dict(zip(BINDINGS,b)) for b in row[8]])
    return out
def cpp_json(v):
    if v is None:return "null"
    if isinstance(v,bool):return "true" if v else "false"
    if isinstance(v,int):return str(v)
    if isinstance(v,float):return format(v,".17g")
    return "["+",".join(cpp_json(x) for x in v)+"]"
def main():
    mode=sys.argv[1] if len(sys.argv)>1 else "prototype";assert mode in ("prototype","production")
    out=RUN/("capture_compact261_"+mode+".json");assert not out.exists()
    decode=unpack
    if mode=="production":
        import inspect_native_failure3d as reader
        decode=reader._unpack_compact_metric
    ready=json.loads((RUN/"capture_ready.json").read_text());assert json.loads((RUN/"terminal_handles.json").read_text())["all_owned_handles_terminal"]
    sha=lambda p:hashlib.file_digest(Path(p).open("rb"),"sha256").hexdigest()
    paths=[RUN/m["files"]["metrics"] for m in ready["manifests"]]
    before={str(p):sha(p) for p in paths};assert all(before[str(p)]==ready["files"][str(p.resolve())]["sha256"] for p in paths)
    started=time.monotonic();rows=[]
    for m,path in zip(ready["manifests"],paths):
        count=compact=largest_clean=0
        with path.open() as stream:
            for line in stream:
                row=json.loads(line);packed=pack(row);encoded=cpp_json(packed)+"\n";assert decode(json.loads(encoded))==row
                size=len(encoded.encode());assert size<=(64<<10);count+=1;compact+=size
                if not any(row[k] for k in ("q_bad","j_bad","geometry_bad","size_bad","height_bad","height_unresolved")):largest_clean=max(largest_clean,size)
        assert count==m["written_rows"]
        missing=m["working_rows"]-count
        rows.append({"rank":m["rank"],"rows":count,"missing_rows":missing,"original_bytes":path.stat().st_size,"compact_cpp17_bytes":compact,"largest_observed_clean_row":largest_clean,"observed_clean_max_missing_scenario_bytes":compact+missing*largest_clean,"cap_bytes":m["cap_bytes"]})
    assert sum(r["rows"] for r in rows)==ready["owned_rows"]==584311
    after={p:sha(p) for p in before};assert before==after
    result={"scope":"Lossless roundtripALLavailableactualmetadata,includingexactfull edgepairs andbindings;NOuncapturedrows reconstructed. Missing-row scenario uses observedcleanmaximum,NOTa proof of futurefullcapture. Existingcapsunchanged.","mode":mode,"rows":sum(r["rows"] for r in rows),"ranks":rows,"input_hashes_before":before,"input_hashes_after":after,"helper_sha256":sha(Path(__file__)),"seconds":time.monotonic()-started}
    out.write_text(json.dumps(result,indent=2)+"\n");print(json.dumps(result))
if __name__=="__main__":main()
