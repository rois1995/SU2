/*!
 * \file b0_analysis.hpp
 * \brief B0 spike: statistics of the gates (B_DESIGN_EXPLORATION.md section 7), matched regions, H-2 degradation
 *        tails, macro-orientation (3.2), B3 face measures (3.1), runtime classifier (plan 6.4).
 */
#pragma once
#include "b0.hpp"

namespace b0 {

/*--- Geometric record of the artificial faces of all levels (former interfaces). ---*/
struct FormerFaces {
  int dim = 0;
  vector<double> fx;      // face vertex coordinates, dim*dim per face
  vector<double> pts;     // samples: centroid, vertices, edge midpoints
  vector<i64> owner;      // sample -> face
  KDTree kd;
  vector<int> step, kind;  // 0 level-0, 1 later cut, 2 patch boundary
  vector<i64> piece;
  vector<double> beforeQ, afterQ;
  vector<i64> gids;
  struct Box { std::array<double, 3> lo{}, hi{}; i64 begin = 0, end = 0; int left = -1, right = -1; };
  vector<Box> boxes;
  vector<i64> order;
  int BuildBoxes(i64 begin, i64 end);
  i64 nFaces() const { return dim ? fx.size() / (dim * dim) : 0; }
  void Add(const Mesh& m, const vector<i64>& faceGids);  // faces given as gid tuples of mesh m
  void AddCoords(int dim, const double* coords, i64 nf);
  void Index();
  /*--- Exact metric point-to-face distance, with conservative AABB/eigenvalue pruning. ---*/
  double Dist(const double* x, const Sym& M, i64* face = nullptr) const;
  double DistBrute(const double* x, const Sym& M, i64* face = nullptr) const;
  double DistApprox(const double* x, const Sym& M, i64* face = nullptr) const;
  double FaceDist(i64 face, const double* x, const Sym& M) const;
};

/*--- Statistics of one mesh in the target metric (interpolated from the level-0 input). ---*/
static const int NAR = 4, NDB = 6;  // AR classes, metric-distance bins [0,1) [1,2) [2,4) [4,8) [8,16) >=16
int DistBin(double d);
int ARClass(double logAR);
struct RegionStat {
  i64 nCell = 0, nEdge = 0, nEdgeIn = 0;
  double q1 = 0, q5 = 0, qmin = 0, bad = 0;  // quality p1, p5, min; bad fraction (Q-3 kind)
};
struct MeshStats {
  vector<vector<double>> regionQuality;
  i64 nv = 0, ne = 0;
  double q1global = 0, q5global = 0, qmin = 0;
  double edgeIn = 0, edgeInNear = 0;   // Q-1 global, near cuts (metric distance < 2)
  i64 nEdge = 0, nEdgeNear = 0;
  double badFrac = 0;                  // Q-3: sliver fraction (3D, min metric dihedral < 5 deg) / q < 0.1 (2D)
  i64 nBad = 0;
  double complexityTarget = 0;         // sum |K| sqrt(det M_target) on this mesh
  double complexityCarried = 0;        // with the carried metric of the mesh
  RegionStat reg[NAR][NDB];
  /*--- surface deviation (ADAP_SURFACE= YES) per marker: mean, max ---*/
  vector<double> devMean, devMax;
  /*--- per-AR-class quality thresholds (p1, p5) ---*/
  double classQ1[NAR] = {0}, classQ5[NAR] = {0};
  /*--- H-2: degradation depth (metric) per former face: p95, p99, max; count of faces with a degraded cell ---*/
  double depthP95 = 0, depthP99 = 0, depthMax = 0;
  i64 nFacesDegraded = 0;
  vector<double> faceDepth;  // per former face (metric distance of its farthest degraded cell, 0 if none)
  vector<i64> perElemFace;   // nearest former face per element (only if requested)
};
struct CellStat {
  int ar = 0, db = 0, cls = 0;
  double quality = 0, distance = 1e300;
  i64 face = -1;
  vector<double> edges;
};
vector<double> FrozenFaceFinalQuality(const Mesh& m, const FormerFaces& ff, const vector<CellStat>& cells);
std::map<FKey, double> FaceQualityMap(const Mesh& m, const Background& target);
double FaceAdjacentQuality(const Mesh& m, const vector<i64>& gids, const Background& target);
vector<CellStat> CellStats(const Mesh& m, const Background& target, const FormerFaces* ff);
Mesh TargetMetricMesh(const Mesh& m, const Background& target);
struct StatsOpts {
  const FormerFaces* ff = nullptr;
  const double* degradeQ = nullptr;  // per AR class threshold for "degraded" (serial p1); null: no depth
  bool perElem = false;
  const Mesh* surfRef = nullptr;     // original surface for the deviation
};
MeshStats ComputeStats(const Mesh& m, const Background& target, const StatsOpts& so);

/*--- Cut faces of a partition and their measures (3.1, 3.2). ---*/
struct CutFace {
  i64 elemA, elemB;   // elemA in the lower part
  int a, b;           // parts a < b
  double area, n[3], c[3];
  double cf, logAR, ratioMid;  // conformity, log10 AR, lambda_max/lambda_mid
  double fine[3];
  double ht;          // tangential size
  bool aniso = false; // n_ref defined
  int macro = 0;      // 0 isotropic (no n_ref), 1 lateral (ok), 2 violation, 3 unresolved
  i64 key[3];         // vertex gids
};
struct MacroReport {
  i64 nCut = 0;
  double areaCut = 0, areaAniso = 0, areaViol = 0, areaUnres = 0;
  i64 nViol = 0, nUnres = 0, nAniso = 0, longestViolRun = 0;
  double meanCf = 0, p95Cf = 0, meanLogAR = 0;
};
vector<CutFace> CutFaces(const Mesh& m, const Adjacency& adj, const vector<int>& part);
MacroReport MacroOrientation(const Mesh& m, vector<CutFace>& cf, double rhoFactor = 3.0, double arP = 10.0,
                             double unresolved = 0.1, double cosMax = std::cos(M_PI / 4));
/*--- B3 weight per interior face of the dual graph: w = clamp(1 + a cf + b log10 AR, 1, 1e4). ---*/
double FaceWeightB3(const Mesh& m, const i64* faceVerts, double a, double b);

/*--- Runtime classifier (plan 6.4). Returns the DEFECT vertices; class densities for calibration. ---*/
struct ClassifierOut {
  vector<i64> defectVerts;
  double nearDensity[8] = {0}, farDensity[8] = {0}, allDensity[8] = {0};
  i64 nearCells[8] = {0}, farCells[8] = {0}, allCells[8] = {0};
  i64 nBadNear = 0, nSeedCells = 0;
};
ClassifierOut Classify(const Mesh& m, const Background& target, const Adjacency& adj, const FormerFaces* ff,
                       double R, const double* allow);

}  // namespace b0
