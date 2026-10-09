/* Native-kernel consistency diagnostic; serial, prescribed smooth fields, no CFD update.
 * Methodology: Diskin & Thomas, AIAA 2012-0609, sections III-IV.
 * Gradient and quadrature errors are reported separately; neither proves solution accuracy.
 */
#include <cmath>
#include <iomanip>
#include <iostream>
#include <memory>
#include <stdexcept>
#include "../../Common/include/geometry/CPhysicalGeometry.hpp"
#include "../../SU2_CFD/include/solvers/CSolver.hpp"
#include "../../SU2_CFD/include/gradients/computeGradientsLeastSquares.hpp"
#include "../../SU2_CFD/include/gradients/computeGradientsGreenGauss.hpp"
#include "../../SU2_CFD/include/gradients/computeHessiansQuadratic.hpp"
#include "../../SU2_CFD/include/numerics_simd/flow/diffusion/common.hpp"

namespace {
constexpr size_t fields = 6;
struct Values { static constexpr size_t nVar = fields; VectorDbl<fields> all; };
struct Error {
  double squared = 0, reference = 0, maximum = 0;
  void add(double error, double exact) {
    if (!std::isfinite(error) || !std::isfinite(exact)) throw std::runtime_error("Nonfinite diagnostic");
    squared += error * error;
    reference += exact * exact;
    maximum = std::max(maximum, std::abs(error));
  }
  double relative() const { return std::sqrt(squared / std::max(reference, 1e-300)); }
};
void evaluate(const su2double* coord, double* value, double gradient[fields][2]) {
  const double x = coord[0], y = coord[1], r = std::hypot(x, y);
  value[0] = x + 2*y; value[1] = x*x; value[2] = x*y; value[3] = y*y;
  value[4] = r; value[5] = std::sin(PI_NUMBER*(x-2*y));
  gradient[0][0] = 1; gradient[0][1] = 2;
  gradient[1][0] = 2*x; gradient[1][1] = 0;
  gradient[2][0] = y; gradient[2][1] = x;
  gradient[3][0] = 0; gradient[3][1] = 2*y;
  gradient[4][0] = r > 0 ? x/r : 0; gradient[4][1] = r > 0 ? y/r : 0;
  gradient[5][0] = PI_NUMBER*std::cos(PI_NUMBER*(x-2*y));
  gradient[5][1] = -2*gradient[5][0];
}
int probe(char* filename) {
  auto* console = std::cout.rdbuf();
  std::cout.rdbuf(nullptr);
  CConfig config(filename, SU2_COMPONENT::SU2_CFD, false);
  if (config.GetnMGLevels() != 0) throw std::runtime_error("Probe requires MGLEVEL=0");
  auto input = std::make_unique<CPhysicalGeometry>(&config, 0, 1);
  input->SetColorGrid_Parallel(&config);
  auto geometry = std::make_unique<CPhysicalGeometry>(input.get(), &config);
  input.reset();
  geometry->SetSendReceive(&config); geometry->SetBoundaries(&config);
  geometry->SetPoint_Connectivity(); geometry->SetElement_Connectivity(); geometry->SetBoundVolume();
  geometry->Check_IntElem_Orientation(&config); geometry->Check_BoundElem_Orientation(&config);
  geometry->SetEdges(); geometry->SetVertex(&config);
  geometry->SetControlVolume(&config, ALLOCATE); geometry->SetBoundControlVolume(&config, ALLOCATE);
  geometry->SetGlobal_to_Local_Point(); geometry->PreprocessP2PComms(geometry.get(), &config);
  if (geometry->GetnDim() != 2) throw std::runtime_error("Probe supports 2D only");
  const auto np = geometry->GetnPoint(), ne = geometry->GetnEdge();
  unsigned long interiorPoints = 0, interiorEdges = 0;
  for (auto p = 0ul; p < np; ++p) interiorPoints += !geometry->nodes->GetBoundary(p);
  for (auto e = 0ul; e < ne; ++e)
    interiorEdges += !geometry->nodes->GetBoundary(geometry->edges->GetNode(e,0)) &&
                     !geometry->nodes->GetBoundary(geometry->edges->GetNode(e,1));
  if (!interiorPoints || !interiorEdges) throw std::runtime_error("Probe needs interior points and edges");
  su2activematrix field(np, fields), limiter(np, fields), integral(ne, 1), area(ne, 2), moment(ne, 4);
  limiter = 1.0; integral = 0.0; area = 0.0; moment = 0.0;
  C3DDoubleMatrix exact(np, fields, 2), grad(np, fields, 2), R(np, 2, 2), hessian(np, fields, 3);
  for (auto p = 0ul; p < np; ++p) {
    double value[fields], derivative[fields][2];
    evaluate(geometry->nodes->GetCoord(p), value, derivative);
    for (size_t v = 0; v < fields; ++v) {
      field(p,v) = value[v];
      for (size_t d = 0; d < 2; ++d) exact(p,v,d) = derivative[v][d];
    }
  }
  // Exact integration of the affine scalar over each median-dual segment, with a=(0.6,0.8).
  for (auto e = 0ul; e < geometry->GetnElem(); ++e) {
    const auto* element = geometry->elem[e];
    const auto* cg = element->GetCG();
    for (size_t f = 0; f < element->GetnFaces(); ++f) {
      const auto p = element->GetNode(element->GetFaces(f,0)), q = element->GetNode(element->GetFaces(f,1));
      const auto edge = geometry->FindEdge(p,q);
      const auto *x = geometry->nodes->GetCoord(p), *y = geometry->nodes->GetCoord(q);
      const double mx = .5*(x[0]+y[0]), my = .5*(x[1]+y[1]), sign = p < q ? 1 : -1;
      const double nx = sign*(cg[1]-my), ny = -sign*(cg[0]-mx);
      area(edge,0) += nx; area(edge,1) += ny;
      const double normals[2] = {nx,ny}, offset[2] = {.5*(cg[0]-mx), .5*(cg[1]-my)};
      for (size_t k = 0; k < 2; ++k)
        for (size_t d = 0; d < 2; ++d) moment(edge,2*k+d) += normals[k]*offset[d];
      integral(edge,0) += (.6*nx+.8*ny)*(.5*(mx+cg[0])+my+cg[1]);
    }
  }
  std::cout.rdbuf(console);
  std::cout << "method,field,node_relative,node_max,reconstruction_relative,viscous_projection_relative,viscous_projection_max\n";
  for (const std::string method : {"exact", "LS", "WLS", "GG", "quadratic_sensor"}) {
    std::cout.rdbuf(nullptr);
    if (method == "exact") grad = exact;
    else if (method == "GG") computeGradientsGreenGauss(nullptr, MPI_QUANTITIES::SOLUTION, PERIODIC_NONE,
        *geometry, config, field, 0, fields, -1, grad, false, false);
    else {
      computeGradientsLeastSquares(nullptr, MPI_QUANTITIES::SOLUTION, PERIODIC_NONE,
          *geometry, config, method != "LS", field, 0, fields, -1, grad, R, false, false);
      if (method == "quadratic_sensor") computeHessiansQuadratic(*geometry, fields, field, grad, hessian);
    }
    std::cout.rdbuf(console);
    Error nodal[fields], reconstruction[fields], diffusion[fields];
    for (auto p = 0ul; p < np; ++p) {
      if (geometry->nodes->GetBoundary(p)) continue;
      for (size_t v = 0; v < fields; ++v)
        for (size_t d = 0; d < 2; ++d) nodal[v].add(grad(p,v,d)-exact(p,v,d), exact(p,v,d));
    }
    for (auto e = 0ul; e < ne; ++e) {
      const auto p = geometry->edges->GetNode(e,0), q = geometry->edges->GetNode(e,1);
      if (geometry->nodes->GetBoundary(p) || geometry->nodes->GetBoundary(q)) continue;
      const auto *x = geometry->nodes->GetCoord(p), *y = geometry->nodes->GetCoord(q);
      su2double midpoint[2] = {.5*(x[0]+y[0]), .5*(x[1]+y[1])};
      double value[fields], derivative[fields][2]; evaluate(midpoint, value, derivative);
      VectorDbl<2> dx; for (size_t d = 0; d < 2; ++d) dx(d) = y[d]-x[d];
      CPair<Values> nodes;
      for (size_t v = 0; v < fields; ++v) { nodes.i.all(v) = field(p,v); nodes.j.all(v) = field(q,v); }
      auto face = nodes;
      reconstruct<fields>(Int(p), Int(q), dx, grad, limiter, LIMITER::NONE, 0, face, su2double(0), su2double(1));
      auto average = averageGradient<fields,2>(Int(p), Int(q), grad);
      correctGradient(nodes, dx, squaredNorm(dx), average);
      const auto* normal = geometry->edges->GetNormal(e);
      const double length = std::hypot(normal[0],normal[1]);
      if (!(length > 0)) throw std::runtime_error("Degenerate dual face");
      for (size_t v = 0; v < fields; ++v) {
        reconstruction[v].add(face.i.all(v)[0]-value[v], value[v]);
        reconstruction[v].add(face.j.all(v)[0]-value[v], value[v]);
        const double projection = derivative[v][0]*normal[0]+derivative[v][1]*normal[1];
        const double error = average(v,0)[0]*normal[0]+average(v,1)[0]*normal[1]-projection;
        diffusion[v].add(error/length, projection/length);
      }
    }
    for (size_t v = 0; v < fields; ++v) {
      std::cout << std::setprecision(15) << method << ',' << v << ',' << nodal[v].relative() << ',' << nodal[v].maximum
                << ',' << reconstruction[v].relative() << ',' << diffusion[v].relative() << ',' << diffusion[v].maximum << '\n';
      if (v == 0 && (method == "LS" || method == "WLS") && nodal[v].maximum > 1e-7)
        throw std::runtime_error("Affine gradient self-check failed");
      if (method == "exact" && v < 4 && diffusion[v].maximum > 1e-7)
        throw std::runtime_error("Exact quadratic viscous correction self-check failed");
    }
  }
  Error quadrature, residual, integratedResidual, correctedQuadrature, correctedResidual;
  double minCosine = 1.0; unsigned long nonpositive = 0;
  su2activevector numerical(np), integrated(np), corrected(np);
  numerical = 0.0; integrated = 0.0; corrected = 0.0;
  for (auto e = 0ul; e < ne; ++e) {
    const auto p = geometry->edges->GetNode(e,0), q = geometry->edges->GetNode(e,1);
    const auto* normal = geometry->edges->GetNormal(e);
    for (size_t d = 0; d < 2; ++d)
      if (std::abs(area(e,d)-normal[d]) > 1e-10*std::max(1.0,std::abs(normal[d])))
        throw std::runtime_error("Independent median-dual normals disagree");
    const auto *x = geometry->nodes->GetCoord(p), *y = geometry->nodes->GetCoord(q);
    const double cosine = ((y[0]-x[0])*normal[0]+(y[1]-x[1])*normal[1]) /
        (std::hypot(y[0]-x[0],y[1]-x[1])*std::hypot(normal[0],normal[1]));
    if (!(cosine > 0)) ++nonpositive;
    minCosine = std::min(minCosine, cosine);
    const double flux = (.6*normal[0]+.8*normal[1])*.5*(field(p,0)+field(q,0));
    // Bellosta et al. (2025), Eqs.10-11: integrated form uses A*S, not normalized S.
    const double correctedFlux = flux + .6*(moment(e,0)+2*moment(e,1)) + .8*(moment(e,2)+2*moment(e,3));
    corrected(p) += correctedFlux; corrected(q) -= correctedFlux;
    numerical(p) += flux; numerical(q) -= flux;
    integrated(p) += integral(e,0); integrated(q) -= integral(e,0);
    if (!geometry->nodes->GetBoundary(p) && !geometry->nodes->GetBoundary(q)) {
      quadrature.add(flux-integral(e,0), integral(e,0));
      correctedQuadrature.add(correctedFlux-integral(e,0), integral(e,0));
    }
  }
  for (auto p = 0ul; p < np; ++p)
    if (!geometry->nodes->GetBoundary(p)) {
      residual.add((numerical(p)-integrated(p))/geometry->nodes->GetVolume(p), 2.2);
      integratedResidual.add(integrated(p)/geometry->nodes->GetVolume(p)-2.2, 2.2);
      correctedResidual.add((corrected(p)-integrated(p))/geometry->nodes->GetVolume(p), 2.2);
    }
  if (integratedResidual.maximum > 1e-7) throw std::runtime_error("Exact integral/divergence self-check failed");
  if (correctedQuadrature.relative() > 1e-12 || correctedResidual.maximum > 1e-7)
    throw std::runtime_error("Published affine flux correction self-check failed");
  std::cout << "# affine_edge_quadrature_relative=" << quadrature.relative()
            << ", affine_convection_residual_max=" << residual.maximum
            << ", corrected_edge_quadrature_relative=" << correctedQuadrature.relative()
            << ", corrected_convection_residual_max=" << correctedResidual.maximum
            << ", exact_integral_residual_max=" << integratedResidual.maximum
            << ", min_edge_normal_cosine=" << minCosine << ", nonpositive_faces=" << nonpositive << '\n';
  return 0;
}
}
int main(int argc, char** argv) {
  int provided;
  SU2_MPI::Init_thread(&argc, &argv, MPI_THREAD_FUNNELED, &provided);
  int status = 0;
  try {
    if (argc != 2 || SU2_MPI::GetSize() != 1) throw std::runtime_error("Usage: gradient_operator_probe case.cfg (serial)");
    status = probe(argv[1]);
  } catch (const std::exception& error) {
    std::cerr << error.what() << '\n'; status = 1;
  }
  SU2_MPI::Finalize();
  return status;
}
