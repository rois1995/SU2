/*!
 * \file CNumericsSIMD_TeCNOExt.cpp
 * \brief Vectorized (SIMD) Roe kernels with the extended adjoint TeCNO options (TECNO_ADJOINT_LIMITER, _JACOBIAN).
 * \note Separate from CNumericsSIMD.cpp so that the default kernels are compiled as without these options.
 * \author P. Gomes
 * \version 8.6.0 "Harrier"
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

#include "CNumericsSIMD.hpp"
#include "flow/convection/upwind.hpp"
#include "flow/diffusion/viscous_fluxes.hpp"

/*--- Extended adjoint TeCNO kernels, CRoeScheme<Decorator, true> (TECNO_ADJOINT_LIMITER, TECNO_ADJOINT_JACOBIAN): kept out
 * of CNumericsSIMD.cpp so that the default Roe kernels there are compiled exactly as without these options. ---*/

namespace {

/*!
 * \brief The extended Roe kernel with the ingredients pass of TECNO_ADJOINT_LIMITER= EDGE_ENTROPY_TVD.
 */
template<class Decorator>
class CRoeSchemeTeCNOExt final : public CRoeScheme<Decorator, true> {
public:
  template<class... Ts>
  CRoeSchemeTeCNOExt(const CConfig& config, Ts&... args) : CRoeScheme<Decorator, true>(config, args...) {}

  void TeCNOAdjEdgeIngredients(Int iEdge, const CConfig& config, const CGeometry& geometry, const CVariable& solution,
                               Double updateMask) const override {
    this->tecnoAdjTVDIngredients(iEdge, config, geometry, static_cast<const CEulerVariable&>(solution), updateMask);
  }
};

template<int nDim>
CNumericsSIMD* createTeCNOAdjExt(const CConfig& config, int iMesh, const CVariable* turbVars) {
  const bool ideal_gas = (config.GetKind_FluidModel() == STANDARD_AIR) ||
                         (config.GetKind_FluidModel() == IDEAL_GAS);
  if (!ideal_gas || config.GetKind_ConvNumScheme_Flow() != SPACE_UPWIND || config.GetKind_Upwind_Flow() != UPWIND::ROE)
    return nullptr;
  if (config.GetViscous()) return new CRoeSchemeTeCNOExt<CCompressibleViscousFlux<nDim> >(config, iMesh, turbVars);
  return new CRoeSchemeTeCNOExt<CNoViscousFlux<nDim> >(config, iMesh, turbVars);
}

} // namespace

CNumericsSIMD* CNumericsSIMD::CreateTeCNOAdjExtNumerics(const CConfig& config, int nDim, int iMesh,
                                                        const CVariable* turbVars) {
  if (nDim == 2) return createTeCNOAdjExt<2>(config, iMesh, turbVars);
  if (nDim == 3) return createTeCNOAdjExt<3>(config, iMesh, turbVars);
  return nullptr;
}
