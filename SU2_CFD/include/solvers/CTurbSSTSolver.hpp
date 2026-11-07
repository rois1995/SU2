/*!
 * \file CTurbSSTSolver.hpp
 * \brief Headers of the CTurbSSTSolver class
 * \author A. Campos, F. Palacios, T. Economon
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

#pragma once

#include "CTurbSolver.hpp"

/*!
 * \class CTurbSSTSolver
 * \brief Main class for defining the turbulence model solver.
 * \ingroup Turbulence_Model
 * \author A. Campos, F. Palacios, T. Economon
 */
class CTurbSSTSolver final : public CTurbSolver {
private:
  su2double constants[11] = {0.0}; /*!< \brief Constants for the model. */
  SST_ParsedOptions sstParsedOptions;

  /*!
   * \brief Compute nu tilde from the wall functions.
   * \param[in] geometry - Geometrical definition of the problem.
   * \param[in] solver_container - Container vector with all the solutions.
   * \param[in] conv_numerics - Description of the numerical method.
   * \param[in] visc_numerics - Description of the numerical method.
   * \param[in] config - Definition of the particular problem.
   * \param[in] val_marker - Surface marker where the boundary condition is applied.
   */
  void SetTurbVars_WF(const CGeometry *geometry,
                     CSolver **solver_container,
                     const CConfig *config,
                     unsigned short val_marker);

  /*!
   * \brief Compute a suitable under-relaxation parameter to limit the change in the solution variables over
   * a nonlinear iteration for stability.
   * \param[in] config - Definition of the particular problem.
   */
  void ComputeUnderRelaxationFactor(CSolver** solver_container, const CConfig *config) override;

  /*!
   * \brief Resolve the compile-time parameters of CScalarFlux_SST and run one of this solver's
   *        boundaries through the shared boundary flux pass.
   * \param[in] opt - Flags of the boundary, from one of ScalarFluxOptions' named constructors.
   */
  void BoundaryFlux(CGeometry* geometry, CSolver** solver_container, CConfig* config, const ScalarFluxOptions& opt,
                    unsigned short val_marker);

  /*!
   * \brief SST with the P1 options and BC_ENTROPY_P1 (matched k, P1_SST_COUPLED.md M2): the k row of the closure C
   *        markers from the flow sums, R_k,a += k_a M+_a + KIn_a, Jacobian M+_a / rho_a.
   */
  void MatchedBoundaryKine(CGeometry* geometry, CSolver** solver_container, const CConfig* config);

  /*!
   * \brief SST with the P1 options (matched k, SSTPATHS_PLAN.md 0): the k rows of the non-edge mass terms of the flow
   *        residual (split pairs, mixed-cell diagonals, nodal boundary and swept terms, axisymmetric source),
   *        R_k,i += P1KNodalRes_i, Jacobian diagonal P1KNodalJac_i.
   */
  void MatchedNodalKine(CGeometry* geometry, CSolver** solver_container, const CConfig* config);

  /*!
   * \brief SST with VISC_P1_ENTROPY and SST_ENERGY_CONSISTENT (M3): the P1 element k diffusion of the k row, the same
   *        numbers as the energy row of CNSSolver::Viscous_Residual_P1Entropy (P1SSTMatched::KineDiffusion).
   */
  void P1KineDiffusion(CGeometry* geometry, CSolver** solver_container, const CConfig* config);

  /*!
   * \brief JST_ENTROPY_BUDGET with the matched form: the k of the mass flux of the JST correction of each edge
   *        (CEulerSolver::JSTEntropyResidual), k_up(m_c) m_c, the same numbers as the energy row (M1).
   */
  void MatchedCorrectionKine(CGeometry* geometry, CSolver** solver_container, const CConfig* config);

  /*!
   * \brief Options of the convective boundary flux of this marker: the k row is skipped where the matched k of closure
   *        C replaces it (far field, inlets, outlets with BC_ENTROPY_P1 and the matched form).
   */
  ScalarFluxOptions ConvectiveBCOptions(const CConfig* config) const {
    auto opt = ScalarFluxOptions::BoundaryConvective(*config, config->GetBounded_Turb());
    opt.skipFirstConvective = config->GetTke_Matched_P1() && config->GetBC_Entropy_P1();
    return opt;
  }

public:
  /*!
   * \brief Constructor.
   * \param[in] geometry - Geometrical definition of the problem.
   * \param[in] config - Definition of the particular problem.
   * \param[in] iMesh - Index of the mesh in multigrid computations.
   */
  CTurbSSTSolver(CGeometry *geometry, CConfig *config, const CSolver* flow_solver, unsigned short iMesh);

