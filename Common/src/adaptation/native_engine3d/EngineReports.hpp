// Private translation-unit reporting helpers; borrow canonical context state and never own it.
#pragma once
#include "EngineContext.hpp"

// The sole original fixed-sample count moved from Adapt's local scope for the reports methods.
constexpr size_t FixedMetricSamples = 16;

struct EngineReports {
  static void RecordRejection(EngineContext&, size_t diagnostic_rejection_limit, Feature, const Patch*,
                              const MetricDiagnosis*, const std::string&);
  static void ConsiderFixedMetricRow(EngineContext&, const OwnedCell&);
  static void UpdateFixedMetricCells(EngineContext&, const std::vector<Dependency>&, const std::vector<OwnedCell>&);
  static void ResetFixedMetricCells(EngineContext&);

  static void InitializeFixedMetricDrift(EngineContext&, bool& available);
  static void ReadFixedMetricDrift(EngineContext&, std::array<FixedMetricReading, FixedMetricSamples>& selected,
                                   std::array<bool, FixedMetricSamples>& have, bool& available);
  static void CompareFixedMetricDrift(EngineContext&,
                                      const std::array<FixedMetricReading, FixedMetricSamples>& selected,
                                      const std::array<bool, FixedMetricSamples>& have, size_t& supported,
                                      size_t& owned, long double& maximum, long double& local_maximum);
  static void EmitFixedMetricDrift(EngineContext&, const char* stage);
  static void CollectQualityCensus(Selection&, std::array<uint64_t, 2>&, std::array<uint64_t, 2>&,
                                   std::array<uint64_t, 3>&, std::array<uint64_t, 3>&, std::array<uint64_t, 6>&,
                                   std::array<uint64_t, 6>&, std::array<uint64_t, 6>&, std::array<uint64_t, 8>&,
                                   uint64_t&, uint64_t&, uint64_t&, uint64_t&, uint64_t&, uint64_t&, uint64_t&);
  static void PrintQualityCensus(EngineContext&, const char*, const StageProgress&, double, double, uint64_t,
                                 const std::array<uint64_t, 2>&, const std::array<uint64_t, 2>&,
                                 const std::array<uint64_t, 3>&, const std::array<uint64_t, 3>&,
                                 const std::array<uint64_t, 6>&, const std::array<uint64_t, 6>&,
                                 const std::array<uint64_t, 6>&, const std::array<uint64_t, 8>&, uint64_t, uint64_t,
                                 uint64_t, uint64_t, uint64_t, uint64_t, uint64_t);
  static void PrintStuckCells(EngineContext&, const char* stage);
  static void EmitLengthPass(EngineContext&, SU2NativeGeometry::AtlasBudget&);
  static void EmitQualityCensus(EngineContext&, const char*, const StageProgress&);
  static void EmitStageRows(EngineContext&, const char*, const StageProgress&, const BoundaryParameterState*,
                            SU2NativeGeometry::AtlasBudget&);
};
