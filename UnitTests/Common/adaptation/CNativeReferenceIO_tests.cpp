/*!
 * \file CNativeReferenceIO_tests.cpp
 * \brief Persist original component/feature truth despite adapted sampling and new CFD point IDs.
 * \version 8.5.0 "Harrier"
 * Copyright 2012-2026, SU2 Contributors (cf. AUTHORS.md)
 * SU2 is free software under the GNU Lesser General Public License, version 2.1 or later.
 */

#include "catch.hpp"
#include "../../../Common/include/adaptation/CNativeReferenceIO.hpp"
#include "../../../Common/include/adaptation/CNativeFirstHeight.hpp"
#include "../../../Common/include/adaptation/CNativeReferenceIO3D.hpp"
#include "../../../Common/include/adaptation/CNativeOptimize3D.hpp"
#include <algorithm>

using namespace SU2NativeBoundary2D;

TEST_CASE("Native3D reference cache quota is shared equally within each host", "[NativeReferenceIO3D]") {
  namespace N=SU2Native3D;
  CHECK(N::ReferenceCacheRankQuota3D(size_t(120),1)==120);
  CHECK(N::ReferenceCacheRankQuota3D(size_t(120),3)==40);
  CHECK(N::ReferenceCacheRankQuota3D(size_t(120),5)==24);
  // Different shared-memory domains divide independently, even with unequal rank counts.
  const std::array<int,2> ranks{{2,5}};
  CHECK(N::ReferenceCacheRankQuota3D(1000,ranks[0])==500);
  CHECK(N::ReferenceCacheRankQuota3D(1000,ranks[1])==200);
  CHECK_THROWS(N::ReferenceCacheRankQuota3D(1000,0));
}

TEST_CASE("Native3D reference cache quota uses bounded MemAvailable with sysconf fallback", "[NativeReferenceIO3D]") {
  namespace N=SU2Native3D;
  const size_t fallback=123456;
  CHECK(N::ReferenceCacheHostQuota3D("MemTotal: 1000 kB\nMemAvailable: 4000000 kB\n",fallback)==1024000000);
  CHECK(N::ReferenceCacheHostQuota3D("MemAvailable: 20000000 kB\n",fallback)==N::ReferenceCacheHostLimit3D);
  CHECK(N::ReferenceCacheHostQuota3D("MemAvailable: 1024 kB\n",fallback)==256*1024);
  CHECK(N::ReferenceCacheHostQuota3D("MemAvailable: 0 kB\n",fallback)==0);
  CHECK(N::ReferenceCacheHostQuota3D("MemAvailable: invalid kB\n",fallback)==fallback);
  CHECK(N::ReferenceCacheHostQuota3D("MemAvailable: 18446744073709551615 kB\n",fallback)==fallback);
  CHECK(N::ReferenceCacheHostQuota3D("MemTotal: 1000 kB\n",fallback)==fallback);
  CHECK(N::ReferenceCacheHostQuota3D("MemAvailable: 1000 MB\n",fallback)==fallback);
  CHECK(N::ReferenceCacheHostQuota3D("MemAvailable: 1000 kB\nMemAvailable: 2000 kB\n",fallback)==fallback);
  CHECK(N::ReferenceCacheHostQuota3D("MemAvailable: 1000 kB\n",N::ReferenceCacheHostLimit3D+1)==N::ReferenceCacheHostLimit3D);
}

