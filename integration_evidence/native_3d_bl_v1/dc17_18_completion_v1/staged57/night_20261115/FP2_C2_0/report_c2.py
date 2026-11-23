"""Aggregate dry-run surface-collapse records into the ordered C2 measurements."""

from argparse import ArgumentParser
from collections import Counter
from dataclasses import dataclass, field
from gzip import open as gzip_open
from hashlib import file_digest
import json
import math
from pathlib import Path
import re

HERE = Path(__file__).resolve().parent
C1 = HERE.parent / "FP2_C1"
CASES = ("N1", "N6P")
CLASSES = ("border", "corner", "feature", "sheet")
CHAIN = ("T1", "T2", "T2s", "T2e", "T3", "T4", "T5", "T5f", "T6", "T7", "T8", "T9", "T10", "T11", "T12")
TESTS = (*CHAIN, "T8m", "T9c", "T12L", "T13")
FAILURE_ORDER = ("T1", "T2", "T2s", "T2e", "T3", "T7", "T8", "T9", "T4", "T5", "T5f", "T6", "T10", "T11", "T12")
BOUND_PREFIX = tuple(test for test in CHAIN if test not in ("T8", "T12"))
HEIGHT_BINS = ("<0", "[0, 0.1)", "[0.1, 0.5)", "[0.5, 1)", "=1")
CONTACT_BINS = ("<1", "[1, 2)", "[2, 10)", "[10, 100)", "100 and above")
RECORD = re.compile(r"\bFP2_C2 (edge|direction|timing) (.*)$")
FIELD = re.compile(r"\b([\w]+)=([^\s]+)")


def read_json(path):
    return json.loads(path.read_text())


def raw_lines(path, expected_digest=None):
    archived = path.with_name(path.name + ".gz")
    stream = path.open("rb") if path.is_file() else gzip_open(archived, "rb")
    with stream:
        if expected_digest:
            assert file_digest(stream, "sha256").hexdigest() == expected_digest, path
            stream.seek(0)
        for line in stream:
            yield line.decode(errors="replace")


def vector(value):
    if value in ("", "-"):
        return None
    coordinates = tuple(float(part) for part in value.split(","))
    assert len(coordinates) == 3 and all(math.isfinite(part) for part in coordinates), value
    return coordinates


def bins(value, boundaries):
    return sum(value >= boundary for boundary in boundaries)


@dataclass(slots=True)
class AcceptedDirection:
    p: int
    keep_border: bool
    longest: float
    star: tuple


@dataclass
class PassData:
    """Store counts and candidates needed for the rank-local, static sweep."""

    edges: dict = field(default_factory=dict)
    directions: dict = field(default_factory=dict)
    classes: Counter = field(default_factory=Counter)
    kept: Counter = field(default_factory=Counter)
    sheet: Counter = field(default_factory=Counter)
    sheet_border: Counter = field(default_factory=Counter)
    accepted: dict = field(default_factory=dict)
    heights: dict = field(default_factory=dict)
    contacts: dict = field(default_factory=dict)
    contact_calls: set = field(default_factory=set)
    timing: dict | None = None


def all_pass(tests, names):
    return all(tests[test] == "P" for test in names)


