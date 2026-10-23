/*!
 * \file CNativeField3D.cpp
 * \brief Canonical indexed P1 sensors; actual-query composition and exclusive query profiling.
 * Copyright 2012-2026, SU2 Contributors (cf. AUTHORS.md).
 * SU2 is free software under the GNU Lesser General Public License, version 2.1 or later.
 */
#include "../../include/adaptation/CNativeField3D.hpp"
#include "../../include/adaptation/CNativeBoundary3D.hpp"
#include "../../include/adaptation/CNativePointConstruction3D.hpp"
#include "../../include/adt/CADTElemClass.hpp"
#include "../../include/option_structure.hpp"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <map>
#include <iomanip>
#include <limits>
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
    if(donor.extension_component!=UINT64_MAX && reference.Declared(donor.extension_component))
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
                         std::shared_ptr<const FrozenMetricIdentity> authority,bool reuse)
    : donors(std::move(original)), composition(std::move(combine)), edge_breakpoints(std::move(breakpoints)), timings(enabled),reuse_interior(reuse),
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
  // The whole-edge integration also inverts its converged panel partition for a starting parameter. Every returned
  // point is certified by the same safeguarded iteration against its own prefix integral (relative 1e-8).
  RequireEdgePolicy();
  LengthInverse inverse{fraction,std::numeric_limits<long double>::quiet_NaN()};
  const auto requested=EdgeLength(a,b,bool(composition),&inverse)*fraction;
  auto solve=[&](double t,unsigned steps,Point& result) {
    long double lo=0,hi=1;
    for(unsigned iteration=0;iteration<steps;++iteration) {
      const Point point=SegmentPoint(a,b,t);
      if((point.x==a.x && point.y==a.y && point.z==a.z) || (point.x==b.x && point.y==b.y && point.z==b.z))
        throw std::runtime_error("Native3D metric-balanced split is not representable.");
      const auto residual=TargetEdgeLength(a,point)-requested;
      if(std::abs(residual)<=1e-8L*requested){result=point;return true;}
      if(residual<0)lo=t;else hi=t;
      const auto speed=MetricNorm(a,b,Query(point).target);
      const auto next=t-residual/speed;
      t=double(next>lo+.1L*(hi-lo) && next<hi-.1L*(hi-lo) ? next : (lo+hi)/2);
    }
    return false;
  };
  Point result;
  const double guess=double(inverse.parameter);
  if(guess>0 && guess<1 && guess!=fraction) {
    // A short seeded attempt; any failure falls back to the complete iteration from the plain fraction, so every
    // split certified without the proposal is still certified with it (a near-root seed can stall on the bracket rule).
    try { if(solve(guess,8,result))return result; } catch(const std::exception&) {}
  }
  if(solve(fraction,48,result))return result;
  throw std::runtime_error("Native3D full-target split balance remains unresolved after48 steps.");
}
Point FrozenField::SegmentPoint(Point a,Point b,double parameter) {
  Timer query_time(stats.query_seconds,timings);++stats.requests;
  try {
    if(!(parameter>0 && parameter<1))throw std::invalid_argument("Native3D split parameter must be strictly interior.");
    const auto bounds=AffinePointBounds(a,b,parameter);
    const Point nearest{double((1-(long double)parameter)*a.x+(long double)parameter*b.x),
        double((1-(long double)parameter)*a.y+(long double)parameter*b.y),
        double((1-(long double)parameter)*a.z+(long double)parameter*b.z)};
    const double lower[]={bounds[0].x,bounds[0].y,bounds[0].z},upper[]={bounds[1].x,bounds[1].y,bounds[1].z};
    const double hint[]={nearest.x,nearest.y,nearest.z};
    std::array<unsigned,3> preferred;for(unsigned k=0;k<3;++k)preferred[k]=hint[k]>=upper[k];
    {Timer search_time(stats.search_seconds,timings);FindCandidates(bounds[0],bounds[1]);}
    stats.box_candidates+=candidates.size();stats.maximum_candidates=std::max(stats.maximum_candidates,candidates.size());
    for(unsigned mask=0;mask<8;++mask) {
      std::array<double,3> key;bool duplicate=false;
      for(unsigned k=0;k<3;++k) {
        const auto flip=(mask>>k)&1;duplicate=duplicate || (flip && lower[k]==upper[k]);
        key[k]=(preferred[k]^flip) ? upper[k]:lower[k];
      }
      if(duplicate)continue;
      const Point point{key[0],key[1],key[2]};
      if((point.x==a.x && point.y==a.y && point.z==a.z) || (point.x==b.x && point.y==b.y && point.z==b.z))continue;
      {Timer cache_time(stats.cache_seconds,timings);if(cache.count(key)){++stats.hits;return point;}}
      SensorValue value;bool found=false;
      {Timer search_time(stats.search_seconds,timings);
        for(auto i:candidates) {
          const auto& v=donors[i].cell.v;++stats.containment_tests;
          if(Barycentric({v[0].p,v[1].p,v[2].p,v[3].p},point,value.weights,&stats.kernel)) {
            value.donor=i;found=true;break;
          }
        }
      }
      if(!found)continue;
      ++stats.misses;
      {Timer interpolate(stats.interpolation_seconds,timings);++stats.interpolations;
        value.tensor=InterpolateSensor(donors[value.donor],value.weights);ValidateTensor(value.tensor,&stats.kernel);}
      CacheSensor(key,value);return point;
    }
    ++stats.misses;throw std::runtime_error("No source-contained binary64 corner of the native3D affine point.");
  }catch(...){++stats.failures;throw;}
}
void FrozenField::CacheSensor(const std::array<double,3>& key,const SensorValue& value) {
  Timer timer(stats.cache_seconds,timings);
  if(cache.size()==MaximumSensorSamples){cache.erase(recent.front());recent.pop_front();++stats.evictions;}
  recent.push_back(key);
  try {cache.emplace(key,value);}catch(...){recent.pop_back();throw;}
  stats.maximum_cache_entries=std::max(stats.maximum_cache_entries,cache.size());
}
long double FrozenField::SensorEdgeLength(Point a, Point b) { return EdgeLength(a,b,false); }
long double FrozenField::EdgeLength(Point a, Point b, bool composed, LengthInverse* inverse) {
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
      if(cells.size()==1)stats.edge_containment_tests+=2;
      // One canonical segment kernel keeps endpoint weights independent of imported support size.
      try {
        pieces=TraceSegment(cells,a,b,&stats.kernel,
            std::count_if(candidates.begin(),candidates.end(),[&](unsigned long i) {
              return donors[i].extension_component==UINT64_MAX;
            }));
      }catch(const std::runtime_error& error) {
        // Keep an exact replay witness; missing support and a geometrically outside proposal need different repairs.
        std::ostringstream why;why<<std::setprecision(17)<<error.what()<<" edge=("<<a.x<<','<<a.y<<','<<a.z
            <<")..("<<b.x<<','<<b.y<<','<<b.z<<") admitted=";
        for(const auto& donor:donors) {
          why<<donor.cell.id<<'[';for(const auto& node:donor.cell.v)why<<node.id<<',';why<<"]";
        }
        why<<" candidates=";for(auto i:candidates)why<<donors[i].cell.id<<',';
        throw std::runtime_error(why.str());
      }
      if(cells.size()==1)++stats.edge_direct_intervals;
    }
    stats.edge_pieces += pieces.size();
    stats.maximum_edge_pieces = std::max(stats.maximum_edge_pieces, pieces.size());
    Timer timer(stats.edge_integral_seconds, timings);
    std::vector<double> cuts;
    std::function<Tensor(long double, Point, Tensor)> pinned;
    if (composed) {
      {
        Timer geometry(stats.edge_geometry_seconds,timings);
        auto resolved = edge_breakpoints(a,b);
        cuts = std::move(resolved.cuts);
        pinned = std::move(resolved.pinned);
      }
      if (cuts.size() < 2 || cuts.size() > MaximumGeometricEdgeCuts || cuts.front() != 0 || cuts.back() != 1)
        throw std::invalid_argument("Invalid native 3D geometric edge coverage.");
      for (size_t i = 0; i < cuts.size(); ++i)
        if (!std::isfinite(cuts[i]) || (i && !(cuts[i] > cuts[i-1])))
          throw std::invalid_argument("Unordered native 3D geometric edge breakpoints.");
      // Interior rules do not need the endpoint-inclusive rule's extra root-bracketing panels.
    }
    long double total = 0;
    constexpr long double node[8]={.991455371120812639206854697526329L,.949107912342758524526189684047851L,
        .864864423359769072789712788640926L,.741531185599394439863864773280788L,.586087235467691130294144838258730L,
        .405845151377397166906606412076961L,.207784955007898467600689403773245L,0};
    constexpr long double kronrod[8]={.022935322010529224963732008058970L,.063092092629978553290700663189204L,
        .104790010322250183839876322541518L,.140653259715525918745189590510238L,.169004726639267902826583426598550L,
        .190350578064785409913256402421014L,.204432940075298892414161999234649L,.209482141084727828012999174891714L};
    // Optional inversion of the converged partition (piece, local low/high, integral): locate the panel holding the
    // requested fraction of the total and solve for its local parameter with Kronrod-15 prefix integrals and Newton
    // steps. A proposal only (NaN if unresolved); it never changes the returned length or its failure behavior.
    struct Segment { size_t piece; long double low, high, value; };
    std::vector<Segment> segments;
    auto invert = [&](auto&& collect, auto&& integrand) {
      inverse->parameter = std::numeric_limits<long double>::quiet_NaN();
      try {
        collect();
        auto start = [&](const Segment& x) { return pieces[x.piece].begin + x.low * pieces[x.piece].width; };
        std::sort(segments.begin(), segments.end(), [&](const Segment& x, const Segment& y) { return start(x) < start(y); });
        const long double target = total * inverse->fraction;
        long double before = 0;
        size_t k = 0;
        while (k + 1 < segments.size() && before + segments[k].value < target) before += segments[k++].value;
        if (segments.empty()) return;
        const auto s = segments[k];
        const long double width = pieces[s.piece].width, remaining = target - before;
        if (!(s.value > 0 && width > 0 && s.high > s.low)) return;
        auto prefix = [&](long double x) {
          const auto middle = (s.low + x) / 2, half = (x - s.low) / 2;
          long double sum = kronrod[7] * integrand(s.piece, middle);
          for (unsigned j = 0; j < 7; ++j)
            sum += kronrod[j] * (integrand(s.piece, middle - half * node[j]) + integrand(s.piece, middle + half * node[j]));
          return width * half * sum;
        };
        // Safeguarded Newton on the monotone panel prefix: the bracket [low, high] always holds the root.
        long double low = s.low, high = s.high;
        long double x = s.low + (s.high - s.low) * std::clamp(remaining / s.value, 0.L, 1.L);
        for (unsigned iteration = 0; iteration < 32 && x > low && x < high; ++iteration) {
          const auto residual = prefix(x) - remaining;
          if (std::abs(residual) <= 1e-13L * target) break;
          (residual < 0 ? low : high) = x;
          auto next = x - residual / (width * integrand(s.piece, x));
          if (!(next > low && next < high)) next = low + (high - low) / 2;
          if (next == x) break;
          x = next;
        }
        inverse->parameter = pieces[s.piece].begin + x * width;
      } catch (...) {
        inverse->parameter = std::numeric_limits<long double>::quiet_NaN();
      }
    };
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
          const auto sensor=InterpolateSensor(donor,weights);
          target=pinned ? pinned(parameter,p,sensor) : composition(p,sensor);
        }
        ValidateTensor(target,&stats.kernel);
        return MetricNorm(a,b,target,&stats.kernel);
      };
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
      // Depth-limited panels leave the heap but stay in the total; the optional inversion keeps them (bounded, no
      // allocation in the integration loop: an exhausted reserve only disables the proposal).
      std::vector<Panel> retired;bool retired_lost=false;
      if (inverse) try { retired.reserve(256); } catch (...) { retired_lost = true; }
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
        if(panel.depth>=20) { // Its unresolved error stays in the total; other panels may still improve.
          if(inverse && retired.size()<retired.capacity())retired.push_back(panel);else retired_lost=true;
          continue;
        }
        const auto middle=(panel.low+panel.high)/2;
        const auto left=measure(panel.piece,panel.low,middle,panel.depth+1),
                   right=measure(panel.piece,middle,panel.high,panel.depth+1);
        total-=panel.value;error=std::max(0.L,error-panel.error);
        retain(left);retain(right);
      }
      if (inverse && total > 0 && std::isfinite(total))
        invert([&] {
          if (retired_lost) throw std::runtime_error("Incomplete native3D edge partition.");
          for (const auto* list : {&pending, &retired})
            for (const auto& panel : *list) segments.push_back({panel.piece, panel.low, panel.high, panel.value});
        }, value);
    } else {
      std::vector<long double> contributions;
      if (inverse) try { contributions.reserve(pieces.size()); } catch (...) {}
      for (size_t index = 0; index < pieces.size(); ++index) {
        const auto& piece = pieces[index];
        const auto& donor = donors[candidates[piece.donor]];
        if (donor.extension_component!=UINT64_MAX) ++stats.extended_edge_pieces;
        const auto first = MetricNorm(a, b, InterpolateSensor(donor, piece.begin_weights), &stats.kernel);
        const auto last = MetricNorm(a, b, InterpolateSensor(donor, piece.end_weights), &stats.kernel);
        const auto largest = std::max(first, last), r = std::min(first, last) / largest;
        // Integral of sqrt(affine quadratic form), avoiding subtraction of nearly equal cubes.
        const long double contribution = piece.width * largest * (2.L / 3) * (1 + r + r * r) / (1 + r);
        total += contribution;
        if (inverse && contributions.size() < contributions.capacity()) contributions.push_back(contribution);
      }
      if (inverse && total > 0 && std::isfinite(total))
        invert([&] {
          if (contributions.size() != pieces.size()) throw std::runtime_error("Incomplete native3D edge partition.");
          for (size_t index = 0; index < pieces.size(); ++index) segments.push_back({index, 0, 1, contributions[index]});
        }, [&](size_t index, long double t) {
          const auto& piece = pieces[index];
          std::array<long double, 4> weights;
          for (unsigned k = 0; k < 4; ++k) weights[k] = (1 - t) * piece.begin_weights[k] + t * piece.end_weights[k];
          return MetricNorm(a, b, InterpolateSensor(donors[candidates[piece.donor]], weights), &stats.kernel);
        });
    }
    if (!(total > 0 && std::isfinite(total)))
      throw std::runtime_error("Unrepresentable native 3D original sensor edge length.");
    return total;
  } catch (...) {
    ++stats.edge_failures;
    throw;
  }
}
FieldSample FrozenField::QueryMidpoint(Point a,Point b) {
  Timer query_time(stats.query_seconds,timings);++stats.requests;
  try {
    for(auto p:{a,b})if(!(std::isfinite(p.x)&&std::isfinite(p.y)&&std::isfinite(p.z))) {
      ++stats.invalid_queries;throw std::invalid_argument("Nonfinite native3D midpoint endpoint.");
    }
    const Point point{double(((long double)a.x+b.x)/2),double(((long double)a.y+b.y)/2),double(((long double)a.z+b.z)/2)};
    if(!(std::isfinite(point.x)&&std::isfinite(point.y)&&std::isfinite(point.z))) {
      ++stats.invalid_queries;throw std::invalid_argument("Unrepresentable native3D midpoint coordinate.");
    }
    ++stats.misses;
    auto lower=[](double x){const auto p=std::nextafter(x,-INFINITY);return std::isfinite(p)?p:x;};
    auto upper=[](double x){const auto p=std::nextafter(x,INFINITY);return std::isfinite(p)?p:x;};
    size_t selected=donors.size();std::array<long double,4> weights;
    {Timer timer(stats.search_seconds,timings);
      // The ideal midpoint lies inside this outward box; exact arithmetic still decides containment.
      FindCandidates({lower(point.x),lower(point.y),lower(point.z)},{upper(point.x),upper(point.y),upper(point.z)});
      stats.box_candidates+=candidates.size();stats.maximum_candidates=std::max(stats.maximum_candidates,candidates.size());
      for(auto i:candidates) {
        const auto& v=donors[i].cell.v;++stats.containment_tests;
        if(BarycentricMidpoint({v[0].p,v[1].p,v[2].p,v[3].p},a,b,weights,&stats.kernel)){selected=i;break;}
      }
      if(selected==donors.size())throw std::runtime_error("Exact midpoint outside native3D immutable original donors.");
    }
    const auto& donor=donors[selected];FieldSample sample;sample.donor=donor.Key();sample.weights=weights;sample.extension_component=donor.extension_component;
    {Timer timer(stats.interpolation_seconds,timings);++stats.interpolations;
      sample.sensor=InterpolateSensor(donor,weights);ValidateTensor(sample.sensor,&stats.kernel);sample.target=sample.sensor;
    }
    if(sample.extension_component!=UINT64_MAX)++stats.extended_queries;
    if(composition){Timer timer(stats.composition_seconds,timings);++stats.compositions;sample.target=composition(point,sample.sensor);ValidateTensor(sample.target,&stats.kernel);}
    return sample;
  }catch(...){++stats.failures;throw;}
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
        bool checked_hint=false,contained_hint=false;
        std::array<long double,4> hint_weights{};
        if(reuse_interior && last_donor<donors.size() && donors[last_donor].extension_component==UINT64_MAX) {
          ++stats.donor_hint_tests;const auto& v=donors[last_donor].cell.v;
          bool boxed=true;
          for(unsigned axis=0;axis<3;++axis) {
            double lo=INFINITY,hi=-INFINITY;
            for(const auto& n:v){const auto x=axis==0?n.p.x:axis==1?n.p.y:n.p.z;lo=std::min(lo,x);hi=std::max(hi,x);}
            boxed=boxed && lo<=key[axis] && key[axis]<=hi;
          }
          checked_hint=true;
          if(boxed){++stats.containment_tests;contained_hint=Barycentric({v[0].p,v[1].p,v[2].p,v[3].p},point,hint_weights,&stats.kernel);}
          // An embedded original source has a unique strict interior. Closed ties and extensions retain canonical lookup.
          if(contained_hint && std::all_of(hint_weights.begin(),hint_weights.end(),[](long double x){return x>0;})) {
            value.donor=last_donor;value.weights=hint_weights;found=true;++stats.donor_hint_hits;
          }
        }
        if(!found) {
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
            if(checked_hint && index==last_donor) {
              if(!contained_hint)continue;
              value.weights=hint_weights;
            }else {
              ++stats.containment_tests;
              if(!Barycentric(t,point,value.weights,&stats.kernel))continue;
            }
            value.donor=index;found=true;break;
          }
        }
        if (!found) {
          std::ostringstream why;why<<std::setprecision(17)<<"Query outside native 3D immutable original donors: point=("
              <<point.x<<','<<point.y<<','<<point.z<<") admitted=";
          for(const auto& donor:donors){why<<donor.cell.id<<'[';for(auto n:donor.cell.v)why<<n.id<<',';why<<"]";}
          throw std::runtime_error(why.str());
        }
      }
      {
        Timer timer(stats.interpolation_seconds, timings);
        ++stats.interpolations;
        value.tensor = InterpolateSensor(donors[value.donor], value.weights);
        ValidateTensor(value.tensor, &stats.kernel);
      }
      CacheSensor(key,value);
    }
    last_donor=value.donor;
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
