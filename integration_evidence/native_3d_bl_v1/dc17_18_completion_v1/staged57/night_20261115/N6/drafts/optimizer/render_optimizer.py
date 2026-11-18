"""Render the N6 optimizer source draft from its pinned accepted base."""
from hashlib import sha256
from pathlib import Path
import json
import re
import subprocess

DRAFT = Path(__file__).resolve().parent
SOURCE = Path("Common/src/adaptation/CNativeEngine3D.cpp")
MOVED = SOURCE.parent / "native_engine3d/MeshOptimizer.cpp"
BASE = "3276cbb8623fab80bf97976985bd612a1e365282"
BASE_SHA = "cb5cd284df9542329f32acec6d90029e561fddbaea2a0097d92889f0aa112039"
base = subprocess.check_output(["git", "show", f"{BASE}:{SOURCE}"])
assert sha256(base).hexdigest() == BASE_SHA
lines = base.decode().splitlines(keepends=True)


def part(first, last):
    return "".join(lines[first - 1 : last])


def replace_once(source, old, new):
    assert source.count(old) == 1, old
    return source.replace(old, new)


def method(signature, comment, body):
    return f"// {comment}\n{signature} {{\n{body}\n}}\n"


def member(result, sig, comment, body):
    return method(f"{result} MeshOptimizer::{sig}", comment, body)


