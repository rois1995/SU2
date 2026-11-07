"""Reuse the existing current-cavity fixture format for actual terminal235 cores."""
from pathlib import Path
import importlib.util
import json
import math
import shutil
import struct
import time

ROOT = Path(__file__).resolve().parent
RUN = ROOT / "m6_runs/euler_build235_geometry_coupled_real_cfd50k"
OUT = RUN / "current_witnesses_v1"
spec = importlib.util.spec_from_file_location("boundary_preparation", ROOT / "prepare_boundary202.py")
old = importlib.util.module_from_spec(spec)
spec.loader.exec_module(old)


def main():
    started = time.monotonic()
    assert not OUT.exists()
    assert json.loads((RUN / "terminal_handles.json").read_text())["all_absent"]
    assert json.loads((RUN / "inspection_export_terminal_handles.json").read_text())["all_absent"]
    selection = RUN / "support235_coverage_assessment.json"
    probe = json.loads(selection.read_text())
    candidates = probe["bounded_selected_contexts"]
    chosen = [row for row in candidates if not row["geometry_bad"]]
    chosen += [row for row in candidates if row["geometry_bad"] and row["within_existing64"]][:2]
    assert len(chosen) == 7 and all(row["four_star_union_size"] <= 64 for row in chosen)
    selected = {cell for case in chosen for cell in case["context_cell_ids"]}
    ready = json.loads((RUN / "capture_ready.json").read_text())
    before = {name: old.sha256(Path(name)) for name in ready["files"]}
    assert all(before[name] == item["sha256"] and Path(name).stat().st_size == item["bytes"]
               for name, item in ready["files"].items())
    for path in (selection, RUN / "capture_ready.json", RUN / "run.cfg", RUN / "m6.su2.native_ref",
                 Path(__file__), ROOT / "prepare_boundary202.py", ROOT / "inspect_native_failure3d.py"):
        before[str(path.resolve())] = old.sha256(path)
    manifests, missing = old.reader._load_manifests([str(RUN / "*_inspection_rank*_manifest.json")])
    assert not missing and len(manifests) == 6
    points, physical, records, expected = {}, {}, {}, {}
    total = 0
    for manifest in manifests:
        files, caps = manifest["_resolved_files"], manifest["_caps"]
        paired = old.reader._jsonl(files["metrics"], caps["metrics"], "metrics")
        with files["owned"].open("rb") as raw:
            for index, cell in enumerate(old.reader._read_owned(files["owned"], manifest["written_rows"], caps["owned"])):
                metric = old.reader._validate_metric(next(paired), cell, files["metrics"])
                assert metric["cache_valid"]
                total += 1
                for node, binding in zip(cell["nodes"], metric["bindings"]):
                    if binding is None:
                        continue
                    assert binding["id"] == node["id"]
                    value = (tuple(node["xyz"]), (binding["feature"], binding["version"],
                                                binding["dimension"], binding["u"], binding["v"]))
                    assert points.setdefault(node["id"], value) == value
                for slot, face in enumerate(cell["physical"]):
                    if face["present"]:
                        ordered = tuple(cell["nodes"][k]["id"] for k in old.FACES[slot])
                        key = tuple(sorted(ordered))
                        assert key not in physical
                        physical[key] = (ordered, face["facet"])
                if cell["id"] not in selected:
                    continue
                assert old.exact_positive(cell["nodes"]) and cell["id"] not in records
                assert metric["q"] is not None and metric["J"] is not None
                raw.seek(8 + old.reader.OWNED_RECORD_BYTES * index)
                records[cell["id"]] = raw.read(old.reader.OWNED_RECORD_BYTES)
                values = [cell["id"], metric["q"], metric["J"]]
                # The unchanged generic fixture accepts q/J oracles but disables edge-oracle comparison.
                # Raw edge caches remain preserved; candidate edges are freshly checked by the fixture.
                for first, second, _ in metric["edges"]:
                    values.extend((first, second, math.nan))
                expected[cell["id"]] = old.METRIC.pack(*values)
        assert next(paired, None) is None
    assert total == ready["owned_rows"] and set(records) == selected == set(expected)
    assert {node for face in physical for node in face} == set(points)
    OUT.mkdir()
    old.encode(OUT / "witness_boundary_points.bin",
               [old.POINT.pack(node, *xyz, *binding) for node, (xyz, binding) in sorted(points.items())], old.POINT)
    old.encode(OUT / "witness_boundary_surface.bin",
               [old.SURFACE.pack(*ordered, facet) for ordered, facet in physical.values()], old.SURFACE)
    lines = []
    for i, case in enumerate(chosen):
        label = ("geometry_bad_" if case["geometry_bad"] else "geometry_clean_") + str(i)
        support = case["context_cell_ids"]
        old.encode(OUT / (label + "_owned.bin"), [records[cell] for cell in support], struct.Struct("416s"))
        old.encode(OUT / (label + "_expected_metrics.bin"), [expected[cell] for cell in support], old.METRIC)
        lines.append(" ".join(map(str, [label, case["id"], len(support),
                           *[x for pair in zip(case["nodes"], case["vertex_star_sizes"]) for x in pair]])))
    (OUT / "current_residual_cases.tsv").write_text("\n".join(lines) + "\n")
    shutil.copyfile(RUN / "run.cfg", OUT / "run.cfg")
    (OUT / "m6.su2.native_ref").symlink_to((RUN / "m6.su2.native_ref").resolve())
    after = {name: old.sha256(Path(name)) for name in before}
    assert before == after
    outputs = {path.name: {"bytes": path.stat().st_size, "sha256": old.sha256(path)} for path in OUT.iterdir()}
    receipt = {"schema": "native_current_witness_preparation_v1", "source_build": "build235", "source_event": 1,
               "paired_rows": total, "cases": chosen, "boundary_points": len(points), "physical_faces": len(physical),
               "input_hashes_before": before, "input_hashes_after": after, "outputs": outputs,
               "seconds": time.monotonic() - started,
               "scope": "Actual complete current stars and cached q/J oracles; fresh associations, not historical private replay. Existing fixture intentionally omits edge-oracle equality; fresh candidate edge gates remain."}
    (OUT / "witness_preparation.json").write_text(json.dumps(receipt, indent=2) + "\n")
    print(json.dumps({key: receipt[key] for key in ("paired_rows", "boundary_points", "physical_faces", "seconds")}))


if __name__ == "__main__":
    main()