TEST_CASE("Native reference sidecar: immutable graph, exact coordinates and accepted bindings round trip",
          "[NativeReferenceIO]") {
  std::vector<PolylineReference::Face> faces;
  for (Id offset : {Id(0), Id(16)}) {
    const double x = double(offset);
    const Node a{(Id(1) << 40) + offset, {x, 0}, 0}, b{(Id(1) << 40) + offset + 1, {x + .25, 0}, 0},
        c{(Id(1) << 40) + offset + 2, {x + 1, 0}, 0}, d{(Id(1) << 40) + offset + 3, {x + 1, 1}, 0},
        e{(Id(1) << 40) + offset + 4, {x, 1}, 0};
    faces.insert(faces.end(), {{a, b, 0}, {b, c, 0}, {c, d, 1}, {d, e, 1}, {e, a, 1}});
  }
  ReferenceState state;
  state.marker_names = {"wall with spaces", "farfield"};
  state.original = std::make_shared<PolylineReference>(faces, 45);
  for (const auto& face : faces)
    state.accepted_edges.emplace(CoordinateKey(face.a.p, face.b.p),
                                 state.original->ComponentOfOriginalFace(face.a.id, face.b.id));
  // Remove an original collinear sample and introduce a new (non-original) wall point in accepted bindings.
  const int component = state.accepted_edges.at(CoordinateKey({0, 0}, {.25, 0}));
  state.accepted_edges.erase(CoordinateKey({0, 0}, {.25, 0}));
  state.accepted_edges.erase(CoordinateKey({.25, 0}, {1, 0}));
  const double sample = std::nextafter(.7, 1.);
  state.accepted_edges.emplace(CoordinateKey({0, 0}, {sample, 0}), component);
  state.accepted_edges.emplace(CoordinateKey({sample, 0}, {1, 0}), component);
  const auto text = EncodeReference(state);
  const auto decoded = DecodeReference(text);
  CHECK(decoded.marker_names == state.marker_names);
  CHECK(decoded.accepted_edges == state.accepted_edges);
  CHECK(decoded.original->Components().size() == 4);
  CHECK(EncodeReference(decoded) == text);
  CHECK_FALSE(decoded.original->IsFeature(Point{.25, 0}));
  CHECK_FALSE(decoded.original->IsFeature(Point{sample, 0}));
  CHECK(decoded.original->IsFeature(Point{0, 0}));
  CHECK(decoded.original->ComponentOfOriginalFace((Id(1) << 40), (Id(1) << 40) + 1) == component);
  CHECK(decoded.original->At(component, .25).x == .25);
  CHECK_THROWS(DecodeReference(text + "trailing data"));
  CHECK_THROWS(DecodeReference(text.substr(0, text.size() / 2)));
  CHECK_THROWS(DecodeReference("SU2_NATIVE_REFERENCE 2\n2 10 10\n"));
  CHECK_THROWS(DecodeReference("SU2_NATIVE_REFERENCE 1\n2 18446744073709551615 10\n"));
  const auto finalLine = text.rfind('\n', text.size() - 2);
  auto duplicate = text;
  // Extra complete accepted edge with a header count that admits it: ownership must still be unique.
  const auto countsStart = duplicate.find('\n') + 1;
  const auto countsEnd = duplicate.find('\n', countsStart);
  duplicate.replace(countsStart, countsEnd - countsStart, "2 10 11");
  duplicate += text.substr(finalLine + 1);
  CHECK_THROWS(DecodeReference(duplicate));
  const std::string filename = "native_reference_roundtrip.native_ref";
  WriteReference(filename, state);
  ReferenceState loaded;
  CHECK(LoadReference(filename, loaded));
  CHECK(EncodeReference(loaded) == text);
  SU2_MPI::Barrier(SU2_MPI::GetComm());
  if (SU2_MPI::GetRank() == MASTER_NODE) std::remove(filename.c_str());
  SU2_MPI::Barrier(SU2_MPI::GetComm());
  CHECK_FALSE(LoadReference(filename, loaded));
  CHECK(EncodeReference(loaded) == text);
}