extra = """
  struct SmoothRejections {
    size_t validity, shape, edge, geometry;
  };
  struct SmoothTrial {
    long double score;
    Point x;
  };
  enum class BoundaryDecision { Next, Admit, Stop, Fail };

  // Keeps the original atlas accounting and borrowed geometry alive for the whole star search.
  struct BoundaryMoveResources {
    SU2NativeGeometry::AtlasBudget* atlas_budget;
    struct CallerByteRestore {
      SU2NativeGeometry::AtlasBudget* budget;
      size_t original;
      ~CallerByteRestore() { if (budget) budget->caller_bytes = original; }
    } caller_restore;
    GeometryGuidanceMemo guidance_memo;
    bool guidance_memo_ready = false;
    std::unique_ptr<PreparedVertexMove> prepared_move;
    bool preparation_attempted = false;
    std::unique_ptr<PlanarReference> projected_context;
    const PlanarReference* projected_geometry;
    SU2NativeGeometry::CurvedAtlas prepared_atlas;
    bool prepared_atlas_ok = false;

    explicit BoundaryMoveResources(const PlanarReference& geometry)
        : atlas_budget(geometry.AtlasReservation()),
          caller_restore{atlas_budget, atlas_budget ? atlas_budget->caller_bytes : 0},
          projected_geometry(&geometry) {}
    bool Charge(const PlanarReference* candidate_context = nullptr);
  };
  struct BoundarySmoothSearch {
    Id p;
    const Patch& patch;
    const PlanarReference& geometry;
    bool debt;
    long double best_score;
    long double geometric_before;
    bool geometry_measured;
  };

  long double ShapeDebt(const std::vector<Cell>& cells);
  bool CollectBoundarySmoothPatch(Id p, std::vector<Id>& old, Patch& patch, std::set<Face>& old_physical);
  bool BoundaryTargets(Id p, const Patch& patch, const PlanarReference& geometry, Point origin,
                       std::vector<Point>& targets, std::set<Id>& neighbors);
  bool FallbackBoundaryTargets(Point origin, const std::set<Id>& neighbors, std::vector<Point>& targets);
  bool PrepareBoundaryGeometry(Id p, const Patch& patch, const PlanarReference& geometry,
                               BoundaryMoveResources& resources);
  bool BoundaryProjection(BoundarySmoothSearch& search, Point target_point, BoundaryMoveResources& resources,
                          std::string& reason);
  BoundaryDecision BuildBoundaryMove(BoundarySmoothSearch& search, Point target_point, BoundaryMoveResources& resources,
                                     Patch& candidate, std::string& reason);
  BoundaryDecision BoundaryGuidance(BoundarySmoothSearch& search, const Patch& candidate,
                                    const PlanarReference& candidate_geometry, const PlanarReference* candidate_context,
                                    BoundaryMoveResources& resources, SU2NativeD9::SmoothChain& d9_chain);
  BoundaryDecision TryBoundaryMove(BoundarySmoothSearch& search, Point target_point, BoundaryMoveResources& resources,
                                   Patch& best, SU2NativeD9::SmoothChain& d9_chain);
  bool SearchBoundarySmooth(BoundarySmoothSearch& search, const std::vector<Id>& old, const std::vector<Point>& targets,
                            long double old_shape_debt, const SmoothRejections& baseline);
  bool CommitBoundarySmooth(BoundarySmoothSearch& search, const std::vector<Id>& old, const Patch& best,
                            long double old_shape_debt, SU2NativeD9::SmoothChain& d9_chain);

  void AddHeightTargets(Id p, const std::vector<Id>& old, Point x0, std::vector<Point>& targets);
  void AddMetricMeanTarget(Point x0, const std::set<Id>& nbr, std::vector<Point>& targets);
  void AddHingeTarget(Id p, const std::vector<Id>& old, Point x0, long double shortest, std::vector<Point>& targets);
  bool PlaceInteriorTargets(Id p, const std::vector<Id>& old, Point x0, bool slides, const std::array<double, 3>& plane,
                            bool debt, std::vector<Point>& targets);
  std::vector<SmoothTrial> CollectInteriorTrials(Id p, const std::vector<Id>& old, Point x0, bool debt,
                                                 long double oldMin, bool placed, const std::vector<Point>& targets,
                                                 Patch& constraints);
  void SlidingBindings(Id p, Id replacement, const std::vector<Id>& old, std::map<Face, FacetBinding>& added) const;
  bool CommitInteriorSmooth(Id p, const std::vector<Id>& old, const std::set<Id>& nbr, long double limit,
                            bool slides, const std::array<double, 3>& plane, bool debt, long double old_shape_debt,
                            std::vector<SmoothTrial>& trials);
  bool SearchInteriorSmooth(Id p, const std::vector<Id>& old, const std::set<Id>& nbr, Point x0, bool debt,
                            long double oldMin, long double old_shape_debt, long double shortest, long double limit,
                            const SmoothRejections& baseline);
  void RememberInteriorFailure(Id p, long double oldMin, const SmoothRejections& baseline);

  bool RemoveInteriorRing(const std::vector<Id>& shell, const std::vector<Id>& order, Node A, Node B,
                          bool lengthDriven);
  bool OrientBoundaryRing(const std::vector<BoundaryFace>& faces, const std::vector<Id>& shell,
                          std::map<Id, std::vector<Id>>& ring, std::vector<Id>& order, Id& a, Id& b, Point& pa, Point& pb);
  bool FlipBoundaryRing(const std::vector<Id>& shell, const std::vector<BoundaryFace>& faces,
                        const std::vector<Id>& order, Node A, Node B);

  bool PrepareCoupledBoundary(const std::vector<Id>& shell, Id a, Id b, Id core, const PlanarReference& geometry);
  bool CertifyConeReplacement(const std::vector<Id>& old, const std::vector<Face>& faces,
                              const std::set<Id>& vertices, const Placement& pl, Insert kind,
                              const std::pair<Id, Id>* split, long double removed, bool debt, long double oldMin);
  bool CommitShellBoundary(const std::vector<Id>& shell, const Patch& fresh, const PlanarReference& geometry,
                           const std::map<Face, FacetBinding>& bindings);
  static Patch CoupledPatch(const Owned& rows, const Patch& original, const Patch& refined);
  bool OptimizeCoupledBoundary(const std::vector<Id>& all_ids, const Patch& original, const Patch& refined,
                               Owned& pending, const PlanarReference& geometry, std::string& reason);
  std::array<size_t, 2> ShapeFailures(const std::vector<Cell>& cells);
  bool CertifyCoupledBoundary(const std::vector<Id>& all_ids, const Patch& original, const Patch& refined,
                              Owned& pending, const PlanarReference& geometry, std::string& reason, Id node, Id cell,
                              OptimizeReport& nested_report, std::set<Id>& final_keep,
                              std::map<Id, std::array<double, 3>>& final_sliding);
  bool CertifyCoupledInterface(const Patch& original, const Patch& candidate, std::set<Face>& old_physical,
                               std::map<Face, Face>& skin, std::vector<Face>& prescribed, std::string& reason);
  bool PublishCoupledBoundary(Owned& pending, Id node, Id cell, Patch& candidate,
                              std::unique_ptr<PlanarReference>& final_context, const OptimizeReport& nested_report,
                              std::set<Id>& final_keep, std::map<Id, std::array<double, 3>>& final_sliding);

  using PoorCells = std::vector<std::pair<long double, Id>>;
  void QualityCensus(size_t& bad, double& minimum);
  void RunSweeps();
  void RunSweepStage(unsigned stage, const std::array<bool, 3>& enabled, size_t& accepted);
  void ImproveSmoothing(const PoorCells& bad, size_t& accepted);
  void ImproveTopology(const PoorCells& bad, bool enrichment, size_t& accepted);
  void ImproveLongEdges(unsigned stage, size_t& accepted);
"""

# Keep support shared by PlaceConeInsertion in the same unnamed namespace.
source = part(1648, 1986) + extra + part(1987, 1987)

