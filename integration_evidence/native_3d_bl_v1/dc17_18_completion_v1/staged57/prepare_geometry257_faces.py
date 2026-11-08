"""Prepare exact terminal face queries, without claiming complete current stars."""
from pathlib import Path
import json
import shutil
import struct

from prepare_boundary202 import POINT, encode, reader, sha256

ROOT = Path(__file__).resolve().parent
RUN = ROOT / "m6_runs/euler_build256_canonical_geometry_debt_mach_only_real_cfd50k"
OUT = RUN / "geometry_faces257_inputs_v1"
assert not OUT.exists()
assert json.loads((RUN / "terminal_handles.json").read_text())["all_owned_handles_terminal"]
location = json.loads((RUN / "geometry_residual_location.json").read_text())
seeds = location["geometry_bad_cells"]
assert len(seeds) == 78 and all(len(r["physical_faces"]) == 1 for r in seeds)
ready = json.loads((RUN / "capture_ready.json").read_text())
before = {p: sha256(Path(p)) for p in ready["files"]}
assert all(before[p] == h["sha256"] for p, h in ready["files"].items())
for p in (RUN / "geometry_residual_location.json", RUN / "run.cfg", RUN / "m6.su2.native_ref", Path(__file__)):
    before[str(p.absolute())] = sha256(p)
chosen = {r["id"]: r for r in seeds}
records, points = {}, {}
manifests, missing = reader._load_manifests([str(RUN / "adapted_mesh_inspection_rank*_manifest.json")])
assert not missing
for m in manifests:
    path = m["_resolved_files"]["owned"]
    with path.open("rb") as raw:
        for i, cell in enumerate(reader._read_owned(path, m["written_rows"], m["_caps"]["owned"])):
            if cell["id"] not in chosen:
                break  # Only the verified bad-first prefix is needed.
            seed = chosen[cell["id"]]
            assert [n["id"] for n in cell["nodes"]] == seed["nodes"]
            raw.seek(8 + i * reader.OWNED_RECORD_BYTES)
            records[cell["id"]] = raw.read(reader.OWNED_RECORD_BYTES)
            for n, b in zip(cell["nodes"], seed["bindings"]):
                if b is None:
                    continue
                assert n["id"] == b["id"]
                value = POINT.pack(n["id"], *n["xyz"], b["feature"], b["version"], b["dimension"], b["u"], b["v"])
                assert points.setdefault(n["id"], value) == value
assert set(records) == set(chosen)
OUT.mkdir()
encode(OUT / "geometry_faces_owned.bin", [records[c] for c in sorted(records)], struct.Struct("416s"))
encode(OUT / "witness_boundary_points.bin", [points[n] for n in sorted(points)], POINT)
(OUT / "geometry_faces_expected.tsv").write_text("".join(f"{c} {chosen[c]['geometry_excess_l2']:.17g}\n" for c in sorted(chosen)))
unknown = sorted((r for r in seeds if r["q"] is None or r["J"] is None), key=lambda r: -r["geometry_excess_l2"])
known = sorted((r for r in seeds if r["q"] is not None and r["J"] is not None), key=lambda r: -r["geometry_excess_l2"])
probe = unknown[:3] + known[:3] + known[-2:]
assert len({r["id"] for r in probe}) == 8
(OUT / "geometry_faces_probe_ids.txt").write_text("\n".join(str(r["id"]) for r in probe) + "\n")
shutil.copyfile(RUN / "run.cfg", OUT / "run.cfg")
(OUT / "m6.su2.native_ref").symlink_to((RUN / "m6.su2.native_ref").resolve())
after = {p: sha256(Path(p)) for p in before}
assert before == after
receipt = {"scope": "78exact physical-face queries;8representative face-only midpoint probes. No complete current surface,volume cavity,dependency or replacement claim.",
           "cells": len(records), "boundary_points": len(points), "probe_ids": [r["id"] for r in probe],
           "input_hashes_before": before, "input_hashes_after": after,
           "outputs": {p.name: {"sha256": sha256(p), "bytes": p.stat().st_size} for p in OUT.iterdir()}}
(OUT / "preparation.json").write_text(json.dumps(receipt, indent=2) + "\n")
print(json.dumps({k: receipt[k] for k in ("cells", "boundary_points", "probe_ids")}))
