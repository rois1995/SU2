/*!
 * \file CNativeRemesher.cpp
 * \brief Native 2D adaptation of actual distributed SU2 geometry and nodal metrics.
 * \version 8.5.0 "Harrier"
 * Copyright 2012-2026, SU2 Contributors (cf. AUTHORS.md)
 * SU2 is free software under the GNU Lesser General Public License, version 2.1 or later.
 */

#include "../../include/adaptation/CNativeRemesher.hpp"
#include "../../include/adaptation/CNativeEngine2D.hpp"
#include "../../include/adaptation/CNativePartition2D.hpp"
#include "../../include/adaptation/CNativeReferenceIO.hpp"
#include "../../include/adaptation/CMeshGather.hpp"
#include "../../include/CConfig.hpp"
#include "../../include/geometry/CGeometry.hpp"
#include <iostream>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <thread>
#include <chrono>
#include <ctime>

using namespace SU2NativeBoundary2D;

CNativeRemesher::CNativeRemesher(std::shared_ptr<ReferenceState> retainedReference,
                                 CompositionFactory geometricConstraints)
    : compositionFactory(std::move(geometricConstraints)), reference(std::move(retainedReference)) {
  if (!reference) throw std::invalid_argument("Native remesher needs driver-owned reference storage.");
}

void CNativeRemesher::CheckSupport(const CConfig& config, const CGeometry& geometry) {
  CLocalFailure failure;
  if (!std::is_same<su2double, double>::value || !std::is_same<passivedouble, double>::value)
    failure.Set(1, 0, "Native adaptation currently requires a primal double-precision build.");
  if (config.GetContinuous_Adjoint() || config.GetDiscrete_Adjoint())
    failure.Set(1, 0, "Native adaptation does not support continuous or discrete adjoint configurations.");
  if (geometry.GetnDim() != 2) failure.Set(1, 0, "Native adaptation currently supports 2D triangles only.");
  if (config.GetMultizone_Problem() || config.GetnZone() > 1 || config.GetnMarker_ZoneInterface() > 0)
    failure.Set(1, 0, "Native adaptation does not support multizone or sliding meshes.");
  if (config.GetnMarker_Periodic() > 0) failure.Set(1, 0, "Native adaptation does not support periodic boundaries.");
  if (config.GetGrid_Movement() || config.GetDynamic_Grid() || config.GetDeform_Mesh())
    failure.Set(1, 0, "Native adaptation does not support moving/deforming meshes.");
  if (config.GetFEMSolver() || !config.GetMarkerCreateCopy().empty())
    failure.Set(1, 0, "Native adaptation does not support FEM or MARKER_CREATE_COPY.");
  if (config.GetTime_Domain()) {
    const auto marching = config.GetTime_Marching();
    if (marching != TIME_MARCHING::DT_STEPPING_1ST && marching != TIME_MARCHING::DT_STEPPING_2ND)
      failure.Set(1, 0, "Native time-domain adaptation requires first- or second-order dual time stepping.");
    const auto method = config.GetKind_Adap_Unsteady_Metric();
    if (method != ADAP_UNSTEADY_METRIC::WINDOW_AVERAGE && method != ADAP_UNSTEADY_METRIC::PREDICT)
      failure.Set(1, 0, "Native time-domain adaptation supports WINDOW_AVERAGE and PREDICT; FIXED_POINT remains unsupported.");
  }
  for (unsigned long e = 0; e < geometry.GetnElem(); ++e)
    if (geometry.elem[e]->GetVTK_Type() != TRIANGLE)
      failure.Set(1, e, "Native adaptation currently rejects mixed or non-triangle volume cells.");
  for (unsigned short m = 0; m < geometry.GetnMarker(); ++m) {
    switch (config.GetMarker_All_KindBC(m)) {
      case PERIODIC_BOUNDARY:
      case ACTDISK_INLET:
      case ACTDISK_OUTLET:
      case NEARFIELD_BOUNDARY:
      case FLUID_INTERFACE:
        failure.Set(1, m, "Native adaptation rejects periodic/paired physical markers.");
        break;
      default:
        break;
    }
    if (config.GetMarker_All_TagBound(m) == "SEND_RECEIVE") continue;
    for (unsigned long e = 0; e < geometry.GetnElem_Bound(m); ++e) {
      const auto type = geometry.bound[m][e]->GetVTK_Type();
      if (type != LINE && type != VERTEX) failure.Set(1, m, "Native adaptation needs line physical faces.");
    }
  }
  for (unsigned short b = 0; b < config.GetnAdap_BL(); ++b)
    for (unsigned short m = 0; m < geometry.GetnMarker(); ++m)
      if (config.GetMarker_All_TagBound(m) == config.GetAdap_BL(b).marker && !config.GetSolid_Wall(m))
        failure.Set(1, b, "ADAP_BL_MARKER: " + config.GetAdap_BL(b).marker + " is not a wall marker.");
  CollectiveFailure(failure, CURRENT_FUNCTION);
}