TEST_CASE("Native reference sidecar records the first-height tolerance and refuses another", "[NativeReferenceIO]") {
  const Node a{1, {0, 0}, 0}, b{2, {1, 0}, 0}, c{3, {1, 1}, 0}, d{4, {0, 1}, 0};
  std::vector<PolylineReference::Face> faces{{a, b, 0}, {b, c, 1}, {c, d, 1}, {d, a, 1}};
  ReferenceState state;
  state.marker_names = {"wall", "farfield"};
  state.original = std::make_shared<PolylineReference>(faces, 45);
  for (const auto& face : faces)
    state.accepted_edges.emplace(CoordinateKey(face.a.p, face.b.p),
                                 state.original->ComponentOfOriginalFace(face.a.id, face.b.id));
  const auto legacy = EncodeReference(state);
  CHECK(legacy.find("BL_FIRST_HEIGHT_TOL") == std::string::npos);
  CHECK(DecodeReference(legacy).first_height_tol == 0);
  state.first_height_tol = 0.125;
  const auto text = EncodeReference(state);
  const auto decoded = DecodeReference(text);
  CHECK(decoded.first_height_tol == 0.125);
  CHECK(EncodeReference(decoded) == text);
  CHECK_THROWS(DecodeReference(text + "BL_FIRST_HEIGHT_TOL 0.125\n"));
  CHECK_THROWS(DecodeReference(legacy + "BL_FIRST_HEIGHT_TOL 1.5\n"));
  CHECK_THROWS(DecodeReference(legacy + "BL_FIRST_HEIGHT_TOLERANCE 0.1\n"));
  // A recorded tolerance must equal the configured one; a legacy sidecar adopts the configured tolerance.
  CHECK(SU2NativeFirstHeight::AdoptRecorded(0, 0.1) == 0.1);
  CHECK(SU2NativeFirstHeight::AdoptRecorded(0.125, 0.125) == 0.125);
  CHECK_THROWS(SU2NativeFirstHeight::AdoptRecorded(0.125, 0.1));
  CHECK_THROWS(SU2NativeFirstHeight::AdoptRecorded(0, 1.0));
}