def tally_tests(counts, row, tests):
    counts["sheets"] += 1
    if tests["T1"] != "P":
        counts["ineligible"] += 1
    if row["exact_failure"] == "1":
        assert tests["T4"] == "P", row
        counts["false_positive_T4"] += 1
    counts["unconverged"] += int(row["unconverged"])

    eligible = tests["T1"] == "P"
    selected = row["strict_selected"] == "1"
    if eligible:
        for test in TESTS[1:]:
            if test not in ("T12", "T12L"):
                assert tests[test] != "-", (test, row)
        sample = row["sample10"] == "1"
        required = all_pass(tests, BOUND_PREFIX)
        measured = tests["T12"] != "-"
        assert selected == (required or sample), row
        assert measured == (tests["T12L"] != "-"), row
        assert not measured or selected, row
        for label, enabled in (("T12", selected), ("T12_sample", sample), ("T12_required", required)):
            counts[f"{label}_planned"] += enabled
            counts[f"{label}_measured"] += enabled and measured
        for test in ("T10", "T11", "T12"):
            counts[f"children_{test}"] += int(row[f"calls_{test}"])
        if required:
            if tests["T12"] == "P":
                counts["without_T8"] += 1
                counts["with_T8m"] += tests["T8m"] == "P"
            elif not measured:
                counts["without_T8_unmeasured"] += 1
                counts["with_T8m_unmeasured"] += tests["T8m"] == "P"
    else:
        assert not selected and all(tests[test] == "-" for test in TESTS[1:]), row
        sample = False

    for test in TESTS:
        status = tests[test]
        counts[f"calls_{test}"] += status != "-"
        counts[f"errors_{test}"] += status == "E"
        alone = status != "-" and (test not in ("T12", "T12L") or sample)
        counts[f"alone_measured_{test}"] += alone
        counts[f"alone_pass_{test}"] += alone and status == "P"

    for test in CHAIN:
        if tests[test] != "P":
            break
        counts[f"chain_{test}"] += 1
    for test in FAILURE_ORDER:
        if tests[test] in ("F", "E"):
            counts[f"first_{test}"] += 1
            break
        if tests[test] == "-":
            assert test == "T12", row
            counts["chain_unmeasured"] += 1
            break
    else:
        counts["passed"] += 1
        counts["passing_non_outward"] += tests["T9c"] == "P"


def add_direction(data, row):
    p, k = int(row["p"]), int(row["k"])
    key = tuple(sorted((p, k)))
    assert p != k and row["pclass"] in CLASSES and row["kclass"] in CLASSES, row
    length = float(row["length"])
    assert (p, k) not in data.directions, row
    data.directions[p, k] = length
    data.classes[row["pclass"]] += 1
    if row["pclass"] != "sheet":
        return
    data.kept[row["kclass"]] += 1
    tests = {test: row[test] for test in TESTS}
    assert all(value in ("P", "F", "E", "-") for value in tests.values()) and tests["T1"] != "-", row
    assert all(row[key] in ("0", "1") for key in ("exact_failure", "sample10", "strict_selected")), row
    expected_sample = (data.sheet["sheets"] + 1) % 10 == 0
    assert (row["sample10"] == "1") == expected_sample, row
    groups = (data.sheet, data.sheet_border) if row["kclass"] == "border" else (data.sheet,)
    for counts in groups:
        tally_tests(counts, row, tests)
    if tests["T13"] != "-":
        data.contact_calls.add(p)
    if not all_pass(tests, CHAIN):
        return

    star = tuple(int(cell) for cell in row["star"].split(",") if cell not in ("", "-"))
    longest = float(row["longest"])
    assert star and math.isfinite(longest), row
    border = row["kclass"] == "border"
    data.accepted.setdefault(key, []).append(AcceptedDirection(p, border, longest, star))
    data.heights[p, k] = (float(row["h"]), border)
    if tests["T13"] == "P":
        distance = float(row["other"])
        contact = {
            "p": p, "xyz": vector(row["xyz"]), "distance": distance,
            "other_id": int(row["other_id"]) if math.isfinite(distance) else None,
            "other_xyz": vector(row["other_xyz"]) if math.isfinite(distance) else None,
        }
        assert not math.isnan(distance) and distance >= 0 and contact["xyz"] is not None, row
        seen = data.contacts.setdefault(p, (contact, []))
        assert seen[0] == contact, row
        seen[1].append(border)


def static_sweep(accepted, edges):
    occupied = set()
    edits = 0
    accepted_border = 0
    for edge in sorted(accepted, key=lambda pair: (edges[pair], pair)):
        chosen = min(accepted[edge], key=lambda direction: (direction.longest, -direction.p))
        if occupied.isdisjoint(chosen.star):
            occupied.update(chosen.star)
            edits += 1
            accepted_border += chosen.keep_border
    return edits, accepted_border


def nearest(contacts):
    return sorted(contacts, key=lambda row: (row["distance_ratio"], row["pass"], row["rank"], row["p"]))[:20]