namespace {
std::map<Id, Cell> ImportNative(const CConfig& config, const CGeometry& geometry, const su2activematrix* metric,
                                ReferenceState& state) {
  World world;
  CLocalFailure failure;
  const auto simplices = OwnedSimplices(geometry);
  std::vector<std::string> tags;
  for (unsigned short m = 0; m < geometry.GetnMarker(); ++m) tags.push_back(config.GetMarker_All_TagBound(m));
  const auto names = CMeshGather::GatherMarkerNames(config, tags, geometry);
  const auto boundaryFaces = OwnedBoundaryFaces(geometry, tags, config);
  std::map<Id, Cell> input;
  {
    // Freeze owner records, including halo tensor values fetched from their true CFD point owners.
    // This directory is distributed; sparse temporary remesher IDs are introduced only after import.
    CPointDirectory points;
    std::vector<uint64_t> gids;
    std::vector<char> records;
    gids.reserve(geometry.GetnPointDomain());
    records.reserve(geometry.GetnPointDomain() * 40);
    for (unsigned long p = 0; p < geometry.GetnPointDomain(); ++p) {
      const auto gid = geometry.nodes->GetGlobalIndex(p);
      gids.push_back(gid);
      std::array<double, 5> values{
          SU2_TYPE::GetValue(geometry.nodes->GetCoord(p, 0)), SU2_TYPE::GetValue(geometry.nodes->GetCoord(p, 1)),
          metric ? SU2_TYPE::GetValue((*metric)(p, 0)) : 1., metric ? SU2_TYPE::GetValue((*metric)(p, 1)) : 0.,
          metric ? SU2_TYPE::GetValue((*metric)(p, 2)) : 1.};
      if (!std::isfinite(values[0]) || !std::isfinite(values[1]) ||
          !(NormalizedDeterminant(values[2], values[3], values[4]) > 1e-14L))
        failure.Set(1, gid, "Nonfinite coordinate or target outside the native SPD guard.");
      RecordStream encode(records);
      encode(values);
    }
    CollectiveFailure(failure, CURRENT_FUNCTION);
    points.Build(gids, records, 40, "native frozen donor points");
    gids.clear();
    gids.reserve(geometry.GetnPoint());
    for (unsigned long p = 0; p < geometry.GetnPoint(); ++p) gids.push_back(geometry.nodes->GetGlobalIndex(p));
    const auto fetched = points.Fetch(gids);
    const auto prefix = CPassiveComm::ExscanSum(simplices.size());
    for (unsigned long e = 0; e < simplices.size(); ++e) {
      Cell cell;
      cell.t.id = prefix + e;
      for (int k = 0; k < 3; ++k) {
        const auto p = simplices.nodes[3 * e + k];
        const char* data = fetched.data() + 40 * p;
        const auto x = GetBytes<double>(data), y = GetBytes<double>(data);
        cell.t.v[k] = {gids[p], {x, y}, 0};
        cell.nodal_target[k] = {GetBytes<double>(data), GetBytes<double>(data), GetBytes<double>(data)};
        if (x != SU2_TYPE::GetValue(geometry.nodes->GetCoord(p, 0)) ||
            y != SU2_TYPE::GetValue(geometry.nodes->GetCoord(p, 1)))
          failure.Set(1, gids[p], "Native import found inconsistent CFD halo coordinates.");
      }
      if (area(cell.t) < 0) {
        std::swap(cell.t.v[1], cell.t.v[2]);
        std::swap(cell.nodal_target[1], cell.nodal_target[2]);
      }
      input.emplace(cell.t.id, cell);
    }
  }
  CollectiveFailure(failure, CURRENT_FUNCTION);
  std::vector<BoundaryLabel> labels;
  for (unsigned long f = 0; f < boundaryFaces.size(); ++f) {
    const auto name = config.GetMarker_CfgFile_TagBound(boundaryFaces.markerId[f]);
    const auto marker = std::find(names.begin(), names.end(), name) - names.begin();
    labels.push_back({geometry.nodes->GetGlobalIndex(boundaryFaces.nodes[2 * f]),
                      geometry.nodes->GetGlobalIndex(boundaryFaces.nodes[2 * f + 1]), int(marker)});
  }
  Associate(world, input, labels, names, SU2_TYPE::GetValue(config.GetAdap_Angle()), state);
  return input;
}
struct PendingBindings {
  std::map<FaceCoordinates, int> edges;
  bool published = false;
};
struct RejectionCount {
  std::array<char, 128> reason{};
  uint64_t count = 0;
  template <class S>
  void Fields(S& s) {
    s(reason, count);
  }
};
struct RejectedLocation {
    uint64_t id = 0;
    int rank = 0;
    double x = 0, y = 0, quality = 0, length = 0;
    template <class S> void Fields(S& s) { s(id, rank, x, y, quality, length); }
};

/* Diagnostic-only output: stream one rank's text at a time, without rebuilding
 * geometry, transferring solution, or publishing reference bindings.
 * ponytail: text buffers scale with one rank's slice; chunk formatting if this
 * diagnostic becomes too large. The complete volume mesh is never gathered. */
void WriteRejectedCandidate(const CConfig& config, const CReaderSlices& slices, World& world, const std::map<Id, Cell>& owned,
                            const ReferenceState& reference, unsigned long attempt) {
  const auto base = CConfig::GetAdap_FileName(config.GetMesh_Out_FileName(), attempt) + "_rejected";
  const auto rank = world.rank;
  std::ofstream mesh, report;
  CLocalFailure failure;
  if (rank == 0) {
    if (std::ifstream(base + ".su2").good() || std::ifstream(base + "_failures.csv").good())
      failure.Set(1, 0, "Refusing to overwrite rejected-mesh diagnostics: " + base);
    else {
      mesh.open(base + ".su2");
      report.open(base + "_failures.csv");
      if (!mesh || !report) failure.Set(1, 0, "Cannot open rejected-mesh diagnostics: " + base);
    }
  }
  CollectiveFailure(failure, CURRENT_FUNCTION);
  auto stream = [&](std::ostream& output, const std::string& local) {
    for (int sender = 0; sender < world.size; ++sender) {
      if (sender == 0 && rank == 0) output << local;
      else if (rank == sender) CPassiveComm::SendRounds(local.data(), local.size(), 0, 19127);
      else if (rank == 0) {
        const auto bytes = CPassiveComm::RecvRounds(sender, 19127);
        output.write(bytes.data(), bytes.size());
      }
    }
  };
  std::ostringstream text;
  text << std::setprecision(17);
  unsigned long localElements = 0;
  for (unsigned long e = 0; e < slices.nElemLocal; ++e) {
    const auto* row = slices.elemRows.data() + e * SU2_CONN_SIZE;
    const auto lowest = std::min({row[2], row[3], row[4]});
    // A reader element is repeated only on slices containing one of its nodes.
    if (lowest < slices.firstPoint || lowest >= slices.firstPoint + slices.nPointLocal) continue;
    text << TRIANGLE << ' ' << row[2] << ' ' << row[3] << ' ' << row[4] << ' ' << row[0] << '\n';
    ++localElements;
  }
  if (CPassiveComm::AllreduceSum(localElements) != slices.nElemGlobal)
    failure.Set(1, 0, "Rejected diagnostic does not own every volume element exactly once.");
  CollectiveFailure(failure, CURRENT_FUNCTION);
  if (rank == 0)
    mesh << "% Rejected candidate: no CFD solution has been transferred to this mesh.\nNDIME= 2\nNELEM= "
         << slices.nElemGlobal << '\n';
  stream(mesh, text.str());
  text.str(""); text.clear();
  for (unsigned long p = 0; p < slices.nPointLocal; ++p)
    text << slices.coord[0][p] << ' ' << slices.coord[1][p] << ' ' << slices.firstPoint + p << '\n';
  if (rank == 0) mesh << "NPOIN= " << slices.nPointGlobal << '\n';
  stream(mesh, text.str());
  if (rank == 0) {
    mesh << "NMARK= " << slices.markerNames.size() << '\n';
    for (size_t m = 0; m < slices.markerNames.size(); ++m) {
      const auto& rows = slices.boundaryRows[m];
      mesh << "MARKER_TAG= " << slices.markerNames[m] << "\nMARKER_ELEMS= " << rows.size() / SU2_CONN_SIZE << '\n';
      for (size_t i = 0; i < rows.size(); i += SU2_CONN_SIZE)
        mesh << LINE << ' ' << rows[i + 2] << ' ' << rows[i + 3] << '\n';
    }
    report << "rank,native_cell_id,quality,max_metric_edge,shape_failure,length_failure,height_failure,reference_failure,"
              "centroid_x,centroid_y,x0,y0,x1,y1,x2,y2,max_relative_height_error,max_reference_deviation\n";
  }
  text.str(""); text.clear();
  std::vector<RejectedLocation> locations;
  const auto policy = reference.original->Policy({});
  for (const auto& entry : owned) {
    const auto& c = entry.second;
    double heightError = 0, deviation = 0;
    for (int k = 0; k < 3; ++k) if (c.marker[k]) {
      const auto a = c.t.v[k], b = c.t.v[(k + 1) % 3];
      deviation = std::max(deviation, policy.deviation({a, b, c.marker[k]}));
      const auto marker = reference.marker_names[reference.original->Marker(c.marker[k])];
      for (unsigned short layer = 0; layer < config.GetnAdap_BL(); ++layer)
        if (config.GetAdap_BL(layer).marker == marker) {
          const auto h = SU2_TYPE::GetValue(config.GetAdap_BL(layer).firstHeight);
          heightError = std::max(heightError, std::abs(static_cast<double>(2 * area(c.t)) / norm(b.p - a.p) / h - 1));
        }
    }
    const auto q = c.target_cache[0];
    const auto length = *std::max_element(c.target_cache.begin() + 1, c.target_cache.end());
    const bool shape = q < .18, size = length > 1.8, height = heightError > 1e-8,
               geometry = deviation > SU2_TYPE::GetValue(config.GetAdap_Hausd());
    if (!(shape || size || height || geometry)) continue;
    const auto center = (c.t.v[0].p + c.t.v[1].p + c.t.v[2].p) * (1. / 3);
    text << rank << ',' << entry.first << ',' << q << ',' << length << ',' << shape << ',' << size << ','
         << height << ',' << geometry << ',' << center.x << ',' << center.y;
    locations.push_back({entry.first, rank, center.x, center.y, q, length});
    std::sort(locations.begin(), locations.end(), [](const auto& a, const auto& b) { return a.quality < b.quality; });
    if (locations.size() > 8) locations.resize(8);
    for (const auto& node : c.t.v) text << ',' << node.p.x << ',' << node.p.y;
    text << ',' << heightError << ',' << deviation << '\n';
  }
  stream(report, text.str());
  auto examples = world.metadata(locations);
  if (rank == 0) {
    std::sort(examples.begin(), examples.end(), [](const auto& a, const auto& b) { return a.quality < b.quality; });
    for (size_t i = 0; i < std::min(size_t(8), examples.size()); ++i) {
      const auto& row = examples[i];
      const Point center{row.x, row.y};
      double distance = std::numeric_limits<double>::infinity();
      int marker = -1;
      for (size_t component = 0; component < reference.original->Components().size(); ++component) {
        const auto id = int(component + 1);
        const auto error = norm(center - reference.original->At(id, reference.original->Parameter(id, center)));
        if (error < distance) { distance = error; marker = reference.original->Marker(id); }
      }
      std::cout << "Native failure location: rank=" << row.rank << ", native cell=" << row.id
                << ", centroid=(" << row.x << ", " << row.y << "), q=" << row.quality
                << ", Lmax=" << row.length << ", nearest boundary=" << reference.marker_names.at(marker)
                << ", distance=" << distance << '\n';
    }
    mesh.close(); report.close();
    if (!mesh || !report) failure.Set(1, 0, "Failed writing rejected-mesh diagnostics: " + base);
    else std::cout << "Rejected candidate saved: " << base << ".su2; failure coordinates and frozen-target values: "
                   << base << "_failures.csv. Native cell IDs differ from the exported reader IDs; match coordinates.\n";
  }
  CollectiveFailure(failure, CURRENT_FUNCTION);
}

}  // namespace

