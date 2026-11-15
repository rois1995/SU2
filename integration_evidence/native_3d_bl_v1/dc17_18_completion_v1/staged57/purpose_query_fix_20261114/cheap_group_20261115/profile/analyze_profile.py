"""Run inherited v2 summaries and add the requested memory/resolution receipts."""
import json, re, subprocess, sys
from pathlib import Path

HERE=Path(__file__).resolve().parent
R=HERE.parents[2]
OLD=R/"measurement_20261114_v2"

def fields(line):
    out={}
    for token in line.strip().split()[1:]:
        if "=" in token:
            k,v=token.split("=",1);out[k]=v.rstrip(",")
    return out

def stage_events(case):
    rows={}
    for log in sorted((case/"profile").glob("rank*.program.log")):
        counters={}
        for line_no,line in enumerate(log.open(errors="replace"),1):
            if not line.startswith("NATIVE3D_STAGE "):continue
            f=fields(line)
            if f.get("stage")!="exit" or "rank" not in f:continue
            rank=int(f["rank"]);elapsed=float(f.get("elapsed","0"))
            event=counters.get(rank,0);counters[rank]=event+1
            key=(event,rank)
            row={"event":event,"rank":rank,"log":str(log),"line":line_no,"elapsed_s":elapsed}
            for k in ("facet_misses","facet_resolutions","facet_build_work"):
                if k in f:row[k]=int(f[k])
            if "facet_misses" in row and "facet_resolutions" in row:
                row["misses_minus_resolutions_x_48954"]=row["facet_misses"]-row["facet_resolutions"]*48954
            # Identical MPI records can appear in more than one wrapper log; keep one record per rank/event.
            if key not in rows or row["elapsed_s"]>=rows[key]["elapsed_s"]:rows[key]=row
    return [rows[k] for k in sorted(rows)]

def native_memory(case, expected_ranks):
    events={}
    for log in sorted((case/"profile").glob("rank*.program.log")):
        counters={}
        for line_no,line in enumerate(log.open(errors="replace"),1):
            if not line.startswith("NATIVE3D rank="):continue
            f=fields(line)
            if "rank" not in f or "process_peak_rss" not in f:continue
            rank=int(f["rank"]);n=counters.get(rank,0);counters[rank]=n+1
            key=(n,rank);value=int(f["process_peak_rss"])
            row={"event":n,"rank":rank,"process_peak_rss_bytes":value,"log":str(log),"line":line_no}
            if key not in events or value>events[key]["process_peak_rss_bytes"]:events[key]=row
    grouped={}
    for (event,rank),row in events.items():grouped.setdefault(event,{})[rank]=row
    result=[]
    for event,by_rank in sorted(grouped.items()):
        ranks=sorted(by_rank)
        total=sum(by_rank[r]["process_peak_rss_bytes"] for r in ranks)
        result.append({"event":event,"ranks":ranks,"missing_ranks":sorted(set(range(expected_ranks))-set(ranks)),
                       "sum_process_peak_rss_bytes":total,"limit_bytes":20_000_000_000,
                       "gate":"pass" if len(ranks)==expected_ranks and total<=20_000_000_000 else "fail" if total>20_000_000_000 else "unevaluated"})
    return result

def gnu_time(case):
    result=[]
    for p in sorted((case/"profile").glob("rank*.time.txt")):
        text=p.read_text(errors="replace")
        m=re.search(r"Maximum resident set size \(kbytes\):\s*(\d+)",text)
        result.append({"file":str(p),"maximum_resident_set_kib":int(m.group(1)) if m else None,
                       "meaning":"GNU time ru_maxrss for the per-rank recorder exec tree; informational when native summary rows exist"})
    return result

def analyze(case):
    subprocess.run([sys.executable,str(OLD/"summarize_samples.py"),str(case)],check=True)
    if case.name=="run_B":subprocess.run([sys.executable,str(OLD/"analyze_B.py"),str(case)],check=True)
    memory=native_memory(case, 1 if case.name=="run_A" else 6)
    result={"case":str(case),"native_process_peak_rss_by_event":memory,
            "native_summary_memory_gate":"evaluated" if memory else "unevaluated: no NATIVE3D process_peak_rss summary row",
            "gnu_time_peak_rss":gnu_time(case),"facet_resolution_work_by_rank_event":stage_events(case),
            "source_method":{"stage_parser":"NATIVE3D_STAGE exit rows; duplicate MPI log rows deduplicated by event/rank",
                             "facet_formula":"facet_misses - facet_resolutions * 48954",
                             "native_memory_gate":"sum native process_peak_rss bytes per event <= 20000000000"}}
    path=case/"requested_profile_receipts.json";path.write_text(json.dumps(result,indent=2)+"\n")
    return result

if __name__=="__main__":
    assert len(sys.argv)==2 and sys.argv[1] in ("A","B")
    case=HERE/("run_"+sys.argv[1])
    status=json.loads((case/"status.json").read_text())
    assert status.get("state")=="terminal" and status.get("all_owned_handles_terminal")
    result=analyze(case)
    print(json.dumps(result,indent=2))
    if any(row["gate"]=="fail" for row in result["native_process_peak_rss_by_event"]):raise SystemExit(1)
