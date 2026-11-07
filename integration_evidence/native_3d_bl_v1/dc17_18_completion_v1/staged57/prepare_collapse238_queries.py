"""Export exact old/collapsed faces from the refused, unmoved one-face238 candidate."""
from pathlib import Path
import importlib.util
import json
import shutil
import struct

ROOT = Path(__file__).resolve().parent
RUN = ROOT / "m6_runs/euler_build237_signed_bounds_real_cfd50k"
SOURCE = RUN / "current_witnesses_v1"
OUT = RUN / "collapse238_geometry_queries_v1"
spec = importlib.util.spec_from_file_location("boundary_preparation", ROOT / "prepare_boundary202.py")
old = importlib.util.module_from_spec(spec)
spec.loader.exec_module(old)


def main():
    assert not OUT.exists()
    launch = json.loads((ROOT / "current238_launch.json").read_text())
    attempt = Path(launch["attempt"])
    assert json.loads((attempt / "terminal_handles.json").read_text())["all_absent"]
    assessment = json.loads((RUN / "current_residual238_assessment.json").read_text())
    direction, = [r for r in assessment["raw_operator_results"] if r.get("case") == "geometry_bad_1"
                  and r["stage"] == "current_directed_collapse" and r["constructed"]]
    result, = [r for r in assessment["candidate_results"] if r["case"] == "geometry_bad_1"
               and r["kind"] == "collapse_then_retained_star_optimizer"]
    assert not result["changed"] and result["boundary_moves"] == result["interior_moves"] == 0
    assert result["new_geometry_excess"] > result["old_geometry_excess"]
    removed, retained = direction["removed"], direction["retained"]
    receipt = json.loads((SOURCE / "witness_preparation.json").read_text())
    before = {str((SOURCE / name).resolve()): old.sha256(SOURCE / name) for name in receipt["outputs"]}
    assert all(before[str((SOURCE / name).resolve())] == item["sha256"] for name, item in receipt["outputs"].items())
    for p in (SOURCE / "witness_preparation.json", RUN / "current_residual238_assessment.json",
              Path(__file__), ROOT / "prepare_boundary202.py", ROOT / "inspect_native_failure3d.py"):
        before[str(p.resolve())] = old.sha256(p)
    raw = (SOURCE / "witness_boundary_points.bin").read_bytes()
    count, = old.U64.unpack_from(raw)
    assert len(raw) == 8 + count * old.POINT.size
    points = {p[0]: p for p in (old.POINT.unpack_from(raw, 8+i*old.POINT.size) for i in range(count))}
    rows = list(old.reader._read_owned(SOURCE / "geometry_bad_1_owned.bin", 52, 128 << 20))
    queries = []
    for cell in rows:
        for slot, physical in enumerate(cell["physical"]):
            if not physical["present"]:
                continue
            face = tuple(cell["nodes"][k]["id"] for k in old.FACES[slot])
            if removed not in face:
                continue
            queries.append(dict(kind="old", facet=physical["facet"], face=face))
            if retained not in face:
                changed = tuple(retained if node == removed else node for node in face)
                i = changed.index(min(changed))
                changed = changed[i:] + changed[:i]
                queries.append(dict(kind="collapsed", facet=physical["facet"], face=changed))
    assert len(queries) == 6 and sum(q["kind"] == "collapsed" for q in queries) == 2
    versions = {points[node][5] for q in queries for node in q["face"]}
    assert len(versions) == 1
    payload = bytearray(struct.pack("<QQQQdQ", 0x4d36515545525931, 2, versions.pop(), len(queries), 1e-4, 401030928))
    for q in queries:
        payload.extend(old.SURFACE.pack(*q["face"], q["facet"]))
        for node in q["face"]:
            payload.extend(old.POINT.pack(*points[node]))
        payload.extend(old.U64.pack(0))
    assert len(payload) == 48 + 256 * len(queries)
    assert before == {name: old.sha256(Path(name)) for name in before}
    OUT.mkdir()
    (OUT / "current_geometry_queries.bin").write_bytes(payload)
    shutil.copyfile(SOURCE / "run.cfg", OUT / "run.cfg")
    (OUT / "m6.su2.native_ref").symlink_to((SOURCE / "m6.su2.native_ref").resolve())
    metadata = dict(schema="native_current_geometry_query_preparation_v1", input_hashes_before=before,
                    input_hashes_after=before, queries=queries, collapse=direction, candidate=result,
                    outputs={p.name: dict(bytes=p.stat().st_size, sha256=old.sha256(p)) for p in OUT.iterdir()},
                    scope="Exact CollapseVertex surface rewrite: replace removed ID by retained ID, omit degenerate faces, retain original facet and coordinates/bindings. Optimizer made no change. Native replay must validate associations and recover geometry-guidance delta before attribution. No synthetic nearby positions or acceptance proof.")
    (OUT / "geometry_query_preparation.json").write_text(json.dumps(metadata, indent=2) + "\n")
    print(json.dumps({"queries": queries, "output": str(OUT)}))


if __name__ == "__main__":
    main()
