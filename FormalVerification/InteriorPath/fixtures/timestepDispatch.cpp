void CEulerSolver::SetTime_Step(CGeometry *geometry, CSolver **solver_container, CConfig *config,
                                unsigned short iMesh, unsigned long Iteration) {
  SU2_ZONE_SCOPED

  /*--- Define an object to compute the speed of sound. ---*/
  struct SoundSpeed {
    FORCEINLINE su2double operator() (const CEulerVariable& nodes, unsigned long iPoint, unsigned long jPoint) const {
      return 0.5 * (nodes.GetSoundSpeed(iPoint) + nodes.GetSoundSpeed(jPoint));
    }

    FORCEINLINE su2double operator() (const CEulerVariable& nodes, unsigned long iPoint) const {
      return nodes.GetSoundSpeed(iPoint);
    }

  } soundSpeed;

  /*--- Define an object to compute the viscous eigenvalue. ---*/
  struct LambdaVisc {
    const su2double gamma, prandtlTurb;

    LambdaVisc(su2double g, su2double pt) : gamma(g), prandtlTurb(pt) {}

    FORCEINLINE su2double lambda(su2double laminarVisc, su2double eddyVisc, su2double density, su2double cp,
                                 su2double thermalCond) const {
      su2double Lambda_1 = (4.0 / 3.0) * (laminarVisc + eddyVisc);
      /// TODO: (REAL_GAS) removing gamma as it cannot work with FLUIDPROP
      su2double Lambda_2 = (thermalCond / cp + eddyVisc / prandtlTurb) * gamma;
      return (Lambda_1 + Lambda_2) / density;
    }

    FORCEINLINE su2double operator() (const CEulerVariable& nodes, unsigned long iPoint, unsigned long jPoint) const {
      su2double laminarVisc = 0.5*(nodes.GetLaminarViscosity(iPoint) + nodes.GetLaminarViscosity(jPoint));
      su2double eddyVisc = 0.5*(nodes.GetEddyViscosity(iPoint) + nodes.GetEddyViscosity(jPoint));
      su2double density = 0.5*(nodes.GetDensity(iPoint) + nodes.GetDensity(jPoint));
      su2double thermalCond = 0.5*(nodes.GetThermalConductivity(iPoint) + nodes.GetThermalConductivity(jPoint));
      su2double cp = 0.5*(nodes.GetSpecificHeatCp(iPoint) + nodes.GetSpecificHeatCp(jPoint));
      return lambda(laminarVisc, eddyVisc, density, cp, thermalCond);
    }

    FORCEINLINE su2double operator() (const CEulerVariable& nodes, unsigned long iPoint) const {
      su2double laminarVisc = nodes.GetLaminarViscosity(iPoint);
      su2double eddyVisc = nodes.GetEddyViscosity(iPoint);
      su2double density = nodes.GetDensity(iPoint);
      su2double thermalCond = nodes.GetThermalConductivity(iPoint);
      su2double cp = nodes.GetSpecificHeatCp(iPoint);
      return lambda(laminarVisc, eddyVisc, density, cp, thermalCond);
    }

  } lambdaVisc(Gamma, Prandtl_Turb);

  /*--- Now instantiate the generic implementation with the two functors above. ---*/

  SetTime_Step_impl(soundSpeed, lambdaVisc, geometry, solver_container, config, iMesh, Iteration);

}
