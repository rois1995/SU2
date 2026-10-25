/*!
 * \file BLRebuild17.inl
 * \brief DC-17 first-row rebuild of a halving chain step (the R2 first-row prism rebuilt at the new first height).
 *        Serial, not reentrant: enter through SU2NativeBLPipeline3D::PipelineScope. Every function/variable is inline
 *        (shared by the production translation unit and the harness translation unit).
 * Copyright 2012-2026, SU2 Contributors (cf. AUTHORS.md).
 * SU2 is free software under the GNU Lesser General Public License, version 2.1 or later.
 */
#pragma once
// DC-17 (DC17_PLAN sect. 1). Diagnosis of the DC-16 chain step (full M6 x16 event 1, 241k bad cells below 3 h0): the
// first row of an event is a prism per wall face f = (a,b,c): lids La,Lb,Lc on the column rays at ~1.5 h0 and the
// first-child apex p_f at h0 inside, star(p_f) = the 8 cells of the prism (98.5 % of the M6 faces). The DC-16 halving
// coned the first child from q at h0' = h0/2 and split only the vertical column edges; the lateral quad diagonals
// (wall vertex -> neighbour lid, 0 -> 3 h0') and the old apex edges were kept, so 74 % of the bad cells were the pieces
// (w, m, p, L_b) and (b, p, m, L_a) spanning the whole old band (dc17/bad_signature_dc16_event1.txt).
// The rebuild uses the current height: (1) every column edge (w, L_w) is split at m_w (the new
// lid, normal height at least1.5 h0' above each incident wall), (2) every lateral quad diagonal is removed by a 4-4 edge flip to the chord (m_a, m_b),
// (3) every prism star(p_f) is rebuilt as the lower prism (a,b,c, m_a,m_b,m_c) coned from the new apex q_f at h0'
// (the R2 first-row pattern at the new scale) and the upper slab (m_a,m_b,m_c, L_a,L_b,L_c) coned from a centre u_f.
// Every operation is a local cavity transaction (owned cells, one-unit commit, fixed oriented interface, positive
// cells, metric validity, certified first heights): ready for the 2D ownership pattern. A face whose prism is not
// regular (fans, sectors, rims, unlifted vertices) or whose rebuild is refused keeps the DC-16 cone split (HalveFace).
// The registry follows: lift(w) = m_w, apex(f) = q_f, so the next chain step finds the same structure.
namespace dc17 {
using namespace dc13;
// TEST-ONLY switches (never set in production): the DC-16 halving + ColumnInsert instead of the rebuild (comparison),
// and a V time cap for diagnostic runs of the frozen full-M6 harness (< 0: none).
inline bool rebuildFirstRow = true;
inline double diagnosticVSeconds = -1;

struct RebuildStats {
  size_t faces = 0, need = 0, columns = 0, split = 0, short_columns = 0, diagonals = 0, flipped = 0, flip_refused = 0,
         no_diagonal = 0, prisms = 0, rebuilt = 0, irregular = 0, refused = 0, fallback = 0;
  HalveStats halve;
  ColumnStats column;
  std::map<std::string, size_t> why;
  double seconds = 0;
};

// Edge removal of an interior edge e with a ring of 4 vertices: the ring polygon is retriangulated by the chord
// (c0, c1) (opposite ring vertices); the 4 cells around e become (e_k, c0, c1, r) for the two other ring vertices r.
inline bool FlipToChord(State& s, Edge e, Id c0, Id c1, std::string& why) {
  const auto ids = Star(s, e);
  if (ids.size() != 4) { why = "ring size " + std::to_string(ids.size()); return false; }
  std::set<Id> ring;
  for (Id id : ids) for (const auto& n : s.mesh.at(id).cell.v) if (n.id != e[0] && n.id != e[1]) ring.insert(n.id);
  if (ring.size() != 4 || !ring.count(c0) || !ring.count(c1)) { why = "ring without the chord"; return false; }
  for (Id id : ids) { const auto& c = s.mesh.at(id).cell; if (CellHas(c, c0) && CellHas(c, c1)) { why = "chord is a ring edge"; return false; } }
  Patch old;
  try { old = PatchOf(s, ids); } catch (const std::exception&) { why = "patch"; return false; }
  for (const auto& sf : old.surface) if (Has(sf.v, e[0]) && Has(sf.v, e[1])) { why = "physical edge"; return false; }
  std::map<Id, Node> nodes;
  for (const auto& c : old.cells) for (const auto& n : c.v) nodes[n.id] = n;
  Patch fresh; fresh.surface = old.surface; fresh.parameters = old.parameters;
  Id next = s.next_cell;
  for (Id r : ring) {
    if (r == c0 || r == c1) continue;
    for (Id end : {e[0], e[1]}) {
      Cell c{next++, {nodes.at(end), nodes.at(c0), nodes.at(c1), nodes.at(r)}};
      if (!Positive(c.v)) std::swap(c.v[2], c.v[3]);
      if (!Positive(c.v)) { why = "degenerate"; return false; }
      fresh.cells.push_back(c);
    }
  }
  InterfaceTopologyMemo memo;
  if (!ValidateFixedInterface(old.cells, fresh.cells, why, memo)) return false;
  for (const auto& c : fresh.cells) if (!Valid(Shape(s, c))) { why = "validity"; return false; }
  try { s.reference->ExportParameters(fresh); } catch (const std::exception& x) { why = std::string("parameters: ") + x.what(); return false; }
  Apply(s, ids, fresh);
  return true;
}

// Worst normalized shape of a candidate cone (min over cells of min(q / .2, J / .05)); -1 when a cell is not positive
// or not metric-valid.
inline double ConeScore(State& s, const std::vector<Cell>& cells) {
  double worst = INFINITY;
  for (const auto& c : cells) {
    if (!Positive(c.v)) return -1;
    const auto sh = Shape(s, c);
    if (!Valid(sh)) return -1;
    worst = std::min(worst, std::min(sh.q / MinimumMeanRatio, sh.J / MinimumScaledJacobian));
  }
  return worst;
}

// Step 3: star(p_f) rebuilt as the lower prism coned from q_f and the upper slab coned from u_f. `col` maps each
// split column's wall vertex to (new lid m, old top L).
inline bool RebuildPrism(State& s, Rows10& rows, std::set<Face>& certified, const std::pair<Id, Face>& wf,
                         const std::map<Id, std::pair<Id, Id>>& col, std::string& why) {
  if (!s.mesh.count(wf.first)) { why = "first child"; return false; }
  const Cell fc = s.mesh.at(wf.first).cell;
  Face out{};
  for (auto f : CellFaces(fc)) if (Sorted(f) == wf.second) out = f;
  Node apex{};
  for (const auto& n : fc.v) if (!Has(wf.second, n.id)) apex = n;
  std::set<Id> lower(wf.second.begin(), wf.second.end()), upper;
  std::array<Id, 3> mid{};
  for (int k = 0; k < 3; ++k) {
    const auto c = col.find(out[k]);
    if (c == col.end()) { why = "irregular: unsplit column"; return false; }
    mid[k] = c->second.first;
    lower.insert(c->second.first); upper.insert(c->second.first); upper.insert(c->second.second);
  }
  if (lower.size() != 6 || upper.size() != 6) { why = "irregular: shared column vertices"; return false; }
  const auto star = StarOf(s, apex.id);
  const std::vector<Id> ids(star.begin(), star.end());
  if (ids.size() > MaximumCavityCells) { why = "irregular: star size"; return false; }
  // the link of the apex (faces opposite it) is the cavity skin; each link face lies in the lower or the upper part
  std::vector<Face> lo, up;
  for (Id id : ids) {
    const auto& c = s.mesh.at(id).cell;
    const auto faces = CellFaces(c);
    for (unsigned k = 0; k < 4; ++k) {
      if (c.v[k].id != apex.id) continue;
      const auto& f = faces[k];
      const bool inL = lower.count(f[0]) && lower.count(f[1]) && lower.count(f[2]);
      const bool inU = upper.count(f[0]) && upper.count(f[1]) && upper.count(f[2]);
      if (inL == inU) { why = "irregular: link face across the new lid level"; return false; }
      (inL ? lo : up).push_back(f);
    }
  }
  Patch old;
  try { old = PatchOf(s, ids); } catch (const std::exception&) { why = "patch"; return false; }
  std::map<Id, Node> nodes;
  for (const auto& c : old.cells) for (const auto& n : c.v) nodes[n.id] = n;
  // plan review 1: every prism node must be in the apex star (an irregular star falls back instead of throwing)
  for (Id x : lower) if (!nodes.count(x)) { why = "irregular: prism node outside the apex star"; return false; }
  for (Id x : upper) if (!nodes.count(x)) { why = "irregular: prism node outside the apex star"; return false; }
  if (lo.empty() || up.empty()) { why = "irregular: empty prism part"; return false; }
  const V a = Of(nodes.at(out[0]).p), b = Of(nodes.at(out[1]).p), cc = Of(nodes.at(out[2]).p);
  const V n = -1.L * Outward(a, b, cc), g = (1.L / 3) * (a + b + cc);
  const long double alt = Dot(Of(apex.p) - a, n), h = FaceH0(s, wf.second);
  Id next = s.next_cell;
  auto cone = [&](const Node& x, const std::vector<Face>& faces, const Face& lid) {
    std::vector<Cell> cells;
    for (const auto& f : faces) cells.push_back(Cell{0, {x, nodes.at(f[0]), nodes.at(f[1]), nodes.at(f[2])}});
    cells.push_back(Cell{0, {x, nodes.at(lid[0]), nodes.at(lid[1]), nodes.at(lid[2])}});
    return cells;
  };
  // lower prism: the mid face oriented outward from the lower part (q below it)
  const Node q1{s.next_node, Pt(g + h * n)}, q2{s.next_node, Pt(Of(apex.p) - (alt - h) * n)};
  Face midL{mid[0], mid[1], mid[2]};
  {const Cell t{0, {q1, nodes.at(midL[0]), nodes.at(midL[1]), nodes.at(midL[2])}}; if (!Positive(t.v)) std::swap(midL[1], midL[2]);}
  const Face midU{midL[0], midL[2], midL[1]};
  std::vector<Cell> lowCells, upCells;
  double lowScore = -1, upScore = -1;
  Node q{}, u{};
  for (const Node& x : {q1, q2}) {
    auto cells = cone(x, lo, midL);
    const double sc = ConeScore(s, cells);
    if (sc > lowScore) { lowScore = sc; lowCells = cells; q = x; }
  }
  if (lowScore < 0) { why = "lower prism: no valid apex"; return false; }
  V centre{};
  for (Id x : upper) centre = centre + Of(nodes.at(x).p);
  const Node u1{s.next_node + 1, Pt((1.L / 6) * centre)};
  for (const Node& x : {u1, apex}) {
    auto cells = cone(x, up, midU);
    const double sc = ConeScore(s, cells);
    if (sc > upScore) { upScore = sc; upCells = cells; u = x; }
  }
  if (upScore < 0) { why = "upper slab: no valid centre"; return false; }
  Patch fresh; fresh.surface = old.surface; fresh.parameters = old.parameters;
  for (auto c : lowCells) { c.id = next++; fresh.cells.push_back(c); }
  for (auto c : upCells) { c.id = next++; fresh.cells.push_back(c); }
  InterfaceTopologyMemo memo;
  if (!ValidateFixedInterface(old.cells, fresh.cells, why, memo)) { why = "interface: " + Key(why); return false; }
  try { s.reference->ExportParameters(fresh); } catch (const std::exception& x) { why = std::string("parameters: ") + x.what(); return false; }
  std::string hw;
  if (!HeightT(s, fresh, hw)) { why = "height: " + Key(hw); return false; }
  Apply(s, ids, fresh);
  if (rows.apex.count(wf.second)) rows.apex[wf.second] = q.id;
  certified.insert(wf.second);
  (void)u;
  return true;
}

// A shared lid must clear every incident wall's current first-height request, even after column optimization.
inline long double ColumnLidFraction(const State& s,Id wall,const V& delta) {
  long double fraction=0;
  for(Id id:StarOf(s,wall)) {
    const auto& row=s.mesh.at(id);const auto faces=CellFaces(row.cell);
    for(unsigned k=0;k<4;++k)if(row.physical[k].present && Has(faces[k],wall)) {
      const auto marker=s.reference->Get(row.physical[k].facet).marker;if(!s.walls.count(marker))continue;
      const auto& f=faces[k];const V a=Of(NodePoint(s,f[0])),b=Of(NodePoint(s,f[1])),c=Of(NodePoint(s,f[2]));
      const auto height=Dot(delta,-1.L*Outward(a,b,c));
      if(!(height>0))return INFINITY;
      fraction=std::max(fraction,1.5L*FaceH0(s,f)/height);
    }
  }
  return fraction>0 && fraction<1 ? fraction : INFINITY;
}

// The chain-step first-row rebuild (steps 1-3 above) over the wall faces whose first-child apex lies above the new
// first-height window; fallback HalveFace (DC-16 cone split) for the others.
inline RebuildStats RebuildFirstRow(State& s, Rows10& rows, std::set<Face>& certified) {
  Scope scope("DC17 RebuildFirstRow");
  const auto t0 = Clock::now();
  RebuildStats st;
  MapLayers(s);
  std::vector<std::pair<Id, Face>> need;
  for (const auto& wf : WallFaces(s)) {
    ++st.faces;
    const Cell& c = s.mesh.at(wf.first).cell;
    Face out{};
    for (auto f : CellFaces(c)) if (Sorted(f) == wf.second) out = f;
    Node apex{};
    for (const auto& n : c.v) if (!Has(wf.second, n.id)) apex = n;
    const V a = Of(NodePoint(s, out[0])), b = Of(NodePoint(s, out[1])), cc = Of(NodePoint(s, out[2]));
    const long double alt = Dot(Of(apex.p) - a, -1.L * Outward(a, b, cc)), h = FaceH0(s, wf.second);
    if (alt > h * (1 + HeightTolerance)) need.push_back(wf); else ++st.halve.skipped_low;
  }
  st.need = need.size();
  // step 1: shared lids sized by all incident requested heights
  std::set<Id> verts;
  for (const auto& wf : need) verts.insert(wf.second.begin(), wf.second.end());
  const auto wallSet = WallVertices(s);
  const auto apices = rows.Apices();
  std::map<Id, std::pair<Id, Id>> col;
  for (Id w : verts) {
    ColumnEdge ce;
    if (!ColumnEdgeOf(s, rows, w, wallSet, apices, false, ce, st.column)) continue;
    ++st.columns;
    const V delta=Of(NodePoint(s,ce.top))-ce.x;
    const long double fraction=ColumnLidFraction(s,w,delta);
    if(!std::isfinite(fraction)){++st.short_columns;continue;}
    const long double z=fraction*Dot(delta,ce.nb);
    const Id m = SplitColumnEdge(s, w, ce.top, ce.x, ce.d, ce.dn, z, st.column);
    if (m == UINT64_MAX) continue;
    col[w] = {m, ce.top};
    rows.lift[w] = m;
    ++st.split;
  }
  // step 2: lateral quad diagonals (w, neighbour top) -> chord (m_a, m_b)
  std::set<Edge> wallEdges;
  for (const auto& wf : need)
    for (int k = 0; k < 3; ++k) { const Id x = wf.second[k], y = wf.second[(k + 1) % 3]; wallEdges.insert({std::min(x, y), std::max(x, y)}); }
  for (const auto& we : wallEdges) {
    const auto ca = col.find(we[0]), cb = col.find(we[1]);
    if (ca == col.end() || cb == col.end()) continue;
    const Edge d1{std::min(we[0], cb->second.second), std::max(we[0], cb->second.second)};
    const Edge d2{std::min(we[1], ca->second.second), std::max(we[1], ca->second.second)};
    const bool e1 = !Star(s, d1).empty(), e2 = !Star(s, d2).empty();
    if (e1 == e2) { ++st.no_diagonal; continue; }
    ++st.diagonals;
    std::string why;
    bool ok = false;
    try { ok = FlipToChord(s, e1 ? d1 : d2, ca->second.first, cb->second.first, why); } catch (const std::exception& x) { why = x.what(); }
    if (ok) ++st.flipped;
    else { ++st.flip_refused; ++st.why["flip: " + Key(why)]; }
  }
  // step 3: prism rebuild, fallback cone split
  for (const auto& wf : need) {
    ++st.prisms;
    std::string why;
    bool ok = false;
    try { ok = RebuildPrism(s, rows, certified, wf, col, why); } catch (const std::exception& x) { why = std::string("exception: ") + x.what(); }
    if (ok) { ++st.rebuilt; continue; }
    (why.rfind("irregular", 0) == 0 ? st.irregular : st.refused)++;
    ++st.why["prism: " + Key(why)];
    ++st.fallback;
    ++st.halve.faces;
    try { HalveFace(s, rows, certified, wf, st.halve); }
    catch (const std::exception& x) { ++st.halve.refused; ++st.halve.why[std::string("exception: ") + Key(x.what())]; }
  }
  st.seconds = Seconds(t0);
  return st;
}

inline std::string JsonRebuild(const RebuildStats& r) {
  std::ostringstream o;
  o << "{\"faces\":" << r.faces << ",\"need\":" << r.need << ",\"columns\":" << r.columns << ",\"split\":" << r.split
    << ",\"short_columns\":" << r.short_columns << ",\"diagonals\":" << r.diagonals << ",\"flipped\":" << r.flipped
    << ",\"flip_refused\":" << r.flip_refused << ",\"no_diagonal\":" << r.no_diagonal << ",\"prisms\":" << r.prisms
    << ",\"rebuilt\":" << r.rebuilt << ",\"irregular\":" << r.irregular << ",\"refused\":" << r.refused
    << ",\"fallback\":" << r.fallback << ",\"fallback_split\":" << r.halve.split << ",\"fallback_refused\":" << r.halve.refused
    << ",\"column_refused\":" << r.column.refused << ",\"seconds\":" << r.seconds << ",\"why\":{";
  bool k = false;
  for (const auto& w : r.why) { o << (k ? "," : "") << junction_wp0::Json(w.first) << ':' << w.second; k = true; }
  for (const auto& w : r.column.why) { o << (k ? "," : "") << junction_wp0::Json("column: " + w.first) << ':' << w.second; k = true; }
  for (const auto& w : r.halve.why) { o << (k ? "," : "") << junction_wp0::Json("fallback: " + w.first) << ':' << w.second; k = true; }
  o << "}}";
  return o.str();
}

// ---- TEST-ONLY diagnostic probe (DC-17 target 2): what the bad cells are in their own target ------------------------
// Never set in production (the frozen full-M6 harness sets probePath from SU2_DC17_PROBE). For every cell below the
// final shape gate and a deterministic 1-in-50 sample of the others: centroid, q, J, wall distance, the metric sizes at
// the centroid (h1 <= h2 <= h3) with the h1 direction, the longest/shortest metric edge (centroid tensor) and the
// largest vertex-vs-centroid metric ratio of a cell edge (how much the target varies across the cell).
inline std::string probePath;
inline void SymmetricEigen3(long double a[3][3],long double w[3],long double v[3][3]) {
  for(int i=0;i<3;++i)for(int j=0;j<3;++j)v[i][j]=i==j;
  for(int sweep=0;sweep<64;++sweep){long double off=0;for(int i=0;i<3;++i)for(int j=i+1;j<3;++j)off+=a[i][j]*a[i][j];if(off<1e-36L*(a[0][0]*a[0][0]+a[1][1]*a[1][1]+a[2][2]*a[2][2]))break;
    for(int p=0;p<3;++p)for(int q=p+1;q<3;++q){if(a[p][q]==0)continue;const long double th=(a[q][q]-a[p][p])/(2*a[p][q]);
      const long double t=(th>=0?1:-1)/(std::abs(th)+std::sqrt(th*th+1)),c=1/std::sqrt(t*t+1),sn=t*c;
      for(int k=0;k<3;++k){const long double x=a[k][p],y=a[k][q];a[k][p]=c*x-sn*y;a[k][q]=sn*x+c*y;}
      for(int k=0;k<3;++k){const long double x=a[p][k],y=a[q][k];a[p][k]=c*x-sn*y;a[q][k]=sn*x+c*y;}
      for(int k=0;k<3;++k){const long double x=v[k][p],y=v[k][q];v[k][p]=c*x-sn*y;v[k][q]=sn*x+c*y;}}}
  for(int i=0;i<3;++i)w[i]=a[i][i];
}
inline void Probe(State& s,const std::string& path) {
  std::ofstream out(path);out<<std::setprecision(9);
  out<<"cell bad x y z q J wall_distance h1 h2 h3 d1x d1y d1z lmax lmin variation\n";
  size_t k=0;
  for(const auto& row:s.mesh) {
    const auto sh=MeshShape(s,row.first);const bool bad=Bad(sh);if(!bad&&(k++%50))continue;
    const auto& c=row.second.cell;const Point g=Centroid(c);Tensor m;
    try{m=s.field->Query(g).target;}catch(const std::exception&){continue;}
    long double a[3][3]={{m.xx,m.xy,m.xz},{m.xy,m.yy,m.yz},{m.xz,m.yz,m.zz}},w[3],v[3][3];SymmetricEigen3(a,w,v);
    int o[3]={0,1,2};std::sort(o,o+3,[&](int x,int y){return w[x]>w[y];});  // largest eigenvalue = smallest size first
    auto len=[](const Tensor& t,const V& e){const long double q=e.x*(t.xx*e.x+t.xy*e.y+t.xz*e.z)+e.y*(t.xy*e.x+t.yy*e.y+t.yz*e.z)+e.z*(t.xz*e.x+t.yz*e.y+t.zz*e.z);return std::sqrt(std::max(q,0.L));};
    long double lmax=0,lmin=INFINITY,variation=0;
    for(unsigned i=0;i<4;++i)for(unsigned j=i+1;j<4;++j){const V e=Of(c.v[j].p)-Of(c.v[i].p);const auto l=len(m,e);lmax=std::max(lmax,l);lmin=std::min(lmin,l);
      for(unsigned x:{i,j}){Tensor t;try{t=eventTensors.Get(s,c.v[x].id);}catch(const std::exception&){continue;}const auto lv=len(t,e);if(lv>0&&l>0)variation=std::max(variation,std::abs(std::log(lv/l)));}}
    long double d=-1;try{d=MinWallDistance(s,Of(g));}catch(const std::exception&){}
    out<<row.first<<' '<<bad<<' '<<g.x<<' '<<g.y<<' '<<g.z<<' '<<sh.q<<' '<<sh.J<<' '<<double(d);
    for(int i=0;i<3;++i)out<<' '<<double(1/std::sqrt(std::max(w[o[i]],1e-300L)));
    out<<' '<<double(v[0][o[0]])<<' '<<double(v[1][o[0]])<<' '<<double(v[2][o[0]])<<' '<<double(lmax)<<' '<<double(lmin)<<' '<<double(variation)<<"\n";
  }
}
}  // namespace dc17
