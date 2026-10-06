/*!
 * \file b0_driver.hpp
 * \brief B0 spike driver: placement (P0/Pb/P3, adversarial), transactions (2.3), resource admission (2.4), the
 *        schedulers S-base (plan 6.5-6.6), S-B4 (4.2) and S-batch (4.3).
 */
#pragma once
#include <sstream>

#include "b0_analysis.hpp"

namespace b0 {

struct RunOpts {
  string placement = "Pb";  // P0, Pb, P3, cutx:x0, line:px,py,nx,ny, plane:px,py,pz,nx,ny,nz, random:seed,
                            // emptymid, file:path
  int P = 4;
  string sched = "sbase";   // level0, sbase, sb4, sbatch
  int L = 2;
  double a = 50, b = 20;    // B3
  double capMemFactor = 4;  // capMem = factor * total work / P
  int maxLevels = 5;        // ADAP_PAR_LEVELS
  int batchBudgetC = 12, batchBudgetD = 4;
  double cE = 0;            // output elements per unit complexity (0: theoretical)
  double memBudgetMB = 0;   // per virtual rank transaction budget (0: from capMem and the bytes model)
  double margin = 1.3;
  double alphaIn = 0, alphaOut = 0;  // MMG bytes model (0: defaults)
  bool level0Stats = false;
  string out = "result.json";
  string dumpVtu;
  string artifacts;
  int snapshotStep = -1;
  vector<double> allow;     // classifier allowances (8)
  vector<string> serialRefs;// serial meshes for the matched-region comparison
  bool verbose = true;
};

struct StepRecord {
  string kind;
  int step = 0, L = 0;
  i64 nPieces = 0, nAccepted = 0, nRolled = 0, nUnadmitted = 0, nExcluded = 0;
  i64 C = 0, D = 0, workElems = 0, largestPiece = 0;
  double largestWork = 0, totalWork = 0;
  i64 nvAfter = 0, neAfter = 0, nLowEdge = 0;
  double capLoad = 0, capMem = 0;
  double tMMGsum = 0, tMMGmax = 0, critPath = 0, tCommit = 0;
  bool committed = true;
  string note;
  vector<double> vrankPred, vrankMeas;  // MB
  vector<std::array<double, 9>> pieceRec;  // nIn, predOut, nOut, tMMG, vrank, nArt, status, capMB, peakMB
  vector<string> failures;
  vector<string> pieceIds;
  vector<double> freeRatio;  // free/frozen MMG input; -1 if no frozen vertices or extraction failed
  vector<std::array<i64, 2>> pieceVerts;  // free, frozen
  string postHash;
  i64 lowFailures = 0;
};

class Driver {
 public:
  int rank = 0, size = 1;
  Params prm;
  RunOpts ro;
  Engine eng;
  Mesh input, mesh;
  Adjacency adj;
  Background bg;
  FormerFaces ff;
  vector<i64> K, W;  // E0 persists these; E3 implements their lifecycle.
  vector<Mesh> references;
  int planMismatches = 0, injectionsObserved = 0;
  vector<i64> level0Junction;  // gids
  vector<StepRecord> steps;
  std::ostringstream log;
  string status = "COMPLETE";
  string stopReason;
  double tStart = 0;
  int stepCounter = 0;

  void Setup(const Mesh& in, const Params& p);
  /*--- placement of level 0 ---*/
  vector<int> Placement();
  /*--- face-connected components of an element set (sorted), labels = smallest element key ---*/
  vector<vector<i64>> Components(const vector<i64>& elems) const;
  /*--- pinch detection and exclusion (plan 6.7) ---*/
  void RepairPinches(vector<i64>& elems, vector<i64>& excluded) const;
  void DetectPinches(vector<i64>& elems, std::set<i64>& pinchV, std::set<std::pair<i64, i64>>& pinchE) const;
  void FillPinches(vector<i64>& elems, vector<int>& owner, int me) const;
  /*--- pinch repair of the pieces of a step at levels >= 1: growth, then exclusion ---*/
  void RepairStep(vector<PieceIn>& pieces0, vector<PieceIn>& pieces, vector<i64>& excluded) const;
  /*--- elements within graph distance L-1 of the seed vertices ---*/
  vector<i64> WorkSet(const vector<i64>& seeds, int L) const;
  /*--- seeds plus a surface ring around the vertices of PENDING faces (closure of 2.2 / plan 6.9.6) ---*/
  vector<i64> SurfaceExpand(const vector<i64>& seeds) const;
  vector<double> PredOut() const;  // predicted output elements per element of the current mesh
  /*--- one transaction; returns false if the step was rolled back as a whole ---*/
  bool Transaction(vector<PieceIn>& pieces, StepRecord& rec, vector<char>& accepted);
  /*--- coverage / quality sets ---*/
  vector<i64> CoverageSet() const;
  vector<i64> DefectSet() const;
  void UpdateDefects();
  /*--- schedulers ---*/
  void Level0();
  void SBase(int firstLevel);
  void SB4();
  void SBatch();
  vector<PieceIn> AssignWhole(vector<vector<i64>>& comps, double capMem, StepRecord& rec,
                              const vector<char>* seedVert, int forceCut);
  void Finish();
  uint64_t StateHash() const;
  uint64_t PlanHash(const vector<PieceIn>& pieces) const;
  void SaveState(const string& file, const vector<PieceIn>& pieces, const StepRecord& rec) const;
  void LoadState(const string& file, vector<PieceIn>& pieces, StepRecord& rec);
  void WriteArtifacts(const string& prefix, bool includeReferences = true) const;
  double Work(const vector<i64>& elems, const vector<double>& pred) const;
  vector<char> lastStageFail;
};

void E0SelfTest();

int RunMain(int argc, char** argv);

}  // namespace b0