# Keep per-attempt storage at the same caller until the full retry body exits.
source += method(
    """template <class Triangle, class RingEdge>
void ScoreRingTriangulation(size_t n, bool debt, const std::set<std::pair<size_t, size_t>>& forbidden,
                            Triangle& triangle, RingEdge& ringEdge, std::vector<std::vector<long double>>& best,
                            std::vector<std::vector<size_t>>& arg)""",
    "Scores the current ring under its frozen diagonal constraints and tie order.",
    part(2021, 2029),
)
source += method(
    """void ReadRingTriangulation(const std::vector<std::vector<size_t>>& arg,
                           std::vector<std::array<size_t, 3>>& used, std::vector<std::pair<size_t, size_t>>& stack)""",
    "Reads the chosen cells in the original depth-first order.",
    part(2032, 2033),
)
source += part(1988, 2009) + "return RemoveInteriorRing(shell, order, {a, pa}, {b, pb}, lengthDriven);\n}\n"
source += member(
    "bool", "RemoveInteriorRing(const std::vector<Id>& shell, const std::vector<Id>& order, Node A, Node B, bool lengthDriven)",
    "Attempts the closed-ring replacements with the original length and fixed-interface rules.",
    """const size_t n = shell.size();
const Point pa = A.p, pb = B.p;
""" + part(2010, 2012) + part(2013, 2013).replace("  const Node A{a,pa},B{b,pb};", "") +
    part(2014, 2020) + "ScoreRingTriangulation(n, debt, forbidden, triangle, ringEdge, best, arg);\n" +
    part(2030, 2031) + "ReadRingTriangulation(arg, used, stack);\n" + part(2034, 2045),
)
source += part(2047, 2082)

# Keep metrics, shape accounting and restoration guards through every original exit.
source += member("long double", "ShapeDebt(const std::vector<Cell>& cells)",
    "Measures the same cached shape debt for original and accepted stars.",
    part(2118, 2118).split("{", 1)[1].rsplit("};", 1)[0])
source += part(2083, 2098) + """
std::vector<Id> old;
Patch patch;
std::set<Face> old_physical;
if (!CollectBoundarySmoothPatch(p, old, patch, old_physical)) return false;
""" + part(2111, 2117) + part(2119, 2126).replace("shape_debt(", "ShapeDebt(") + """
std::vector<Point> targets;
std::set<Id> neighbors;
if (!BoundaryTargets(p, patch, geometry, origin, targets, neighbors)) return false;
BoundarySmoothSearch search{p, patch, geometry, debt, before, geometric_before, geometry_measured};
return SearchBoundarySmooth(search, old, targets, old_shape_debt, {validity_start, shape_start, edge_start, geometry_start});
}
"""
source += member("bool", "CollectBoundarySmoothPatch(Id p, std::vector<Id>& old, Patch& patch, std::set<Face>& old_physical)",
    "Collects a complete movable physical star while holding the artificial interface fixed.",
    part(2099, 2110).replace("const auto old=found->second;Patch patch;", "old=found->second;").replace("std::set<Face> old_physical;", "") + "return true;")
source += member("bool", "BoundaryTargets(Id p, const Patch& patch, const PlanarReference& geometry, Point origin, std::vector<Point>& targets, std::set<Id>& neighbors)",
    "Enumerates forward binding targets and keeps the existing fallback search.",
    part(2128, 2159) + "return !targets.empty() || FallbackBoundaryTargets(origin, neighbors, targets);")
source += member("bool", "FallbackBoundaryTargets(Point origin, const std::set<Id>& neighbors, std::vector<Point>& targets)",
    "Adds the original metric mean and axis proposals when no forward target exists.",
    part(2160, 2167).replace("if(targets.empty())try", "try") + "return true;")
source += member("bool", "BoundaryMoveResources::Charge(const PlanarReference* candidate_context)",
    "Charges exactly the retained overlay, atlas, context and memo storage.",
    part(2177, 2186))
