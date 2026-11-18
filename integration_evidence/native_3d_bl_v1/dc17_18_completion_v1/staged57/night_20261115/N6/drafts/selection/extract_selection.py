"""Prepare a scoped Selection patch from the accepted pre-N6 sources."""
from collections import Counter
from difflib import unified_diff
from hashlib import sha256
import json
from pathlib import Path
import re
import subprocess
from textwrap import indent

DRAFT = Path(__file__).resolve().parent
REPO = next(parent for parent in DRAFT.parents if (parent / ".clang-format").is_file())
BASE = "3276cbb862"
SOURCE = Path("Common/src/adaptation")
PRIVATE = SOURCE / "native_engine3d"
UNITY = SOURCE / "CNativeEngine3D.cpp"
CENSUS = set("""
invalid construction_invalid quality_invalid unmeasured_shape debt progress_consistent census_started
cells edges vertex_cells vertex_links
""".split())
QUEUES = set("""
queue surface_queue geometry_queue geometry_split_priorities geometry_priority_mode construction_frontier
edge_history blocked_edges blocked_links cell_repairs repair_epoch repair_users repair_queue
repair_cell_severity repair_severity repair_severity_queue repair_zero repair_bytes split_lanes lane
""".split())
OWNERS = {name: "census" for name in CENSUS} | {name: "queues" for name in QUEUES}
CENSUS_METHODS = set("""
Account RequestedWall SurfaceCensus Progress Diagnose Cached Drop ChangePhase ClassifyFeatures
RefreshQuality Add Restore SwapState Reset Bytes CollectCellDebt MeasureGeometryFace
AccumulateGeometryScores MeasureGeometryDebt AddCellEdges
""".split())
COMMENTS = {
    "GeometryPriority": "Uses the predicted improvement when available, otherwise keeps the raw ordering rule.",
    "GeometryPriorityBytes": "Accounts for one priority-map entry per eligible geometry edge.",
    "PrioritizeGeometrySplits": "Refreshes geometry priorities in queue order within the existing time limit.",
    "Note": "Records the latest disposition for an edge already tracked in the generation.",
    "RefreshHistory": "Retires resolved lengths and ages each remaining demand once per generation.",
    "Priority": "Orders the length queue by short or long edges for the current direction.",
    "Candidate": "Applies the current length gate to one cached edge measurement.",
    "Queue": "Refreshes both directed queue entries without reopening a rejected edge.",
    "Bytes": "Accounts for every live census and queue allocation admitted by the engine.",
    "RepairWeight": "Ranks local required debt before the complete distributed trial certifies it.",
    "AddRepairSeverity": "Adds one cell contribution and reinserts the live feature priority.",
    "RemoveRepairSeverity": "Releases one cell contribution after removing its old priority key.",
    "UpdateRepairWeight": "Reweights the repair features of an already cached cell.",
    "Account": "Applies one cached debt contribution in the same order for entry and retirement.",
    "RequestedWall": "Identifies an owned cell carrying a physical face with a requested wall height.",
    "SurfaceCensus": "Counts only required wall cells with current cached measurements.",
    "Progress": "Reports census completeness and queue sizes from the current published state.",
    "Diagnose": "Exports the matching cell version, edge measurements and active repair history.",
    "Best": "Selects the current first repair, geometry or length candidate.",
    "RepairBatch": "Selects the oldest repair, then severe candidates with disjoint local stars.",
    "CollectRepairStar": "Collects the owned cells and vertices of a feature under the existing scan bounds.",
    "Batch": "Selects local-disjoint length candidates without replacing complete-star reservation.",
    "Cached": "Reuses certificates for canonical edges present in the current census.",
    "Block": "Retires a rejected feature or records its directed edge for later reactivation.",
    "Forget": "Removes one retired edge direction and its blocked-link accounting.",
    "TouchRepairSupport": "Reopens a rejected feature when its complete support may have changed.",
    "Touch": "Reactivates blocked edges incident to a changed vertex when still eligible.",
    "FilterCollapse": "Applies existing local geometry and orientation screens before a collapse trial.",
    "Drop": "Removes a published cell and retires its census and queue contributions in order.",
    "RemoveRepairs": "Drops one cell's demands while retaining zero-user history through publication.",
    "UpdateRepairs": "Brings one cached cell's demand into line with the active repair requirements.",
    "ChangePhase": "Retargets the versioned census and rebuilds queues in the original phase order.",
    "EnableSizeRepairs": "Updates existing cells for the residual size repair stage.",
    "EnableGeometryRepairs": "Updates existing cells for the residual geometry repair stage.",
    "ClassifyFeatures": "Records whether a measured cell touches a known lower-dimensional feature.",
    "RefreshQuality": "Replaces shape and height debt without changing the immutable edge certificate.",
    "EnableCollapseFilter": "Requeues measured lengths under the active collapse screen.",
    "LeavesFacet": "Checks whether a candidate leaves a fixed source facet.",
    "Forbidden": "Records both locally forbidden collapse directions of an admitted edge.",
    "Add": "Publishes a measured cell into the census and updates its queue contributions.",
    "CollectCellDebt": "Measures shape, size and height debt before publication to the indexes.",
    "MeasureGeometryFace": "Measures one physical face and records its contribution to geometry debt.",
    "AccumulateGeometryScores": "Adds the face-weighted geometry excess to the same canonical edge keys.",
    "MeasureGeometryDebt": "Measures physical faces in slot order before the cell is indexed.",
    "AddCellEdges": "Updates the versioned edge census and queue in metric-edge order.",
    "AddRepairs": "Seeds one cell's required repair features and accounts for the retained capacity.",
    "Service": "Updates the age and availability of a serviced repair feature.",
    "FinishRepairs": "Releases zero-user demands only after publication finishes.",
    "HasCoarseningDemand": "Searches measured short edges for at least one unblocked direction.",
    "SeedFrontier": "Snapshots original length directions for the next construction generation.",
    "ServiceEdge": "Removes a served direction and all corresponding live queue entries.",
    "ServiceClaimedVertex": "Retires frontier edges whose support may have changed after publication.",
    "Restore": "Restores a migrated cell certificate and accounts for it without seeding the queue.",
    "RepairKeys": "Recreates the fixed vertex, edge and face order of a transported repair mask.",
    "SaveRepairState": "Stores live repair epochs and block bits in the existing migration row.",
    "RestoreRepairState": "Rebuilds feature demand and merges age and refusal from the restored census.",
    "SwapState": "Adopts the pending indexes without swapping modes, references or repair epoch.",
    "Reset": "Drops the cached census and queues while retaining directed rejection history.",
}
TRIVIA = re.compile(r'/\*.*?\*/|//[^\n]*|"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\'', re.S)
MICRO_TOKENS = re.compile(r'[A-Za-z_]\w*|(?:0[xX][0-9A-Fa-f]+|\d+(?:\.\d*)?(?:[eE][+-]?\d+)?)[A-Za-z_]*|[^\s]')
METHOD = re.compile(r"^  (?:static )?[A-Za-z][^\n;]*?\b([A-Z][A-Za-z]+)\(", re.M)