  /*!
   * \brief Destructor of the class.
   */
  ~CTurbSSTSolver() = default;

  /*!
   * \brief Restart residual and compute gradients.
   * \param[in] geometry - Geometrical definition of the problem.
   * \param[in] solver_container - Container vector with all the solutions.
   * \param[in] config - Definition of the particular problem.
   * \param[in] iMesh - Index of the mesh in multigrid computations.
   * \param[in] iRKStep - Current step of the Runge-Kutta iteration.
   * \param[in] RunTime_EqSystem - System of equations which is going to be solved.
   * \param[in] Output - boolean to determine whether to print output.
   */
  void Preprocessing(CGeometry *geometry,
                     CSolver **solver_container,
                     CConfig *config,
                     unsigned short iMesh,
                     unsigned short iRKStep,
                     unsigned short RunTime_EqSystem,
                     bool Output) override;

  /*!
   * \brief Computes the eddy viscosity.
   * \param[in] geometry - Geometrical definition of the problem.
   * \param[in] solver_container - Container vector with all the solutions.
   * \param[in] config - Definition of the particular problem.
   * \param[in] iMesh - Index of the mesh in multigrid computations.
   */
  void Postprocessing(CGeometry *geometry,
                      CSolver **solver_container,
                      CConfig *config,
                      unsigned short iMesh) override;

  /*!
   * \brief Compute the spatial integration using the CScalarFlux_SST edge kernel, which computes
   *        and writes both the convective and the diffusive term of every edge.
   * \param[in] geometry - Geometrical definition of the problem.
   * \param[in] solver_container - Container vector with all the solutions.
   * \param[in] numerics_container - Unused, kept only for the boundary conditions.
   * \param[in] config - Definition of the particular problem.
   * \param[in] iMesh - Index of the mesh in multigrid computations.
   */
  void Upwind_Residual(CGeometry* geometry, CSolver** solver_container, CNumerics** numerics_container,
                       CConfig* config, unsigned short iMesh) override;

  /*!
   * \brief Source term computation.
   * \param[in] geometry - Geometrical definition of the problem.
   * \param[in] solver_container - Container vector with all the solutions.
   * \param[in] numerics_container - Description of the numerical method.
   * \param[in] config - Definition of the particular problem.
   * \param[in] iMesh - Index of the mesh in multigrid computations.
   */
  void Source_Residual(CGeometry *geometry,
                       CSolver **solver_container,
                       CNumerics **numerics_container,
                       CConfig *config,
                       unsigned short iMesh) override;

  /*!
   * \brief Source term computation.
   * \param[in] geometry - Geometrical definition of the problem.
   * \param[in] solver_container - Container vector with all the solutions.
   * \param[in] numerics - Description of the numerical method.
   * \param[in] config - Definition of the particular problem.
   * \param[in] iMesh - Index of the mesh in multigrid computations.
   */
  void Source_Template(CGeometry *geometry,
                       CSolver **solver_container,
                       CNumerics *numerics,
                       CConfig *config,
                       unsigned short iMesh) override;

  /*!
   * \brief Impose the Navier-Stokes wall boundary condition.
   * \param[in] geometry - Geometrical definition of the problem.
   * \param[in] solver_container - Container vector with all the solutions.
   * \param[in] conv_numerics - Description of the numerical method.
   * \param[in] visc_numerics - Description of the numerical method.
   * \param[in] config - Definition of the particular problem.
   * \param[in] val_marker - Surface marker where the boundary condition is applied.
   */
  void BC_HeatFlux_Wall(CGeometry *geometry,
                        CSolver **solver_container,
                        CNumerics *conv_numerics,
                        CNumerics *visc_numerics,
                        CConfig *config,
                        unsigned short val_marker) override;

  void BC_HeatFlux_Wall_Residual(CGeometry* geometry, CSolver** solver_container, CNumerics* conv_numerics,
                                 CNumerics* visc_numerics, CConfig* config, unsigned short val_marker,
                                 unsigned long val_element, unsigned short iNode,
                                 su2double* residualBuffer) override;

  void BC_Viscous_Wall_Strong(const CGeometry* geometry,
                              CSolver** solver_container,
                              const CConfig* config, unsigned short val_marker) override;

