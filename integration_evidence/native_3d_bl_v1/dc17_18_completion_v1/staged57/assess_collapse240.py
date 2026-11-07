"""Join exact collapsed faces, native guidance and rigorous outside witnesses."""
from pathlib import Path
from fractions import Fraction as F
import importlib.util
import json
import math
import struct

ROOT = Path(__file__).resolve().parent
RUN = ROOT / "m6_runs/euler_build237_signed_bounds_real_cfd50k"
QUERY = RUN / "collapse238_geometry_queries_v1"
ATTEMPT = ROOT / "cases/geometry_current_queries_20261107T080717_3175207"
spec = importlib.util.spec_from_file_location("prior_assessment", ROOT / "assess_current235.py")
prior = importlib.util.module_from_spec(spec)
spec.loader.exec_module(prior)


def main():
    status = json.loads((ATTEMPT / "status.json").read_text())
    assert status["state"] == "terminal" and status["exit_code"] == 0 and status["pins_verified"]
    assert json.loads((ATTEMPT / "terminal_handles.json").read_text())["all_absent"]
    assert json.loads((ROOT / "query240_launch.json").read_text())["additional_pins_verified"]
    paths = [Path(__file__), ROOT / "assess_current235.py", QUERY / "geometry_query_preparation.json",
             QUERY / "current_geometry_queries.bin", ROOT / "query240_launch.json"]
    paths += [ATTEMPT / name for name in ("status.json", "pins.json", "command.json", "terminal_handles.json",
                                        "m6_surface_queries.json", "m6_surface_witnesses.jsonl")]
    before = {str(p): prior.sha(p) for p in paths}
    metadata = json.loads((QUERY / "geometry_query_preparation.json").read_text())
    results = json.loads((ATTEMPT / "m6_surface_queries.json").read_text())
    samples = [json.loads(line) for line in (ATTEMPT / "m6_surface_witnesses.jsonl").read_text().splitlines()]
    raw = (QUERY / "current_geometry_queries.bin").read_bytes()
    assert len(results["queries"]) == len(metadata["queries"]) == 6
    old, new, faces = 0., 0., []
    for i, (face, result) in enumerate(zip(metadata["queries"], results["queries"])):
        assert result["id"] == i
        points = [struct.unpack_from("<QdddQQQdd", raw, 48+256*i+32+72*k)[1:4] for k in range(3)]
        selected = [r for r in samples if r["query"] == i]
        for row in selected:
            if row.get("unknown"):
                continue
            u, v = F(row["u"]), F(row["v"])
            assert u >= 0 and v >= 0 and u+v <= 1
            ideal = [F(points[0][d])+u*(F(points[1][d])-F(points[0][d]))+v*(F(points[2][d])-F(points[0][d])) for d in range(3)]
            displacement2 = sum((ideal[d]-F(row["point"][d]))**2 for d in range(3))
            assert displacement2 <= F(row["source_roundoff"])**2
            corrected = max(F(0), F(row["distance_lower"])-F(row["source_roundoff"])-F(row["model_error_allowance"]))
            assert F(row["face_point_distance_lower"]) <= corrected
            assert row["outside_proved"] == (row["face_point_distance_lower"] > results["tolerance"])
        assert result["outside_witness_proved"] == any(r.get("outside_proved", False) for r in selected)
        a = [points[1][d]-points[0][d] for d in range(3)]
        b = [points[2][d]-points[0][d] for d in range(3)]
        area2 = math.hypot(a[1]*b[2]-a[2]*b[1], a[2]*b[0]-a[0]*b[2], a[0]*b[1]-a[1]*b[0])
        excess = max(0., result["production_guidance_upper"]-results["tolerance"])**2*area2
        if face["kind"] == "old": old += excess
        else: new += excess
        faces.append(dict(face, **result, area_times_two=area2, guidance_excess=excess))
    expected = metadata["candidate"]["new_geometry_excess"]-metadata["candidate"]["old_geometry_excess"]
    assert math.isclose(new-old, expected, rel_tol=1e-12, abs_tol=1e-26)
    assert all(f["outside_witness_proved"] for f in faces if f["kind"] == "collapsed")
    assert all(not r.get("unknown", False) for r in samples)
    assert before == {str(p): prior.sha(p) for p in paths}
    result = {"source_build": "build240", "production_changed": False, "inputs_sha256": before,
              "status": status, "faces": faces, "guidance_delta": new-old, "native_candidate_delta": expected,
              "witness_count": len(samples), "outside_witness_count": sum(r["outside_proved"] for r in samples),
              "decision": "Both collapsed faces violate the stored curved authority, even with extra global1e-11 mapping-error margin. The unchanged7876-7868 chord already violates100um by a certified midpoint lower248.334um. Certification alone cannot make those faces satisfy the final100um gate; this does not establish that the intermediate weighted upper-bound merit is tight. Assess existing geometry split/reconstruction on that actual chord and complete neighboring volume; preserved historical rejection proves it was attempted, not why it failed. Do not introduce a quality-construction policy, larger cavities, another local fallback or relaxed gate from this result.",
              "limits": "Outside proofs only, to the whole hinted same-marker cubic component; no separate CAD claim. Unsampled faces/points are not accepted by this diagnostic. Stored query upper may be looser than production's tighter direct correspondence bound; use production_guidance_upper for score reconstruction. Geometry setup/replay uses fresh contexts, not historical publication. No accepted mesh, nonlocal contact/publication, nonzero-height, transfer or restart proof."}
    (RUN / "collapse240_geometry_assessment.json").write_text(json.dumps(result, indent=2) + "\n")
    print(json.dumps({k: result[k] for k in ("guidance_delta", "native_candidate_delta", "witness_count", "outside_witness_count", "decision")}))


if __name__ == "__main__":
    main()
