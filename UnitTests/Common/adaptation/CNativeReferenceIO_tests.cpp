/*!
 * \file CNativeReferenceIO_tests.cpp
 * \brief Persist original component/feature truth despite adapted sampling and new CFD point IDs.
 * \version 8.5.0 "Harrier"
 * Copyright 2012-2026, SU2 Contributors (cf. AUTHORS.md)
 * SU2 is free software under the GNU Lesser General Public License, version 2.1 or later.
 */

#include "catch.hpp"
#include "../../../Common/include/adaptation/CNativeReferenceIO.hpp"

using namespace SU2NativeBoundary2D;

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
