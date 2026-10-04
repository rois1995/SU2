/*!
 * \file DistributedSearch_tests.cpp
 * \brief Building blocks of the distributed solution transfer (MPI_TRANSFER_PLAN.md M0): protected accurate sums,
 *        ADT extensions, box tree, directory, failure election, ownership and the mesh invariants R1, R2, R4, R6.
 *        Built in the normal, reverse (AD) and forward (DD) test drivers; valid with any number of ranks, e.g.
 *        mpirun -n 3 test_driver "[DistributedSearch]".
 * \version 8.5.0 "Harrier"
 *
 * SU2 Project Website: https://su2code.github.io
 *
 * The SU2 Project is maintained by the SU2 Foundation
 * (http://su2foundation.org)
 *
 * Copyright 2012-2026, SU2 Contributors (cf. AUTHORS.md)
 *
 * SU2 is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * SU2 is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with SU2. If not, see <http://www.gnu.org/licenses/>.
 */

#include "catch.hpp"

#include <algorithm>
#include <cfloat>
#include <cmath>
#include <cstdint>
#include <limits>
#include <map>
#include <numeric>
#include <set>

#include "../../../Common/include/adaptation/CAccurateSum.hpp"
#include "../../../Common/include/adaptation/CDistributedSearch.hpp"
#include "../../../Common/include/adaptation/TransferTolerances.hpp"
#include "../../../Common/include/adt/CADTElemClass.hpp"
#include "../../../Common/include/parallelization/CPassiveComm.hpp"
#include "../../SU2_CFD/adaptation/TransferTestCase.hpp"

namespace {

/*--- Number of ranks where a check failed (native MPI, independent of the code under test). ---*/
unsigned long FailedRanks(bool ok) {
  unsigned long bad = ok ? 0 : 1, total = 0;
#ifdef HAVE_MPI
  MPI_Allreduce(&bad, &total, 1, MPI_UNSIGNED_LONG, MPI_SUM, SU2_MPI::GetComm());
#else
  total = bad;
#endif
  return total;
}

/*--- Deterministic pseudo-random numbers (the same sequence on every rank for the same seed). ---*/
struct Random {
  uint64_t state;
  explicit Random(uint64_t seed) : state(seed * 6364136223846793005ull + 1442695040888963407ull) {}
  uint64_t Next() {
    state ^= state << 13;
    state ^= state >> 7;
    state ^= state << 17;
    return state;
  }
  double Uniform() { return (Next() >> 11) * (1.0 / 9007199254740992.0); }
  long Int(long lo, long hi) { return lo + static_cast<long>(Next() % static_cast<uint64_t>(hi - lo + 1)); }
};

/*--- Exact sum of terms m 2^e (|m| < 2^24, -40 <= e <= 40) as a 128-bit integer multiple of 2^-40. ---*/
struct ExactTerms {
  std::vector<double> terms;
  __int128 exact = 0;
  void Add(long m, int e) {
    terms.push_back(std::ldexp(static_cast<double>(m), e));
    exact += static_cast<__int128>(m) << (e + 40);
  }
  /*--- Correctly rounded (the conversion of a 128-bit integer rounds to nearest; scaling by 2^-40 is exact). ---*/
  double Exact() const { return std::ldexp(static_cast<double>(exact), -40); }
};

}  // namespace