source += member("bool", "PrepareBoundaryGeometry(Id p, const Patch& patch, const PlanarReference& geometry, BoundaryMoveResources& resources)",
    "Prepares and charges the projected context and curved atlas when requested.",
    """auto& projected_context = resources.projected_context;
const auto*& projected_geometry = resources.projected_geometry;
auto* atlas_budget = resources.atlas_budget;
auto& prepared_atlas = resources.prepared_atlas;
bool& prepared_atlas_ok = resources.prepared_atlas_ok;
""" + part(2188, 2201).replace("charge_retained_geometry", "resources.Charge") + "return true;")
source += member("bool", "BoundaryProjection(BoundarySmoothSearch& search, Point target_point, BoundaryMoveResources& resources, std::string& reason)",
    "Tests the projection and volume score before expensive geometry reconstruction.",
    """const auto p = search.p;
const auto& patch = search.patch;
const auto& geometry = search.geometry;
const bool debt = search.debt;
const auto best_score = search.best_score;
const auto* projected_geometry = resources.projected_geometry;
auto& prepared_atlas = resources.prepared_atlas;
const bool& prepared_atlas_ok = resources.prepared_atlas_ok;
""" + part(2211, 2237).replace("charge_retained_geometry", "resources.Charge").replace("continue;", "return false;") + "return true;")
source += member("MeshOptimizer::BoundaryDecision", "BuildBoundaryMove(BoundarySmoothSearch& search, Point target_point, BoundaryMoveResources& resources, Patch& candidate, std::string& reason)",
    "Prepares the live move once and evaluates the original raw proposal.",
    """const auto p = search.p;
const auto& patch = search.patch;
const auto& geometry = search.geometry;
auto& prepared_move = resources.prepared_move;
bool& preparation_attempted = resources.preparation_attempted;
auto* atlas_budget = resources.atlas_budget;
""" + part(2238, 2252).replace("charge_retained_geometry", "resources.Charge").replace("continue;", "return BoundaryDecision::Next;").replace("break;", "return BoundaryDecision::Stop;") + "return BoundaryDecision::Admit;")
source += member("MeshOptimizer::BoundaryDecision", "BoundaryGuidance(BoundarySmoothSearch& search, const Patch& candidate, const PlanarReference& candidate_geometry, const PlanarReference* candidate_context, BoundaryMoveResources& resources, SU2NativeD9::SmoothChain& d9_chain)",
    "Measures old guidance lazily and rejects the candidate if its geometry regresses.",
    """const auto p = search.p;
const auto& patch = search.patch;
const auto& geometry = search.geometry;
auto& geometry_measured = search.geometry_measured;
auto& geometric_before = search.geometric_before;
auto& guidance_memo = resources.guidance_memo;
auto& guidance_memo_ready = resources.guidance_memo_ready;
""" + part(2264, 2283).replace("atlas_budget&&!charge_retained_geometry(candidate_parameters.get())","resources.atlas_budget && !resources.Charge(candidate_context)").replace("return false;", "return BoundaryDecision::Fail;").replace("continue;", "return BoundaryDecision::Next;") + "return BoundaryDecision::Admit;")
source += member("MeshOptimizer::BoundaryDecision", "TryBoundaryMove(BoundarySmoothSearch& search, Point target_point, BoundaryMoveResources& resources, Patch& best, SU2NativeD9::SmoothChain& d9_chain)",
    "Carries one candidate through exactly the original ordered gates and counter scope.",
    """const auto& geometry = search.geometry;
const bool debt = search.debt;
auto& best_score = search.best_score;
auto* atlas_budget = resources.atlas_budget;
""" + part(2206, 2210).replace("charge_retained_geometry", "resources.Charge").replace("break;", "return BoundaryDecision::Stop;") + """
if (!BoundaryProjection(search, target_point, resources, reason)) return BoundaryDecision::Next;
if (const auto decision = BuildBoundaryMove(search, target_point, resources, candidate, reason);
    decision != BoundaryDecision::Admit) return decision;
""" + part(2253, 2263).replace("charge_retained_geometry", "resources.Charge").replace("continue;", "return BoundaryDecision::Next;") + """
if (const auto decision = BoundaryGuidance(search, candidate, candidate_geometry, candidate_parameters.get(), resources, d9_chain);
    decision != BoundaryDecision::Admit) return decision;
""" + part(2284, 2287).replace("break;", "return BoundaryDecision::Stop;") + "return BoundaryDecision::Next;")
source += member("bool", "SearchBoundarySmooth(BoundarySmoothSearch& search, const std::vector<Id>& old, const std::vector<Point>& targets, long double old_shape_debt, const SmoothRejections& baseline)",
    "Keeps the prepared resources and candidate-chain scope through the winning publication.",
    """const auto p = search.p;
const auto& geometry = search.geometry;
const auto before = search.best_score;
BoundaryMoveResources resources(geometry);
if (!PrepareBoundaryGeometry(p, search.patch, geometry, resources)) return false;
Patch best;
SU2NativeD9::SmoothChain d9_chain;
for (Point target_point : targets) {
  if (Limited()) break;
  const auto decision = TryBoundaryMove(search, target_point, resources, best, d9_chain);
  if (decision == BoundaryDecision::Stop) break;
  if (decision == BoundaryDecision::Fail) return false;
}
""" + part(2289, 2296).replace("shape_start", "baseline.shape").replace("validity_start", "baseline.validity").replace("edge_start", "baseline.edge").replace("geometry_start", "baseline.geometry") + """
resources.prepared_atlas = {};
resources.projected_context.reset();
resources.prepared_move.reset();
resources.guidance_memo = GeometryGuidanceMemo{};
if (resources.atlas_budget) resources.atlas_budget->caller_bytes = resources.caller_restore.original;
return CommitBoundarySmooth(search, old, best, old_shape_debt, d9_chain);
""")
source += member("bool", "CommitBoundarySmooth(BoundarySmoothSearch& search, const std::vector<Id>& old, const Patch& best, long double old_shape_debt, SU2NativeD9::SmoothChain& d9_chain)",
    "Publishes the certified move with the original binding and relabel order.",
    """const auto p = search.p;
const auto& geometry = search.geometry;
const auto best_score = search.best_score;
const auto debt = search.debt;
""" + part(2299, 2315).replace("shape_debt(", "ShapeDebt("))

