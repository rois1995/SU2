"""Reuse the current-context fixture on the certified under-resolved chord only."""
from pathlib import Path
import importlib.util
import json
import shutil

ROOT = Path(__file__).resolve().parent
RUN = ROOT / "m6_runs/euler_build237_signed_bounds_real_cfd50k"
SOURCE = RUN / "current_witnesses_v1"
OUT = RUN / "construction241_witnesses_v1"
spec = importlib.util.spec_from_file_location("prior_assessment", ROOT / "assess_current235.py")
old = importlib.util.module_from_spec(spec)
spec.loader.exec_module(old)


def main():
    assert not OUT.exists()
    query = json.loads((RUN / "collapse240_geometry_assessment.json").read_text())
    assert query["source_build"] == "build240"
    assert all(r["outside_witness_proved"] for r in query["faces"] if r["kind"] == "collapsed")
    receipt = json.loads((SOURCE / "witness_preparation.json").read_text())
    before = dict(receipt["input_hashes_before"])
    for name, item in receipt["outputs"].items():
        path = SOURCE / name
        assert path.stat().st_size == item["bytes"] and old.sha(path) == item["sha256"]
        before[str(path.resolve())] = item["sha256"]
    for path in (Path(__file__), ROOT / "assess_current235.py", SOURCE / "witness_preparation.json",
                 RUN / "collapse240_geometry_assessment.json"):
        before[str(path.resolve())] = old.sha(path)
    selected, = [row for row in receipt["cases"] if row["id"] == 230222079]
    assert selected["four_star_union_size"] == 52 and selected["within_existing64"]
    assert {7868, 7876} <= set(selected["nodes"])
    lines = [line for line in (SOURCE / "current_residual_cases.tsv").read_text().splitlines()
             if int(line.split()[1]) == selected["id"]]
    assert len(lines) == 1
    label = lines[0].split()[0]
    assert all(old.sha(Path(p)) == h for p, h in before.items())
    OUT.mkdir()
    for name in ("witness_boundary_points.bin", "witness_boundary_surface.bin", "m6.su2.native_ref",
                 label+"_owned.bin", label+"_expected_metrics.bin"):
        (OUT / name).symlink_to((SOURCE / name).resolve())
    shutil.copyfile(SOURCE / "run.cfg", OUT / "run.cfg")
    (OUT / "current_residual_cases.tsv").write_text(lines[0]+"\n")
    (OUT / "current_geometry_edges.tsv").write_text(label+" 7868 7876\n")
    assert not (OUT / "run.cfg").is_symlink()
    outputs = {path.name: {"bytes": path.stat().st_size, "sha256": old.sha(path)} for path in OUT.iterdir()}
    assert before == {p: old.sha(Path(p)) for p in before}
    result = dict(receipt, cases=[selected], input_hashes_before=before, input_hashes_after=before,
                  outputs=outputs, planned_construction_edges=[{"case": label, "edge": [7868, 7876]}],
                  scope="Same actual complete52-cell context and nodal/association inputs; selected chord has certified248.334um midpoint violation. Existing constructors on complete edge star and known complete context, no larger cap or production change. Measurements occur after validity-only construction; this is not historical replay.")
    (OUT / "witness_preparation.json").write_text(json.dumps(result, indent=2)+"\n")
    print(json.dumps({"output": str(OUT), "case": label, "edge": [7868, 7876], "context_cells": 52}))


if __name__ == "__main__":
    main()