TEST_CASE("Distributed search: accurate sums", "[AdaptationMPI][DistributedSearch]") {
  SECTION("cancellation") {
    for (const double big : {1e16, 1e300}) {
      const double x[] = {big, 1.0, -big};
      double triple[3], result = 0.0;
      CHECK(CAccurateSum::Local(x, 3, 1, triple));
      CHECK(CAccurateSum::Merge(triple, 1, &result));
      CHECK(result == 1.0);
      /*--- Split over two "ranks" (two triples). ---*/
      double triples[6];
      CAccurateSum::Local(x, 1, 1, triples);
      CAccurateSum::Local(x + 1, 2, 1, triples + 3);
      CHECK(CAccurateSum::Merge(triples, 2, &result));
      CHECK(result == 1.0);
    }
    CNeumaierSum sum;
    for (const double v : {1e16, 1.0, -1e16, 1.0}) sum.Add(v);
    CHECK(sum.Sum() == 2.0);
  }

  SECTION("random terms of mixed sign and magnitude, split over the ranks") {
    const int size = SU2_MPI::GetSize(), rank = SU2_MPI::GetRank();
    double worstNaive = 0.0, worstAccurate = 0.0;
    bool ok = true;
    for (uint64_t seed = 1; seed <= 20; ++seed) {
      Random random(seed);
      ExactTerms all;
      const long n = 2000 + 37 * static_cast<long>(seed);
      for (long i = 0; i < n; ++i) all.Add(random.Int(-(1l << 23), 1l << 23), static_cast<int>(random.Int(-40, 40)));
      const double exact = all.Exact();
      double absSum = 0.0;
      for (const auto t : all.terms) absSum += std::fabs(t);

      /*--- Local (one triple) and over the ranks (uneven slices, the middle rank without terms). ---*/
      double triple[3], local = 0.0;
      ok &= CAccurateSum::Local(all.terms.data(), all.terms.size(), 1, triple);
      ok &= CAccurateSum::Merge(triple, 1, &local);
      /*--- The bound of the plan (3.6): 2 eps |S| + O((n + P) eps^2) A, plus half an ulp of the rounded reference. ---*/
      const double eps = std::ldexp(1.0, -53);
      const double bound = 2.5 * eps * std::fabs(exact) + 4.0 * (n + size) * eps * eps * absSum;
      ok &= std::fabs(local - exact) <= bound;

      const int emptyRank = size >= 3 ? size / 2 : -1;
      auto boundary = [&](int r) { return all.terms.size() * r * r / (size * size); };
      size_t first = boundary(rank), last = boundary(rank + 1);
      if (rank == emptyRank) first = last = 0;
      if (emptyRank >= 0 && rank == emptyRank + 1) first = boundary(emptyRank);
      CAccurateSumBatch batch;
      const auto q = batch.Add(all.terms.data() + first, last - first);
      batch.Reduce();
      ok &= batch.Finite(q);
      ok &= std::fabs(batch.Get(q) - exact) <= bound;
      /*--- A naive sum of the same terms (fast math, this unit) for the record: the accurate sums are far better. ---*/
      double naive = 0.0;
      for (const auto t : all.terms) naive += t;
      worstNaive = std::max(worstNaive, std::fabs(naive - exact) / absSum);
      worstAccurate = std::max(worstAccurate, std::fabs(local - exact) / absSum);
    }
    CHECK(FailedRanks(ok) == 0);
    CHECK(worstAccurate <= worstNaive);
  }

  SECTION("overflow and nonfinite terms") {
    const double huge[] = {DBL_MAX / 2, DBL_MAX / 2, -DBL_MAX / 2};
    double triple[3], result = 0.0;
    size_t firstBad = 0;
    CHECK_FALSE(CAccurateSum::Local(huge, 3, 1, triple, &firstBad));
    CHECK(firstBad == 3);
    const double withNaN[] = {1.0, 2.0, std::numeric_limits<double>::quiet_NaN(), 3.0};
    CHECK_FALSE(CAccurateSum::Local(withNaN, 4, 1, triple, &firstBad));
    CHECK(firstBad == 2);
    CHECK_FALSE(CAccurateSum::Merge(triple, 1, &result));
    const double withInf[] = {1.0, std::numeric_limits<double>::infinity()};
    CHECK_FALSE(CAccurateSum::Local(withInf, 2, 1, triple, &firstBad));
    CHECK(firstBad == 1);

    /*--- A nonfinite term on one rank makes the global sum nonfinite on every rank. ---*/
    const int rank = SU2_MPI::GetRank(), size = SU2_MPI::GetSize();
    CAccurateSumBatch batch;
    std::vector<double> terms = {1.0, 2.0};
    if (rank == size - 1) terms.push_back(std::numeric_limits<double>::quiet_NaN());
    const auto q = batch.Add(terms);
    const auto q2 = batch.AddValue(1.0);
    batch.Reduce();
    CHECK_FALSE(batch.Finite(q));
    CHECK(batch.Finite(q2));
    CHECK(batch.Get(q2) == static_cast<double>(size));
    CHECK(batch.LocalFirstBad(q) == (rank == size - 1 ? 2u : 2u));
  }

  SECTION("active terms") {
    CAccurateSumBatch batch;
    std::vector<su2double> terms(3);
    for (int i = 0; i < 3; ++i) {
      terms[i] = 1.0 + i;
#ifdef CODI_FORWARD_TYPE
      SU2_TYPE::SetDerivative(terms[i], 0.5 * i);
#endif
    }
    const auto q = batch.AddActive(terms);
    batch.Reduce();
    const auto size = SU2_MPI::GetSize();
    CHECK(SU2_TYPE::GetValue(batch.GetActive(q)) == 6.0 * size);
#ifdef CODI_FORWARD_TYPE
    CHECK(SU2_TYPE::GetDerivative(batch.GetActive(q)) == 1.5 * size);
#endif
  }
}