def source(path):
    return subprocess.check_output(["git", "show", f"{BASE}:{path}"], cwd=REPO, text=True)


def replace_one(text, before, after):
    assert text.count(before) == 1, before
    return text.replace(before, after, 1)


def mask(text):
    return TRIVIA.sub(lambda match: re.sub(r"[^\n]", " ", match[0]), text)


def end_of(text, first, opening, closing):
    depth = 0
    for token in re.finditer(f"[{re.escape(opening + closing)}]", text[first:]):
        depth += 1 if token[0] == opening else -1
        if depth == 0:
            return first + token.start()
    raise ValueError((opening, first))


def format_source(path, text, lines=()):
    args = ["clang-format", "--style=file", f"--assume-filename={path}"]
    args.extend(f"--lines={start}:{end}" for start, end in lines)
    return subprocess.run(args, input=text, text=True, stdout=subprocess.PIPE, cwd=REPO, check=True).stdout


def lines_in(text, first, last):
    return text.splitlines(keepends=True)[first - 1:last]


def parse_methods(text, names):
    """Find only known top-level methods and preserve their exact bodies."""
    masked = mask(text)
    previous_end = 0
    result = {}
    for match in METHOD.finditer(masked):
        name = match[1]
        if name not in names or match.start() < previous_end:
            continue
        signature_end = end_of(masked, match.end() - 1, "(", ")")
        opening = masked.index("{", signature_end)
        assert ";" not in masked[signature_end:opening], name
        closing = end_of(masked, opening, "{", "}")
        assert name not in result, name
        result[name] = (text[match.start():opening].strip(), text[opening:closing + 1])
        previous_end = closing + 1
    assert set(result) == names, (set(result), names)
    return result


