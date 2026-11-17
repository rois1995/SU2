#!/usr/bin/env python3
"""Parse post-only M26 and D9 prepare rows from a two-event, six-rank full run."""
import argparse
import csv
import importlib.util
import json
import math
import re
import tempfile
from pathlib import Path

HERE = Path(__file__).resolve().parent
STAGED = HERE.parents[3]
BASE_SCRIPT = (STAGED / "night_20261115" / "N8" / "A2_A3_N12" / "section6_8" / "reports" /
               "report_lane_census.py")
spec = importlib.util.spec_from_file_location("report_lane_census", BASE_SCRIPT)
base = importlib.util.module_from_spec(spec)
spec.loader.exec_module(base)
strip_mpi_prefixes = base.strip_mpi_prefixes

WALL = re.compile(r"\bNATIVE3D_WALL_LANE_CENSUS\s+(.*)")
PREPARE = re.compile(r"\bNATIVE3D_WALL_PREPARE\s+(.*)")
FINAL = base.FINAL
WALL_INTS = (
    "commits", "class_s", "class_w1", "class_w2m", "class_w2o", "w1_kind0", "w1_kind1",
    "w1_kind2", "w2m_kind1", "w1_link_local", "w2m_link_local", "w1_first_proof",
    "w1_link_local_first_proof",
)
WALL_SECONDS = (
    "seconds_s", "seconds_w1", "seconds_w2m", "seconds_w2o", "seconds_w1_link_local",
    "seconds_w1_link_local_first_proof", "seconds_w2m_link_local",
)
PREP_PHASES = ("preparation", "metric", "construction", "surface", "quality", "final_proof", "transfer", "output")
PREP_INTS = ("calls", "edits", "faces")


def fields(text):
    return dict(re.findall(r"([A-Za-z_][A-Za-z_0-9]*)=([^\s]+)", text))


def integer(row, key, path, line):
    try:
        value = int(row[key])
    except (KeyError, ValueError) as exc:
        raise ValueError(f"{path}:{line}: {key} must be an integer") from exc
    if value < 0:
        raise ValueError(f"{path}:{line}: {key} must be nonnegative")
    return value


def finite_seconds(row, key, path, line):
    try:
        value = float(row[key])
    except (KeyError, ValueError) as exc:
        raise ValueError(f"{path}:{line}: {key} must be numeric") from exc
    if not math.isfinite(value) or value < 0:
        raise ValueError(f"{path}:{line}: {key} must be finite and nonnegative")
    return value


def parse_wall(row, path, line, expected_rank):
    missing = (set(WALL_INTS) | set(WALL_SECONDS)) - row.keys()
    if missing:
        raise ValueError(f"{path}:{line}: M26 missing fields {sorted(missing)}")
    if row.get("rank") != str(expected_rank):
        raise ValueError(f"{path}:{line}: M26 rank {row.get('rank')} != {expected_rank}")
    result = {key: integer(row, key, path, line) for key in WALL_INTS}
    result.update({key: finite_seconds(row, key, path, line) for key in WALL_SECONDS})
    classes = sum(result[key] for key in ("class_s", "class_w1", "class_w2m", "class_w2o"))
    if classes != result["commits"]:
        raise ValueError(f"{path}:{line}: M26 class sum {classes} != commits {result['commits']}")
    if result["w1_kind0"] + result["w1_kind1"] + result["w1_kind2"] > result["class_w1"]:
        raise ValueError(f"{path}:{line}: W1 ladder subtotals exceed class_w1")
    if result["w2m_kind1"] > result["class_w2m"]:
        raise ValueError(f"{path}:{line}: W2M kind-1 count exceeds class_w2m")
    if result["w1_link_local"] > result["class_w1"] or result["w2m_link_local"] > result["class_w2m"]:
        raise ValueError(f"{path}:{line}: link_local count exceeds its class")
    if result["w1_first_proof"] > result["class_w1"]:
        raise ValueError(f"{path}:{line}: first_proof count exceeds class_w1")
    if result["w1_link_local_first_proof"] > result["w1_link_local"]:
        raise ValueError(f"{path}:{line}: link_local first_proof exceeds W1 link_local")
    return result


def parse_prepare(row, path, line, expected_rank):
    needed = set(PREP_INTS) | {"seconds", "rank", "stage"}
    missing = needed - row.keys()
    if missing:
        raise ValueError(f"{path}:{line}: WALL_PREPARE missing fields {sorted(missing)}")
    if row.get("rank") != str(expected_rank) or row.get("stage") not in PREP_PHASES:
        raise ValueError(f"{path}:{line}: invalid WALL_PREPARE rank/stage {row}")
    clean = {key: integer(row, key, path, line) for key in PREP_INTS}
    clean["seconds"] = finite_seconds(row, "seconds", path, line)
    clean["rank"] = expected_rank
    clean["stage"] = row["stage"]
    return clean