  /*!
   * \brief Impose the Navier-Stokes wall boundary condition.
   * \param[in] geometry - Geometrical definition of the problem.
   * \param[in] solver_container - Container vector with all the solutions.
   * \param[in] conv_numerics - Description of the numerical method.
   * \param[in] visc_numerics - Description of the numerical method.
   * \param[in] config - Definition of the particular problem.
   * \param[in] val_marker - Surface marker where the boundary condition is applied.
   */
  void BC_Isothermal_Wall(CGeometry *geometry,
                          CSolver **solver_container,
                          CNumerics *conv_numerics,
                          CNumerics *visc_numerics,
                          CConfig *config,
                          unsigned short val_marker) override;

  void BC_Isothermal_Wall_Residual(CGeometry* geometry, CSolver** solver_container, CNumerics* conv_numerics,
                                  CNumerics* visc_numerics, CConfig* config, unsigned short val_marker,
                                  unsigned long val_element, unsigned short iNode,
                                  su2double* residualBuffer) {

      BC_HeatFlux_Wall_Residual(geometry,solver_container,conv_numerics,visc_numerics,config,
                              val_marker,val_element,iNode,residualBuffer);

  };
  
  /*!
   * \brief Impose the inlet boundary condition.
   * \param[in] geometry - Geometrical definition of the problem.
   * \param[in] solver_container - Container vector with all the solutions.
   * \param[in] conv_numerics - Description of the numerical method.
   * \param[in] visc_numerics - Description of the numerical method.
   * \param[in] config - Definition of the particular problem.
   * \param[in] val_marker - Surface marker where the boundary condition is applied.
   */
  void BC_Inlet(CGeometry *geometry,
                CSolver **solver_container,
                CNumerics *conv_numerics,
                CNumerics *visc_numerics,
                CConfig *config,
                unsigned short val_marker) override;

  /*!
   * \brief Impose the inlet boundary condition.
   * \param[in] geometry - Geometrical definition of the problem.
   * \param[in] solver_container - Container vector with all the solutions.
   * \param[in] conv_numerics - Description of the numerical method.
   * \param[in] visc_numerics - Description of the numerical method.
   * \param[in] config - Definition of the particular problem.
   * \param[in] val_marker - Surface marker where the boundary condition is applied.
   */
  void BC_Inlet_Turbo(CGeometry *geometry,
                      CSolver **solver_container,
                      CNumerics *conv_numerics,
                      CNumerics *visc_numerics,
                      CConfig *config,
                      unsigned short val_marker) override;

  /*!
   * \brief Impose the inlet boundary condition.
   * \param[in] geometry - Geometrical definition of the problem.
   * \param[in] solver_container - Container vector with all the solutions.
   * \param[in] conv_numerics - Description of the numerical method.
   * \param[in] visc_numerics - Description of the numerical method.
   * \param[in] config - Definition of the particular problem.
   * \param[in] val_marker - Surface marker where the boundary condition is applied.
   */
  void BC_Inlet_MixingPlane(CGeometry *geometry,
                            CSolver **solver_container,
                            CNumerics *conv_numerics,
                            CNumerics *visc_numerics,
                            CConfig *config,
                            unsigned short val_marker) override;
  /*!
   * \brief Impose the outlet boundary condition.
   * \param[in] geometry - Geometrical definition of the problem.
   * \param[in] solver_container - Container vector with all the solutions.
   * \param[in] conv_numerics - Description of the numerical method.
   * \param[in] visc_numerics - Description of the numerical method.
   * \param[in] config - Definition of the particular problem.
   * \param[in] val_marker - Surface marker where the boundary condition is applied.
   */
  void BC_Outlet(CGeometry *geometry,
                 CSolver **solver_container,
                 CNumerics *conv_numerics,
                 CNumerics *visc_numerics,
                 CConfig *config,
                 unsigned short val_marker) override;

  /*!
   * \brief Impose the far-field boundary condition, via the CScalarFlux_SST edge kernel.
   * \param[in] geometry - Geometrical definition of the problem.
   * \param[in] solver_container - Container vector with all the solutions.
   * \param[in] conv_numerics - Unused, kept only for the boundary condition dispatch.
   * \param[in] visc_numerics - Unused, kept only for the boundary condition dispatch.
   * \param[in] config - Definition of the particular problem.
   * \param[in] val_marker - Surface marker where the boundary condition is applied.
   */
  void BC_Far_Field(CGeometry *geometry,
                    CSolver **solver_container,
                    CNumerics *conv_numerics,
                    CNumerics *visc_numerics,
                    CConfig *config,
                    unsigned short val_marker) override;

