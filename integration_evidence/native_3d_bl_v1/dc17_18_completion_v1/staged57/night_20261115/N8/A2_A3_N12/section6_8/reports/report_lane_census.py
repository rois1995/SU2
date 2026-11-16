#!/usr/bin/env python3
"""Parse and report the two-event, six-rank native lane census."""
import argparse
import csv
import importlib.util
import json
import math
import re
import tempfile
from pathlib import Path

HERE = Path(__file__).resolve().parent
STRIPPER = (HERE.parents[4] / "nobl_clean_check_20261114" / "extract_markers.py")
spec = importlib.util.spec_from_file_location("extract_markers", STRIPPER)
extract_markers = importlib.util.module_from_spec(spec)
spec.loader.exec_module(extract_markers)
strip_mpi_prefixes = extract_markers.strip_mpi_prefixes

CLASSES = ("local_interior", "local_wall", "shared_interior", "shared_wall")
STAGES = ("construction", "quality")
INTS = ("trials", "approved", "commits", "work", "same_skin", "no_parameter_edits", "no_coverage")
FIELDS = INTS[:3] + ("seconds", "work") + INTS[4:]
LANE = re.compile(r"\bNATIVE3D_LANE_CENSUS\s+(.*)")
FINAL = re.compile(r"\bNATIVE3D_TIMER_SUMMARY\s+(.*)")
QUALITY = re.compile(r"\bNATIVE3D_QUALITY\s+(.*)")
LADDER = re.compile(r"\bNATIVE3D_QUALITY_LADDER\s+(.*)")


def fields(text):
    return dict(re.findall(r"([A-Za-z_][A-Za-z_0-9]*)=([^\s]+)", text))


def parse_rank(path, expected_rank):
    events = [dict() for _ in range(2)]
    quality, ladder = ([[] for _ in range(2)] for _ in range(2))
    finals = 0
    for number, raw in enumerate(path.read_text(errors="replace").splitlines(), 1):
        line = strip_mpi_prefixes(raw)
        legacy = QUALITY.search(line)
        ladder_match = LADDER.search(line)
        if finals < 2 and legacy:
            row = fields(legacy.group(1))
            if row.get("rank") == str(expected_rank):
                quality[finals].append(row)
        if finals < 2 and ladder_match:
            row = fields(ladder_match.group(1))
            if row.get("rank") == str(expected_rank):
                ladder[finals].append(row)
        match = LANE.search(line)
        if match:
            if finals >= 2:
                raise ValueError(f"{path}:{number}: lane row after two final boundaries")
            row = fields(match.group(1))
            key = (row.get("stage"), row.get("class"))
            if row.get("rank") != str(expected_rank):
                raise ValueError(f"{path}:{number}: lane rank {row.get('rank')} != {expected_rank}")
            if key[0] not in STAGES or key[1] not in CLASSES:
                raise ValueError(f"{path}:{number}: unknown stage/class {key}")
            if key in events[finals]:
                raise ValueError(f"{path}:{number}: duplicate event {finals + 1} row {key}")
            events[finals][key] = validate_row(row, path, number)
            continue
        summary = FINAL.search(line)
        if summary:
            info = fields(summary.group(1))
            if info.get("root") == "remesh" and info.get("rank") == str(expected_rank):
                if info.get("boundary") != "final":
                    continue
                if finals >= 2:
                    raise ValueError(f"{path}:{number}: more than two remesh final boundaries")
                require_complete(events[finals], path, finals + 1)
                finals += 1
    if finals != 2:
        raise ValueError(f"{path}: expected two remesh final boundaries, found {finals}")
    return {"lane": events, "quality": quality, "ladder": ladder}


def validate_row(row, path, number):
    missing = set(FIELDS) - row.keys()
    if missing:
        raise ValueError(f"{path}:{number}: missing fields {sorted(missing)}")
    clean = {}
    for name in INTS:
        try:
            value = int(row[name])
        except ValueError as exc:
            raise ValueError(f"{path}:{number}: {name} must be an integer") from exc
        if value < 0:
            raise ValueError(f"{path}:{number}: {name} must be nonnegative")
        clean[name] = value
    try:
        seconds = float(row["seconds"])
    except ValueError as exc:
        raise ValueError(f"{path}:{number}: seconds must be numeric") from exc
    if not math.isfinite(seconds) or seconds < 0:
        raise ValueError(f"{path}:{number}: seconds must be finite and nonnegative")
    clean["seconds"] = seconds
    for name in ("same_skin", "no_parameter_edits", "no_coverage"):
        if clean[name] > clean["commits"]:
            raise ValueError(f"{path}:{number}: {name} exceeds commits")
    return clean