TEST_CASE("Distributed search: box tree against brute force", "[AdaptationMPI][DistributedSearch]") {
  const int size = SU2_MPI::GetSize(), rank = SU2_MPI::GetRank();
  for (const unsigned short nDim : {2, 3}) {
    /*--- Boxes of every rank (the middle rank of three or more has none), generated identically on all ranks. ---*/
    std::vector<std::vector<double>> rankBoxes(size);
    for (int r = 0; r < size; ++r) {
      if (size >= 3 && r == size / 2) continue;
      Random random(100 + r);
      const int nBox = 5 + 7 * r;
      for (int b = 0; b < nBox; ++b) {
        double lo[3], hi[3];
        for (unsigned short iDim = 0; iDim < nDim; ++iDim) {
          lo[iDim] = random.Uniform();
          hi[iDim] = lo[iDim] + 0.2 * random.Uniform();
        }
        rankBoxes[r].insert(rankBoxes[r].end(), lo, lo + nDim);
        rankBoxes[r].insert(rankBoxes[r].end(), hi, hi + nDim);
      }
    }
    CRankBoxTree tree;
    tree.Build(nDim, rankBoxes[rank]);
    bool ok = true;
    std::vector<double> all;
    std::vector<int> owner;
    for (int r = 0; r < size; ++r) {
      all.insert(all.end(), rankBoxes[r].begin(), rankBoxes[r].end());
      owner.insert(owner.end(), rankBoxes[r].size() / (2 * nDim), r);
    }
    ok &= tree.GetnBox() == owner.size();
    for (auto i = 0ul; ok && i < owner.size(); ++i)
      ok &= tree.BoxRank(i) == owner[i] && std::equal(all.begin() + 2 * nDim * i, all.begin() + 2 * nDim * (i + 1), tree.Box(i));

    Random random(7);
    for (int iQuery = 0; iQuery < 400; ++iQuery) {
      double x[3], lo[3], hi[3];
      for (unsigned short iDim = 0; iDim < nDim; ++iDim) {
        x[iDim] = 1.4 * random.Uniform() - 0.2;
        lo[iDim] = x[iDim] - 0.05 * random.Uniform();
        hi[iDim] = x[iDim] + 0.05 * random.Uniform();
      }
      /*--- A query exactly on a box corner (bounds included). ---*/
      if (iQuery % 10 == 0 && !all.empty()) std::copy_n(all.begin() + 2 * nDim * (iQuery % owner.size()), nDim, x);
      const double r = 0.1 * random.Uniform();
      std::vector<unsigned long> contain, intersect, within;
      for (auto i = 0ul; i < owner.size(); ++i) {
        const double* b = &all[2 * nDim * i];
        bool in = true, cut = true;
        double d2 = 0.0;
        for (unsigned short iDim = 0; iDim < nDim; ++iDim) {
          in &= x[iDim] >= b[iDim] && x[iDim] <= b[nDim + iDim];
          cut &= lo[iDim] <= b[nDim + iDim] && hi[iDim] >= b[iDim];
          const double ds = std::min(0.0, x[iDim] - b[iDim]) + std::max(0.0, x[iDim] - b[nDim + iDim]);
          d2 += ds * ds;
        }
        if (in) contain.push_back(i);
        if (cut) intersect.push_back(i);
        if (d2 <= r * r) within.push_back(i);
      }
      std::vector<unsigned long> result;
      tree.BoxesContaining(x, result);
      ok &= result == contain;
      tree.BoxesIntersecting(lo, hi, result);
      ok &= result == intersect;
      tree.BoxesWithinDistance(x, r, result);
      ok &= result == within;
      std::vector<int> ranks, expected;
      for (const auto i : contain) expected.push_back(owner[i]);
      std::sort(expected.begin(), expected.end());
      expected.erase(std::unique(expected.begin(), expected.end()), expected.end());
      tree.RanksContaining(x, ranks);
      ok &= ranks == expected;
    }
    CHECK(FailedRanks(ok) == 0);
  }

  /*--- Bisection boxes: every item in its group's box, at most the requested number of groups. ---*/
  Random random(3);
  std::vector<double> items, centroids;
  for (int i = 0; i < 100; ++i) {
    double c[2] = {random.Uniform(), random.Uniform()};
    centroids.insert(centroids.end(), c, c + 2);
    const double box[4] = {c[0] - 0.01, c[1] - 0.02, c[0] + 0.03, c[1] + 0.01};
    items.insert(items.end(), box, box + 4);
  }
  std::vector<unsigned long> group;
  const auto boxes = BisectionBoxes(2, items, centroids, 8, &group);
  CHECK(boxes.size() == 8 * 4);
  bool inside = true;
  for (int i = 0; i < 100; ++i) {
    const double* b = &boxes[4 * group[i]];
    inside &= items[4 * i] >= b[0] && items[4 * i + 1] >= b[1] && items[4 * i + 2] <= b[2] && items[4 * i + 3] <= b[3];
  }
  CHECK(inside);
  CHECK(BisectionBoxes(2, {}, {}, 8, &group).empty());
}