# Separate interior targets, score sorting and publication without sharing their distinct physical tests.
source += part(2317, 2337) + part(2339, 2353).replace("shape_debt(", "ShapeDebt(") + """
return SearchInteriorSmooth(p, old, nbr, x0, debt, oldMin, old_shape_debt, shortest, limit,
                            {validity_start, shape_start, edge_start, geometry_start});
}
"""
source += member("void", "AddHeightTargets(Id p, const std::vector<Id>& old, Point x0, std::vector<Point>& targets)",
    "Adds the existing requested-height proposals before metric proposals.",
    part(2355, 2371))
source += member("void", "AddMetricMeanTarget(Point x0, const std::set<Id>& nbr, std::vector<Point>& targets)",
    "Samples the neighbor tensors for the metric mean proposal.",
    part(2372, 2372))
source += member("void", "AddHingeTarget(Id p, const std::vector<Id>& old, Point x0, long double shortest, std::vector<Point>& targets)",
    "Adds the damped hinge step when its original solve succeeds.",
    part(2373, 2390))
source += member("bool", "PlaceInteriorTargets(Id p, const std::vector<Id>& old, Point x0, bool slides, const std::array<double, 3>& plane, bool debt, std::vector<Point>& targets)",
    "Projects proposals onto the existing diagnostic plane and runs the bounded link search.",
    part(2391, 2392) + part(2394, 2402) + "return placed;")
source += member("std::vector<MeshOptimizer::SmoothTrial>", "CollectInteriorTrials(Id p, const std::vector<Id>& old, Point x0, bool debt, long double oldMin, bool placed, const std::vector<Point>& targets, Patch& constraints)",
    "Measures each damped position in proposal order without memoizing a trial identity.",
    part(2404, 2422).replace("struct Trial{long double score;Point x;};std::vector<Trial> trials;", "std::vector<SmoothTrial> trials;") + "return trials;")
source += member("void", "SlidingBindings(Id p, Id replacement, const std::vector<Id>& old, std::map<Face, FacetBinding>& added) const",
    "Carries the old binding when the diagnostic slide changes the skin vertex.",
    part(2437, 2438).replace("if(slides){", "").replace("x=P.id", "x=replacement").rsplit("}",1)[0])
source += member("bool", "CommitInteriorSmooth(Id p, const std::vector<Id>& old, const std::set<Id>& nbr, long double limit, bool slides, const std::array<double, 3>& plane, bool debt, long double old_shape_debt, std::vector<SmoothTrial>& trials)",
    "Certifies scored positions in stable order and publishes the first admissible move.",
    part(2431, 2436).replace("const Trial&", "const SmoothTrial&") + """
if (slides) SlidingBindings(p, P.id, old, added);
""" + part(2439, 2448).replace("shape_debt(", "ShapeDebt(") + "return false;")
source += member("void", "RememberInteriorFailure(Id p, long double oldMin, const SmoothRejections& baseline)",
    "Keeps the original pure-shape refusal cache predicate and budget exclusions.",
    part(2449, 2453).replace("shape_start", "baseline.shape").replace("validity_start", "baseline.validity").replace("edge_start", "baseline.edge").replace("geometry_start", "baseline.geometry"))
source += member("bool", "SearchInteriorSmooth(Id p, const std::vector<Id>& old, const std::set<Id>& nbr, Point x0, bool debt, long double oldMin, long double old_shape_debt, long double shortest, long double limit, const SmoothRejections& baseline)",
    "Runs target generation, scoring, and ordered interior certification under the caller's sweep.",
    """std::vector<Point> targets;
AddHeightTargets(p, old, x0, targets);
AddMetricMeanTarget(x0, nbr, targets);
AddHingeTarget(p, old, x0, shortest, targets);
""" + part(2393, 2393) + """
const bool placed = PlaceInteriorTargets(p, old, x0, slides, plane, debt, targets);
Patch constraints;
auto trials = CollectInteriorTrials(p, old, x0, debt, oldMin, placed, targets, constraints);
if (trials.empty()) {
  RememberInteriorFailure(p, oldMin, baseline);
  return false;
}
if (CommitInteriorSmooth(p, old, nbr, limit, slides, plane, debt, old_shape_debt, trials)) return true;
RememberInteriorFailure(p, oldMin, baseline);
return false;
""")