  /*!
   * \brief Impose the fluid interface (sliding mesh) boundary condition, via the CScalarFlux_SST
   *        edge kernel. The convective term is a per-donor weighted average, computed in the same
   *        pass that fills the ghost row of each donor; the diffusive term is computed once per
   *        vertex, after the donor loop, from the ghost state the last donor left behind.
   * \param[in] geometry - Geometrical definition of the problem.
   * \param[in] solver_container - Container vector with all the solutions.
   * \param[in] conv_numerics - Unused, kept only for the boundary condition dispatch.
   * \param[in] visc_numerics - Unused, kept only for the boundary condition dispatch.
   * \param[in] config - Definition of the particular problem.
   */
  void BC_Fluid_Interface(CGeometry *geometry, CSolver **solver_container, CNumerics *conv_numerics,
                          CNumerics *visc_numerics, CConfig *config) override;

  /*!
   * \brief Get the constants for the SST model.
   * \return A pointer to an array containing a set of constants
   */
  inline const su2double* GetConstants() const override { return constants; }

  /*!
   * \brief Store of a set of provided inlet profile values at a vertex.
   * \param[in] val_inlet - vector containing the inlet values for the current vertex.
   * \param[in] iMarker - Surface marker where the coefficient is computed.
   * \param[in] iVertex - Vertex of the marker <i>iMarker</i> where the inlet is being set.
   */
  void SetInletAtVertex(const su2double *val_inlet,
                        unsigned short iMarker,
                        unsigned long iVertex) override;

  /*!
   * \brief Get the set of values imposed at an inlet.
   * \param[in] iMarker - Index of the surface marker.
   * \param[in] iVertex - Vertex of the marker <i>iMarker</i> where the inlet is being set.
   * \param[in] geometry - Geometrical definition of the problem.
   * \param[in,out] val_inlet - vector returning the inlet values for the current vertex.
   * \return Value of the face area at the vertex.
   */
  su2double GetInletAtVertex(unsigned short iMarker, unsigned long iVertex,
                             const CGeometry* geometry, su2double* val_inlet) const override;

  /*!
   * \brief Set a uniform inlet profile
   *
   * The values at the inlet are set to match the values specified for
   * inlets in the configuration file.
   *
   * \param[in] config - Definition of the particular problem.
   * \param[in] iMarker - Surface marker where the coefficient is computed.
   */
  void SetUniformInlet(const CConfig* config, unsigned short iMarker) override;

  /*!
   * \brief Get the value of the turbulent kinetic energy.
   * \return Value of the turbulent kinetic energy.
   */
  inline su2double GetTke_Inf(void) const override { return Solution_Inf[0]; }

  /*!
   * \brief The inlet k of BC_Inlet at a vertex, for closure C of the flow (matched k).
   */
  su2double GetMatchedInletKine(const CConfig* config, CSolver* flowSolver, unsigned short iMarker,
                                unsigned long iVertex) const override;

  /*!
   * \brief Get the value of the turbulent frequency.
   * \return Value of the turbulent frequency.
   */
  inline su2double GetOmega_Inf(void) const override { return Solution_Inf[1]; }

    /*!
   * \brief Compute the convective terms of the goal-oriented metric.
   * \param[in] solver - Physical definition of the problem.
   * \param[in] geometry - Geometrical definition of the problem.
   * \param[in] config - Definition of the particular problem.
   * \param[in] iPoint - Index of current node.
   * \param[in] weights - Weights of each Hessian in the metric.
   */
  void ConvectiveError(CSolver **solver, const CGeometry *geometry, const CConfig *config,
                       unsigned long iPoint, vector<vector<double> > &weights) final;

  /*!
   * \brief Compute the viscous terms of the goal-oriented metric.
   * \param[in] solver - Physical definition of the problem.
   * \param[in] geometry - Geometrical definition of the problem.
   * \param[in] config - Definition of the particular problem.
   * \param[in] iPoint - Index of current node.
   * \param[in] weights - Weights of each Hessian in the metric.
   */
  void ViscousError(CSolver **solver, const CGeometry *geometry, const CConfig *config,
                   unsigned long iPoint, vector<vector<double> > &weights) final;

  /*!
   * \brief Compute the turbulent terms of the goal-oriented metric.
   * \param[in] solver - Physical definition of the problem.
   * \param[in] geometry - Geometrical definition of the problem.
   * \param[in] config - Definition of the particular problem.
   * \param[in] iPoint - Index of current node.
   * \param[in] weights - Weights of each Hessian in the metric.
   */
  void TurbulentError(CSolver **solver, const CGeometry *geometry, const CConfig *config,
                      unsigned long iPoint, vector<vector<double> > &weights) final;

};