def require_complete(rows, path, event):
    wanted = {(stage, cls) for stage in STAGES for cls in CLASSES}
    if rows.keys() != wanted:
        missing, extra = sorted(wanted - rows.keys()), sorted(rows.keys() - wanted)
        raise ValueError(f"{path}: event {event}: missing={missing}, extra={extra}")


def collect(directory):
    files = [directory / "rank_output" / "1" / f"rank.{rank}" / "stdout" for rank in range(6)]
    absent = [str(path) for path in files if not path.is_file()]
    if absent:
        raise ValueError("missing rank logs: " + ", ".join(absent))
    ranks = [parse_rank(path, rank) for rank, path in enumerate(files)]
    result = []
    for event in range(2):
        rows = []
        for rank, pair in enumerate(ranks):
            lane = pair["lane"][event]
            require_complete(lane, files[rank], event + 1)
            for stage in STAGES:
                for cls in CLASSES:
                    rows.append(dict(event=event + 1, rank=rank, stage=stage, **{"class": cls},
                                     **lane[(stage, cls)]))
        legacy = [legacy_comparison(rank, ranks[rank], event, files[rank]) for rank in range(6)]
        result.append({"rows": rows, "legacy": legacy})
    return result


def legacy_comparison(rank, parsed, event, path):
    quality = parsed["quality"][event]
    ladder = parsed["ladder"][event]
    exit_rows = [row for row in quality if row.get("stage") == "exit" and row.get("complete") == "1"]
    if not exit_rows:
        raise ValueError(f"{path}: event {event + 1}: missing complete NATIVE3D_QUALITY stage=exit row")
    try:
        final_commits = int(exit_rows[-1]["commits_total"])
    except (KeyError, ValueError) as exc:
        raise ValueError(f"{path}: event {event + 1}: invalid final quality commits_total") from exc
    lane_quality = sum(row["commits"] for (stage, _), row in parsed["lane"][event].items()
                       if stage == "quality")
    ladder_totals = {"trials": 0, "optimizer_work": 0, "rows": len(ladder)}
    ladder_details = []
    for row in ladder:
        try:
            trials, work = int(row["trials"]), int(row["optimizer_work"])
            if trials < 0 or work < 0:
                raise ValueError("negative ladder counter")
            ladder_totals["trials"] += trials
            ladder_totals["optimizer_work"] += work
            ladder_details.append({"census": int(row["census"]), "kind": int(row["kind"]),
                                   "trials": trials, "optimizer_work": work})
        except (KeyError, ValueError) as exc:
            raise ValueError(f"{path}: event {event + 1}: invalid quality ladder row {row}") from exc
    lane_trials = sum(row["trials"] for (stage, _), row in parsed["lane"][event].items() if stage == "quality")
    lane_work = sum(row["work"] for (stage, _), row in parsed["lane"][event].items() if stage == "quality")
    return {"rank": rank, "final_quality_commits_total": final_commits,
            "lane_quality_commits": lane_quality, "commits_equal": final_commits == lane_quality,
            "quality_ladder": ladder_totals, "quality_ladder_rows": ladder_details,
            "lane_quality_trials": lane_trials,
            "lane_quality_work": lane_work,
            "optimizer_work_equal": ladder_totals["optimizer_work"] == lane_work,
            "trials_equal": ladder_totals["trials"] == lane_trials,
            "trials_note": "reported only; ladder and lane trials are different populations"}