def replace_tokens(text, replacements):
    """Rewrite only identifiers in code, keeping comments and string values intact."""
    masked = mask(text)
    for match in reversed(list(re.finditer(r"\b[A-Za-z_]\w*\b", masked))):
        replacement = replacements.get(match[0])
        if replacement and not masked[max(0, match.start() - 2):match.start()].endswith((".", "->", "::")):
            text = text[:match.start()] + replacement + text[match.end():]
    return text


def qualified_body(name, body):
    """Preserve cached/transported field access while grouping Selection-owned members."""
    aliases = {item: f"{owner}.{item}" for item, owner in OWNERS.items()}
    if name == "Batch":
        # This is a star-local counter, not the census cell map.
        aliases["cells"] = "star_cell_count"
    body = replace_tokens(body, aliases)
    if name == "SwapState":
        for item, owner in OWNERS.items():
            body = re.sub(rf"\bother\.{item}\b", f"other.{owner}.{item}", body)
    return body


def token_stream(code):
    """Compare code tokens ignoring formatting and comments, while preserving literals."""
    parts = re.split(f"({TRIVIA.pattern})", code, flags=re.S)
    for part in parts:
        if part[:2] in ("//", "/*"):
            continue
        if part.startswith(('"', "'")):
            yield part
        else:
            yield from MICRO_TOKENS.findall(part)


def add_helpers(defs):
    """Move Add's contiguous stages without changing indexing, timers, or statement order."""
    signature, body = defs["Add"]
    debt_start = body.index("    row.construction_valid=metric.maximum_length")
    geometry_start = body.index("    std::map<Edge,long double> geometry_scores;")
    indexed_start = body.index("    for (size_t i=0;i<4;++i)")
    edges_start = body.index("    size_t slot=0;", indexed_start)
    close_cell = body.index("    row.construction_valid=row.construction_valid")
    debt = body[debt_start:geometry_start]
    geometry = body[geometry_start:indexed_start]
    geometry_loop = geometry.index("      for(size_t slot")
    face_start = geometry.index("        if(surface_planning")
    scores_start = geometry.index("        size_t face_edge = 0;")
    face_end = geometry.index("      }\n    }\n    if(!d.geometry_measured)", scores_start)
    defargs = {
        "CollectCellDebt": ("void CollectCellDebt(const OwnedCell& owned, const PatchMetric& metric, "
                            "const FirstHeightReport* height, bool height_valid, CellState& row) const",
                            "{\n    const auto& cell=owned.cell;\n" + debt + "  }"),
        "MeasureGeometryDebt": (
            "void MeasureGeometryDebt(const OwnedCell& owned, const BoundaryParameterState* parameters, "
            "CellState& row, std::map<Edge,long double>& geometry_scores)",
            "{\n    const auto& cell=owned.cell;\n    auto& d=row.debt;\n" +
            geometry[geometry.index("    if (!geometry.ExtensionComponents()"):face_start] +
            "        MeasureGeometryFace(owned, faces[slot], slot, parameters, row, geometry_scores);\n" +
            geometry[face_end:] + "  }"),
        "MeasureGeometryFace": (
            "void MeasureGeometryFace(const OwnedCell& owned, const Face& face, size_t slot, "
            "const BoundaryParameterState* parameters, CellState& row, "
            "std::map<Edge,long double>& geometry_scores)",
            "{\n    const auto& cell=owned.cell;\n    auto& d=row.debt;\n"
            "    auto hinge=[](long double value){return std::max(0.L,value);};\n" +
            geometry[face_start:scores_start].replace("continue;", "return;").replace("faces[slot]", "face") +
            "    AccumulateGeometryScores(face, triangle, error, allowed, law_weights, geometry_scores);\n  }"),
        "AccumulateGeometryScores": (
            "void AccumulateGeometryScores(const Face& face, const std::array<Point,3>& triangle, "
            "double error, double allowed, const WallFaceWeights& law_weights, "
            "std::map<Edge,long double>& geometry_scores) const",
            "{\n" + geometry[scores_start:face_end].replace("faces[slot]", "face") + "  }"),
        "AddCellEdges": ("void AddCellEdges(const OwnedCell& owned, const PatchMetric& metric, "
                         "std::map<Edge,long double>& geometry_scores, CellState& row)",
                         "{\n    const auto& cell=owned.cell;\n" + body[edges_start:close_cell] + "  }"),
    }
    assert geometry_loop < face_start
    defs["Add"] = (signature, body[:debt_start] +
        "    CollectCellDebt(owned, metric, height, height_valid, row);\n"
        "    std::map<Edge,long double> geometry_scores;\n"
        "    MeasureGeometryDebt(owned, parameters, row, geometry_scores);\n" +
        body[indexed_start:edges_start] +
        "    AddCellEdges(owned, metric, geometry_scores, row);\n" + body[close_cell:])
    defs.update(defargs)