def finish_pass(data, rank, phase):
    """Apply length/ID tie order to immutable stars and normalize geometry bins."""
    timing = data.timing
    assert timing is not None and float(timing["tolerance"]) > 0, (rank, phase, timing)
    tolerance = float(timing["tolerance"])
    assert len(data.directions) == 2 * len(data.edges), (rank, phase)
    for (a, b), length in data.edges.items():
        assert data.directions[a, b] == data.directions[b, a] == length, (rank, phase, a, b)
    assert data.sheet["passed"] == sum(map(len, data.accepted.values())), (rank, phase)
    assert data.sheet["passed"] + data.sheet["chain_unmeasured"] + sum(
        data.sheet[f"first_{test}"] for test in FAILURE_ORDER
    ) == data.sheet["sheets"], (rank, phase)
    all_completed = data.sheet["T12_planned"] == data.sheet["T12_measured"]
    assert timing["t12_stopped"] in ("0", "1"), timing
    assert (timing["t12_stopped"] == "0") == all_completed, (rank, phase)
    edit_count, border_edits = static_sweep(data.accepted, data.edges)

    for height, border in data.heights.values():
        ratio = height / tolerance
        assert ratio <= 1, (rank, phase, height, tolerance)
        label = HEIGHT_BINS[bins(ratio, (0, .1, .5, 1))]
        data.sheet[f"height_{label}"] += 1
        data.sheet_border[f"height_{label}"] += border

    contacts, border_contacts = [], []
    for contact, directions in data.contacts.values():
        ratio = contact["distance"] / tolerance
        label = CONTACT_BINS[bins(ratio, (1, 2, 10, 100))]
        missing_point = math.isinf(ratio)
        data.sheet[f"contact_{label}"] += len(directions)
        data.sheet[f"contact_inf"] += len(directions) if missing_point else 0
        border_count = sum(directions)
        data.sheet_border[f"contact_{label}"] += border_count
        data.sheet_border[f"contact_inf"] += border_count if missing_point else 0
        if not missing_point:
            entry = {**contact, "distance_ratio": ratio, "rank": rank, "pass": phase}
            contacts.append(entry)
            if border_count:
                border_contacts.append(entry)
    for counts in (data.sheet, data.sheet_border):
        counts["contact_unmeasured"] = counts["passed"] - sum(
            counts[f"contact_{label}"] for label in CONTACT_BINS
        )

    return {
        "rank": rank, "pass": phase, "edges": len(data.edges), "classes": dict(data.classes), "kept": dict(data.kept),
        "sheets": dict(data.sheet), "border_k": dict(data.sheet_border),
        "contacts": nearest(contacts), "border_contacts": nearest(border_contacts),
        "edits": edit_count, "border_edits": border_edits, "contact_calls": len(data.contact_calls),
        "times": {key: float(timing[f"seconds_{key}"]) for key in ("T10", "T11", "T12", "other")},
        "t12_stopped": int(timing["t12_stopped"]),
        "tolerance": tolerance, "limit": float(timing["limit"]),
    }


def parse_passes(lines, rank):
    data = {}
    for line in lines:
        record = RECORD.search(line)
        if not record:
            continue
        kind, tail = record.groups()
        row = dict(FIELD.findall(tail))
        assert int(row["rank"]) == rank, (rank, line)
        phase = int(row["pass"])
        current = data.setdefault(phase, PassData())
        if kind == "edge":
            edge = (int(row["a"]), int(row["b"]))
            assert edge[0] < edge[1] and edge not in current.edges, row
            current.edges[edge] = float(row["length"])
        elif kind == "direction":
            add_direction(current, row)
        else:
            assert current.timing is None, row
            current.timing = row
    return [finish_pass(group, rank, phase) for phase, group in sorted(data.items())]


def combine(rows, phase):
    result = {"rank": "sum", "pass": phase}
    for key in ("edges", "edits", "border_edits", "contact_calls", "t12_stopped"):
        result[key] = sum(row[key] for row in rows)
    for key in ("classes", "kept", "sheets", "border_k", "times"):
        totals = Counter()
        for row in rows:
            totals.update(row[key])
        result[key] = dict(totals)
    for key in ("contacts", "border_contacts"):
        result[key] = nearest(record for row in rows for record in row[key])
    return result


