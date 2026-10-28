/*!
 * \file CNativeReferenceIO3D.cpp
 * \brief Bounded passive I/O and validated planar authority publication.
 * Copyright 2012-2026, SU2 Contributors (cf. AUTHORS.md).
 * SU2 is free software under the GNU Lesser General Public License, version 2.1 or later.
 */

#include "../../include/adaptation/CNativeReferenceIO3D.hpp"
#include "../../include/adaptation/CNativeCurvedAtlas3D.hpp"
#include <cstdio>
#include <cmath>
#include <charconv>
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <locale>
#include <set>
#include <sstream>
namespace SU2Native3D {
namespace {
struct Header {
  uint64_t markers = 0, facets = 0, analytic = 0, affine = 0;
  uint64_t errors=0,points=0,surface=0,geometry_version=0,revision=0,curves=0,atlas_words=0;
  double geometry_tolerance=0;
  unsigned version = 1;
};
Header ReadHeader(std::istream& in, size_t bytes) {
  std::string magic;
  unsigned version = 0;
  Header h;
  // Each facet needs fourteen numeric tokens; file length bounds counts before allocating any facet storage.
  if (!(in >> magic >> version >> h.markers >> h.facets) || magic != "SU2_NATIVE_REFERENCE_3D" || (version < 1 || version > 9) ||
      !h.facets || h.facets > bytes / 14 || !h.markers || h.markers > h.facets)
    throw std::invalid_argument("Invalid native3D reference header/counts/version.");
  h.version=version;
  if (version>=2 && (!(in>>h.analytic) || (!h.analytic && version<4) || h.analytic>h.facets || h.analytic>bytes/9))
    throw std::invalid_argument("Invalid native3D analytic reference counts.");
  if(version>=4 && (!(in>>h.affine) || (!h.affine && version==4) || h.affine>h.facets || h.affine>bytes/11 || h.analytic>h.facets-h.affine))
    throw std::invalid_argument("Invalid native3D affine reference counts.");
  if(version>=5 && (!(in>>h.errors>>h.points>>h.surface>>h.geometry_version>>h.revision) ||
      !h.errors || h.errors>h.facets || h.errors>bytes/2 || !h.points || h.points>bytes/9 ||
      !h.surface || h.surface>bytes/4 || !h.geometry_version || !h.revision || h.analytic))
    throw std::invalid_argument("Invalid/unsupported native3D mapped reference counts/version.");
  if(version>=7 && (!(in>>h.curves) || (!h.curves&&version==7) || h.curves>h.facets || h.curves>bytes/31))
    throw std::invalid_argument("Invalid native3D curved control count.");
  if(version>=8 && (!(in>>h.atlas_words)||h.atlas_words<11||h.atlas_words>bytes/2))throw std::invalid_argument("Invalid native3D atlas word count.");
  if(version>=9 && (!(in>>h.geometry_tolerance)||!std::isfinite(h.geometry_tolerance)||h.geometry_tolerance<=0))throw std::invalid_argument("Invalid native3D entity geometry tolerance.");
  return h;
}
size_t DecodeBytes(size_t bytes, const Header& h) {
  return transfer_memory::Add(transfer_memory::Mul(bytes, 8), PlanarReference::ConstructionBytes(h.facets,h.analytic,h.affine),
      h.version>=5?transfer_memory::Add(transfer_memory::Mul(h.facets,4096),transfer_memory::Mul(h.points,2304),
          transfer_memory::Mul(h.surface,8256),transfer_memory::Mul(h.atlas_words,256),
          h.version>=8?SU2NativeGeometry::CurvedAtlasWorkspaceBytes(transfer_memory::Mul(h.atlas_words,128)):0):0);
}
}  // namespace
std::string EncodeReference(const ReferenceState& state) {
  if (!state.original || state.marker_names.empty() || state.facets.empty())
    throw std::invalid_argument("Cannot persist an uninitialized native3D reference.");
  std::ostringstream out;
  out.imbue(std::locale::classic());
  out << std::setprecision(17);
  const auto& analytic=state.original->Analytic();
  const auto& affine=state.original->Affine();
  const bool axis=std::any_of(analytic.begin(),analytic.end(),[](const auto& row){return unsigned(row.second.kind)>=3;});
  const auto* parameters=state.original->Parameters();
  const bool atlas=parameters&&(parameters->Mobility().identity||!parameters->Coverage().faces.empty()||!parameters->Coverage().traces.empty());
  const unsigned version=parameters?(parameters->EntityGeometry()?9:atlas?8:!parameters->Geometry().Curves().empty()?7:parameters->Geometry().ReferenceComponents()?6:5):!affine.empty()?4:analytic.empty()?1:axis?3:2;
  out << "SU2_NATIVE_REFERENCE_3D " << version << '\n'
      << state.marker_names.size() << ' ' << state.facets.size();
  if (version>=2) out << ' ' << analytic.size();
  if (version>=4) out << ' ' << affine.size();
  if(parameters)out << ' ' << parameters->Geometry().Errors().size() << ' ' << parameters->Records().size()
      << ' ' << parameters->Surface().size() << ' ' << parameters->Geometry().Version() << ' ' << parameters->Revision();
  if(version>=7)out << ' ' << parameters->Geometry().Curves().size();
  if(version>=8)out << ' ' << BoundaryAtlasBytes(parameters->Coverage(),parameters->Mobility())/8;
  if(version>=9) {if(!parameters->Coverage().faces.empty())throw std::invalid_argument("Entity geometry cannot persist face covers.");
    const double tolerance=parameters->GeometryTolerance();
    if(!std::isfinite(tolerance)||tolerance<=0)throw std::invalid_argument("Invalid native3D entity geometry tolerance.");
    out << ' ' << tolerance;}
  out << '\n';
  for (const auto& name : state.marker_names) out << std::quoted(name) << '\n';
  for (const auto& f : state.facets) {
    const auto& authority=state.original->Get(f.id);
    if (authority.marker!=f.marker) throw std::invalid_argument("Native3D persisted marker differs from its authority.");
    for (size_t i=0;i<3;++i) {
      const auto a=authority.v[i],b=f.v[i];
      if (a.id!=b.id || a.p.x!=b.p.x || a.p.y!=b.p.y || a.p.z!=b.p.z)
        throw std::invalid_argument("Native3D persisted facet differs from its immutable authority.");
    }
    out << f.id << ' ' << f.marker;
    for (auto n : f.v) out << ' ' << n.id << ' ' << n.p.x << ' ' << n.p.y << ' ' << n.p.z;
    out << '\n';
  }
  for (const auto& row : analytic) {
    const auto& g=row.second;
    out << row.first << ' ' << unsigned(g.kind) << ' ' << g.origin.x << ' ' << g.origin.y << ' ' << g.origin.z
        << ' ' << g.extent.x << ' ' << g.extent.y << ' ' << g.extent.z << ' ' << g.tolerance;
    if(axis || version>=4)out << ' ' << g.axis.x << ' ' << g.axis.y << ' ' << g.axis.z;
    out << '\n';
  }
  for(const auto& row:affine) {
    const auto& g=row.second;
    out << row.first << ' ' << g.identity << ' ' << g.version << ' ' << g.dimension << ' '
        << g.origin.x << ' ' << g.origin.y << ' ' << g.origin.z << ' '
        << g.normal.x << ' ' << g.normal.y << ' ' << g.normal.z << ' ' << g.maximum_error << '\n';
  }
  if(parameters) {
    for(const auto& row:parameters->Geometry().Errors())out << row.first << ' ' << row.second << '\n';
    for(const auto& row:parameters->Geometry().Curves()) {
      out << row.first;for(Point p:row.second)out << ' ' << p.x << ' ' << p.y << ' ' << p.z;out << '\n';
    }
    for(const auto& row:parameters->Records()) {
      const auto& p=row.second.point;const auto& b=p.binding;
      out << p.node.id << ' ' << p.node.p.x << ' ' << p.node.p.y << ' ' << p.node.p.z << ' '
          << b.feature << ' ' << b.version << ' ' << b.dimension << ' ' << b.u << ' ' << b.v << '\n';
    }
    for(const auto& row:parameters->Surface()) {
      const auto& f=row.second;out << f.v[0] << ' ' << f.v[1] << ' ' << f.v[2] << ' ' << f.facet << '\n';
    }
  }
  if(version>=8) {
    const auto encoded=EncodeBoundaryAtlas(parameters->Coverage(),parameters->Mobility());RecordStream input(encoded);
    for(size_t i=0;i<encoded.size()/8;++i){uint64_t word;input(word);out << word << '\n';}
  }
  // Optional trailing policy record (any version): readers without it see a legacy sidecar.
  if(state.first_height_tol!=0) {
    if(!(state.first_height_tol>0 && state.first_height_tol<1))throw std::invalid_argument("Invalid native3D first-height policy.");
    out << "BL_FIRST_HEIGHT_TOL " << state.first_height_tol << '\n';
  }
  if (!out)
    throw std::runtime_error("Native3D reference exceeds file limit.");
  auto bytes = out.str();
  DecodeReference(bytes);
  return bytes;
}
static ReferenceState DecodeReferenceImpl(const std::string& bytes,size_t ceiling,size_t& maximum_bytes) {
  if (transfer_memory::Mul(bytes.size(), 8) > ceiling)
    throw std::length_error("Native3D reference parsing exceeds storage budget.");
  std::istringstream in(bytes);
  in.imbue(std::locale::classic());
  const auto header = ReadHeader(in, bytes.size());
  const auto markers = header.markers, facets = header.facets;
  const size_t construction = PlanarReference::ConstructionBytes(facets,header.analytic,header.affine), requested = DecodeBytes(bytes.size(), header);
  maximum_bytes=std::max(maximum_bytes,requested);
  if (requested == SIZE_MAX || requested > ceiling)
    throw std::length_error("Native3D reference decoding exceeds storage budget.");
  ReferenceState result;
  std::set<std::string> names;
  std::set<int> used;
  std::vector<Face> skin;
  for (uint64_t i = 0; i < markers; ++i) {
    std::string name;
    if (!(in >> std::quoted(name)) || name.empty() || name.size() > 1024 || !names.insert(name).second)
      throw std::invalid_argument("Invalid native3D reference marker name.");
    result.marker_names.push_back(std::move(name));
  }
  // Marker indices follow SU2's shared config order, which need not be alphabetical.
  for (uint64_t i = 0; i < facets; ++i) {
    Facet f;
    if (!(in >> f.id >> f.marker) || f.marker < 0 || uint64_t(f.marker) >= markers)
      throw std::invalid_argument("Invalid native3D reference facet marker.");
    for (auto& n : f.v)
      if (!(in >> n.id >> n.p.x >> n.p.y >> n.p.z)) throw std::invalid_argument("Invalid native3D reference point.");
    used.insert(f.marker);
    skin.push_back({f.v[0].id, f.v[1].id, f.v[2].id});
    result.facets.push_back(f);
  }
  std::map<Id,AnalyticSurface> analytic;
  for (uint64_t i=0;i<header.analytic;++i) {
    Id id; unsigned kind; AnalyticSurface g;
    if (!(in>>id>>kind>>g.origin.x>>g.origin.y>>g.origin.z>>g.extent.x>>g.extent.y>>g.extent.z>>g.tolerance))
      throw std::invalid_argument("Invalid native3D analytic reference record.");
    if(header.version>=3 && !(in>>g.axis.x>>g.axis.y>>g.axis.z))throw std::invalid_argument("Invalid native3D analytic axis record.");
    if(header.version<3 && kind>2)throw std::invalid_argument("Axis authority requires native3D version3.");
    g.kind=static_cast<AnalyticSurface::Kind>(kind);
    if (!analytic.emplace(id,g).second) throw std::invalid_argument("Duplicate native3D analytic reference chart.");
  }
  std::map<Id,SU2NativeGeometry::AffineAuthority> affine;
  for(uint64_t i=0;i<header.affine;++i) {
    Id id;SU2NativeGeometry::AffineAuthority g;
    if(!(in>>id>>g.identity>>g.version>>g.dimension>>g.origin.x>>g.origin.y>>g.origin.z>>g.normal.x>>g.normal.y>>g.normal.z>>g.maximum_error) ||
       !affine.emplace(id,g).second)throw std::invalid_argument("Invalid/duplicate native3D affine reference record.");
  }
  std::map<Id,double> errors;BoundaryPoints points;std::vector<SurfaceFace> surface;
  for(uint64_t i=0;i<header.errors;++i) {
    Id id;double value;
    if(!(in>>id>>value) || !errors.emplace(id,value).second)throw std::invalid_argument("Invalid/duplicate native3D mapping-error record.");
  }
  SU2NativeGeometry::MappedFacetReference::CurvedNets curves;
  for(uint64_t i=0;i<header.curves;++i) {
    Id id;SU2NativeGeometry::MappedFacetReference::CubicNet net;
    if(!(in>>id))throw std::invalid_argument("Invalid native3D curved facet identity.");
    for(auto& p:net)if(!(in>>p.x>>p.y>>p.z))throw std::invalid_argument("Invalid native3D curved control record.");
    if(!curves.emplace(id,net).second)throw std::invalid_argument("Duplicate native3D curved control record.");
  }
  for(uint64_t i=0;i<header.points;++i) {
    BoundaryPoint p;auto& b=p.binding;
    if(!(in>>p.node.id>>p.node.p.x>>p.node.p.y>>p.node.p.z>>b.feature>>b.version>>b.dimension>>b.u>>b.v) ||
        !points.emplace(p.node.id,p).second)throw std::invalid_argument("Invalid/duplicate native3D forward-parameter record.");
  }
  surface.reserve(header.surface);
  for(uint64_t i=0;i<header.surface;++i) {
    SurfaceFace f;if(!(in>>f.v[0]>>f.v[1]>>f.v[2]>>f.facet))throw std::invalid_argument("Invalid native3D current-surface record.");
    surface.push_back(f);
  }
  SU2NativeGeometry::BoundaryCoverage coverage;SU2NativeGeometry::MobilityPolicy mobility;
  if(header.version>=8) {
    std::vector<char> encoded;encoded.reserve(transfer_memory::Mul(header.atlas_words,8));RecordStream output(encoded);
    for(uint64_t i=0;i<header.atlas_words;++i){uint64_t word;std::string token;
      if(!(in>>token)||token.size()>20||token.empty()||(token.size()>1&&token[0]=='0')||token.find_first_not_of("0123456789")!=std::string::npos)
        throw std::invalid_argument("Invalid canonical native3D atlas word.");
      const auto decoded=std::from_chars(token.data(),token.data()+token.size(),word);
      if(decoded.ec!=std::errc{}||decoded.ptr!=token.data()+token.size())throw std::invalid_argument("Overflow in native3D atlas word.");output(word);}
    DecodeBoundaryAtlas(encoded,coverage,mobility,requested,transfer_memory::Mul(bytes.size(),8));
    if(header.version>=9 && !coverage.faces.empty())throw std::invalid_argument("Entity geometry cannot contain face covers.");
  }
  in >> std::ws;
  if (!in.eof()) {
    std::string key;double tolerance=0;
    if(!(in>>key>>tolerance) || key!="BL_FIRST_HEIGHT_TOL" || !(tolerance>0 && tolerance<1))
      throw std::invalid_argument("Trailing data or invalid native3D first-height policy record.");
    result.first_height_tol=tolerance;in >> std::ws;
  }
  if (!in.eof() || used.size() != markers) throw std::invalid_argument("Trailing data or unused native3D markers.");
  ValidateDomainSurfaceTopology(skin);
  result.original = std::make_shared<PlanarReference>(result.facets, construction, std::move(analytic),std::move(affine));
  if(header.version>=5) {
    auto mapped=std::make_shared<SU2NativeGeometry::MappedFacetReference>(result.original,errors,header.geometry_version,requested,header.version==6,curves);
    const auto caller=transfer_memory::Add(construction,transfer_memory::Mul(bytes.size(),8),
        transfer_memory::Mul(header.points,256),transfer_memory::Mul(header.surface,64));
    auto reservation=BoundaryAtlasBudget(header.version>=8?transfer_memory::Mul(header.atlas_words,8):0,ceiling,caller);
    try {
      auto parameters=std::make_shared<BoundaryParameterState>(mapped,points,surface,header.revision,ceiling,caller,coverage,mobility,header.version>=8?&reservation:nullptr,header.geometry_tolerance);
      result.original=PlanarReference::WithParameters(result.original,std::move(parameters));
    } catch(const std::exception& error) {
      maximum_bytes=std::max(maximum_bytes,reservation.peak_bytes);
      // Nested certificate helpers return reasons; recover the resource refusal without weakening validation.
      if(reservation.peak_bytes>reservation.maximum_bytes)throw std::length_error(error.what());
      throw;
    }
    maximum_bytes=std::max(maximum_bytes,reservation.peak_bytes);
  }
  return result;
}
ReferenceState DecodeReference(const std::string& bytes,size_t ceiling) {
  size_t maximum_bytes=0;
  try{return DecodeReferenceImpl(bytes,ceiling,maximum_bytes);}
  catch(const std::length_error& error){throw std::invalid_argument(error.what());}
}
bool LoadReference(const std::string& filename, ReferenceState& state, bool& loaded, std::string& reason,
                   size_t ceiling, size_t caller_bytes, size_t& maximum_bytes) {
  World world;
  loaded = false;
  reason.clear();
  auto admit = [&](size_t requested) {
    const auto total = transfer_memory::Add(caller_bytes, requested);
    maximum_bytes = std::max(maximum_bytes, total);
    CLocalFailure local;
    if (total == SIZE_MAX || total > ceiling) local.Set(3, 0, "Native3D reference loading exceeds storage budget.");
    const auto elected = world.elect(local);
    reason = elected.message;
    return !elected.any;
  };
  CLocalFailure failure;
  uint64_t n = 0;
  if (!world.rank) try {
      if (std::filesystem::exists(filename)) {
        n = std::filesystem::file_size(filename);
        if (!n) throw std::runtime_error("Invalid native3D reference file size.");
      }
    } catch (const std::exception& e) {
      failure.Set(1, 0, filename + ": " + e.what());
    }
  CollectiveFailure(failure, CURRENT_FUNCTION);
  n = CPassiveComm::Allreduce(n, CPassiveComm::Op::SUM, world.comm);
  const size_t transport = transfer_memory::TransportBytes(world.size);
  if (!admit(transfer_memory::Add(transfer_memory::Mul(n, 8), transport))) return false;
  if (!n) return true;
  std::vector<char> bytes;
  if (!world.rank) try {
      bytes.resize(n);
      std::ifstream input(filename, std::ios::binary);
      if (!input.read(bytes.data(), bytes.size()) || input.peek() != std::char_traits<char>::eof())
        throw std::runtime_error("Cannot read native3D reference or file size changed.");
    } catch (const std::exception& e) {
      failure.Set(1, 0, filename + ": " + e.what());
    }
  CollectiveFailure(failure, CURRENT_FUNCTION);
  CPassiveComm::BcastRounds(bytes, 0);
  const std::string text(bytes.begin(), bytes.end());
  size_t requested = SIZE_MAX;
  try {
    std::istringstream in(text);
    in.imbue(std::locale::classic());
    requested = DecodeBytes(text.size(), ReadHeader(in, text.size()));
  } catch (const std::exception& e) {
    failure.Set(1, 0, filename + ": " + e.what());
  }
  CollectiveFailure(failure, CURRENT_FUNCTION);
  if (!admit(transfer_memory::Add(requested, transport))) return false;
  ReferenceState pending;CLocalFailure memory_failure;size_t decode_peak=0;
  const auto retained=transfer_memory::Add(caller_bytes,transport);
  try {
    pending=DecodeReferenceImpl(text,ceiling-retained,decode_peak);
  } catch(const std::length_error& e) {
    memory_failure.Set(3,0,filename+": "+e.what());
  } catch (const std::exception& e) {
    failure.Set(1, 0, filename + ": " + e.what());
  }
  maximum_bytes=std::max(maximum_bytes,transfer_memory::Add(retained,decode_peak));
  CollectiveFailure(failure, CURRENT_FUNCTION);
  const auto refused=world.elect(memory_failure);
  if(refused.any){reason=refused.message;return false;}
  state = std::move(pending);
  loaded = true;
  return true;
}
bool LoadReference(const std::string& filename, ReferenceState& state) {
  bool loaded = false;
  std::string reason;
  size_t maximum = 0;
  CLocalFailure failure;
  if (!LoadReference(filename, state, loaded, reason, ReferenceStorageLimit3D, ReferenceStorageBytes(state), maximum))
    failure.Set(3, 0, reason);
  CollectiveFailure(failure, CURRENT_FUNCTION);
  return loaded;
}
void WriteReference(const std::string& filename, const ReferenceState& state) {
  CLocalFailure failure;
  if (!SU2_MPI::GetRank()) try {
      const auto bytes = EncodeReference(state), temporary = filename + ".tmp";
      {
        std::ofstream output(temporary, std::ios::binary | std::ios::trunc);
        if (!output.write(bytes.data(), bytes.size())) throw std::runtime_error("Cannot write native3D reference.");
        output.close();
        if (!output) throw std::runtime_error("Cannot close native3D reference.");
      }
      if (std::rename(temporary.c_str(), filename.c_str()))
        throw std::runtime_error("Cannot publish native3D reference.");
    } catch (const std::exception& e) {
      failure.Set(1, 0, filename + ": " + e.what());
    }
  CollectiveFailure(failure, CURRENT_FUNCTION);
}
}  // namespace SU2Native3D