TEST_CASE("Native v8 atlas sidecar preserves exact covers traces and independent policy", "[NativeAtlasReferenceIO3D]") {
  namespace N=SU2Native3D;namespace G=SU2NativeGeometry;
  const std::array<N::Node,4> nodes{{{1,{0,0,0}},{2,{1,0,0}},{3,{0,1,0}},{4,{0,0,1}}}};
  std::vector<N::Facet> facets{{10,0,{nodes[1],nodes[2],nodes[3]}},{11,0,{nodes[0],nodes[3],nodes[2]}},
                               {12,0,{nodes[0],nodes[1],nodes[3]}},{13,0,{nodes[0],nodes[2],nodes[1]}}};
  auto definition=std::make_shared<N::PlanarReference>(facets);std::map<N::Id,double> errors;
  for(const auto& f:facets)errors.emplace(f.id,1e-12);
  auto geometry=std::make_shared<G::MappedFacetReference>(definition,errors,37);
  G::AtlasBudget budget{1000000,size_t(1)<<24,4096};G::MobilityPolicy policy;std::string reason;
  REQUIRE(geometry->BuildMobilityPolicy(35,{{0,2}},policy,budget,reason));
  N::BoundaryPoints points;std::vector<N::SurfaceFace> surface;G::BoundaryCoverage coverage;
  for(const auto& facet:facets) {
    N::Face face{facet.v[0].id,facet.v[1].id,facet.v[2].id};std::rotate(face.begin(),std::min_element(face.begin(),face.end()),face.end());
    surface.push_back({face,facet.id});std::array<G::SurfaceBinding,3> bindings;
    for(unsigned k=0;k<3;++k){bindings[k]=geometry->Vertex(face[k]);points.emplace(face[k],N::BoundaryPoint{nodes[face[k]-1],bindings[k]});}
    G::FaceCoverage cover;REQUIRE(geometry->ResolveSingleChart(facet.id,bindings,cover,budget,reason));coverage.faces.emplace(face,cover);
    std::array<G::EdgeTrace,3> traces;REQUIRE(geometry->CoverageTraces(cover,traces,budget,reason));
    // Store only forward-ID traces; their other incident face must recover the same accepted path.
    for(unsigned k=0;k<3;++k)if(face[k]<face[(k+1)%3])coverage.traces.emplace(N::Edge{face[k],face[(k+1)%3]},traces[k]);
  }
  std::shared_ptr<N::BoundaryParameterState> parameters;
  REQUIRE_NOTHROW(parameters=std::make_shared<N::BoundaryParameterState>(geometry,points,surface,9,SIZE_MAX,0,coverage,policy,&budget));
  N::ReferenceState state;state.facets=facets;state.marker_names={"body"};state.original=N::PlanarReference::WithParameters(definition,parameters);
  std::string encoded;REQUIRE_NOTHROW(encoded=N::EncodeReference(state));REQUIRE(encoded.find("SU2_NATIVE_REFERENCE_3D 8\n")==0);
  N::ReferenceState recovered;REQUIRE_NOTHROW(recovered=N::DecodeReference(encoded));REQUIRE(recovered.original->Parameters());
  CHECK(recovered.original->Parameters()->Geometry().Version()==37);CHECK(recovered.original->Parameters()->Mobility().version==1);
  CHECK(recovered.original->Parameters()->Mobility().identity==policy.identity);CHECK(N::EncodeReference(recovered)==encoded);
  const auto payload=parameters->Payload();CHECK(payload.size()==parameters->PayloadBytes());
  std::shared_ptr<N::BoundaryParameterState> restored;REQUIRE_NOTHROW(restored=N::BoundaryParameterState::FromPayload(geometry,payload));CHECK(restored->Snapshot()==parameters->Snapshot());
  auto truncated=payload;truncated.pop_back();CHECK_THROWS(N::BoundaryParameterState::FromPayload(geometry,truncated));
  CHECK_THROWS(N::BoundaryParameterState::FromPayload(geometry,payload,1));CHECK_THROWS(N::DecodeReference(encoded,1));
  CHECK_THROWS(N::DecodeReference(encoded+"trailing"));
  auto archive=N::EncodeBoundaryAtlas(coverage,policy);G::BoundaryCoverage decoded;G::MobilityPolicy decoded_policy;
  N::DecodeBoundaryAtlas(archive,decoded,decoded_policy);CHECK(N::EncodeBoundaryAtlas(decoded,decoded_policy)==archive);
  auto noncanonical=archive;std::fill(noncanonical.end()-8,noncanonical.end(),0);
  CHECK_THROWS(N::DecodeBoundaryAtlas(noncanonical,decoded,decoded_policy));
  archive.pop_back();CHECK_THROWS(N::DecodeBoundaryAtlas(archive,decoded,decoded_policy));
  CHECK(decoded_policy.identity==policy.identity);CHECK(N::EncodeBoundaryAtlas(decoded,decoded_policy)==N::EncodeBoundaryAtlas(coverage,policy));
  N::Owned mesh;N::OwnedCell row;row.cell={0,nodes};for(unsigned k=0;k<4;++k)row.physical[k]={true,10+k};mesh.emplace(0,row);
  N::RegionMetric target([](N::Point){return N::Tensor{};},[](N::Point,N::Point){return 1.L;});
  N::OptimizeOptions options;options.geometry=state.original.get();N::OptimizeReport report;N::Id next_node=5,next_cell=1;
  CHECK_FALSE(N::OptimizeMetricMesh(mesh,target,next_node,next_cell,options,report));CHECK(report.stop=="atlas reservation missing");
  CHECK(report.trials==0);CHECK(mesh.size()==1);CHECK(next_node==5);CHECK(next_cell==1);
  // No policy or descriptor upgrade is inferred for the legacy parameter authority.
  auto legacy=std::make_shared<N::BoundaryParameterState>(geometry,points,surface);
  state.original=N::PlanarReference::WithParameters(definition,legacy);const auto old=N::EncodeReference(state);
  CHECK(old.find("SU2_NATIVE_REFERENCE_3D 5\n")==0);CHECK(N::DecodeReference(old).original->Parameters()->Mobility().identity==0);
}