def input_cases(build):
    for name, nproc, npass in (("N1", 1, 1), ("N6P", 6, 2)):
        root = HERE / "cases" / f"build{build}" / name
        receipt_path = root / "measurement.json"
        receipt = read_json(receipt_path)
        assert receipt["passed"] and receipt["build"] == build and receipt["ranks"] == nproc, receipt_path
        assert receipt["wait"]["terminal"], receipt_path
        local = []
        global_states = set()
        for rank in range(nproc):
            log = root / "test_outer.log" if nproc == 1 else root / f"rank_output/1/rank.{rank}/stdout"
            digest = receipt["artifacts"][str(log.relative_to(root))]
            passes = parse_passes(raw_lines(log, digest), rank)
            assert [row["pass"] for row in passes] == list(range(1, npass + 1)), (name, rank)
            local.extend(passes)
            state, = receipt["rows_by_rank"][str(rank)]["state"]
            global_states.add((int(state["faces"]), int(state["points"])))
        faces, points = next(iter(global_states))
        assert len(global_states) == 1, (name, global_states)
        assert sum(int(rows["state"][0]["owned_faces"])
                   for rows in receipt["rows_by_rank"].values()) == faces, name
        local.sort(key=lambda row: (row["pass"], row["rank"]))
        totals = [combine([row for row in local if row["pass"] == p], p) for p in range(1, npass + 1)]
        yield name, {
            "rank_passes": local, "pass_sums": totals,
            "case_sum": combine(local, "all"), "handover_faces": faces, "handover_points": points,
            "receipt": str(receipt_path),
        }


def engine_reference():
    receipts = sorted(C1.glob("G5/build*/S/run_receipt.json"))
    if not receipts:
        return {"build": 640, "seed_faces": 86254, "exit_faces": 80540, "removed": 2857, "fallback": True}
    path, = receipts
    receipt = read_json(path)
    assert receipt["passed"], path
    log = Path(receipt["case"]) / "rank_output/1/rank.0/stdout"
    digest = receipt["artifacts"][str(log.relative_to(path.parent))]
    seed, end = None, None
    for line in raw_lines(log, digest):
        if "NATIVE3D_LOCAL_SURFACE_STATE rank=0 " in line:
            assert seed is None, log
            seed = int(dict(FIELD.findall(line))["faces"])
        if "NATIVE3D_STAGE rank=0 stage=length_collapse_exit " in line:
            end = int(dict(FIELD.findall(line))["working_faces"])
            break
    assert seed is not None and end is not None and seed >= end and (seed - end) % 2 == 0, log
    return {"build": receipt["build"], "seed_faces": seed, "exit_faces": end, "removed": (seed - end) // 2,
            "fallback": False, "log": str(log), "stdout_sha256": digest, "receipt": str(path)}


def table(headers, rows):
    return [
        "| " + " | ".join(headers) + " |",
        "| " + " | ".join("---" for _ in headers) + " |",
        *["| " + " | ".join(map(str, row)) + " |" for row in rows],
        "",
    ]