TEST_CASE("Distributed search: directory of point records", "[AdaptationMPI][DistributedSearch]") {
  const int size = SU2_MPI::GetSize(), rank = SU2_MPI::GetRank();
  for (const size_t roundBytes : {CPassiveComm::DEFAULT_ROUND_BYTES, size_t(200)}) {
    const auto saved = CPassiveComm::GetRoundBytes();
    CPassiveComm::SetRoundBytes(roundBytes);
    /*--- N = 5 P + 3 points dealt to the ranks in a scrambled way (the middle rank owns none). ---*/
    const uint64_t n = 5 * size + 3;
    std::vector<uint64_t> gids;
    std::vector<char> records;
    const size_t recordBytes = 3 * FieldValueBytes();
    for (uint64_t g = 0; g < n; ++g) {
      const uint64_t scrambled = (g * 7 + 3) % n;
      int owner = static_cast<int>(scrambled % size);
      if (size >= 3 && owner == size / 2) owner = 0;
      if (owner != rank) continue;
      gids.push_back(g);
      su2double values[3] = {su2double(1.0 * g), su2double(-2.5 * g), su2double(1e-300 * (g + 1))};
#ifdef CODI_FORWARD_TYPE
      for (int k = 0; k < 3; ++k) SU2_TYPE::SetDerivative(values[k], 0.25 * k + g);
#endif
      records.resize(records.size() + recordBytes);
      PackFieldValues(&records[records.size() - recordBytes], values, 3);
    }
    CPointDirectory directory;
    directory.Build(gids, records, recordBytes, "test");
    CHECK(directory.GetnGlobal() == n);

    /*--- Fetch: repeated and unordered indices; the middle rank asks for nothing. ---*/
    std::vector<uint64_t> request;
    if (!(size >= 3 && rank == size / 2)) {
      for (uint64_t k = 0; k < n + 4; ++k) request.push_back((k * (rank + 3)) % n);
    }
    const auto fetched = directory.Fetch(request);
    bool ok = fetched.size() == request.size() * recordBytes;
    for (auto i = 0ul; ok && i < request.size(); ++i) {
      su2double values[3];
      UnpackFieldValues(&fetched[i * recordBytes], values, 3);
      const auto g = request[i];
      ok &= SU2_TYPE::GetValue(values[0]) == 1.0 * g && SU2_TYPE::GetValue(values[1]) == -2.5 * g &&
            SU2_TYPE::GetValue(values[2]) == passivedouble(1e-300 * (g + 1));
#ifdef CODI_FORWARD_TYPE
      for (int k = 0; k < 3; ++k) ok &= SU2_TYPE::GetDerivative(values[k]) == 0.25 * k + g;
#endif
    }
    CHECK(FailedRanks(ok) == 0);
    CPassiveComm::SetRoundBytes(saved);
  }
}

TEST_CASE("Distributed search: failure election", "[AdaptationMPI][DistributedSearch]") {
  const int size = SU2_MPI::GetSize(), rank = SU2_MPI::GetRank();
  /*--- No failure anywhere. ---*/
  CLocalFailure none;
  CHECK_FALSE(ElectFailure(none).any);

  /*--- A failure on the last rank only (others empty). ---*/
  CLocalFailure last;
  if (rank == size - 1) last.Set(1, 42, "last rank");
  auto elected = ElectFailure(last);
  CHECK(elected.any);
  CHECK(elected.rank == size - 1);
  CHECK(elected.message == "last rank");
  CHECK(elected.gid == 42);

  /*--- Failures on every rank: the higher severity wins over a smaller global index; ties by global index, then
   *    by rank. ---*/
  CLocalFailure all;
  all.Set(1, 10 + rank, "rank " + std::to_string(rank));
  if (rank == size / 2) all.Set(2, 1000, "severe " + std::to_string(rank));
  elected = ElectFailure(all);
  CHECK(elected.rank == size / 2);
  CHECK(elected.message == "severe " + std::to_string(size / 2));

  CLocalFailure tied;
  tied.Set(1, rank == 0 ? 9 : 5, "tied " + std::to_string(rank));
  elected = ElectFailure(tied);
  CHECK(elected.rank == (size == 1 ? 0 : 1));
  CHECK(elected.gid == (size == 1 ? 9u : 5u));

  /*--- Set keeps the worst local failure. ---*/
  CLocalFailure local;
  local.Set(1, 7, "a");
  local.Set(1, 3, "b");
  local.Set(1, 5, "c");
  CHECK(local.message == "b");
  local.Set(0, 0, "ignored");
  CHECK(local.message == "b");
}

