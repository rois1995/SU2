"""Read existing bounded actual-cavity trace; no repair or certification decisions."""
from collections import Counter, defaultdict
from fractions import Fraction
from itertools import combinations
from pathlib import Path
import hashlib
import json
import math
import re
import shutil
import struct

ROOT = Path(__file__).resolve().parent
ATTEMPT = ROOT / "cases/current_compound_20261107T052520_2804746"
RUN = ROOT / "m6_runs/euler_build235_geometry_coupled_real_cfd50k"


def sha(path):
    h = hashlib.sha256()
    with path.open("rb") as stream:
        for chunk in iter(lambda: stream.read(8 << 20), b""):
            h.update(chunk)
    return h.hexdigest()


def main():
    status = json.loads((ATTEMPT / "status.json").read_text())
    assert status["state"] == "terminal" and status["exit_code"] == 0 and status["pins_verified"]
    assert json.loads((ROOT / "current_compound235_launch.json").read_text())["terminal"]["all_owned_terminal"]
    paths = [ATTEMPT / name for name in ("status.json", "pins.json", "run.log", "native_current_residual_result.jsonl")]
    paths += [Path(__file__), RUN / "current_witnesses_v1/witness_preparation.json", RUN / "run.cfg", RUN / "m6.su2.native_ref"]
    before = {str(p): sha(p) for p in paths}
    queries, current, completions = [], None, 0
    for line in (ATTEMPT / "run.log").open():
        if line.startswith("NATIVE_SURFACE_QUERY "):
            assert current is None
            match = re.fullmatch(r"NATIVE_SURFACE_QUERY tolerance=(\S+) model=(\d+) mesh=(\d+) facets=(\S+) points=(\S+)\n", line)
            assert match and (match[2], match[3]) == ("0", "1")
            facets = tuple(int(x) for x in match[4].strip(",").split(","))
            points = tuple(tuple(float(x) for x in p.split(",")) for p in match[5].strip(";").split(";"))
            assert 0 < len(facets) <= 64 and list(facets) == sorted(set(facets))
            assert len(points) == 3 and all(len(p) == 3 and all(map(math.isfinite, p)) for p in points)
            current = {"id": len(queries), "facets": facets, "points": points, "tolerance": float(match[1]), "domains": 0}
        elif current is not None and line.startswith("NATIVE_SURFACE_DOMAIN model=0 "):
            fields = dict(re.findall(r"(\w+)=([^ ]+)", line))
            if current["domains"] == 0:
                assert int(fields["depth"]) == 0
                current["root_upper"] = float(fields["upper"])
            current["domains"] += 1
        elif line.startswith("NATIVE_GEOMETRY_WORK operation=QuerySurfaceUnionAtResolution "):
            completions += 1
            if current is not None:
                assert current["domains"] > 0
                current["work"] = int(re.search(r" total=(\d+)", line)[1])
                queries.append(current)
                current = None
    assert current is None and len(queries) > 0
    exact, edge_history, pairs = Counter(), defaultdict(list), []
    root_proof_hits = []
    for q in queries:
        identity = (q["facets"], q["tolerance"], q["points"])
        exact[identity] += 1
        nearest = None
        for edge in combinations(sorted(q["points"]), 2):
            key = (q["facets"], q["tolerance"], edge)
            for old in edge_history[key]:
                if old["points"] == q["points"]:
                    continue
                a, = set(old["points"]) - set(edge)
                b, = set(q["points"]) - set(edge)
                squared = sum((Fraction(x) - Fraction(y)) ** 2 for x, y in zip(a, b))
                if not squared:
                    continue  # Same triangle with another vertex order is not a moved-position query.
                pair = {"old": old["id"], "new": q["id"], "displacement": math.sqrt(float(squared)),
                        "new_work": q["work"], "old_root_upper": old["root_upper"]}
                if nearest is None or pair["displacement"] < nearest["displacement"]:
                    nearest = pair
                slack = Fraction(q["tolerance"]) - Fraction(old["root_upper"])
                if slack >= 0 and squared <= slack * slack:
                    root_proof_hits.append(pair)
            edge_history[key].append(q)
        if nearest is not None:
            pairs.append(nearest)
    eligible_ids = {p["new"] for p in root_proof_hits}
    ordered = sorted(pairs, key=lambda p: (-p["new_work"], p["displacement"]))
    selected, used = [], set()
    # Six disjoint pairs keep the existing twelve-query replay bound.
    for pair in ordered:
        if pair["displacement"] >= queries[pair["new"]]["tolerance"] or {pair["old"], pair["new"]} & used:
            continue
        selected.append(pair)
        used.update((pair["old"], pair["new"]))
        if len(selected) == 6:
            break
    assert len(selected) <= 6
    result = {"status": status, "inputs_sha256": before,
              "scope": "Actual terminal235 cavity diagnostic miss stream; exact memo hits are absent; supports above64 are untraced. Not a historical M6 private-query replay or production timing benchmark.",
              "queries": len(queries), "all_traversal_completions": completions,
              "traced_query_work": sum(q["work"] for q in queries),
              "distinct_exact_queries": len(exact), "repeated_exact_misses": sum(n - 1 for n in exact.values()),
              "distinct_support_sets": len({q["facets"] for q in queries}),
              "nearby_same_support_two_fixed_vertex_queries": len(pairs),
              "root_upper_slack_proved_queries": len(eligible_ids),
              "root_upper_slack_eligible_query_work": sum(queries[i]["work"] for i in eligible_ids),
              "root_proof_examples": root_proof_hits[:8], "selected_replay_pairs": selected,
              "selected_queries": [queries[i] for p in selected for i in (p["old"], p["new"])],
              "limits": "Root upper slack is a sufficient-only exact-rational test for fixed full support. Query work includes preparation and traversal. Pair selection is diagnostic, not a production cache search policy. Nearby candidates with an unresolved root need fresh replay bounds."}
    assert before == {str(p): sha(p) for p in paths}
    (RUN / "query235_trace_assessment.json").write_text(json.dumps(result, indent=2) + "\n")
    folder = RUN / "geometry_trial_queries_v1"
    assert not folder.exists(), "Never overwrite prepared inputs"
    with (RUN / "m6.su2.native_ref").open() as stream:
        assert stream.readline().strip() == "SU2_NATIVE_REFERENCE_3D 7"
        header = stream.readline().split()
        assert len(header) == 10
        geometry_version = int(header[7])
    selected_queries = result["selected_queries"]
    assert len(selected_queries) == 12
    payload = bytearray(struct.pack("<QQQQdQ", 0x4D36515545525931, 1, geometry_version, len(selected_queries), 1e-4, 401030928))
    for q in selected_queries:
        assert q["tolerance"] == 1e-4
        payload.extend(struct.pack("<QQQQ", 1, 2, 3, q["facets"][0]))
        # Version1 replays a physical triangle against explicit full support; bindings are not interrogated.
        for i, point in enumerate(q["points"], 1):
            payload.extend(struct.pack("<QdddQQQdd", i, *point, q["facets"][0], geometry_version, 2, 0., 0.))
        payload.extend(struct.pack("<Q", len(q["facets"])))
        payload.extend(struct.pack("<" + "Q" * len(q["facets"]), *q["facets"]))
    assert len(payload) == 48 + sum(256 + 8 * len(q["facets"]) for q in selected_queries)
    folder.mkdir()
    (folder / "current_geometry_queries.bin").write_bytes(payload)
    shutil.copyfile(RUN / "run.cfg", folder / "run.cfg")
    (folder / "m6.su2.native_ref").symlink_to((RUN / "m6.su2.native_ref").resolve())
    outputs = {p.name: {"sha256": sha(p), "bytes": p.stat().st_size} for p in folder.iterdir()}
    preparation = {"schema": "native_current_geometry_query_preparation_v1", "geometry_definition_version": geometry_version,
                   "input_hashes_before": before, "input_hashes_after": {str(p): sha(p) for p in paths},
                   "outputs": outputs, "query_ids": [q["id"] for q in selected_queries],
                   "scope": "Exact physical coordinates, tolerance and immutable support from current235 diagnostic trial misses. Existing version1 raw query replay; placeholder record IDs/bindings are unused. Not association, requested-height or publication proof."}
    assert preparation["input_hashes_before"] == preparation["input_hashes_after"]
    (folder / "geometry_query_preparation.json").write_text(json.dumps(preparation, indent=2) + "\n")
    print(json.dumps({k: v for k, v in result.items() if k not in ("inputs_sha256", "selected_queries", "root_proof_examples")}))


if __name__ == "__main__":
    main()