def report_markdown(build, data, engine):
    first = data["N1"]["pass_sums"][0]
    second = data["N6P"]["pass_sums"][1]

    def summary(key):
        return f'N1 {first["sheets"].get(key, 0):,} · N6P {second["sheets"].get(key, 0):,}'

    def strict_coverage(row):
        measured = row["sheets"].get("T12_measured", 0)
        planned = row["sheets"].get("T12_planned", 0)
        share = f" ({measured / planned:.1%})" if planned else ""
        return f"{measured:,} of {planned:,}{share}"

    lines = [
        f"# FP2-C2 step 0, build{build}", "",
        f"- Whole chain passed (N1 pass 1 and N6P pass 2) · {summary('passed')}",
        f'- One-sweep estimate · N1 {first["edits"]:,} points and {2 * first["edits"]:,} faces'
        f' · N6P {second["edits"]:,} points and {2 * second["edits"]:,} faces',
        f"- Whole chain with T8 omitted · {summary('without_T8')} · with T8m substituted · {summary('with_T8m')}", "",
        f'N1 evaluated {strict_coverage(first)} scheduled T12 directions. N6P evaluated '
        f'{strict_coverage(data["N6P"]["case_sum"])} over its two passes. The chain counts and sweep estimates '
        "use evaluated directions.",
        "",
        "Both labs retained the passing C1 wall-collapse setting and matched the required original integer fields, "
        "including proof work, modeled peak and the final cell digests. The test-driver build preserved the C1 "
        f'solver and production sources. [N1 sanity](../cases/build{build}/N1/measurement.json)'
        f' · [N6P sanity](../cases/build{build}/N6P/measurement.json)',
        "",
        "The dry runs evaluated candidate directions on the frozen rank-local mesh. Rank sums count rank-local "
        "observations, including shared edges if present. All-pass sums combine observations from independent passes.",
        "",
        "The probe runs after the existing pre-writeback rows. The inherited pass summary still follows writeback. "
        "The strict queries use captured child bindings and coordinates and run after the original proof and digest. "
        "A direction interrupted by the strict deadline keeps its child-call count and remains unevaluated.",
        "", "## R1 · Short edges and direction classes", "",
    ]
    scopes = [(name, row) for name, case in data.items() for row in
              (*case["rank_passes"], *case["pass_sums"], *((case["case_sum"],) if name == "N6P" else ()))]
    prefixes = ("Case", "Pass", "Rank")
    lines += table((*prefixes, "Edges", *[f"p {c}" for c in CLASSES], *[f"k {c} (sheet p)" for c in CLASSES],
                    "T1 ineligible"),
                   ([case, row["pass"], row["rank"], row["edges"], *[row["classes"].get(c, 0) for c in CLASSES],
                     *[row["kept"].get(c, 0) for c in CLASSES], row["sheets"].get("ineligible", 0)]
                    for case, row in scopes))

    def groups():
        for case, row in scopes:
            yield case, row, "all", row["sheets"]
            if case == "N6P":
                yield case, row, "border k", row["border_k"]

    lines += [
        "## R2 · Individual tests and cumulative chain", "",
        "Alone is passed / measured directions. T1 starts from all sheet directions. Later tests start from those "
        "that pass T1. T12 and T12L alone use only the flagged one-in-ten sample. T12L measures a one-sided "
        "classification bound that can be loose below tolerance. T13 measured includes an infinite distance when "
        "no other point exists.", "",
        "Each cell is **alone passed / measured · cumulative pass · errors**. The dash marks tests outside the chain.",
        "",
    ]
    def test_cell(counts, test):
        get = counts.get
        chain = get(f"chain_{test}", 0) if test in CHAIN else "—"
        return f"{get(f'alone_pass_{test}', 0)}/{get(f'alone_measured_{test}', 0)} · {chain} · {get(f'errors_{test}', 0)}"

    for tests in (CHAIN[:8], CHAIN[8:], tuple(test for test in TESTS if test not in CHAIN)):
        lines += table((*prefixes, "Sheet scope", *tests),
                       ([case, row["pass"], row["rank"], scope, *[test_cell(counts, test) for test in tests]]
                        for case, row, scope, counts in groups()))
    lines += [
        "T12 availability is measured / scheduled below. Known complete-chain passes exclude candidates whose "
        "T12 evaluation was unfinished. Truncation leaves those outcomes unmeasured.", "",
    ]
    completed = all(case["case_sum"]["t12_stopped"] == 0 for case in data.values())
    completeness = ("Every scheduled T12/T12L direction completed." if completed else
                    "The rank-local T12 cap left some scheduled directions unmeasured.")
    lines += [completeness, ""]
    labels = ("T12", "T12_sample", "T12_required")
    lines += table((*prefixes, "Sheet scope", "T12 overall", "T12 1-in-10", "T12 required without T8",
                    "Whole chain known pass", "Chain awaiting T12", "T4 float pass / exact fail", "T10 unconverged"),
                   ([case, row["pass"], row["rank"], scope,
                     *[f"{counts.get(f'{tag}_measured', 0)}/{counts.get(f'{tag}_planned', 0)}" for tag in labels],
                     counts.get("passed", 0), counts.get("chain_unmeasured", 0), counts.get("false_positive_T4", 0),
                     counts.get("unconverged", 0)]
                    for case, row, scope, counts in groups()))
    lines += ["## R3 · Reverse witness variants", ""]
    lines += table((*prefixes, "Sheet scope", "T8 omitted", "T8m substituted", "Omitted awaiting T12",
                    "Substituted awaiting T12"),
                   ([case, row["pass"], row["rank"], scope,
                     *[counts.get(key, 0) for key in ("without_T8", "with_T8m", "without_T8_unmeasured",
                                                       "with_T8m_unmeasured")]]
                    for case, row, scope, counts in groups()))
    lines += ["## R4 · Outward height of passing directions", "",
              "Heights are divided by tolerance. T9 admits equality, which has its own endpoint column.", ""]
    lines += table((*prefixes, "Sheet scope", "T9c pass", *HEIGHT_BINS),
                   ([case, row["pass"], row["rank"], scope, counts.get("passing_non_outward", 0),
                     *[counts.get(f"height_{label}", 0) for label in HEIGHT_BINS]]
                    for case, row, scope, counts in groups()))
    lines += ["## R5 · Other-surface contact for passing directions", "",
              "Distances are divided by tolerance. Infinite distances are included in the last bin and shown "
              "separately. Nearest lists include each removed point once per rank and pass.", ""]
    lines += table((*prefixes, "Sheet scope", *CONTACT_BINS, "inf", "Unmeasured"),
                   ([case, row["pass"], row["rank"], scope, *[counts.get(f"contact_{label}", 0) for label in CONTACT_BINS],
                     counts.get("contact_inf", 0), counts.get("contact_unmeasured", 0)]
                    for case, row, scope, counts in groups()))
    for case, row, scope, _ in groups():
        near = row["contacts" if scope == "all" else "border_contacts"]
        if not near:
            continue
        lines += [f"**{case} · pass {row['pass']} · rank {row['rank']} · {scope}**", ""]
        lines += table(("Pass", "Rank", "Removed p", "p coordinates", "Other point", "Other coordinates", "Distance",
                        "Distance / tol"),
                       ([c["pass"], c["rank"], c["p"], ", ".join(map(str, c["xyz"])), c["other_id"],
                         ", ".join(map(str, c["other_xyz"])), f"{c['distance']:.9g}", f"{c['distance_ratio']:.7g}"]
                        for c in near))
    lines += ["## R6 · One-sweep estimate", "",
              "The estimate uses the fixed input stars and the specified length, point-ID and longest-new-edge "
              "ordering. Each selected star is disjoint from all previously selected stars on the same rank and pass.",
              ""]
    lines += table((*prefixes, "Points removed", "Faces removed", "Selected with border k"),
                   ([case, row["pass"], row["rank"], row["edits"], 2 * row["edits"], row["border_edits"]]
                    for case, row in scopes))
    lines += ["## R7 · First failure in cost order", "",
              "Errors count as failures. A direction awaiting T12 is shown separately.", ""]
    lines += table((*prefixes, "Sheet scope", *FAILURE_ORDER, "Awaiting T12"),
                   ([case, row["pass"], row["rank"], scope,
                     *[counts.get(f"first_{test}", 0) for test in FAILURE_ORDER],
                     counts.get("chain_unmeasured", 0)]
                    for case, row, scope, counts in groups()))
    lines += ["## R8 · Separate test costs", "",
              "Summed seconds add rank/pass costs. Calls include errors. Underlying calls count child-face "
              "evaluations. Other calls count the remaining direction tests with one contact lookup per p. "
              "T12L shares the T12 bound and its recorded time.", ""]
    clocks = ("T10", "T11", "T12", "other")
    lines += table((*prefixes, *[f"{clock} seconds" for clock in clocks], "T10 calls", "T11 calls", "T12 calls",
                    "Other calls", "Unique T13 calls", "T10 child calls", "T11 child calls", "T12 child calls",
                    "Capped rank/passes"),
                   ([case, row["pass"], row["rank"], *[f"{row['times'][clock]:.6f}" for clock in clocks],
                     *[row["sheets"].get(f"calls_{clock}", 0) for clock in clocks[:3]],
                     sum(row["sheets"].get(f"calls_{test}", 0)
                         for test in TESTS if test not in (*clocks[:3], "T12L", "T13")) + row["contact_calls"],
                     row["contact_calls"],
                     *[row["sheets"].get(f"children_{clock}", 0) for clock in clocks[:3]], row["t12_stopped"]]
                    for case, row in scopes))
    baseline = "FP2-B S1 fallback" if engine["fallback"] else "FP2-C1 S event 1"
    lines += [
        "## R9 · Handover surface and engine reference", "",
        *table(("Case", "Unique surface faces at handover", "Unique surface points at handover"),
               ([case, row["handover_faces"], row["handover_points"]] for case, row in data.items())),
        *table(("Reference", "Build", "Seed faces", "Collapse-exit faces", "Surface points removed in engine collapse"),
               [(baseline, engine["build"], engine["seed_faces"], engine["exit_faces"], engine["removed"])]),
        "The accompanying JSON records the terminal measurement receipts, timing-cap counts, and the source "
        "of the engine observation.", "",
    ]
    return "\n".join(lines)