# Preserve the separate open-ring and closed-ring interface contracts.
source += part(2456, 2466) + """
std::map<Id, std::vector<Id>> ring;
std::vector<Id> order;
Point pa, pb;
if (!OrientBoundaryRing(faces, shell, ring, order, a, b, pa, pb)) return false;
return FlipBoundaryRing(shell, faces, order, {a, pa}, {b, pb});
}
"""
source += member("bool", "OrientBoundaryRing(const std::vector<BoundaryFace>& faces, const std::vector<Id>& shell, std::map<Id, std::vector<Id>>& ring, std::vector<Id>& order, Id& a, Id& b, Point& pa, Point& pb)",
    "Builds and orients the open boundary ring before its original planarity test.",
    """const auto n = shell.size();
""" + part(2468, 2478).replace("std::vector<Id> order{r0};", "order = {r0};").replace("Point pa=points.at(a),pb=points.at(b);", "pa=points.at(a);pb=points.at(b);") + "return true;")
source += member("bool", "FlipBoundaryRing(const std::vector<Id>& shell, const std::vector<BoundaryFace>& faces, const std::vector<Id>& order, Node A, Node B)",
    "Attempts a boundary ring replacement without changing its prescribed skin rules.",
    part(2479, 2487).replace("const Node A{a,pa},B{b,pb};", "") +
    "std::vector<std::vector<long double>> best(m,std::vector<long double>(m,-INFINITY));std::vector<std::vector<size_t>> arg(m,std::vector<size_t>(m,SIZE_MAX));\n"
    "ScoreRingTriangulation(m, debt, forbidden, triangle, ringEdge, best, arg);\n" +
    part(2493, 2494) + "ReadRingTriangulation(arg, used, stack);\n" + part(2497, 2506))
source += part(2508, 2552) + """
return CertifyConeReplacement(old, faces, vertices, pl, kind, split, removed, debt, oldMin);
}
"""
source += member("bool", "CertifyConeReplacement(const std::vector<Id>& old, const std::vector<Face>& faces, const std::set<Id>& vertices, const Placement& pl, Insert kind, const std::pair<Id, Id>* split, long double removed, bool debt, long double oldMin)",
    "Checks the cone with exact measures before replacing the old cells.",
    part(2553, 2567).replace("  if(kept.size()", "  // Reject the replacement if a cavity vertex would vanish.\n  if(kept.size()").replace("  // a cavity vertex would vanish", ""))
source += part(2569, 2629) + "return CommitShellBoundary(shell, fresh, geometry, bindings);\n}\n"
source += member("bool", "CommitShellBoundary(const std::vector<Id>& shell, const Patch& fresh, const PlanarReference& geometry, const std::map<Face, FacetBinding>& bindings)",
    "Publishes the certified boundary shell and refreshes the parameter overlay.",
    part(2630, 2638))
source += part(2640, 2690)

source += part(2691, 2712) + "return PrepareCoupledBoundary(shell, a, b, core.cell.id, geometry);\n}\n"
source += member("bool", "PrepareCoupledBoundary(const std::vector<Id>& shell, Id a, Id b, Id core, const PlanarReference& geometry)",
    "Builds the speculative shell split and retains its original patch through certification.",
    part(2713, 2729).replace("core.cell.id", "core") +
    "return OptimizeCoupledBoundary(all_ids, original, refined, pending, geometry, reason);")
source += member("Patch", "CoupledPatch(const Owned& rows, const Patch& original, const Patch& refined)",
    "Builds the private mesh patch with the original overlay and coverage order.",
    part(2731, 2737))
source += member("bool", "OptimizeCoupledBoundary(const std::vector<Id>& all_ids, const Patch& original, const Patch& refined, Owned& pending, const PlanarReference& geometry, std::string& reason)",
    "Runs the nested optimizer with the original remaining time, work and ID bounds.",
    part(2739, 2754).replace("patch(pending)", "CoupledPatch(pending, original, refined)") +
    "return CertifyCoupledBoundary(all_ids, original, refined, pending, geometry, reason, node, cell, nested_report, final_keep, final_sliding);")
source += member("std::array<size_t, 2>", "ShapeFailures(const std::vector<Cell>& cells)",
    "Counts the exact shape failures without memoizing provisional cell identities.",
    part(2763, 2764).split("{", 1)[1].rsplit("};", 1)[0])