TEST_CASE("Distributed search: ADT extensions against brute force", "[AdaptationMPI][DistributedSearch]") {
  for (const unsigned short nDim : {2, 3}) {
    const auto mesh = transfer_test::BoxMesh(nDim, nDim == 2 ? 4 : 2, true);
    const unsigned short nNode = nDim + 1;
    const auto nElem = mesh.GetnElem();
    std::vector<su2double> coord(mesh.coord.begin(), mesh.coord.end());
    auto build = [&](const std::vector<unsigned long>& elems) {
      std::vector<su2double> c = coord;
      std::vector<unsigned long> conn;
      for (const auto e : elems) conn.insert(conn.end(), &mesh.elem[e * nNode], &mesh.elem[e * nNode] + nNode);
      std::vector<unsigned short> types(elems.size(), nDim == 2 ? TRIANGLE : TETRAHEDRON), markers(elems.size(), 0);
      std::vector<unsigned long> ids = elems;
      return std::make_unique<CADTElemClass>(nDim, c, conn, types, markers, ids, false);
    };
    std::vector<unsigned long> allElems(nElem);
    std::iota(allElems.begin(), allElems.end(), 0ul);
    auto adt = build(allElems);
    std::vector<std::unique_ptr<CADTElemClass>> single;
    for (auto e = 0ul; e < nElem; ++e) single.push_back(build({e}));

    /*--- Inflated box of each element as the ADT computes it. ---*/
    std::vector<su2double> boxes(nElem * 2 * nDim);
    for (auto e = 0ul; e < nElem; ++e) {
      su2double* lo = &boxes[e * 2 * nDim];
      su2double* hi = lo + nDim;
      for (unsigned short iDim = 0; iDim < nDim; ++iDim) lo[iDim] = hi[iDim] = coord[mesh.elem[e * nNode] * nDim + iDim];
      for (unsigned short k = 1; k < nNode; ++k)
        for (unsigned short iDim = 0; iDim < nDim; ++iDim) {
          lo[iDim] = min(lo[iDim], coord[mesh.elem[e * nNode + k] * nDim + iDim]);
          hi[iDim] = max(hi[iDim], coord[mesh.elem[e * nNode + k] * nDim + iDim]);
        }
      CADTElemClass::InflateBox(nDim, lo, hi);
    }

    Random random(11 + nDim);
    bool ok = true;
    unsigned long nMultiple = 0;
    for (int iQuery = 0; iQuery < 300; ++iQuery) {
      su2double x[3] = {0.0, 0.0, 0.0}, lo[3], hi[3];
      for (unsigned short iDim = 0; iDim < nDim; ++iDim) {
        x[iDim] = (nDim == 2 && iDim == 0 ? 2.2 : 1.1) * random.Uniform() - 0.05;
        lo[iDim] = x[iDim] - 0.1 * random.Uniform();
        hi[iDim] = x[iDim] + 0.1 * random.Uniform();
      }
      /*--- Every third query on a mesh point or an edge midpoint (several containing elements). ---*/
      if (iQuery % 3 == 0) {
        const auto e = static_cast<unsigned long>(random.Int(0, nElem - 1));
        for (unsigned short iDim = 0; iDim < nDim; ++iDim)
          x[iDim] = 0.5 * (coord[mesh.elem[e * nNode] * nDim + iDim] + coord[mesh.elem[e * nNode + 1] * nDim + iDim]);
      }
      std::vector<unsigned long> ids;
      std::vector<su2double> weights;
      adt->DetermineContainingElements(x, ids, weights);
      std::set<unsigned long> found(ids.begin(), ids.end()), expected;
      for (auto e = 0ul; e < nElem; ++e) {
        unsigned short markerID;
        unsigned long id;
        int rankID;
        su2double parCoor[3], w[8];
        if (single[e]->DetermineContainingElement(x, markerID, id, rankID, parCoor, w)) expected.insert(e);
      }
      ok &= found == expected && ids.size() == found.size() && weights.size() == 8 * ids.size();
      nMultiple += found.size() > 1;

      adt->DetermineIntersectingElements(lo, hi, ids);
      std::set<unsigned long> cut(ids.begin(), ids.end()), cutExpected;
      const su2double radius = 0.07 * random.Uniform();
      for (auto e = 0ul; e < nElem; ++e) {
        const su2double* b = &boxes[e * 2 * nDim];
        bool intersects = true;
        for (unsigned short iDim = 0; iDim < nDim; ++iDim) intersects &= !(b[iDim] > hi[iDim] || b[nDim + iDim] < lo[iDim]);
        if (intersects) cutExpected.insert(e);
      }
      ok &= cut == cutExpected;

      adt->DetermineElementsWithinDistance(x, radius, ids);
      std::set<unsigned long> near(ids.begin(), ids.end()), nearExpected;
      for (auto e = 0ul; e < nElem; ++e) {
        const su2double* b = &boxes[e * 2 * nDim];
        su2double d2 = 0.0;
        for (unsigned short iDim = 0; iDim < nDim; ++iDim) {
          const su2double ds = min(0.0, x[iDim] - b[iDim]) + max(0.0, x[iDim] - b[nDim + iDim]);
          d2 += ds * ds;
        }
        if (d2 <= radius * radius) nearExpected.insert(e);
      }
      ok &= near == nearExpected;
    }
    CHECK(ok);
    /*--- Coverage of the multi-element cases (in single precision a rounded edge midpoint is often inside one element
     *    only). ---*/
    CHECK(nMultiple > (transfer_tol::kSinglePrecision ? 5ul : 10ul));

    /*--- An empty tree answers nothing. ---*/
    auto empty = build({});
    std::vector<unsigned long> ids = {1, 2};
    std::vector<su2double> weights;
    su2double x[3] = {0.5, 0.5, 0.5};
    empty->DetermineContainingElements(x, ids, weights);
    CHECK(ids.empty());
    empty->DetermineIntersectingElements(x, x, ids);
    CHECK(ids.empty());
    empty->DetermineElementsWithinDistance(x, 10.0, ids);
    CHECK(ids.empty());
  }
}