def repair_helper(defs):
    """Name the existing complete-star walk without changing either caller."""
    signature, body = defs["RepairBatch"]
    masked = mask(body)
    lambda_start = body.index("    auto feature_cells")
    opening = masked.index("{", lambda_start)
    closing = end_of(masked, opening, "{", "}")
    assert body[closing + 1] == ";"
    defs["CollectRepairStar"] = (
        "void CollectRepairStar(const Owned& working, const RepairFeature& key, "
        "std::vector<Id>& ids, std::set<Id>& vertices, bool& too_large) const",
        body[opening:closing + 1],
    )
    body = body[:lambda_start] + body[closing + 3:]
    assert body.count("feature_cells(") == 2
    defs["RepairBatch"] = signature, body.replace("feature_cells(", "CollectRepairStar(working,")


def make_new_files(defs, unity):
    """Build the facade declaration and both implementation fragments."""
    header = """// Owns the versioned cell census and its construction and repair queues in one lifetime.
#pragma once

// Shares the published selection state between measurement and candidate scheduling.
class Selection {
 public:
"""
    header += "".join(lines_in(unity, 431, 431))
    header += "  // Starts with empty indexes that borrow the event's geometry, policies and statistics.\n"
    header += "".join(lines_in(unity, 434, 447))
    header += "\n" + "".join(lines_in(unity, 448, 457)) + "\n"
    parts = (
        ("Record of the latest disposition and generation age of a tracked edge.", 494, 494),
        ("Owns the reusable length certificate and incidence counts for a canonical edge.", 533, 533),
        ("Stores separately measured validity and quality debt without changing its wire encoding.", 534, 542),
        ("Binds one certified cell version to its edge measurements and diagnostic values.", 545, 554),
        ("Carries the cell certificate and required-feature history in ownership migration.", 555, 559),
        ("Keeps the shared age and availability of one required feature.", 566, 566),
    )
    for comment, first, last in parts:
        block = "".join(lines_in(unity, first, last))
        block = re.sub(r"(template<class S>\s*void Fields)", r"// Serializes fields in the existing wire order.\n    \1", block)
        header += f"  // {comment}\n{block}\n"
    header += """  // Counts live cache rows and keeps the topology that indexes their certificates.
  struct CensusState {
    size_t invalid = 0, construction_invalid = 0, quality_invalid = 0, unmeasured_shape = 0;
    StageProgress debt;
    bool progress_consistent = true, census_started = false;
    std::map<Id, CellState> cells;
    std::map<Edge, EdgeState> edges;
    std::map<Id, std::set<Id>> vertex_cells;
    size_t vertex_links = 0;
  } census;

  // Keeps ordered construction and repair candidates, blocked directions, and waiting age.
  struct QueueState {
    bool split_lanes = false;
    Lane lane = Lane::Surface;
    std::set<std::tuple<long double, Id, Edge>> geometry_queue;
    std::map<Edge, long double> geometry_split_priorities;
    bool geometry_priority_mode = false;
    std::set<std::pair<long double, Edge>> surface_queue;
    // A generation contains its entry edges even while their measurements are seeded.
    // Publishing new cells cannot extend it.
    std::set<Edge> construction_frontier;
    std::map<Edge, History> edge_history;
    std::set<std::pair<long double, Edge>> queue;
    std::map<Id, std::set<Id>> blocked_edges;
    size_t blocked_links = 0;
    std::map<Id, std::vector<RepairFeature>> cell_repairs;
    uint64_t repair_epoch = 0;
    std::map<RepairFeature, RepairDemand> repair_users;
    std::set<std::pair<uint64_t, RepairFeature>> repair_queue;
    std::map<Id, long double> repair_cell_severity;
    std::map<RepairFeature, long double> repair_severity;
    std::set<std::pair<long double, RepairFeature>> repair_severity_queue;
    std::set<RepairFeature> repair_zero;
    size_t repair_bytes = 0;
  } queues;

"""
    census_path = PRIVATE / "EngineSelectionCensus.cpp"
    queue_path = PRIVATE / "EngineSelectionQueues.cpp"
    bodies = {
        census_path: "// Manages versioned cell and edge certificates, completeness and debt.\n",
        queue_path: "// Maintains the ordered construction, geometry and repair frontiers.\n",
    }
    for name, (signature, body) in defs.items():
        header += indent(signature + ";", "  ") + "\n"
        signature = re.sub(r"\s*=\s*(?:nullptr|true|false)\b", "", signature.removeprefix("static "))
        signature = replace_one(signature, name + "(", "Selection::" + name + "(")
        path = census_path if name in CENSUS_METHODS else queue_path
        bodies[path] += f"\n// {COMMENTS[name]}\n{signature} {qualified_body(name, body)}\n"
    bodies[PRIVATE / "EngineSelection.hpp"] = header + "};\n"
    return {path: format_source(path, text) for path, text in bodies.items()}