def event_sidecars(path, rank):
    walls = [None, None]
    prepares = [dict() for _ in range(2)]
    timers = [[] for _ in range(2)]
    event = 0
    for number, raw in enumerate(path.read_text(errors="replace").splitlines(), 1):
        line = strip_mpi_prefixes(raw)
        match = WALL.search(line)
        if match:
            if event >= 2:
                raise ValueError(f"{path}:{number}: M26 row after two event boundaries")
            if walls[event] is not None:
                raise ValueError(f"{path}:{number}: duplicate M26 row in event {event + 1}")
            walls[event] = parse_wall(fields(match.group(1)), path, number, rank)
        match = PREPARE.search(line)
        if match:
            if event >= 2:
                raise ValueError(f"{path}:{number}: WALL_PREPARE row after two event boundaries")
            row = parse_prepare(fields(match.group(1)), path, number, rank)
            if row["stage"] in prepares[event]:
                raise ValueError(f"{path}:{number}: duplicate WALL_PREPARE stage {row['stage']} event {event + 1}")
            prepares[event][row["stage"]] = row
        match = FINAL.search(line)
        if match:
            summary = fields(match.group(1))
            if summary.get("rank") != str(rank):
                raise ValueError(f"{path}:{number}: timer summary rank mismatch")
            if event >= 2:
                raise ValueError(f"{path}:{number}: more than two rank timer events")
            try:
                fixed = int(summary["fixedstorage"])
            except (KeyError, ValueError) as exc:
                raise ValueError(f"{path}:{number}: timer summary missing integer fixedstorage") from exc
            if fixed != 4120:
                raise ValueError(f"{path}:{number}: timer fixedstorage {fixed} != 4120")
            timers[event].append(summary)
            if summary.get("root") == "remesh" and summary.get("boundary") == "final":
                if walls[event] is None:
                    raise ValueError(f"{path}:{number}: missing M26 row before event {event + 1} final boundary")
                if set(prepares[event]) != set(PREP_PHASES):
                    missing = sorted(set(PREP_PHASES) - prepares[event].keys())
                    raise ValueError(f"{path}:{number}: event {event + 1} missing WALL_PREPARE stages {missing}")
                event += 1
    if event != 2:
        raise ValueError(f"{path}: expected two remesh final boundaries; found {event}")
    if any(not rows for rows in timers):
        raise ValueError(f"{path}: event has no NATIVE3D_TIMER_SUMMARY rows")
    return [{"wall": walls[i], "prepare": prepares[i], "timer_summaries": timers[i]} for i in range(2)]


def collect(run_dir):
    paths = [run_dir / "rank_output" / "1" / f"rank.{rank}" / "stdout" for rank in range(6)]
    absent = [str(path) for path in paths if not path.is_file()]
    if absent:
        raise ValueError("missing rank logs: " + ", ".join(absent))
    parsed = [base.parse_rank(path, rank) for rank, path in enumerate(paths)]
    sidecars = [event_sidecars(path, rank) for rank, path in enumerate(paths)]
    events = []
    for event in range(2):
        rows, prep_rows = [], []
        for rank in range(6):
            lane = parsed[rank]["lane"][event]
            quality_local_wall = lane[("quality", "local_wall")]["commits"]
            wall = sidecars[rank][event]["wall"]
            if wall["commits"] != quality_local_wall:
                raise ValueError(f"rank {rank} event {event + 1}: M26 commits {wall['commits']} != N12 quality/local_wall commits {quality_local_wall}")
            rows.append({"rank": rank, "quality_local_wall_commits": quality_local_wall, **wall})
            prep_rows.extend(sidecars[rank][event]["prepare"].values())
        events.append({"event": event + 1, "rows": rows, "shares": shares(rows),
                       "wall_prepare": sorted(prep_rows, key=lambda row: (row["rank"], PREP_PHASES.index(row["stage"]))),
                       "timer_summary_count": sum(len(sidecars[rank][event]["timer_summaries"]) for rank in range(6))})
    combined_rows = [row for event in events for row in event["rows"]]
    return {"events": events, "combined_shares": shares(combined_rows)}


def shares(rows):
    commits = sum(row["commits"] for row in rows)
    seconds = sum(sum(row[key] for key in WALL_SECONDS[:4]) for row in rows)
    groups = {
        "S": ("class_s", "seconds_s"),
        "W1_link_local": ("w1_link_local", "seconds_w1_link_local"),
        "W1_link_local_first_proof": ("w1_link_local_first_proof", "seconds_w1_link_local_first_proof"),
        "W2M_link_local": ("w2m_link_local", "seconds_w2m_link_local"),
    }
    result = {"denominators": {"commits": commits, "seconds": seconds}}
    for name, (count_key, seconds_key) in groups.items():
        count = sum(row[count_key] for row in rows)
        busy = sum(row[seconds_key] for row in rows)
        result[name] = {"commits": count, "commit_share": count / commits if commits else None,
                        "seconds": busy, "seconds_share": busy / seconds if seconds else None}
    return result