TEST_CASE("Distributed search: ownership and mesh invariants", "[AdaptationMPI][DistributedSearch]") {
  /*--- Partitioned boxes from the memory reader: every element owned once (R1 keys unique), every rank holds every
   *    element of its owned points (R2), element node order and coordinates as in the input on every rank (R4, R6),
   *    owned boundary faces once per marker, global bounding box. ---*/
  for (const unsigned short nDim : {2, 3}) {
    SECTION("nDim " + std::to_string(nDim)) {
      auto config = transfer_test::MakeConfig(nDim, "SOLVER= EULER\n");
      const auto input = transfer_test::BoxMesh(nDim, nDim == 2 ? 6 : 3, true);
      transfer_test::MeshSolution mesh(config.get(), input, 0);
      const auto& geometry = mesh.Fine();
      const unsigned short nNode = nDim + 1;

      const auto owned = OwnedSimplices(geometry);
      CHECK(CPassiveComm::AllreduceSum(owned.size()) == input.GetnElem());
      std::map<CSimplexKey, std::vector<uint64_t>> inputElems;
      for (auto e = 0ul; e < input.GetnElem(); ++e) {
        std::vector<uint64_t> nodes(&input.elem[e * nNode], &input.elem[e * nNode] + nNode);
        inputElems[MakeSimplexKey(nodes.data(), nNode)] = nodes;
      }
      bool ok = true;
      /*--- R4: every element has the same node order on every rank that holds it (the geometry may reorient elements,
       *    the same way on every rank); R6: every local point has the input's coordinates. ---*/
      std::vector<uint64_t> local;
      for (auto iElem = 0ul; iElem < geometry.GetnElem(); ++iElem) {
        std::vector<uint64_t> gids(nNode);
        for (unsigned short k = 0; k < nNode; ++k) gids[k] = geometry.nodes->GetGlobalIndex(geometry.elem[iElem]->GetNode(k));
        ok &= inputElems.count(MakeSimplexKey(gids.data(), nNode)) == 1;
        local.insert(local.end(), gids.begin(), gids.end());
      }
      const auto allElems = CPassiveComm::Allgatherv(local, nullptr);
      std::map<CSimplexKey, std::vector<uint64_t>> order;
      for (size_t i = 0; i + nNode <= allElems.size(); i += nNode) {
        std::vector<uint64_t> gids(&allElems[i], &allElems[i] + nNode);
        const auto key = MakeSimplexKey(gids.data(), nNode);
        const auto it = order.find(key);
        if (it == order.end()) {
          order[key] = gids;
        } else {
          ok &= it->second == gids;
        }
      }
      for (auto iPoint = 0ul; iPoint < geometry.GetnPoint(); ++iPoint) {
        const auto g = geometry.nodes->GetGlobalIndex(iPoint);
        for (unsigned short iDim = 0; iDim < nDim; ++iDim)
          ok &= SU2_TYPE::GetValue(geometry.nodes->GetCoord(iPoint, iDim)) == input.coord[g * nDim + iDim];
      }
      /*--- R2: the local elements of each owned point are all its elements. ---*/
      std::vector<unsigned long> inputCount(input.GetnPoint(), 0), localCount(geometry.GetnPoint(), 0);
      for (const auto e : input.elem) inputCount[e]++;
      for (auto iElem = 0ul; iElem < geometry.GetnElem(); ++iElem)
        for (unsigned short k = 0; k < nNode; ++k) localCount[geometry.elem[iElem]->GetNode(k)]++;
      for (auto iPoint = 0ul; iPoint < geometry.GetnPointDomain(); ++iPoint)
        ok &= localCount[iPoint] == inputCount[geometry.nodes->GetGlobalIndex(iPoint)];
      /*--- Owned keys unique over the ranks. ---*/
      std::vector<uint64_t> keys;
      for (const auto& key : owned.keys) keys.insert(keys.end(), key.begin(), key.end());
      const auto allKeys = CPassiveComm::Allgatherv(keys, nullptr);
      std::set<std::vector<uint64_t>> unique;
      for (size_t i = 0; i + 4 <= allKeys.size(); i += 4) unique.insert(std::vector<uint64_t>(&allKeys[i], &allKeys[i] + 4));
      ok &= unique.size() == input.GetnElem();
      CHECK(FailedRanks(ok) == 0);

      /*--- Boundary faces: once per marker over the ranks, ids = config positions. ---*/
      const auto faces = OwnedBoundaryFaces(geometry, mesh.markerTags, *config);
      unsigned long nInputFaces = 0;
      for (const auto& marker : input.markers) nInputFaces += marker.GetnElem(nDim);
      CHECK(CPassiveComm::AllreduceSum(faces.size()) == nInputFaces);
      bool ids = true;
      for (const auto id : faces.markerId) ids &= id < config->GetnMarker_CfgFile();
      CHECK(FailedRanks(ids) == 0);

      double xMin[3], xMax[3], diagonal = 0.0;
      GlobalBoundingBox(geometry, xMin, xMax, &diagonal);
      CHECK(xMin[0] == 0.0);
      CHECK(xMax[0] == (nDim == 2 ? 2.0 : 1.0));
      CHECK(diagonal == Approx(nDim == 2 ? std::sqrt(5.0) : std::sqrt(3.0)).epsilon(1e-15));
    }
  }
}

