/*!
 * \file CNativeRemesher3D_tests.cpp
 * \brief Default CFD ownership and private-progress rejection in the native tetrahedral runtime.
 * Copyright 2012-2026, SU2 Contributors (cf. AUTHORS.md).
 * SU2 is free software under the GNU Lesser General Public License, version 2.1 or later.
 */
#include "TransferTestCase.hpp"
#include "../../../Common/include/adaptation/CNativeRemesher3D.hpp"
#include "../../../Common/include/adaptation/CNativeReferenceIO3D.hpp"
#include "../../../SU2_CFD/include/output/CMeshOutput.hpp"
using namespace transfer_test;
using namespace SU2Native3D;

namespace {
std::vector<double> Snapshot(const MeshSolution& mesh) {
  std::vector<double> values;
  const auto& geometry = mesh.Fine();
  const auto* nodes = mesh.solver[MESH_0][FLOW_SOL]->GetNodes();
  for (unsigned long p = 0; p < geometry.GetnPoint(); ++p) {
    values.push_back(geometry.nodes->GetGlobalIndex(p));
    for (size_t d = 0; d < 3; ++d) values.push_back(SU2_TYPE::GetValue(geometry.nodes->GetCoord(p, d)));
    for (size_t k = 0; k < 5; ++k) values.push_back(SU2_TYPE::GetValue(nodes->GetSolution(p, k)));
  }
  for (unsigned long e = 0; e < geometry.GetnElem(); ++e)
    for (size_t k = 0; k < 4; ++k) values.push_back(geometry.elem[e]->GetNode(k));
  return values;
}
}  // namespace

TEST_CASE("Native3D default ownership survives rejection after private accepted edits", "[NativeDefault3D]") {
  World world;
  auto config = MakeConfig(3,
                           "SOLVER= EULER\nCOMPUTE_METRIC= YES\nADAP_SENSOR= PRESSURE\n"
                           "ADAP_REMESHER= NATIVE_CAVITY\nADAP_SURFACE= YES\nADAP_HESSIAN_NOISE= 0\n"
                           "ADAP_NATIVE_REPARTITION= NO\nADAP_NATIVE_RANKS= 0\n");
  MeshSolution donor(config.get(), BoxMesh(3, 1, false), 0);
  const auto before = Snapshot(donor);
  su2activematrix sensor(donor.Fine().GetnPointDomain(), 6);
  for (unsigned long p = 0; p < sensor.rows(); ++p)
    for (size_t k = 0; k < 6; ++k) sensor(p, k) = (k == 0 || k == 3 || k == 5) ? 4. : 0.;
  auto reference = std::make_shared<ReferenceState>();
  EngineOptions limited;
  limited.maximum_rounds = 1;
  CNativeRemesher3D rejected(reference, limited);
  const auto bad = rejected.Remesh(*config, donor.Fine(), sensor);
  CHECK(bad.status != CRemeshResult::Status::COMPLETE);
  CHECK_FALSE(bad.onAccepted);
  CHECK_FALSE(reference->original);
  CHECK(Snapshot(donor) == before);
  CHECK(CPassiveComm::Allreduce(rejected.Statistics().refine.accepted, CPassiveComm::Op::SUM) > 0);
  CHECK(rejected.Statistics().workers == world.size);
  CHECK(rejected.Statistics().partition.destinations.empty());
  // Retry the same accepted CFD mesh and unchanged frozen sensor, without partitioning it.
  CNativeRemesher3D backend(reference);
  const auto result = backend.Remesh(*config, donor.Fine(), sensor);
  REQUIRE(result.status == CRemeshResult::Status::COMPLETE);
  CHECK_FALSE(reference->original);
  CHECK(backend.Statistics().partition.destinations.empty());
  CHECK(backend.Statistics().workers == world.size);
  CHECK(Snapshot(donor) == before);
  for (unsigned long p = 0; p < sensor.rows(); ++p)
    for (size_t k = 0; k < 6; ++k) CHECK(sensor(p, k) == ((k == 0 || k == 3 || k == 5) ? 4. : 0.));
  MeshSolution accepted(config.get(), result.slices, 0);
  const auto transported = result.slices.FetchPointMetric(accepted.Fine());
  REQUIRE(transported.size() == 6 * accepted.Fine().GetnPoint());
  for (size_t k = 0; k < transported.size(); ++k)
    CHECK(transported[k] == ((k % 6 == 0 || k % 6 == 3 || k % 6 == 5) ? 4. : 0.));
  result.onAccepted();
  REQUIRE(reference->original);
  const auto encoded = EncodeReference(*reference);
  result.onAccepted();
  CHECK(EncodeReference(*reference) == encoded);
  {
    Mute mute;
    CMeshOutput::WriteMesh(config.get(), &accepted.Fine(), "default_native3d");
  }
  WriteReference("default_native3d.su2.native_ref", *reference);
  ReferenceState reloaded;
  CHECK(LoadReference("default_native3d.su2.native_ref", reloaded));
  CHECK(EncodeReference(reloaded) == encoded);
}