def summarize(rows):
    result = {}
    for stage in (*STAGES, "combined"):
        subset = [row for row in rows if stage == "combined" or row["stage"] == stage]
        total_seconds = sum(row["seconds"] for row in subset)
        total_commits = sum(row["commits"] for row in subset)
        local = [row for row in subset if row["class"] == "local_interior"]
        local_seconds = sum(row["seconds"] for row in local)
        local_commits = sum(row["commits"] for row in local)
        result[stage] = {
            "local_interior_seconds": local_seconds, "all_seconds": total_seconds,
            "local_interior_seconds_share": local_seconds / total_seconds if total_seconds else None,
            "local_interior_commits": local_commits, "all_commits": total_commits,
            "local_interior_commits_share": local_commits / total_commits if total_commits else None,
        }
    return result


def render_markdown(events):
    lines = ["# N12 lane census", "",
             "Each table has six ranks × two stages × four classes. Shares count `local_interior`.",
             "Legacy quality commits and optimizer work are compared where scopes align; trial populations are shown side by side.", ""]
    for event, event_data in enumerate(events, 1):
        rows = event_data["rows"]
        lines += [f"## Event {event}", "", "| rank | stage | class | trials | approved | commits | seconds | work | same_skin | no_parameter_edits | no_coverage |",
                  "|---:|---|---|---:|---:|---:|---:|---:|---:|---:|---:|"]
        for row in rows:
            lines.append("| " + " | ".join(str(row[key]) for key in
                         ("rank", "stage", "class", "trials", "approved", "commits", "seconds", "work",
                          "same_skin", "no_parameter_edits", "no_coverage")) + " |")
        lines += ["", "| stage | local interior busy seconds | share | local interior commits | share |",
                  "|---|---:|---:|---:|---:|"]
        for stage, values in summarize(rows).items():
            lines.append(f"| {stage} | {values['local_interior_seconds']:.9g} / {values['all_seconds']:.9g} | "
                         f"{format_share(values['local_interior_seconds_share'])} | "
                         f"{values['local_interior_commits']} / {values['all_commits']} | "
                         f"{format_share(values['local_interior_commits_share'])} |")
        lines += ["", "### Legacy quality counter reconciliation", "",
                  "Commit and optimizer-work differences are report observations, not gates. "
                  "Construction has no corresponding ladder counter.", "",
                  "| rank | final quality commits | lane quality commits | equal | ladder trials | lane trials (different population) | ladder optimizer work | lane work | work equal |",
                  "|---:|---:|---:|---|---:|---:|---:|---:|---|"]
        for row in event_data["legacy"]:
            lines.append(f"| {row['rank']} | {row['final_quality_commits_total']} | {row['lane_quality_commits']} | "
                         f"{row['commits_equal']} | {row['quality_ladder']['trials']} | {row['lane_quality_trials']} | "
                         f"{row['quality_ladder']['optimizer_work']} | {row['lane_quality_work']} | "
                         f"{row['optimizer_work_equal']} |")
            lines.append(f"\nRank {row['rank']} ladder census/kind rows: `{json.dumps(row['quality_ladder_rows'], separators=(',', ':'))}`")
        lines.append("")
    return "\n".join(lines)


def format_share(value):
    return "n/a" if value is None else f"{100 * value:.4f}%"


def write_reports(events, prefix):
    rows = [row for event in events for row in event["rows"]]
    payload = {"scope": "N12 lane census; event blocks paired to each rank's next remesh final timer summary",
               "legacy_counter_scope": "Quality lane commits/work compared with final NATIVE3D_QUALITY commits_total and summed NATIVE3D_QUALITY_LADDER optimizer_work. Mismatches are reported, not gated. Quality trials are shown side by side because their populations differ. Construction has no corresponding ladder counter.",
               "events": [{"event": index + 1, "rows": event["rows"], "shares": summarize(event["rows"]),
                           "legacy_quality": event["legacy"]} for index, event in enumerate(events)]}
    prefix.with_suffix(".json").write_text(json.dumps(payload, indent=2) + "\n")
    with prefix.with_suffix(".csv").open("w", newline="") as stream:
        writer = csv.DictWriter(stream, fieldnames=("event", "rank", "stage", "class", *FIELDS))
        writer.writeheader()
        writer.writerows(rows)
    prefix.with_suffix(".md").write_text(render_markdown(events))


