"""Bounded diagnostic classification of exact, permuted and changed-position queries."""
from collections import Counter, defaultdict, deque
from fractions import Fraction
from pathlib import Path
import hashlib
import json
import math
import struct

ROOT = Path(__file__).resolve().parent
RUN = ROOT / "m6_runs/euler_build244_mach_only_real_cfd50k"


def number(bits):
    return struct.unpack(">d", int(bits).to_bytes(8, "big"))[0]


def parse(lines):
    queries, pending = [], {}
    words = 0
    for line in lines:
        if not line.startswith(("NATIVE_SURFACE_QUERY ", "NATIVE_SURFACE_QUERY_RESULT ")):
            continue
        if line.startswith("NATIVE_SURFACE_QUERY ") and not line.startswith("NATIVE_SURFACE_QUERY authority="):
            continue  # Existing lower traversal trace also prints its own input record under this tag.
        tag, *items = line.split()
        row = dict(item.split("=", 1) for item in items)
        key = (row["authority"], row["budget"], int(row["start"]))
        if tag == "NATIVE_SURFACE_QUERY":
            assert key not in pending and len(queries) < 16384
            row["tolerance_bits"] = int(row.pop("tolerance"), 16)
            row["support"] = tuple(int(x) for x in row["support"].split(",") if x)
            row["triangles"] = tuple(tuple(int(x, 16) for x in triangle.split(",") if x)
                                     for triangle in row["triangles"].split(";") if triangle)
            assert row["support"] and row["triangles"] and all(len(t) == 9 for t in row["triangles"])
            assert math.isfinite(number(row["tolerance_bits"])) and number(row["tolerance_bits"]) >= 0
            assert all(math.isfinite(number(x)) for t in row["triangles"] for x in t)
            words += len(row["support"]) + 9 * len(row["triangles"])
            assert words <= 1000000
            queries.append(row)
            pending[key] = row
        else:
            assert key in pending and row["cache"] in ("hit", "miss", "bypass")
            old = pending.pop(key)
            old.update(cache=row["cache"], work=int(row["work"]), bound_bits=int(row["bound"], 16))
            assert old["work"] >= 0
            assert not math.isfinite(number(old["bound_bits"])) or number(old["bound_bits"]) >= 0
    return queries


def analyze(queries):
    counts = Counter(unresolved_queries=0, identical_repeat_misses=0, identical_answer_changes=0)
    work = Counter()
    exact, canonical, history = {}, set(), defaultdict(lambda: deque(maxlen=8))
    examples = []
    for index, q in enumerate(queries):
        if "cache" not in q or not math.isfinite(number(q["bound_bits"])):
            counts["unresolved_queries"] += 1
            continue
        counts["completed_queries"] += 1
        counts["cache_" + q["cache"]] += 1
        work[q["cache"]] += q["work"]
        scope = (q["authority"], q["tolerance_bits"], q["support"])
        key = (*scope, q["triangles"])
        vertices = [tuple(tuple(t[k:k + 3]) for k in (0, 3, 6)) for t in q["triangles"]]
        normalized = (*scope, tuple(sorted(tuple(sorted(t)) for t in vertices)))
        if key in exact:
            counts["identical_repeats"] += 1
            if q["cache"] != "hit":
                counts["identical_repeat_misses"] += 1
                work["identical_repeat_misses"] += q["work"]
            if exact[key] != q["bound_bits"]:
                counts["identical_answer_changes"] += 1
        elif normalized in canonical:
            counts["permutation_alias_queries"] += 1
            if q["cache"] != "hit":
                counts["permutation_alias_misses"] += 1
                work["permutation_alias_misses"] += q["work"]
        elif len(vertices) == 1 and len(set(vertices[0])) == 3:
            points = tuple(sorted(vertices[0]))
            shared, certified = False, None
            for moving in range(3):
                pair = tuple(p for k, p in enumerate(points) if k != moving)
                for third, bound_bits, prior in history[(*scope, pair)]:
                    if third == points[moving]:
                        continue
                    shared = True
                    tolerance = Fraction.from_float(number(q["tolerance_bits"]))
                    margin = tolerance - Fraction.from_float(number(bound_bits))
                    if margin < 0:
                        continue
                    delta2 = sum((Fraction.from_float(number(a)) - Fraction.from_float(number(b))) ** 2
                                 for a, b in zip(third, points[moving]))
                    # Fixed model/support: directed distance is 1-Lipschitz under corresponding vertex movement.
                    if delta2 <= margin * margin:
                        certified = prior
                        break
                if certified is not None:
                    break
            counts["new_single_triangle_queries"] += 1
            counts["new_positions_sharing_edge"] += shared
            if certified is not None:
                counts["numeric_margin_proved_queries"] += 1
                if q["cache"] != "hit":
                    counts["numeric_margin_proved_misses"] += 1
                    work["numeric_margin_proved_misses"] += q["work"]
                if len(examples) < 12:
                    examples.append({"query": index, "prior_query": certified, "work": q["work"], "cache": q["cache"]})
        if len(vertices) == 1 and len(set(vertices[0])) == 3:
            points = tuple(sorted(vertices[0]))
            for moving in range(3):
                pair = tuple(p for k, p in enumerate(points) if k != moving)
                history[(*scope, pair)].append((points[moving], q["bound_bits"], index))
        exact[key] = q["bound_bits"]
        canonical.add(normalized)
    assert counts["completed_queries"] + counts["unresolved_queries"] == len(queries)
    return {"traced_queries": len(queries), "counts": dict(counts), "counted_work": dict(work), "numeric_margin_examples": examples}