def render_markdown(report):
    lines = ["# N15 / M26 report", "", "Rows are grouped by each rank's next `NATIVE3D_TIMER_SUMMARY root=remesh boundary=final`. "
             "The report requires 6 ranks × 2 events, reconciles per-rank quality/local-wall commits, and checks every timer summary has `fixedstorage=4120`.", ""]
    for item in report["events"]:
        lines += [f"## Event {item['event']}", "", "### M26 counts by rank", "",
                  "| rank | quality local-wall commits | M26 commits | S | W1 | W2M | W2O | W1 kind0 | W1 kind1 | W1 kind2 | W2M kind1 | W1 link | W2M link | W1 proof | W1 link+proof |",
                  "|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|"]
        for row in item["rows"]:
            values = (row["rank"], row["quality_local_wall_commits"], row["commits"], row["class_s"], row["class_w1"],
                      row["class_w2m"], row["class_w2o"], row["w1_kind0"], row["w1_kind1"], row["w1_kind2"],
                      row["w2m_kind1"], row["w1_link_local"], row["w2m_link_local"], row["w1_first_proof"],
                      row["w1_link_local_first_proof"])
            lines.append("| " + " | ".join(map(str, values)) + " |")
        lines += ["", "### M26 busy seconds by rank", "",
                  "| rank | S | W1 | W2M | W2O | W1 link_local | W1 link_local + first_proof | W2M link_local |",
                  "|---:|---:|---:|---:|---:|---:|---:|---:|"]
        for row in item["rows"]:
            values = (row["rank"], row["seconds_s"], row["seconds_w1"], row["seconds_w2m"], row["seconds_w2o"],
                      row["seconds_w1_link_local"], row["seconds_w1_link_local_first_proof"],
                      row["seconds_w2m_link_local"])
            lines.append("| " + " | ".join(map(str, values)) + " |")
        lines += ["", "### Six-rank shares", "", "Shares use all M26 commits or all M26 class seconds as denominator.", "",
                  "| subset | commits / total | commit share | seconds / total | seconds share |", "|---|---:|---:|---:|---:|"]
        lines.extend(share_lines(item["shares"]))
        lines += ["", "### D9 WALL_PREPARE rows", "", "| rank | stage | calls | seconds | edits | faces |", "|---:|---|---:|---:|---:|---:|"]
        for row in item["wall_prepare"]:
            lines.append(f"| {row['rank']} | {row['stage']} | {row['calls']} | {row['seconds']} | {row['edits']} | {row['faces']} |")
        lines.append(f"\nTimer-summary rows checked: {item['timer_summary_count']}.")
    lines += ["", "## Combined two-event shares", "", "| subset | commits / total | commit share | seconds / total | seconds share |", "|---|---:|---:|---:|---:|"]
    lines.extend(share_lines(report["combined_shares"]))
    lines += ["", "This post-processing report does not make a production result-gate decision."]
    return "\n".join(lines) + "\n"


def share_lines(summary):
    lines = []
    for name in ("S", "W1_link_local", "W1_link_local_first_proof", "W2M_link_local"):
        item = summary[name]
        lines.append(f"| {name} | {item['commits']} / {summary['denominators']['commits']} | "
                     f"{percent(item['commit_share'])} | {item['seconds']:.9g} / {summary['denominators']['seconds']:.9g} | "
                     f"{percent(item['seconds_share'])} |")
    return lines


def percent(value):
    return "n/a" if value is None else f"{100 * value:.4f}%"


def write_reports(report, prefix):
    payload = {"scope": "N15 M26 post-only report; no result-gate decision", **report}
    prefix.with_suffix(".json").write_text(json.dumps(payload, indent=2) + "\n")
    fields = ("event", "rank", "quality_local_wall_commits", *WALL_INTS, *WALL_SECONDS)
    with prefix.with_suffix(".csv").open("w", newline="") as stream:
        writer = csv.DictWriter(stream, fieldnames=fields)
        writer.writeheader()
        for event in report["events"]:
            for row in event["rows"]:
                writer.writerow({"event": event["event"], **row})
    prep_fields = ("event", "rank", "stage", *PREP_INTS, "seconds")
    with prefix.with_name(prefix.name + "_prepare").with_suffix(".csv").open("w", newline="") as stream:
        writer = csv.DictWriter(stream, fieldnames=prep_fields)
        writer.writeheader()
        for event in report["events"]:
            for row in event["wall_prepare"]:
                writer.writerow({"event": event["event"], **row})
    prefix.with_suffix(".md").write_text(render_markdown(report))