void CNativeRemesher::PrepareReference(const CConfig& config, const CGeometry& geometry) {
  CheckSupport(config, geometry);
  if (reference->original) return;
  const auto filename = config.GetMesh_FileName() + ".native_ref";
  const bool loaded = LoadReference(filename, *reference);
  CLocalFailure failure;
  if (config.GetRestart() && !loaded)
    failure.Set(1, 0, "Native solution restart requires the original geometry sidecar: " + filename);
  CollectiveFailure(failure, CURRENT_FUNCTION);
  ImportNative(config, geometry, nullptr, *reference);  // Geometry preflight before solving; no target yet.
}

CRemeshResult CNativeRemesher::Remesh(const CConfig& config, const CGeometry& geometry, const su2activematrix& metric) {
  const auto remeshStarted = SU2_MPI::Wtime();
  CheckSupport(config, geometry);
  ++remeshAttempt;
  World world;
  CLocalFailure failure;
  if (metric.rows() < geometry.GetnPointDomain() || metric.cols() != 3)
    failure.Set(1, 0, "Native adaptation needs three metric values at every owned point.");
  if (config.GetAdap_Native_Ranks() > static_cast<unsigned long>(world.size))
    failure.Set(1, 0, "ADAP_NATIVE_RANKS exceeds the CFD communicator size.");
  CollectiveFailure(failure, CURRENT_FUNCTION);
  const int adaptationRanks = config.GetAdap_Native_Ranks() ? int(config.GetAdap_Native_Ranks()) : world.size;
  PrepareReference(config, geometry);
  auto input = ImportNative(config, geometry, &metric, *reference);
  const auto importedAt = world.seconds();
  const auto& names = reference->marker_names;
  std::map<int, double> heights;
  for (unsigned short b = 0; b < config.GetnAdap_BL(); ++b) {
    const auto& layer = config.GetAdap_BL(b);
    const auto marker = std::find(names.begin(), names.end(), layer.marker);
    if (marker == names.end())
      failure.Set(1, b, "Native BL marker has no physical faces: " + layer.marker);
    else
      heights.emplace(marker - names.begin(), SU2_TYPE::GetValue(layer.firstHeight));
  }
  CollectiveFailure(failure, CURRENT_FUNCTION);
  const auto policy = reference->original->Policy(heights);
  for (auto& entry : input)
    for (int k = 0; k < 3; ++k)
      if (entry.second.marker[k]) {
        auto& cell = entry.second;
        const auto h = policy.height(cell.marker[k]);
        if (h > 0 &&
            std::abs(static_cast<double>(2 * area(cell.t)) / norm(cell.t.v[(k + 1) % 3].p - cell.t.v[k].p) / h - 1) <
                1e-8)
          cell.t.protected_cell = 1;
      }
  EngineOptions control;
  if (compositionFactory) control.metric_composition = compositionFactory(config, geometry, metric);
  // Account for refinement demand as well as input size. Original frozen P1
  // tensors are averaged at donor centroids; no tensor/target is altered.
  long double localComplexity = 0;
  for (const auto& entry : input) {
    const auto& cell = entry.second;
    Tensor m{0, 0, 0};
    for (const auto& node : cell.nodal_target) {
      m.xx += node.xx / 3;
      m.xy += node.xy / 3;
      m.yy += node.yy / 3;
    }
    const auto scale = std::max({std::abs(m.xx), std::abs(m.xy), std::abs(m.yy)});
    localComplexity += area(cell.t) * scale * std::sqrt(NormalizedDeterminant(m.xx, m.xy, m.yy));
  }
  CLocalFailure workFailure;
  if (!(std::isfinite(localComplexity) && localComplexity >= 0 &&
        localComplexity <= std::numeric_limits<double>::max()))
    workFailure.Set(1, 0, "Unrepresentable native frozen-target work estimate.");
  CollectiveFailure(workFailure, CURRENT_FUNCTION);
  const auto complexity = CPassiveComm::Allreduce(static_cast<double>(localComplexity), CPassiveComm::Op::SUM);
  if (!std::isfinite(complexity)) workFailure.Set(1, 0, "Unrepresentable global native work estimate.");
  CollectiveFailure(workFailure, CURRENT_FUNCTION);
  const auto globalCells = CPassiveComm::Allreduce(uint64_t(input.size()), CPassiveComm::Op::SUM);
  // A unit-edge equilateral metric triangle has area sqrt(3)/4. This is a
  // work estimate, not a requested cell count or a guarantee of completion.
  const long double sensorCells = 4 * static_cast<long double>(complexity) / std::sqrt(3.L);
  long double layerCells = 0;
  std::map<int, double> wallRows;
  if (control.metric_composition)
    for (unsigned short b = 0; b < config.GetnAdap_BL(); ++b) {
      const auto& layer = config.GetAdap_BL(b);
      const long double h = SU2_TYPE::GetValue(layer.firstHeight), g = SU2_TYPE::GetValue(layer.growth),
                        thickness = SU2_TYPE::GetValue(layer.thickness);
      // Approximate two triangles per geometric row and original wall edge.
      // This accounts for BL work without coarse-area weighting of a pointwise
      // wall tensor. Corners/rotated sensor intersections can need more work;
      // the existing ceiling still bounds each phase, not guarantees completion.
      const long double rows = g == 1 ? thickness / h : std::log1p((g - 1) * thickness / h) / std::log(g);
      const auto& components = reference->original->Components();
      for (size_t c = 0; c < components.size(); ++c)
        if (names.at(components[c].marker) == layer.marker) {
          layerCells += 2 * (components[c].nodes.size() - 1) * std::ceil(rows);
          wallRows.emplace(c + 1, double(std::ceil(rows)));
        }
    }
  const long double estimatedCells = sensorCells + layerCells;
  if (!(std::isfinite(estimatedCells) && estimatedCells >= 0))
    workFailure.Set(1, 0, "Unrepresentable geometric BL work allowance.");
  CollectiveFailure(workFailure, CURRENT_FUNCTION);
  const auto rounds = std::ceil(std::max(static_cast<long double>(globalCells), estimatedCells) / (4 * adaptationRanks));
  control.phase_rounds = int(std::min(20000.L, std::max(300.L, rounds)));
  control.geometry_tolerance = SU2_TYPE::GetValue(config.GetAdap_Hausd());
  control.fixed_boundary = !config.GetAdap_Surface();
  if (world.rank == 0)
    std::cout << "Native contract: q>=0.18, frozen-target Simpson edge length<=1.8, reference deviation<="
              << control.geometry_tolerance
              << ", requested wall altitude relative error<=1e-8; dependency budget=" << control.dependency_bytes
              << " bytes per rank, original P1 sensor target"
              << (control.metric_composition ? " + geometric BL constraints" : "") << ", boundary sampling "
              << (control.fixed_boundary ? "fixed" : "adaptive") << ".\n";
  if (world.rank == 0)
    std::cout << "Native work allowance: " << control.phase_rounds << " collective rounds per phase, "
              << control.sweeps << " sweeps, for " << globalCells << " input cells on " << adaptationRanks
              << " ranks; frozen P1 centroid complexity=" << complexity << ", estimated unit triangles="
              << estimatedCells << " (geometric BL row allowance=" << layerCells << ").\n";
  if (world.rank == 0)
    for (const auto& height : heights)
      std::cout << "Native requested first altitude: " << names[height.first] << " = " << height.second << '\n';
  const auto preparedAt = world.seconds();
  if (config.GetAdap_Native_Repartition() || adaptationRanks < world.size) {
    PartitionStats partition;
    std::vector<uint32_t> weights;
    const auto started = world.seconds();
    try {
      weights = WorkWeights(input, control.metric_composition, wallRows);
    } catch (const std::exception& error) {
      failure.Set(1, 0, error.what());
    }
    CollectiveFailure(failure, CURRENT_FUNCTION);
    const auto estimateSeconds = world.seconds() - started;
    if (!Repartition(world, input, weights, adaptationRanks, partition))
      failure.Set(1, 0, "Native working partition migration was not admitted.");
    CollectiveFailure(failure, CURRENT_FUNCTION);
    const std::array<double, 4> localSeconds{estimateSeconds, partition.graph_seconds,
        partition.partition_seconds, partition.migration_seconds};
    std::array<double, 4> maximumSeconds{};
    CPassiveComm::Allreduce(localSeconds.data(), maximumSeconds.data(), localSeconds.size(), CPassiveComm::Op::MAX);
    const auto bytes = CPassiveComm::Allreduce(uint64_t(partition.migration_bytes), CPassiveComm::Op::MAX);
    if (world.rank == 0) {
      std::cout << "Native working partition: weighted, CFD ranks=" << world.size
                << ", adaptation ranks=" << adaptationRanks << "; moved cells=" << partition.moved_cells
                << ", dual edge cuts=" << partition.edgecut << ", largest transport/staging estimate=" << bytes << '\n';
      std::cout << "Native working partition seconds estimate/graph/ParMETIS/migration (maximum across ranks):";
      for (const auto value : maximumSeconds) std::cout << ' ' << value;
      std::cout << '\n';
      for (int r = 0; r < world.size; ++r)
        std::cout << "Native working rank " << r << " cells before/after=" << partition.cells_before[r]
                  << '/' << partition.cells_after[r]
                  << ", predicted work before/after=" << partition.work_before[r] << '/'
                  << partition.work_after[r] << '\n';
    }
  }
  const auto partitionedAt = world.seconds();
  CRemeshResult result;
  auto workerComm = world.comm;
#ifdef HAVE_MPI
  if (adaptationRanks < world.size)
    MPI_Comm_split(world.comm, world.rank < adaptationRanks ? 0 : MPI_UNDEFINED, world.rank, &workerComm);
#endif
  const auto workerStarted = world.seconds();
  if (world.rank < adaptationRanks) {
    World workers(workerComm);
    try {
      Engine engine(workers, std::move(input), policy, control);
      const auto engineReadyAt = workers.seconds();
      uint64_t initialShape = 0, initialSize = 0;
      for (const auto& entry : engine.owned) {
        initialShape += entry.second.target_cache[0] < .18;
        initialSize += *std::max_element(entry.second.target_cache.begin() + 1, entry.second.target_cache.end()) > 1.8;
      }
      initialShape = CPassiveComm::Allreduce(initialShape, CPassiveComm::Op::SUM, workers.comm);
      initialSize = CPassiveComm::Allreduce(initialSize, CPassiveComm::Op::SUM, workers.comm);
      if (workers.rank == 0)
        std::cout << "Native incoming residuals: shape=" << initialShape << ", edge length=" << initialSize << '\n';
      const auto adaptStarted = workers.seconds();
      engine.adapt();
      const auto adaptedAt = workers.seconds();
      uint64_t missedQuality = 0, missedHeight = 0, missedGeometry = 0, missedShape = 0, missedSize = 0;
      double minQuality = 1, maxLength = 0;
      for (const auto& entry : engine.owned) {
        const auto& cell = entry.second;
        minQuality = std::min(minQuality, cell.target_cache[0]);
        const auto largest = *std::max_element(cell.target_cache.begin() + 1, cell.target_cache.end());
        maxLength = std::max(maxLength, largest);
        missedQuality += cell.target_cache[0] < .18 || largest > 1.8;
        missedShape += cell.target_cache[0] < .18;
        missedSize += largest > 1.8;
        for (int k = 0; k < 3; ++k)
          if (cell.marker[k]) {
            const auto a = cell.t.v[k], b = cell.t.v[(k + 1) % 3];
            const auto h = policy.height(cell.marker[k]);
            missedGeometry += policy.deviation({a, b, cell.marker[k]}) > control.geometry_tolerance;
            if (h > 0) missedHeight += std::abs(static_cast<double>(2 * area(cell.t)) / norm(b.p - a.p) / h - 1) > 1e-8;
          }
      }
      missedQuality = CPassiveComm::Allreduce(missedQuality, CPassiveComm::Op::SUM, workers.comm);
      missedHeight = CPassiveComm::Allreduce(missedHeight, CPassiveComm::Op::SUM, workers.comm);
      missedGeometry = CPassiveComm::Allreduce(missedGeometry, CPassiveComm::Op::SUM, workers.comm);
      missedShape = CPassiveComm::Allreduce(missedShape, CPassiveComm::Op::SUM, workers.comm);
      missedSize = CPassiveComm::Allreduce(missedSize, CPassiveComm::Op::SUM, workers.comm);
      minQuality = CPassiveComm::Allreduce(minQuality, CPassiveComm::Op::MIN, workers.comm);
      maxLength = CPassiveComm::Allreduce(maxLength, CPassiveComm::Op::MAX, workers.comm);
      result.status = missedHeight || missedGeometry
                          ? CRemeshResult::Status::INCOMPLETE_COVERAGE
                          : missedQuality ? CRemeshResult::Status::INCOMPLETE_QUALITY : CRemeshResult::Status::COMPLETE;
      const auto checkedAt = workers.seconds();
      const auto commits = workers.sum(engine.stats.commits), cross = workers.sum(engine.stats.cross_rank);
      const auto jointCommits = workers.sum(engine.stats.joint_commits);
      const auto jointSeconds = CPassiveComm::Allreduce(engine.stats.joint_seconds, CPassiveComm::Op::MAX, workers.comm);
      const auto rejectedSize = workers.sum(engine.stats.size_rejected),
                 rejectedMemory = workers.sum(engine.stats.memory_rejected),
                 rejectedStale = workers.sum(engine.stats.stale_rejected), conflicts = workers.sum(engine.stats.conflicts);
      const auto bytes = CPassiveComm::Allreduce(uint64_t(workers.bytes_sent), CPassiveComm::Op::SUM, workers.comm);
      const auto transportEstimate =
          CPassiveComm::Allreduce(uint64_t(workers.max_exchange_work_bytes), CPassiveComm::Op::MAX, workers.comm);
      std::array<uint64_t, 3> fieldQueries;
      CPassiveComm::Allreduce(engine.stats.field_queries.data(), fieldQueries.data(), fieldQueries.size(),
                             CPassiveComm::Op::SUM, workers.comm);
      const std::array<uint64_t, 2> localSearch{engine.donor.search_candidates, engine.donor.full_scan_equivalent};
      std::array<uint64_t, 2> donorSearch{};
      CPassiveComm::Allreduce(localSearch.data(), donorSearch.data(), localSearch.size(), CPassiveComm::Op::SUM, workers.comm);
      const auto donorIndexBytes = CPassiveComm::Allreduce(uint64_t(engine.donor.SearchBytes()), CPassiveComm::Op::MAX, workers.comm);
      std::array<uint64_t, 8> actions, selectionScans;
      std::array<double, 8> phaseSeconds, choiceSeconds;
      for (size_t a = 0; a < actions.size(); ++a) {
        actions[a] = CPassiveComm::Allreduce(uint64_t(engine.stats.accepted[a]), CPassiveComm::Op::SUM, workers.comm);
        phaseSeconds[a] = CPassiveComm::Allreduce(engine.stats.phase_seconds[a], CPassiveComm::Op::MAX, workers.comm);
        choiceSeconds[a] = CPassiveComm::Allreduce(engine.stats.choice_seconds[a], CPassiveComm::Op::MAX, workers.comm);
        selectionScans[a] = CPassiveComm::Allreduce(engine.stats.selection_scans[a], CPassiveComm::Op::SUM, workers.comm);
      }
      std::vector<RejectionCount> localReasons;
      for (const auto& entry : engine.stats.rejected) {
        RejectionCount record;
        std::copy_n(entry.first.begin(), std::min(entry.first.size(), record.reason.size() - 1), record.reason.begin());
        record.count = entry.second;
        localReasons.push_back(record);
      }
      const auto allReasons = workers.metadata(localReasons);
      // Whole-operation timers avoid adding clocks to the millions of private
      // metric queries. Tracked collectives include waiting and exclude the
      // separate validation elections; these components are not additive wall time.
      std::array<double, 15> localCost{0, engine.stats.reconstruction_seconds,
          workers.collective_seconds, engine.stats.reconstruction_longest};
      std::copy(engine.stats.transaction_seconds.begin(), engine.stats.transaction_seconds.end(), localCost.begin() + 4);
      localCost[10] = engineReadyAt - workerStarted;
      localCost[11] = adaptStarted - engineReadyAt;
      localCost[12] = adaptedAt - adaptStarted;
      localCost[13] = checkedAt - adaptedAt;
      for (const auto seconds : engine.stats.choice_seconds) localCost[0] += seconds;
      localCost[14] = localCost[12] - localCost[0];
      for (const auto seconds : engine.stats.transaction_seconds) localCost[14] -= seconds;
      std::array<double, 15> costMinimum{}, costSum{}, costMaximum{};
      CPassiveComm::Allreduce(localCost.data(), costMinimum.data(), localCost.size(), CPassiveComm::Op::MIN, workers.comm);
      CPassiveComm::Allreduce(localCost.data(), costSum.data(), localCost.size(), CPassiveComm::Op::SUM, workers.comm);
      CPassiveComm::Allreduce(localCost.data(), costMaximum.data(), localCost.size(), CPassiveComm::Op::MAX, workers.comm);
      std::map<std::string, uint64_t> reasons;
      for (const auto& entry : allReasons) reasons[entry.reason.data()] += entry.count;
      if (workers.rank == 0)
        std::cout << "Native adaptation: " << commits << " commits (" << cross << " across owners), qmin=" << minQuality
                  << ", Lmax=" << maxLength << ", residual cells=" << missedQuality << ", height faces=" << missedHeight
                  << ", reference faces=" << missedGeometry << ". No remesher-side target floor.\n";
      if (workers.rank == 0)
        std::cout << "Native remaining residuals: shape=" << missedShape << ", edge length=" << missedSize << '\n';
      if (workers.rank == 0) {
        std::cout << "Native coordinated repair: " << jointCommits << " commits, " << jointSeconds
                  << " seconds (maximum across ranks).\n";
        std::cout << "Native private target requests/evaluations/dynamic evictions: " << fieldQueries[0] << ' '
                  << fieldQueries[1] << ' ' << fieldQueries[2] << '\n';
        std::cout << "Native donor search candidates/full-scan equivalent: " << donorSearch[0] << ' ' << donorSearch[1]
                  << "; local immutable index retained bytes (max rank): " << donorIndexBytes << '\n';
        std::cout << "Native operations (height/split/remove/redistribute/bulk remove/split/flip/move):";
        for (const auto count : actions) std::cout << ' ' << count;
        std::cout << "; conflicts=" << conflicts << ", dependency-size rejects=" << rejectedSize
                  << ", memory rejects=" << rejectedMemory << ", stale rejects=" << rejectedStale
                  << ", engine encoded bytes=" << bytes << ", largest transport admission estimate=" << transportEstimate
                  << '\n';
        std::cout << "Native phase elapsed seconds (same action order, maximum across ranks):";
        for (const auto seconds : phaseSeconds) std::cout << ' ' << seconds;
        std::cout << '\n';
        std::cout << "Native candidate selection seconds (same action order, maximum across ranks):";
        for (const auto seconds : choiceSeconds) std::cout << ' ' << seconds;
        std::cout << '\n';
        const std::array<const char*, 15> costNames{"selection", "private reconstruction", "tracked collectives",
            "largest private transaction", "round protocol", "round dependency import", "round donor import and IDs",
            "round reconstruction", "round validation", "round commit", "engine initialization",
            "incoming residual check", "engine adapt", "final contract check", "adapt unclassified"};
        for (size_t k = 0; k < costNames.size(); ++k)
          std::cout << "Native rank cost " << costNames[k] << " seconds min/mean/max: " << costMinimum[k] << ' '
                    << costSum[k] / workers.size << ' ' << costMaximum[k] << '\n';
        std::cout << "Native cost scopes: tracked collectives include waiting, exclude validation elections; "
                     "round scopes are exclusive within each rank, include MPI waits, and exclude local temporary "
                     "destruction after round return; rank maxima are not additive wall phases.\n";
        std::cout << "Native cached selection full scans (same action order, sum across ranks):";
        for (const auto scans : selectionScans) std::cout << ' ' << scans;
        std::cout << '\n';
        for (const auto& entry : reasons)
          std::cout << "Native deferred/rejected candidate: " << entry.first << " (" << entry.second << ").\n";
      }
      input = std::move(engine.owned);
    } catch (const CElectedFailure& elected) {
      // All workers have elected the same failure and unwind together. Return
      // to N before entering SU2's fatal protocol; accepted CFD state is intact.
      failure.Set(elected.severity, elected.gid, elected.message);
    } catch (const std::exception& error) {
      // Unexpected rank-local errors cannot safely unwind a collective engine.
      SU2_MPI::Error(error.what(), CURRENT_FUNCTION);
    }
  }
  const auto workerSeconds = world.seconds() - workerStarted;
  const auto waitStarted = world.seconds();
  const auto waitCpuStarted = std::clock();
#ifdef HAVE_MPI
  if (adaptationRanks < world.size) {
#if MPI_VERSION >= 3
    // ponytail: 1 ms progress polling avoids idle CFD ranks spinning; revisit
    // the interval only if measured wake-up latency affects total cost.
    MPI_Request ready;
    MPI_Ibarrier(world.comm, &ready);
    int complete = 0;
    while (!complete) {
      MPI_Test(&ready, &complete, MPI_STATUS_IGNORE);
      if (!complete) std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
#else
    MPI_Barrier(world.comm);  // MPI-2 fallback may spin on inactive ranks.
#endif
    if (world.rank < adaptationRanks) MPI_Comm_free(&workerComm);
  }
#endif
  const double waitCpu = double(std::clock() - waitCpuStarted) / CLOCKS_PER_SEC;
  const double waitSeconds = world.seconds() - waitStarted;
  CollectiveFailure(failure, CURRENT_FUNCTION);
  result.status = static_cast<CRemeshResult::Status>(CPassiveComm::Allreduce(int(result.status), CPassiveComm::Op::MAX));
  const auto returnStarted = world.seconds();
  result.slices = ReaderSlices(input, *reference);
  result.markers = result.slices.markersWithElements;
  auto pending = std::make_shared<PendingBindings>();
  pending->edges = Bindings(world, input);
  result.onAccepted = [state = reference, pending]() {
    if (!pending->published) {
      state->accepted_edges.swap(pending->edges);
      pending->published = true;
    }
  };
  const auto returnedAt = world.seconds();
  const std::array<double, 7> localExecution{workerSeconds, returnedAt - returnStarted,
      world.rank >= adaptationRanks ? waitSeconds : 0., world.rank >= adaptationRanks ? waitCpu : 0.,
      importedAt - remeshStarted, preparedAt - importedAt, partitionedAt - preparedAt};
  std::array<double, 7> maximumExecution{};
  CPassiveComm::Allreduce(localExecution.data(), maximumExecution.data(), localExecution.size(), CPassiveComm::Op::MAX);
  if (world.rank == 0) {
    std::cout << "Native execution seconds workers/return-to-CFD/idle-wall/idle-CPU (maximum across ranks):";
    for (size_t k = 0; k < 4; ++k) std::cout << ' ' << maximumExecution[k];
    std::cout << "; CFD ranks=" << world.size << ", adaptation ranks=" << adaptationRanks << '\n';
    std::cout << "Native setup seconds import-reference/target-policy/working-partition (maximum across ranks):";
    for (size_t k = 4; k < maximumExecution.size(); ++k) std::cout << ' ' << maximumExecution[k];
    std::cout << '\n';
  }
  if (result.status != CRemeshResult::Status::COMPLETE && config.GetWrt_Adap_Mesh())
    WriteRejectedCandidate(config, result.slices, world, input, *reference, remeshAttempt);
  return result;
}