source += member("bool", "CertifyCoupledBoundary(const std::vector<Id>& all_ids, const Patch& original, const Patch& refined, Owned& pending, const PlanarReference& geometry, std::string& reason, Id node, Id cell, OptimizeReport& nested_report, std::set<Id>& final_keep, std::map<Id, std::array<double, 3>>& final_sliding)",
    "Checks the complete coupled patch before it changes authoritative optimizer state.",
    (part(2755, 2762).replace("patch(pending)", "CoupledPatch(pending, original, refined)") +
     part(2765, 2778).replace("counts(", "ShapeFailures(") +
     """std::set<Face> old_physical;
     std::map<Face, Face> skin;
     std::vector<Face> prescribed;
     if (!CertifyCoupledInterface(original, candidate, old_physical, skin, prescribed, reason)) return false;
""" +
     part(2784, 2784) + "return PublishCoupledBoundary(pending, node, cell, candidate, final_context, nested_report, final_keep, final_sliding);"))
source += member("bool", "CertifyCoupledInterface(const Patch& original, const Patch& candidate, std::set<Face>& old_physical, std::map<Face, Face>& skin, std::vector<Face>& prescribed, std::string& reason)",
    "Retains the artificial interface and replaces the physical skin with the candidate faces.",
    part(2779, 2783).replace("std::set<Face> old_physical;", "").replace("std::map<Face,Face> skin;", "").replace("std::vector<Face> prescribed;", "") + "return true;")
source += member("bool", "PublishCoupledBoundary(Owned& pending, Id node, Id cell, Patch& candidate, std::unique_ptr<PlanarReference>& final_context, const OptimizeReport& nested_report, std::set<Id>& final_keep, std::map<Id, std::array<double, 3>>& final_sliding)",
    "Publishes the private mesh and gives credit only after certification.",
    part(2785, 2789))

source += part(2791, 2794) + """
QualityCensus(report.bad_before, report.minimum_before);
RunSweeps();
""" + part(2837, 2841).replace("census(", "QualityCensus(")
source += member("void", "QualityCensus(size_t& bad, double& minimum)",
    "Reports the existing score census over cells excluded only by the frozen-cell set.",
    part(2795, 2795).split("{", 1)[1].rsplit("};", 1)[0])
source += member("void", "RunSweeps()", "Drives the enabled optimizer stages within each existing sweep budget.",
    part(2797, 2801) +
    "RunSweepStage(stage, enabled, accepted);\n}\n" +
    part(2832, 2836).replace("census(", "QualityCensus("))
source += member("void", "RunSweepStage(unsigned stage, const std::array<bool, 3>& enabled, size_t& accepted)",
    "Reserves the original stage work and invokes the existing ordered operators.",
    part(2802, 2810).replace("continue;", "return;", 1) + """
if (stage == 0) {
  ImproveSmoothing(bad, accepted);
} else {
  ImproveTopology(bad, stage == 2, accepted);
  ImproveLongEdges(stage, accepted);
}
""")
source += member("void", "ImproveSmoothing(const PoorCells& bad, size_t& accepted)",
    "Visits each bad-cell vertex once, maintaining its initial occurrence order.",
    part(2812, 2815))
source += member("void", "ImproveTopology(const PoorCells& bad, bool enrichment, size_t& accepted)",
    "Tries topology changes on the still-bad cells in their saved priority order.",
    part(2817, 2818).replace("stage==2", "enrichment"))
source += member("void", "ImproveLongEdges(unsigned stage, size_t& accepted)",
    "Scans long edges and tries removal or insertion in the original stage order.",
    part(2819, 2829))

source += part(2842, 2850)

# Keep factor and vertex buffers in PlaceCone until the entire search exits. The
# point callback is deliberately called again for each face even for repeated ids.
cone_bases = method(
    """bool FillConeBases(const std::vector<Face>& faces, const std::function<Point(Id)>& point,
                       const Tensor& t0, const std::map<Id, Tensor>& vt,
                       std::vector<std::array<Point, 3>>& base, std::vector<Factor3>& factor)""",
    "Builds one constant proposal metric and coordinate base per face.",
    part(1706, 1708).replace("if(!c.ok)return out", "if(!c.ok)return false") + "return true;",
)
source = replace_once(source, part(1694, 1700), cone_bases + part(1694, 1700))
source = replace_once(source, part(1706, 1708), "if (!FillConeBases(faces, point, t0, vt, base, factor)) return out;\n")