/*------------------------------------------------------------------------------------------------------------------*/
/*--- Single precision compatibility (REVIEW5 F1): run in every build ---*/
/*------------------------------------------------------------------------------------------------------------------*/

int NativeDoubleSizeOrder1();
int NativeDoubleSizeOrder2();
int NativeDoubleSizeOrder3();

TEST_CASE("Single precision: passive double reductions", "[AdaptationMPI][DistributedSearch][SinglePrecision]") {
  /*--- The MPI type of a passive double is the library's double in every include order and precision. ---*/
#ifdef HAVE_MPI
  CHECK(NativeDoubleSizeOrder1() == static_cast<int>(sizeof(double)));
  CHECK(NativeDoubleSizeOrder2() == static_cast<int>(sizeof(double)));
  CHECK(NativeDoubleSizeOrder3() == static_cast<int>(sizeof(double)));
#else
  CHECK(NativeDoubleSizeOrder1() == 0);
  CHECK(NativeDoubleSizeOrder2() == 0);
  CHECK(NativeDoubleSizeOrder3() == 0);
#endif
  /*--- A 2^-40 term survives a double reduction (lost by MPI_FLOAT). ---*/
  const int rank = SU2_MPI::GetRank(), size = SU2_MPI::GetSize();
  const double tiny = std::ldexp(1.0, -40);
  double send[3] = {1.0 + tiny, 1.0 + rank * tiny, 1.0 + rank * tiny}, sum = 0.0, lo = 0.0, hi = 0.0;
  CPassiveComm::Allreduce(&send[0], &sum, 1, CPassiveComm::Op::SUM);
  CPassiveComm::Allreduce(&send[1], &lo, 1, CPassiveComm::Op::MIN);
  CPassiveComm::Allreduce(&send[2], &hi, 1, CPassiveComm::Op::MAX);
  CHECK(sum == size * (1.0 + tiny));
  CHECK(lo == 1.0);
  CHECK(hi == 1.0 + (size - 1) * tiny);
}

