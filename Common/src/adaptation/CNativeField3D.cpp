/*!
 * \file CNativeField3D.cpp
 * \brief Canonical indexed P1 sensors; actual-query composition and exclusive query profiling.
 * Copyright 2012-2026, SU2 Contributors (cf. AUTHORS.md).
 * SU2 is free software under the GNU Lesser General Public License, version 2.1 or later.
 */
#include "../../include/adaptation/CNativeField3D.hpp"
#include "../../include/adaptation/CNativeBoundary3D.hpp"
#include "../../include/adt/CADTElemClass.hpp"
#include "../../include/option_structure.hpp"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <map>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <type_traits>

namespace SU2Native3D {
namespace {
class Timer {
 public:
  Timer(double& value, bool enabled) : value(value), enabled(enabled) {
    if (enabled) start = std::chrono::steady_clock::now();
  }
  ~Timer() {
    if (enabled) value += std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();
  }

 private:
  double& value;
  bool enabled;
  std::chrono::steady_clock::time_point start;
};
std::array<double, 6> Components(Tensor m) { return {m.xx, m.xy, m.xz, m.yy, m.yz, m.zz}; }
void Canonical(std::vector<DonorCell>& original) {
  if (original.size() > MaximumDonors) throw std::invalid_argument("Native 3D original donor patch exceeds budget.");
  std::sort(original.begin(), original.end(), [](const DonorCell& a, const DonorCell& b) { return a.Key() < b.Key(); });
  for (size_t i = 1; i < original.size(); ++i)
    if (original[i].Key() == original[i - 1].Key())
      throw std::invalid_argument("Duplicate native 3D original donor simplex.");
}
}  // namespace
Tensor InterpolateSensor(const DonorCell& donor, const std::array<long double, 4>& weights) {
  std::array<long double, 6> total{};
  for (size_t i = 0; i < 4; ++i) {
    const auto m = Components(donor.sensor[i]);
    for (size_t j = 0; j < 6; ++j) total[j] += weights[i] * m[j];
  }
  return {double(total[0]), double(total[1]), double(total[2]), double(total[3]), double(total[4]), double(total[5])};
}
std::array<Id, 4> DonorCell::Key() const {
  std::array<Id, 4> key;
  for (size_t i = 0; i < 4; ++i) key[i] = cell.v[i].id;
  std::sort(key.begin(), key.end());
  return key;
}
std::vector<Id> AssociatedExtensions(const std::vector<DonorCell>& source,const PlanarReference& reference) {
  std::vector<Id> components;
  for(const auto& donor:source)
    if(donor.extension_component!=UINT64_MAX && reference.Analytic().count(donor.extension_component))
      components.push_back(donor.extension_component);
  std::sort(components.begin(),components.end());components.erase(std::unique(components.begin(),components.end()),components.end());
  return components;
}
size_t FrozenField::Hash::operator()(const std::array<double, 3>& point) const {
  size_t value = 0;
  for (auto x : point) value ^= std::hash<double>{}(x) + size_t(0x9e3779b9) + (value << 6) + (value >> 2);
  return value;
}
FrozenField::FrozenField(std::vector<DonorCell> original, MetricComposition combine, bool enabled,
                         std::vector<Id> components, MetricEdgeBreakpoints breakpoints,
                         std::shared_ptr<const FrozenMetricIdentity> authority)
    : donors(std::move(original)), composition(std::move(combine)), edge_breakpoints(std::move(breakpoints)), timings(enabled),
      identity(authority ? std::move(authority) : std::make_shared<const FrozenMetricIdentity>()) {
  Timer timer(stats.build_seconds, timings);
  Canonical(donors);
  std::sort(components.begin(),components.end());
  if (std::binary_search(components.begin(),components.end(),UINT64_MAX))
    throw std::invalid_argument("Original native3D source cannot be declared as an extension component.");
  std::map<Id, std::pair<Point, std::array<double, 6>>> nodes;
  for (const auto& donor : donors) {
    if (!(Orientation(donor.cell.v[0].p, donor.cell.v[1].p, donor.cell.v[2].p, donor.cell.v[3].p, &stats.kernel) > 0))
      throw std::invalid_argument("Nonpositive native 3D original donor.");
    const auto key = donor.Key();
    if (std::adjacent_find(key.begin(), key.end()) != key.end())
      throw std::invalid_argument("Repeated native donor vertex.");
    for (size_t i = 0; i < 4; ++i) {
      const auto& n = donor.cell.v[i];
      const auto inserted = nodes.emplace(n.id, std::make_pair(n.p, Components(donor.sensor[i])));
      if (inserted.second)
        ValidateTensor(donor.sensor[i], &stats.kernel);
      else {
        const auto& old = inserted.first->second;
        if (old.first.x != n.p.x || old.first.y != n.p.y || old.first.z != n.p.z ||
            old.second != Components(donor.sensor[i]))
          throw std::invalid_argument("Inconsistent shared native 3D original sensor/coordinates.");
      }
    }
  }
  donors.erase(std::remove_if(donors.begin(),donors.end(),[&](const DonorCell& d) {
    return d.extension_component!=UINT64_MAX && !std::binary_search(components.begin(),components.end(),d.extension_component);
  }),donors.end());
  if (!components.empty()) std::stable_partition(donors.begin(),donors.end(),[](const DonorCell& d) {
    return d.extension_component==UINT64_MAX;
  });
  if (donors.size() <= 1) return;  // One donor needs no tree or spatial traversal buffers.
  if constexpr (std::is_same<su2double, double>::value) {
    std::vector<su2double> coordinates;
    std::vector<unsigned long> connectivity, ids;
    std::vector<unsigned short> types(donors.size(), LINE), markers(donors.size(), 0);
    coordinates.reserve(6 * donors.size());
    connectivity.reserve(2 * donors.size());
    ids.reserve(donors.size());
    for (const auto& donor : donors) {
      for (int side = 0; side < 2; ++side)
        for (int axis = 0; axis < 3; ++axis) {
          double bound = axis == 0 ? donor.cell.v[0].p.x : axis == 1 ? donor.cell.v[0].p.y : donor.cell.v[0].p.z;
          for (const auto& n : donor.cell.v) {
            const double p = axis == 0 ? n.p.x : axis == 1 ? n.p.y : n.p.z;
            bound = side ? std::max(bound, p) : std::min(bound, p);
          }
          coordinates.push_back(bound);
        }
      connectivity.push_back(connectivity.size());
      connectivity.push_back(connectivity.size());
      ids.push_back(ids.size());
    }
    // Reuse the 2D import's two-corner LINE box representation; no ADT element-containment tolerance is used.
    search = std::make_unique<CADTElemClass>(3, coordinates, connectivity, types, markers, ids, false);
    stats.index_bytes = search->GetAllocatedBytes();
  }
}
FrozenField::~FrozenField() = default;
void FrozenField::FindCandidates(Point lo, Point hi) {
  if (search) {
    const std::array<su2double, 3> lower{lo.x, lo.y, lo.z}, upper{hi.x, hi.y, hi.z};
    search->DetermineIntersectingElements(lower.data(), upper.data(), candidates);
  } else {
    candidates.resize(donors.size());
    for (size_t i = 0; i < donors.size(); ++i) candidates[i] = i;
  }
  std::sort(candidates.begin(), candidates.end());
}
void FrozenField::RequireEdgePolicy() const {
  if (composition && !edge_breakpoints)
    throw std::runtime_error("Native 3D composed target requires geometry-resolved edge integration.");
}
long double FrozenField::TargetEdgeLength(Point a, Point b) {
  RequireEdgePolicy();
  return EdgeLength(a, b, bool(composition));
}
Point FrozenField::BalancedSplitPoint(Point a,Point b,double fraction) {
  if(!(fraction>0 && fraction<1) || !std::isfinite(fraction))throw std::invalid_argument("Native3D split length fraction must lie strictly between zero and one.");
  const auto requested=TargetEdgeLength(a,b)*fraction;
  long double lo=0,hi=1,t=fraction;
  for(unsigned iteration=0;iteration<48;++iteration) {
    const Point point{double((1-t)*a.x+t*b.x),double((1-t)*a.y+t*b.y),double((1-t)*a.z+t*b.z)};
    if((point.x==a.x && point.y==a.y && point.z==a.z) || (point.x==b.x && point.y==b.y && point.z==b.z))
      throw std::runtime_error("Native3D metric-balanced split is not representable.");
    const auto residual=TargetEdgeLength(a,point)-requested;
    if(std::abs(residual)<=1e-8L*requested)return point;
    if(residual<0)lo=t;else hi=t;
    const auto speed=MetricNorm(a,b,Query(point).target);
    const auto next=t-residual/speed;
    t=next>lo+.1L*(hi-lo) && next<hi-.1L*(hi-lo) ? next : (lo+hi)/2;
  }
  throw std::runtime_error("Native3D full-target split balance remains unresolved after48 steps.");
}
long double FrozenField::SensorEdgeLength(Point a, Point b) { return EdgeLength(a,b,false); }
long double FrozenField::EdgeLength(Point a, Point b, bool composed) {
  Timer edge_time(stats.edge_seconds, timings);
  ++stats.edge_requests;
  try {
    if (!(std::isfinite(a.x) && std::isfinite(a.y) && std::isfinite(a.z) && std::isfinite(b.x) && std::isfinite(b.y) &&
          std::isfinite(b.z)))
      throw std::invalid_argument("Nonfinite native 3D sensor edge.");
    if (a.x == b.x && a.y == b.y && a.z == b.z) throw std::invalid_argument("Native 3D sensor edge must be nonzero.");
    std::vector<const Cell*> cells;
    {
      Timer timer(stats.edge_search_seconds, timings);
      FindCandidates({std::min(a.x, b.x), std::min(a.y, b.y), std::min(a.z, b.z)},
                     {std::max(a.x, b.x), std::max(a.y, b.y), std::max(a.z, b.z)});
      stats.edge_box_candidates += candidates.size();
      stats.maximum_edge_candidates = std::max(stats.maximum_edge_candidates, candidates.size());
      cells.reserve(candidates.size());
      for (auto i : candidates) cells.push_back(&donors.at(i).cell);
    }
    std::vector<SegmentPiece> pieces;
    {
      Timer timer(stats.edge_trace_seconds, timings);
      if (cells.size() == 1) {
        const auto& v = cells.front()->v;
        const Tetrahedron t{v[0].p, v[1].p, v[2].p, v[3].p};
        SegmentPiece piece;
        ++stats.edge_containment_tests;
        const bool first = Barycentric(t, a, piece.begin_weights, &stats.kernel);
        ++stats.edge_containment_tests;
        const bool last = Barycentric(t, b, piece.end_weights, &stats.kernel);
        if (!(first && last)) throw std::runtime_error("Uncovered native 3D singleton sensor edge.");
        // A tetrahedron is convex: no internal source crossing exists between two contained endpoints.
        pieces.push_back(piece);
        ++stats.edge_direct_intervals;
      } else
        pieces = TraceSegment(cells, a, b, &stats.kernel,
            std::count_if(candidates.begin(),candidates.end(),[&](unsigned long i) {
              return donors[i].extension_component==UINT64_MAX;
            }));
    }
    stats.edge_pieces += pieces.size();
    stats.maximum_edge_pieces = std::max(stats.maximum_edge_pieces, pieces.size());
    Timer timer(stats.edge_integral_seconds, timings);
    std::vector<double> cuts;
    if (composed) {
      { Timer geometry(stats.edge_geometry_seconds,timings); cuts = edge_breakpoints(a,b); }
      if (cuts.size() < 2 || cuts.size() > MaximumGeometricEdgeCuts || cuts.front() != 0 || cuts.back() != 1)
        throw std::invalid_argument("Invalid native 3D geometric edge coverage.");
      for (size_t i = 0; i < cuts.size(); ++i)
        if (!std::isfinite(cuts[i]) || (i && !(cuts[i] > cuts[i-1])))
          throw std::invalid_argument("Unordered native 3D geometric edge breakpoints.");
      // Interior rules do not need the endpoint-inclusive rule's extra root-bracketing panels.
    }
    long double total = 0;
    if (composed) {
      struct Panel {
        size_t piece;
        long double low, high, value, error;
        unsigned depth;
        bool operator<(const Panel& other) const { return error < other.error; }
      };
      static_assert(sizeof(Panel)<=160,"Composed edge heap exceeds admitted workspace per panel.");
      size_t evaluations=0;
      auto value = [&](size_t index,long double t) {
        if (++evaluations > 65536) {
          std::ostringstream why;why<<std::setprecision(21)<<"Native 3D composed edge evaluation budget exhausted: edge=("
              <<a.x<<','<<a.y<<','<<a.z<<")..("<<b.x<<','<<b.y<<','<<b.z<<") piece="<<index<<" parameter="<<t;
          throw std::runtime_error(why.str());
        }
        const auto& piece=pieces[index];const auto& donor=donors[candidates[piece.donor]];
        const auto parameter=piece.begin+t*piece.width;
        Point p{double((1-parameter)*a.x+parameter*b.x),double((1-parameter)*a.y+parameter*b.y),
                double((1-parameter)*a.z+parameter*b.z)};
        std::array<long double,4> weights;
        for(unsigned k=0;k<4;++k)weights[k]=(1-t)*piece.begin_weights[k]+t*piece.end_weights[k];
        Tensor target;
        {
          Timer timer(stats.edge_composition_seconds,timings);
          ++stats.compositions;++stats.edge_compositions;
          target=composition(p,InterpolateSensor(donor,weights));
        }
        ValidateTensor(target,&stats.kernel);
        return MetricNorm(a,b,target,&stats.kernel);
      };
      constexpr long double node[8]={.991455371120812639206854697526329L,.949107912342758524526189684047851L,
          .864864423359769072789712788640926L,.741531185599394439863864773280788L,.586087235467691130294144838258730L,
          .405845151377397166906606412076961L,.207784955007898467600689403773245L,0};
      constexpr long double kronrod[8]={.022935322010529224963732008058970L,.063092092629978553290700663189204L,
          .104790010322250183839876322541518L,.140653259715525918745189590510238L,.169004726639267902826583426598550L,
          .190350578064785409913256402421014L,.204432940075298892414161999234649L,.209482141084727828012999174891714L};
      constexpr long double gauss[4]={.129484966168869693270611432679082L,.279705391489276667901467771423780L,
          .381830050505118944950369775488975L,.417959183673469387755102040816327L};
      auto measure = [&](size_t index,long double low,long double high,unsigned depth) {
        const auto middle=(low+high)/2,half=(high-low)/2;
        if(!(middle>low && middle<high))throw std::runtime_error("Native 3D composed edge panel is unrepresentable.");
        // Interior7/15 rules keep rounded feature switches at a cut out of the neighboring large panels.
        const auto center=value(index,middle);
        long double lower=gauss[3]*center,higher=kronrod[7]*center;
        for(unsigned j=0;j<7;++j) {
          const auto pair=value(index,middle-half*node[j])+value(index,middle+half*node[j]);
          higher+=kronrod[j]*pair;if(j%2)lower+=gauss[j/2]*pair;
        }
        const auto scale=pieces[index].width*half;
        return Panel{index,low,high,scale*higher,scale*std::abs(higher-lower),depth};
      };
      long double error=0;
      std::vector<Panel> pending;
      auto retain = [&](Panel panel) {
        total+=panel.value;error+=panel.error;
        pending.push_back(panel);std::push_heap(pending.begin(),pending.end());
      };
      for(size_t index=0;index<pieces.size();++index) {
        const auto& piece=pieces[index];
        if(donors[candidates[piece.donor]].extension_component!=UINT64_MAX)++stats.extended_edge_pieces;
        long double low=0;
        for(const auto cut:cuts) {
          if(cut<=piece.begin || cut>=piece.end)continue;
          const auto high=std::clamp((cut-piece.begin)/piece.width,0.L,1.L);
          if(high>low)retain(measure(index,low,high,0));
          low=high;
        }
        if(low<1)retain(measure(index,low,1,0));
      }
      // The error budget belongs to the whole edge, including tiny source/geometric panels.
      while(error>1e-9L*std::abs(total)+1e-12L) {
        if(pending.empty()) {
          std::ostringstream why;why<<std::setprecision(21)<<"Native 3D composed edge quadrature did not converge: edge=("
              <<a.x<<','<<a.y<<','<<a.z<<")..("<<b.x<<','<<b.y<<','<<b.z<<") integral="<<total<<" error="<<error
              <<" allowance="<<(1e-9L*std::abs(total)+1e-12L)<<" evaluations="<<evaluations;
          throw std::runtime_error(why.str());
        }
        std::pop_heap(pending.begin(),pending.end());auto panel=pending.back();pending.pop_back();
        if(panel.depth>=20)continue; // Its unresolved error stays in the total; other panels may still improve.
        const auto middle=(panel.low+panel.high)/2;
        const auto left=measure(panel.piece,panel.low,middle,panel.depth+1),
                   right=measure(panel.piece,middle,panel.high,panel.depth+1);
        total-=panel.value;error=std::max(0.L,error-panel.error);
        retain(left);retain(right);
      }
    } else for (const auto& piece : pieces) {
      const auto& donor = donors[candidates[piece.donor]];
      if (donor.extension_component!=UINT64_MAX) ++stats.extended_edge_pieces;
      const auto first = MetricNorm(a, b, InterpolateSensor(donor, piece.begin_weights), &stats.kernel);
      const auto last = MetricNorm(a, b, InterpolateSensor(donor, piece.end_weights), &stats.kernel);
      const auto largest = std::max(first, last), r = std::min(first, last) / largest;
      // Integral of sqrt(affine quadratic form), avoiding subtraction of nearly equal cubes.
      total += piece.width * largest * (2.L / 3) * (1 + r + r * r) / (1 + r);
    }
    if (!(total > 0 && std::isfinite(total)))
      throw std::runtime_error("Unrepresentable native 3D original sensor edge length.");
    return total;
  } catch (...) {
    ++stats.edge_failures;
    throw;
  }
}
FieldSample FrozenField::Query(Point point) {
  Timer query_time(stats.query_seconds, timings);
  ++stats.requests;
  try {
    if (!(std::isfinite(point.x) && std::isfinite(point.y) && std::isfinite(point.z))) {
      ++stats.invalid_queries;
      throw std::invalid_argument("Nonfinite native 3D sensor query.");
    }
    const std::array<double, 3> key{point.x, point.y, point.z};
    SensorValue value;
    bool found = false;
    {
      Timer timer(stats.cache_seconds, timings);
      const auto hit = cache.find(key);
      if (hit != cache.end()) {
        value = hit->second;
        found = true;
        ++stats.hits;
      } else
        ++stats.misses;
    }
    if (!found) {
      {
        Timer timer(stats.search_seconds, timings);
        FindCandidates(point, point);
        stats.box_candidates += candidates.size();
        stats.maximum_candidates = std::max(stats.maximum_candidates, candidates.size());
        for (auto index : candidates) {
          const auto& c = donors.at(index).cell;
          const Tetrahedron t{c.v[0].p, c.v[1].p, c.v[2].p, c.v[3].p};
          // Inflated broad-phase boxes never authorize containment.
          bool boxed = true;
          for (size_t axis = 0; axis < 3; ++axis) {
            const bool vertex_lo = std::any_of(
                t.begin(), t.end(), [&](Point p) { return (axis == 0 ? p.x : axis == 1 ? p.y : p.z) <= key[axis]; });
            const bool vertex_hi = std::any_of(
                t.begin(), t.end(), [&](Point p) { return (axis == 0 ? p.x : axis == 1 ? p.y : p.z) >= key[axis]; });
            boxed = boxed && vertex_lo && vertex_hi;
          }
          if (!boxed) continue;
          ++stats.containment_tests;
          if (Barycentric(t, point, value.weights, &stats.kernel)) {
            value.donor = index;
            found = true;
            break;
          }
        }
        if (!found) throw std::runtime_error("Query outside native 3D immutable original donors.");
      }
      {
        Timer timer(stats.interpolation_seconds, timings);
        ++stats.interpolations;
        value.tensor = InterpolateSensor(donors[value.donor], value.weights);
        ValidateTensor(value.tensor, &stats.kernel);
      }
      {
        Timer timer(stats.cache_seconds, timings);
        if (cache.size() == MaximumSensorSamples) {
          cache.erase(recent.front());
          recent.pop_front();
          ++stats.evictions;
        }
        recent.push_back(key);
        try {
          cache.emplace(key, value);
        } catch (...) {
          recent.pop_back();
          throw;
        }
        stats.maximum_cache_entries = std::max(stats.maximum_cache_entries, cache.size());
      }
    }
    FieldSample sample{value.tensor, value.tensor, donors[value.donor].Key(), value.weights,
                       donors[value.donor].extension_component};
    if (sample.extension_component!=UINT64_MAX) ++stats.extended_queries;
    if (composition) {
      Timer timer(stats.composition_seconds, timings);
      ++stats.compositions;
      sample.target = composition(point, sample.sensor);
      ValidateTensor(sample.target, &stats.kernel);
    }
    return sample;
  } catch (...) {
    ++stats.failures;
    throw;
  }
}
}  // namespace SU2Native3D
