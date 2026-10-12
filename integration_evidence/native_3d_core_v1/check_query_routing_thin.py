"""Verify the fixed thin-domain matrix and exact affine covariance; no timing claims."""
from pathlib import Path
import hashlib
import json

root = Path(__file__).resolve().parent
cases = ("query_routing_thin_h10_n4_n4_m2_v1",
         "query_routing_thin_h20_n4_n1_m1_v1",
         "query_routing_thin_h20_n4_n4_m2_v1")
baseline = root / "query_routing_n4_n4_m2_v1"
sha = lambda p: hashlib.sha256(p.read_bytes()).hexdigest()
qualified = json.loads((root / "query_routing_build_v1/validation.json").read_text())
assert qualified["status"] == "PASS"
rows = []
for name in cases:
    folder = root / name
    receipt = json.loads((folder / "validation.json").read_text())
    assert receipt["status"] == "PASS", name
    assert receipt["binary_sha256"] == qualified["binary_sha256"], name
    for field in ("source_drift", "object_drift", "archive_drift", "tool_drift"):
        assert not receipt[field], (name, field)
    for artifact, digest in receipt["artifact_sha256"].items():
        assert sha(folder / artifact) == digest, (name, artifact)
    audit = json.loads((folder / "independent_audit.json").read_text())
    assert audit["status"] == "PASS" and len(audit["frames"]) == 8 and len(audit["states"]) == 3
    parameters = audit["parameters"]
    assert parameters["size"] == 4 and parameters["diagnostic_rounds"] == 512
    accepted = [f for f in audit["frames"] if f["file"] in {f"envelope_s{i}.json" for i in range(3)}]
    assert len(accepted) == 3
    for frame in accepted:
        assert frame["strict_metric_gate"] and frame["minimum_mean_ratio"] >= .20
        assert frame["minimum_scaled_jacobian"] >= .05 and frame["maximum_length"] <= 1.8
    normalized_matches = []
    if parameters["ranks"] == 4 and parameters["workers"] == 2:
        control = json.loads((baseline / "validation.json").read_text())
        assert control["status"] == "PASS" and control["binary_sha256"] == receipt["binary_sha256"]
        for artifact, digest in control["artifact_sha256"].items():
            assert sha(baseline / artifact) == digest, artifact
        factor = 2 ** parameters["height_power"]
        for frame in audit["frames"]:
            filename = frame["file"]
            mesh = json.loads((folder / filename).read_text())
            reference = json.loads((baseline / filename).read_text())
            assert mesh != reference, (name, filename, "physical domains must differ")
            for node in mesh["nodes"]:
                node[3] *= factor
            for facet in mesh["facets"]:
                for node in facet["vertices"]:
                    node[3] *= factor
            for tet in mesh["original"]:
                for node in tet["vertices"]:
                    node[3] *= factor
                    node[4][2] /= factor
                    node[4][4] /= factor
                    node[4][5] /= factor ** 2
            assert mesh == reference, (name, filename, "affine covariance")
            normalized_matches.append(filename)
    rows.append(dict(case=name, validation_sha256=sha(folder / "validation.json"),
                     audit_sha256=sha(folder / "independent_audit.json"), parameters=parameters,
                     accepted_frames=accepted, states=audit["states"], reload_metrics=audit["reload_metrics"],
                     exact_normalized_geometry_and_donor_frames=normalized_matches))
report = dict(status="PASS", checker_sha256=sha(Path(__file__)), binary_sha256=qualified["binary_sha256"],
              cases=rows, scope="Fresh full three-event thin-domain geometry/metric/state/readback correctness. "
              "N4M2 physical z and original-P1 tensor normalization match eight unscaled frames exactly. "
              "Physical affine solution states are audited independently, not equated across heights. "
              "This is manufactured thin-domain covariance, not BL construction or performance qualification.")
(root / "QUERY_ROUTING_THIN_VALIDATION.json").write_text(json.dumps(report, indent=2) + "\n")
print("PASS: three thin cases; sixteen exact normalized N4M2 geometry/donor frames")