def rewrite_callers(text):
    """Qualify only explicit accesses through the existing local Selection handles."""
    return re.sub(
        r"\b((?:pending_)?selection)(\.|->)([A-Za-z_]\w*)\b",
        lambda match: f"{match[1]}{match[2]}{OWNERS[match[3]]}.{match[3]}" if match[3] in OWNERS else match[0],
        text,
    )


def audit_bodies(old, new):
    """Verify even the expanded type member functions and all unchanged method bodies."""
    formatted = "\n".join(text for path, text in new.items() if path.suffix == ".cpp")
    new_defs = parse_methods(
        re.sub(r"(?m)^(?!\s|/|$)(.*?Selection::)", r"  \1", formatted).replace("Selection::", ""),
        set(COMMENTS),
    )
    equal = []
    lengths = {}
    lambdas = {}
    for name, (_, body) in new_defs.items():
        lengths[name] = max(1, body.count("\n") - 1)
        code = mask(body)
        for index, match in enumerate(re.finditer(r"\[(?:&|=)?\]\s*(?:\([^)]*\)\s*)?\{", code), 1):
            end = end_of(code, match.end() - 1, "{", "}")
            lambdas[f"{name}:{index}"] = max(1, code.count("\n", match.end(), end) - 1)
        if name in ("Add", "RepairBatch") or name not in old:
            continue
        before = old[name][1]
        if name == "Batch":
            before = replace_tokens(before, {"cells": "star_cell_count"})
        normalized = re.sub(r"\b(?:census|queues)\.(?=\w)", "", body)
        assert list(token_stream(before)) == list(token_stream(normalized)), name
        equal.append(name)
    for type_name in ("CellDebt", "CellState", "CachedRow", "EdgeState", "RepairDemand", "History"):
        def block(text):
            code = mask(text)
            start = code.index(f"struct {type_name} ")
            opening = code.index("{", start)
            return list(token_stream(text[start:end_of(code, opening, "{", "}") + 1]))
        assert block(original_selection) == block(new[PRIVATE / "EngineSelection.hpp"]), type_name
    assert max(lengths.values()) <= 80, lengths
    assert max(lambdas.values()) <= 15, lambdas
    return equal, lengths, lambdas