def self_check():
    bits = lambda x: struct.unpack(">Q", struct.pack(">d", x))[0]
    triangle = ((0., 0., 0.), (1., 0., 0.), (0., 1., 0.))
    cases = [(triangle, .25, "miss", 100, "0x1", (7,), 1.),
             (triangle, .25, "hit", 14, "0x1", (7,), 1.),
             (triangle[1:] + triangle[:1], .25, "miss", 50, "0x1", (7,), 1.),
             (triangle[:2] + ((0., 1.1, 0.),), .35, "miss", 200, "0x1", (7,), 1.),
             (triangle[:2] + ((0., 3., 0.),), 2., "miss", 300, "0x1", (7,), 1.),
             (triangle, .25, "miss", 101, "0x1", (7,), 1.),
             (triangle, .25, "miss", 90, "0x1", (8,), 1.),
             (triangle, .25, "miss", 90, "0x3", (7,), 1.),
             (triangle, .25, "bypass", 90, "0x1", (7,), 2.)]
    lines = []
    for i, (points, bound, cache, work, authority, support, tolerance) in enumerate(cases):
        identity = f"authority={authority} budget=0x2 start={i}"
        xyz = ",".join(f"{bits(x):016x}" for p in points for x in p) + ",;"
        lines += [f"NATIVE_SURFACE_QUERY {identity} tolerance={bits(tolerance):016x} support={','.join(map(str,support))}, triangles={xyz}\n",
                  f"NATIVE_SURFACE_QUERY_RESULT {identity} cache={cache} work={work} bound={bits(bound):016x}\n"]
    result = analyze(parse(lines))
    assert len(parse(["NATIVE_SURFACE_QUERY tolerance=.1 model=0 mesh=1 facets=7, points=0,0,0;\n", *lines])) == 9
    assert result["counts"]["completed_queries"] == 9
    assert result["counts"]["identical_repeat_misses"] == 1
    assert result["counts"]["permutation_alias_misses"] == 1
    assert result["counts"]["new_positions_sharing_edge"] == 2
    assert result["counts"]["numeric_margin_proved_queries"] == 1
    assert result["counted_work"]["numeric_margin_proved_misses"] == 200
    assert sum(result["counted_work"].get(k, 0) for k in ("hit", "miss", "bypass")) == 1035
    assert "cache" not in parse(lines[:1])[0]


def main():
    self_check()
    launch = json.loads((RUN / "current_query245_attempt.json").read_text())
    attempt = Path(launch["case"])
    status = json.loads((attempt / "status.json").read_text())
    assert status["state"] == "terminal" and status["exit_code"] == 0 and status["pins_verified"]
    log = attempt / "run.log"
    assert log.stat().st_size <= 256 << 20
    digest = hashlib.sha256(log.read_bytes()).hexdigest()
    with log.open() as stream:
        result = analyze(parse(stream))
    assert result["counts"].get("completed_queries", 0) > 0
    assert hashlib.sha256(log.read_bytes()).hexdigest() == digest
    result.update(source_attempt=str(attempt), log_sha256=digest,
                  analysis_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
                  scope="Bounded diagnostic only. Exact support/reference/tolerance identity retained; numeric motion margin uses exact rational squared-distance comparison against an existing conservative bound. This does not certify association,orientation,features,body separation,heights,publication or acceptance. No production motion cache or speedup established; timings under verbose tracing are not performance evidence.")
    (RUN / "mach245_query_identity_assessment.json").write_text(json.dumps(result, indent=2) + "\n")
    print(json.dumps(result, indent=2))


if __name__ == "__main__":
    main()
