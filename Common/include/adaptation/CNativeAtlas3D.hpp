/*!
 * \file CNativeAtlas3D.hpp
 * \brief Bounded exact coordinates and boundary correspondence records for curved atlases.
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */
#pragma once
#include "CNativeTopology3D.hpp"
#include <cstddef>
#include <cstdint>
#include <map>
#include <string>
#include <vector>

namespace SU2NativeGeometry {
struct ExactRatio {
  bool negative=false;
  std::vector<uint32_t> numerator,denominator{1};  // Little-endian, reduced; zero is +[]/[1].
};
struct ExactBary {ExactRatio u,v;};  // Third coordinate is exactly 1-u-v.
struct TracePiece {SU2Native3D::Id facet=0;ExactBary first,last;ExactRatio end;};
struct EdgeTrace {std::vector<TracePiece> pieces;};
struct CoveragePiece {SU2Native3D::Id facet=0;std::array<ExactBary,3> reference,active;};
struct FaceCoverage {std::vector<CoveragePiece> pieces;};
struct BoundaryCoverage {
  std::map<SU2Native3D::Edge,EdgeTrace> traces;
  std::map<SU2Native3D::Face,FaceCoverage> faces;
};
struct AtlasBudget {
  size_t maximum_work=0,maximum_bytes=0,maximum_integer_bits=0;
  size_t work=0,caller_bytes=0,peak_bytes=0;  // Peak is historical across admitted storage windows.
};
// Limits cover input/output storage and temporary limbs in addition to caller_bytes.
// Failed operations leave outputs unchanged; attempted work and peak storage remain charged.
bool ExactInteger(int64_t,ExactRatio&,AtlasBudget&,std::string&);
bool ExactBinary64(double,ExactRatio&,AtlasBudget&,std::string&);
bool ExactValidate(const ExactRatio&,AtlasBudget&,std::string&);
bool ExactAdd(const ExactRatio&,const ExactRatio&,ExactRatio&,AtlasBudget&,std::string&);
bool ExactSubtract(const ExactRatio&,const ExactRatio&,ExactRatio&,AtlasBudget&,std::string&);
bool ExactMultiply(const ExactRatio&,const ExactRatio&,ExactRatio&,AtlasBudget&,std::string&);
bool ExactDivide(const ExactRatio&,const ExactRatio&,ExactRatio&,AtlasBudget&,std::string&);
bool ExactCompare(const ExactRatio&,const ExactRatio&,int&,AtlasBudget&,std::string&);
// Equal endpoints for representable values; otherwise adjacent outward doubles, possibly infinite.
bool ExactBounds(const ExactRatio&,std::array<double,2>&,AtlasBudget&,std::string&);
// Three fixed words (sign,numerator count,denominator count), then 32-bit limbs in 64-bit words.
// Encode replaces its output; Decode advances offset only on success. No variable-size Fields walk.
bool ExactEncode(const ExactRatio&,std::vector<uint64_t>&,AtlasBudget&,std::string&);
bool ExactDecode(const std::vector<uint64_t>&,size_t&,ExactRatio&,AtlasBudget&,std::string&);
}  // namespace SU2NativeGeometry