def expect_error(function, expected):
    try:
        function()
    except ValueError as exc:
        if expected not in str(exc):
            raise AssertionError(f"expected {expected!r}, got {exc}") from exc
    else:
        raise AssertionError(f"expected error containing {expected!r}")


def synthetic_log(rank):
    lines = []
    for event in range(2):
        for stage in base.STAGES:
            for cls in base.CLASSES:
                commits = 2 if stage == "quality" and cls == "local_wall" else 1
                lines.append(f"NATIVE3D_LANE_CENSUS rank={rank} stage={stage} class={cls} trials=3 approved={commits} commits={commits} seconds=0.5 work=1 same_skin=0 no_parameter_edits=0 no_coverage=0")
        lines.append(f"NATIVE3D_WALL_LANE_CENSUS rank={rank} commits=2 class_s=1 class_w1=1 class_w2m=0 class_w2o=0 w1_kind0=0 w1_kind1=1 w1_kind2=0 w2m_kind1=0 w1_link_local=1 w2m_link_local=0 w1_first_proof=1 w1_link_local_first_proof=1 seconds_s=0.25 seconds_w1=0.75 seconds_w2m=0 seconds_w2o=0 seconds_w1_link_local=0.75 seconds_w1_link_local_first_proof=0.75 seconds_w2m_link_local=0")
        for stage in PREP_PHASES:
            lines.append(f"NATIVE3D_WALL_PREPARE rank={rank} stage={stage} calls=1 seconds=0.1 edits=2 faces=3")
        lines.append(f"NATIVE3D_TIMER_SUMMARY root=remesh boundary=final rank={rank} ranks=6 fixedstorage=4120")
    return lines


def self_check():
    with tempfile.TemporaryDirectory() as temp:
        run = Path(temp)
        for rank in range(6):
            target = run / "rank_output" / "1" / f"rank.{rank}" / "stdout"
            target.parent.mkdir(parents=True)
            target.write_text("\n".join(synthetic_log(rank)) + "\n")
        report = collect(run)
        assert [len(event["rows"]) for event in report["events"]] == [6, 6]
        assert [len(event["wall_prepare"]) for event in report["events"]] == [48, 48]
        assert report["events"][0]["shares"]["W1_link_local_first_proof"]["commits"] == 6
        assert report["events"][0]["shares"]["W1_link_local_first_proof"]["commit_share"] == 0.5
        markdown = render_markdown(report)
        assert "W1 link_local + first_proof" in markdown and "D9 WALL_PREPARE rows" in markdown
        output = run / "synthetic_report"
        write_reports(report, output)
        assert output.with_suffix(".json").is_file() and output.with_suffix(".csv").is_file()
        assert output.with_suffix(".md").is_file() and output.with_name("synthetic_report_prepare.csv").is_file()
        path = run / "rank_output" / "1" / "rank.0" / "stdout"
        original = path.read_text()
        duplicate = next(line for line in synthetic_log(0) if "NATIVE3D_WALL_LANE_CENSUS" in line)
        original_lines = original.splitlines()
        final_index = next(i for i, line in enumerate(original_lines) if "boundary=final" in line)
        original_lines.insert(final_index, duplicate)
        path.write_text("\n".join(original_lines) + "\n")
        expect_error(lambda: collect(run), "duplicate M26")
        path.write_text("\n".join(line for line in original.splitlines() if "NATIVE3D_WALL_LANE_CENSUS" not in line) + "\n")
        expect_error(lambda: collect(run), "missing M26")
        path.write_text(original.replace("class_w2o=0", "class_w2o=1", 1))
        expect_error(lambda: collect(run), "class sum")
        path.write_text(original.replace("w1_link_local=1", "w1_link_local=2", 1))
        expect_error(lambda: collect(run), "link_local count")
        (run / "rank_output" / "1" / "rank.5" / "stdout").unlink()
        expect_error(lambda: collect(run), "missing rank logs")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("input", type=Path, nargs="?", help="full two-event workflow directory with per-rank stdout")
    parser.add_argument("--out", type=Path, help="output prefix for JSON, CSV, Markdown, and prepare CSV")
    parser.add_argument("--self-check", action="store_true")
    args = parser.parse_args()
    if args.self_check:
        self_check()
        print("self-check passed: 6-rank/two-event parsing, shares, duplicate/missing, class-sum, subset, and rank-coverage rejection")
        return
    if args.input is None or args.out is None:
        parser.error("fullrun directory and --out prefix are required")
    report = collect(args.input)
    write_reports(report, args.out)
    print(f"wrote {args.out.with_suffix('.json')}, .csv, .md, and {args.out.name}_prepare.csv")


if __name__ == "__main__":
    main()