def git_patch(old_files, changed):
    """Keep MeshOptimizer and other independent regions outside every diff hunk."""
    patch = []
    for path, after in sorted(changed.items()):
        before = old_files.get(path)
        patch.append(f"diff --git a/{path} b/{path}\n")
        if before is None:
            patch.append("new file mode 100644\n")
        patch.extend(unified_diff((before or "").splitlines(keepends=True), after.splitlines(keepends=True),
                                  fromfile=f"a/{path}" if before is not None else "/dev/null",
                                  tofile=f"b/{path}", n=3))
    return "".join(patch)


def main():
    names = set(COMMENTS) - {"CollectCellDebt", "MeasureGeometryFace", "AccumulateGeometryScores",
                            "MeasureGeometryDebt", "AddCellEdges", "CollectRepairStar"}
    definitions = parse_methods(original_selection, names)
    original_methods = definitions.copy()
    add_helpers(definitions)
    repair_helper(definitions)
    new_files = make_new_files(definitions, unity)
    equal, lengths, lambdas = audit_bodies(original_methods, new_files)
    prefix = source(UNITY)
    target = replace_one(prefix, original_selection,
        '#include "native_engine3d/EngineSelection.hpp"\n'
        '#include "native_engine3d/EngineSelectionCensus.cpp"\n'
        '#include "native_engine3d/EngineSelectionQueues.cpp"')
    old_files = {UNITY: prefix}
    changed = {UNITY: rewrite_callers(target)}
    search = subprocess.check_output(
        ["git", "grep", "-l", "-E", r"\b(selection|pending_selection)(\.|->)",
         BASE, "--", str(PRIVATE)], cwd=REPO, text=True).splitlines()
    rewritten_fields = Counter()
    for hit in search:
        path = Path(hit.split(":", 1)[1])
        before = source(path)
        after = rewrite_callers(before)
        if before != after:
            changed_lines = [(line, line) for line, (left, right) in
                             enumerate(zip(before.splitlines(), after.splitlines()), 1) if left != right]
            after = format_source(path, after, changed_lines)
            restored = re.sub(r"\b((?:pending_)?selection)(\.|->)(?:census|queues)\.", r"\1\2", after)
            assert list(token_stream(restored)) == list(token_stream(before)), path
            old_files[path] = before
            changed[path] = after
        rewritten_fields.update(re.findall(r"\b(?:pending_)?selection(?:\.|->)(?:census|queues)\.(\w+)", after))
    changed.update(new_files)
    assert prefix[prefix.index("class MeshOptimizer {"):] ==            changed[UNITY][changed[UNITY].index("class MeshOptimizer {"):]
    patch = git_patch(old_files, changed)
    (DRAFT / "selection.patch").write_text(patch)
    for path, text in new_files.items():
        (DRAFT / path.name).write_text(text)
    (DRAFT / "mechanical_checks.json").write_text(json.dumps({
        "base": BASE,
        "untouched_mesh_optimizer_tail": True,
        "body_tokens_equal_after_unqualifying_state_members": equal,
        "extracted_bodies": ["Add", "RepairBatch"],
        "transport_and_modeled_type_tokens_equal": ["CellDebt", "CellState", "CachedRow", "EdgeState",
                                                    "RepairDemand", "History"],
        "emitted_file_sha256": {str(p): sha256(text.encode()).hexdigest() for p, text in changed.items()},
        "mutated_caller_paths": sorted(str(p) for p in old_files),
        "caller_field_access_counts": rewritten_fields,
        "function_body_lines": lengths,
        "lambda_body_lines": lambdas,
        "ten_longest": sorted(lengths.items(), key=lambda item: (-item[1], item[0]))[:10],
    }, indent=2) + "\n")


unity = source(UNITY)
start = unity.index("class Selection {")
end = unity.index("\nstruct BlockedEdge {", start)
original_selection = unity[start:end].rstrip("\n")
if __name__ == "__main__":
    main()