TEST_CASE("Single precision: accurate sums of float terms", "[AdaptationMPI][DistributedSearch][SinglePrecision]") {
  /*--- Float terms are widened exactly: the triple is bitwise that of the widened doubles. ---*/
  auto same = [](const std::vector<float>& x, size_t n, size_t stride) {
    std::vector<double> wide(x.begin(), x.end());
    double a[3], b[3];
    size_t badA = 0, badB = 0;
    const bool okA = CAccurateSum::Local(x.data(), n, stride, a, &badA);
    const bool okB = CAccurateSum::Local(wide.data(), n, stride, b, &badB);
    bool equal = okA == okB && badA == badB;
    for (int k = 0; k < 3; ++k) equal &= (std::isnan(a[k]) && std::isnan(b[k])) || a[k] == b[k];
    return std::make_pair(equal, std::make_pair(okA, badA));
  };
  /*--- Cancellation: 2^24 + 1 - 2^24 is 0 in float arithmetic, 1 here. ---*/
  const std::vector<float> cancel = {16777216.0f, 1.0f, -16777216.0f, 3e-8f, 1e30f, -1e30f, 0.5f};
  auto r = same(cancel, cancel.size(), 1);
  CHECK(r.first);
  CHECK(r.second.first);
  double triple[3];
  CAccurateSum::Local(cancel.data(), cancel.size(), 1, triple);
  CHECK(triple[0] + triple[1] == 1.5 + static_cast<double>(3e-8f));
  /*--- Stride 3. ---*/
  std::vector<float> strided(30);
  for (int i = 0; i < 30; ++i)
    strided[i] = static_cast<float>(std::ldexp(1.0 + i, (i % 7) * 10 - 30)) * (i % 2 ? -1 : 1);
  CHECK(same(strided, 10, 3).first);
  /*--- Empty. ---*/
  r = same({}, 0, 1);
  CHECK(r.first);
  CHECK(r.second.first);
  /*--- Nonfinite terms: reported with the first index. ---*/
  for (const float bad : {std::numeric_limits<float>::quiet_NaN(), std::numeric_limits<float>::infinity()}) {
    std::vector<float> x = {1.0f, 2.0f, 3.0f, 4.0f, bad, 5.0f, bad};
    r = same(x, x.size(), 1);
    CHECK(r.first);
    CHECK_FALSE(r.second.first);
    CHECK(r.second.second == 4);
  }
  /*--- Collective: the batch of float terms equals the batch of the widened doubles. ---*/
  Random random(17 + SU2_MPI::GetRank());
  std::vector<float> local(100 + 10 * SU2_MPI::GetRank());
  for (auto& v : local) v = static_cast<float>((random.Uniform() - 0.5) * std::ldexp(1.0, random.Int(-20, 20)));
  CAccurateSumBatch floats, doubles;
  floats.Add(local);
  doubles.Add(std::vector<double>(local.begin(), local.end()));
  floats.Reduce();
  doubles.Reduce();
  CHECK(floats.Get(0) == doubles.Get(0));
  CHECK(floats.GetAbs(0) == doubles.GetAbs(0));
  CHECK(floats.Finite(0));
}

TEST_CASE("Single precision: transfer tolerance classes", "[DistributedSearch][SinglePrecision]") {
  /*--- Double builds: every class returns its double value unchanged. Single precision: explicit acceptance gates,
   *    geometric thresholds at least the round-off floor 16 eps, diagnostics scaled by 2^29. ---*/
  if (!transfer_tol::kSinglePrecision) {
    CHECK(TransferTol(1e-12, 1e-5) == passivedouble(1e-12));
    CHECK(TransferTol(1e-15, 1e-6) == passivedouble(1e-15));
    for (const double k : {1e-13, 1e-12, 1e-10, 1e-8}) CHECK(DecisionThreshold(k) == passivedouble(k));
    CHECK(DiagnosticTol(1e-12) == passivedouble(1e-12));
  } else {
    CHECK(TransferTol(1e-12, 1e-5) == passivedouble(1e-5));
    const passivedouble floor = 16 * std::numeric_limits<passivedouble>::epsilon();
    for (const double k : {1e-13, 1e-12, 1e-10}) CHECK(DecisionThreshold(k) == floor);
    CHECK(DecisionThreshold(1e-3) == passivedouble(1e-3));
    CHECK(DiagnosticTol(1e-12) == passivedouble(1e-12 * 536870912.0));
  }
}
