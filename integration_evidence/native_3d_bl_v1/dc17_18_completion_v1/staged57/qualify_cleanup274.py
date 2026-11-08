"""Seal the connected qualification before the immediate full M6 attempt."""
from pathlib import Path
import hashlib
import json
import re
import sys
import time

ROOT = Path(__file__).resolve().parent
assert len(sys.argv) == 4
cases = [Path(p).resolve() for p in sys.argv[1:3]]
assert all(p.parent == (ROOT / "cases").resolve() for p in cases)
output = ROOT / "cleanup274_qualification.json"
assert not output.exists()
sha = lambda p: hashlib.file_digest(p.open("rb"), "sha256").hexdigest()
pins, stages = {}, []
focused=Path(sys.argv[3]).resolve()
assert focused.parent==(ROOT/'cases').resolve()
assert json.loads((focused/'status.json').read_text())['exit_code']==0
assert json.loads((focused/'terminal_handles.json').read_text())['all_owned_handles_terminal']
assert re.search(r'All tests passed \([0-9]+ assertions in 3 test cases\)',(focused/'run.log').read_text())
pins.update(json.loads((focused/'pins.json').read_text()))
for name in ('status.json','terminal_handles.json','command.json','pins.json','run.log'):
    pins[str(focused/name)]=sha(focused/name)
for index, case in enumerate(cases):
    status = json.loads((case / "status.json").read_text())
    assert status["state"] == "terminal" and status["exit_code"] == 0 and status["pins_verified"]
    assert json.loads((case / "terminal_handles.json").read_text())["all_owned_handles_terminal"]
    command = json.loads((case / "command.json").read_text())
    assert command["build"] == "build274" and command["ranks"] == (1 if index == 0 else 6)
    clean_log = re.sub(r"\x1b\[[0-9;]*m", "", (case / "run.log").read_text())
    summaries = [list(map(int, row)) for row in re.findall(r"All tests passed \((\d+) assertions in (\d+) test cases?\)", clean_log)]
    assert len(summaries) == (1 if index == 0 else 6)
    assert all(assertions > 0 and count >= (58 if index == 0 else 99) for assertions, count in summaries)
    for path, digest in json.loads((case / "pins.json").read_text()).items():
        assert pins.setdefault(path, digest) == digest
    for name in ("status.json", "terminal_handles.json", "command.json", "pins.json", "run.log"):
        path = case / name
        pins[str(path)] = sha(path)
    stages.append({"case": str(case), "status": status, "summaries": summaries})
rows_path = cases[0] / "native_current_residual_result.jsonl"
rows = [json.loads(line) for line in rows_path.read_text().splitlines()]
results = [r for r in rows if r["stage"] == "bounded_star_result"]
oracles = [r for r in rows if r["stage"] == "current_metric_match"]
assert len(oracles)==1 and oracles[0]["production_current_source"]
assert oracles[0]["known_q_oracles"]==39 and oracles[0]["unknown_q_oracles"]==3
assert max(oracles[0][k] for k in ("q_error","J_error","edge_error"))<=1e-10
witnesses=[r for r in results if r["production_geometry_admissible"]
    and r["new_q_bad"]==0 and r["new_J_bad"]==0 and r["new_geometry_bad"]<r["old_geometry_bad"]
    and r["new_bad_physical_faces"]<r["old_bad_physical_faces"] and r["new_maximum_edge"]<=1.8]
assert witnesses,"No useful complete source-gated split/reclosure candidate survives production strip ordering."
cleanup_rows_path=focused/'native_current_residual_result.jsonl'
cleanup_rows=[json.loads(line) for line in cleanup_rows_path.read_text().splitlines()]
cleanup_oracles=[r for r in cleanup_rows if r['stage']=='current_metric_match']
assert len(cleanup_oracles)==4
assert sum(r['known_q_oracles'] for r in cleanup_oracles)==144
assert all(r['unknown_q_oracles']==0 and max(r[k] for k in ('q_error','J_error','edge_error'))<=1e-10 for r in cleanup_oracles)
cleanup_success=[r for r in cleanup_rows if r.get('diagnostic_cleanup_feasible')]
assert len(cleanup_success)>=4
assert sum(r["kind"]=="collapse_before_retained_star_optimizer" for r in cleanup_success)>=3
assert all(r['coarsen_metric_valid'] and r['new_q_bad']==0 and r['new_J_bad']==0 and r['new_geometry_bad']==0 and r['new_maximum_edge']<=1.8 for r in cleanup_success)
pins[str(cleanup_rows_path)]=sha(cleanup_rows_path)
for path in (rows_path,Path(__file__).resolve(),ROOT/"cleanup274_validate.py",ROOT/"launch_m6274.py"):
    pins[str(path)]=sha(path)
changed=[path for path,digest in pins.items() if sha(Path(path))!=digest]
assert not changed,changed
result={"time":time.time(),"all_required_checks_passed":True,"mpi_ranks":6,
    "source_pins_sha256":sha(ROOT/"build274_source_pins.json"),"stages":stages,
    "useful_source_gated_candidates":witnesses,"cleanup_source_gated_candidates":cleanup_success,"qualification_pins":pins,
    "actual_m6_runtime_verified":False,"successful_cleanup_publication_proven":False,"scope":"Connected shape-preserving metric cleanup and coupled repair serial/MPI6 qualification. Not M6 acceptance, transfer, adapted CFD or disk restart proof."}
output.write_text(json.dumps(result,indent=2)+"\n")
print(json.dumps({"all_required_checks_passed":True,"stages":stages,"witnesses":len(witnesses)},indent=2))