def self_check():
    lane = "NATIVE3D_LANE_CENSUS rank=0 stage={} class={} trials=2 approved=1 commits=1 seconds=0.5 work=7 same_skin=1 no_parameter_edits=0 no_coverage=1"
    final = "NATIVE3D_TIMER_SUMMARY root=remesh boundary=final rank=0 ranks=6 root_seconds=1 sumexclusive=1 unattributed=0 coverage=1 fixedstorage=4120"
    quality = "NATIVE3D_QUALITY rank=0 stage=exit census={} complete=1 commits_total=4"
    ladder = "NATIVE3D_QUALITY_LADDER rank=0 census={} kind={} trials=9 optimizer_work={}"
    lines = []
    for event in range(2):
        lines.append(quality.format(event))
        lines.extend(ladder.format(event, kind, 28 if kind == 0 else 0) for kind in range(3))
        lines.extend(lane.format(stage, cls) for stage in STAGES for cls in CLASSES)
        lines.append(final)
    with tempfile.TemporaryDirectory() as temp:
        run = Path(temp)
        for rank in range(6):
            rank_lines = [line.replace("rank=0", f"rank={rank}") for line in lines]
            path_for_rank = run / "rank_output" / "1" / f"rank.{rank}" / "stdout"
            path_for_rank.parent.mkdir(parents=True)
            path_for_rank.write_text("\n".join(rank_lines) + "\n")
        events_all = collect(run)
        assert [len(event["rows"]) for event in events_all] == [48, 48]
        assert summarize(events_all[0]["rows"])["combined"]["local_interior_seconds_share"] == 0.25
        legacy0 = events_all[0]["legacy"][0]
        assert legacy0["commits_equal"] and legacy0["optimizer_work_equal"]
        assert not legacy0["trials_equal"] and legacy0["quality_ladder"]["trials"] == 27
        path = run / "rank_output" / "1" / "rank.0" / "stdout"
        lane_lines = [line for line in lines if LANE.search(line)]
        check_lines = lane_lines[:8] + [final] + lane_lines[8:] + [final]
        path.write_text("\n".join(check_lines) + "\n")
        parsed = parse_rank(path, 0)
        assert [len(event) for event in parsed["lane"]] == [8, 8]
        one = dict(parsed["lane"][0][("construction", "local_interior")], event=1, rank=0,
                   stage="construction", **{"class": "local_interior"})
        assert summarize([one])["construction"]["local_interior_commits"] == 1
        path.write_text("\n".join([lane_lines[0]] + lane_lines[:8] + [final] + lane_lines[8:] + [final]) + "\n")
        expect_error(path, "duplicate")
        path.write_text("\n".join(lane_lines[:7] + [final] + lane_lines[8:] + [final]) + "\n")
        expect_error(path, "missing")
        bad = check_lines.copy()
        bad[0] = bad[0].replace("trials=2", "trials=-1")
        path.write_text("\n".join(bad) + "\n")
        expect_error(path, "nonnegative")
        bad = check_lines.copy()
        bad[0] = bad[0].replace("seconds=0.5", "seconds=nan")
        path.write_text("\n".join(bad) + "\n")
        expect_error(path, "finite")
        bad = check_lines.copy()
        bad[0] = bad[0].replace("same_skin=1", "same_skin=2")
        path.write_text("\n".join(bad) + "\n")
        expect_error(path, "exceeds commits")
    assert strip_mpi_prefixes("NATIVE3D_LANE_CENSUS rank=0") == "NATIVE3D_LANE_CENSUS rank=0"


def expect_error(path, text):
    try:
        parse_rank(path, 0)
    except ValueError as exc:
        assert text in str(exc), str(exc)
    else:
        raise AssertionError(f"{text} case accepted")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("input", type=Path, nargs="?", help="full workflow output directory")
    parser.add_argument("--out", type=Path, help="output path prefix (JSON, CSV, Markdown)")
    parser.add_argument("--self-check", action="store_true")
    args = parser.parse_args()
    if args.self_check:
        self_check()
        print("self-check passed: synthetic two-event shape; missing, duplicate, negative, and non-finite cases rejected")
        return
    if args.input is None or args.out is None:
        parser.error("input directory and --out prefix are required")
    events = collect(args.input)
    write_reports(events, args.out)
    print(f"wrote {args.out.with_suffix('.json')}, .csv, .md")


if __name__ == "__main__":
    main()