# Define pre-existing small operations where their original body already fits.
comments = {
    "struct CoordinateHash {": "Hashes the original coordinate bits used by the tensor memo.",
    "std::array<Face,4> OppositeFaces": "Returns the outward face order indexed by opposite vertex.",
    "bool CellHas": "Reports whether a cell contains the indicated identity.",
    "struct CellQuality {": "Holds the measured shape scores and the original validity sentinels.",
    "Factor3 FactorOf": "Factors the point tensor used to place a proposal.",
    "struct Factor3 {": "Holds the upper factor of the symmetric positive definite metric.",
    "struct Placement {": "Records the best approximate point and the number of evaluations.",
    "class MeshOptimizer {": "Owns one metric optimizer invocation and its local topology and metric caches.",
    "  bool Limited()": "Checks the sticky invocation deadline and the current stage's work allotment.",
    "  void Bound()": "Clears each cache at its original configured entry bound.",
    "  void Index()": "Rebuilds topology and boundary constraints from the current mesh.",
    "  Tensor At(": "Looks up or measures a point tensor and accounts for a miss.",
    "  double Score(": "Returns the exact cell shape score.",
    "  double Height(": "Measures requested first height with the matching private parameters.",
    "  CellQuality RowQuality(": "Reads the caller's cached shape if available, then the local memo.",
    "  double RowHeight(": "Memoizes the current cell's requested-height score.",
    "  double RowScore(": "Returns the lesser of the current shape and height scores.",
    "  long double RowPriority(": "Ranks cells by the active merit without certifying proposals.",
    "  double MinScore(": "Finds the minimum score in the listed current cells.",
    "  long double OldMerit(": "Measures the current cavity under its original timer boundary.",
    "  long double FreshMerit(": "Measures candidate shape and rejects a new requested-height failure.",
    "  bool Progress(": "Applies the active comparison to the measured candidate and original merit.",
    "  long double Estimate(": "Estimates a target edge length and charges the surrogate query.",
    "  long double Exact(": "Caches the integrated edge length under the undirected coordinate key.",
    "  bool Frozen(": "Checks whether topology changes can touch the named current cell.",
    "  bool Movable(": "Checks the fixed-vertex rule and the bounded, unfrozen incident star.",
    "  static std::vector<Id> Common(": "Intersects two incident-cell lists in canonical order.",
    "  bool Adjacent(": "Checks for a current edge from the first vertex to the second.",
    "  void RememberRejectedSmooth(": "Caches a pure-shape refusal up to the existing entry bound.",
    "  struct BoundaryFace": "Carries the oriented mesh face and its physical binding.",
    "  std::vector<BoundaryFace> BoundaryFacesOf(": "Collects each matching physical face in cell order.",
    "  static long double Norm3(": "Measures the distance between two points.",
    "  static long double Area2(": "Returns twice the triangle area and its unnormalized normal.",
    "  struct SmoothRejections": "Captures the four cumulative counters before a star search.",
    "  struct SmoothTrial": "Holds one measured interior trial coordinate and score.",
    "    struct CallerByteRestore": "Restores the exact borrowed atlas byte charge on every exit.",
    "  struct BoundarySmoothSearch": "Borrows the exported star and tracks its current merit and guidance.",
    "bool MeshOptimizer::Smooth(Id": "Dispatches a movable star and keeps the original interior measurement scope.",
    "bool MeshOptimizer::ShellBoundary(Id": "Exposes a physical diagonal through a certified shell replacement.",
    "bool MeshOptimizer::TryCell(Id": "Tries the enabled operators in their original order on a current cell.",
    "void MeshOptimizer::AccountNested": "Charges nested work once and credits progress only on publication.",
    "bool MeshOptimizer::CoupledBoundary()": "Attempts the bounded shell and insertion recovery with its borrowed atlas.",
    "bool MeshOptimizer::Run()": "Runs the original metric optimization sweep and reports the final census.",
    "bool OptimizeMetricMesh(": "Checks the borrowed atlas admission and optimizes the caller's mesh in place.",
}
for anchor, description in comments.items():
    indent = anchor[:len(anchor) - len(anchor.lstrip())]
    source = replace_once(source, "\n" + anchor, f"\n{indent}// {description}\n{anchor}")

# clang-format uses the path of the intended production destination.
formatted = subprocess.check_output(
    ["clang-format-18", f"--assume-filename={MOVED.resolve()}"], input=source.encode()
)
include = f'#include "native_engine3d/{MOVED.name}"\n'
main = part(1, 1647).encode() + include.encode() + part(2851, len(lines)).encode()
assert main.replace(include.encode(), part(1648, 2850).encode(), 1) == base
assert (SOURCE).read_bytes() == base, "Production main source changed during draft generation"
(DRAFT / SOURCE.name).write_bytes(main)
(DRAFT / MOVED.name).write_bytes(formatted)

manifest = {
    "base_commit": BASE,
    "source_range": {"first_line": 1648, "last_line": 2850},
    "files": [
        {"path": str(SOURCE), "draft": SOURCE.name, "base_sha256": BASE_SHA, "sha256": sha256(main).hexdigest()},
        {"path": str(MOVED), "draft": MOVED.name, "base_sha256": None, "sha256": sha256(formatted).hexdigest()},
    ],
}
(DRAFT / "manifest.json").write_text(json.dumps(manifest, indent=2) + "\n")