def self_check():
    tests = dict.fromkeys(TESTS, "P")
    row = {"p": "10", "k": "11", "pclass": "sheet", "kclass": "border", "length": "0.3",
           "longest": "1.1", "sample10": "0", "strict_selected": "1",
           "exact_failure": "0", "unconverged": "0", "star": "1,2",
           "h": "0.1", "other": "inf", "other_id": "-", "xyz": "1,2,3", "other_xyz": "-",
           "calls_T10": "3", "calls_T11": "3", "calls_T12": "3", **tests}
    data = PassData(edges={(10, 11): 0.3})
    add_direction(data, row)
    add_direction(data, {**row, "p": "11", "k": "10", "pclass": "border", "kclass": "sheet"})
    data.timing = {"tolerance": "1", "limit": "0.125", "t12_stopped": "0",
                   "seconds_T10": "1", "seconds_T11": "2", "seconds_T12": "3", "seconds_other": "4"}
    values = finish_pass(data, 0, 1)
    assert values["edits"] == values["border_edits"] == 1
    assert values["sheets"]["height_[0.1, 0.5)"] == values["border_k"]["contact_inf"] == 1
    assert values["sheets"]["without_T8"] == values["sheets"]["with_T8m"] == 1
    counts = Counter()
    tally_tests(counts, row, {**tests, "T8": "F", "T12": "-", "T12L": "-"})
    assert counts["first_T8"] == counts["without_T8_unmeasured"] == counts["with_T8m_unmeasured"] == 1
    accepted = {(10, 11): [AcceptedDirection(10, True, 1, (1,)), AcceptedDirection(11, False, 1, (2,))],
                (12, 13): [AcceptedDirection(12, False, 1, (2,))]}
    assert static_sweep(accepted, {(10, 11): .4, (12, 13): .5}) == (1, 0)
    print("FP2-C2 report self-check passed.")


def main():
    parser = ArgumentParser(description=__doc__)
    parser.add_argument("build", type=int, nargs="?")
    parser.add_argument("--self-check", action="store_true")
    args = parser.parse_args()
    if args.self_check:
        self_check()
        return
    if args.build is None:
        parser.error("provide the measured build label or --self-check")
    data = dict(input_cases(args.build))
    assert tuple(data) == CASES
    engine = engine_reference()
    out = HERE / "reports" / f"FP2_C2_0_{args.build}.md"
    out.parent.mkdir(exist_ok=True)
    out.write_text(report_markdown(args.build, data, engine))
    out.with_suffix(".json").write_text(json.dumps({"build": args.build, "cases": data, "engine": engine},
                                                  indent=2, allow_nan=False) + "\n")
    print(out)


if __name__ == "__main__":
    main()
