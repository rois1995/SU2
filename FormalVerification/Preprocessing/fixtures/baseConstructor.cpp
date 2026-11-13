CVariable::CVariable(unsigned long npoint, unsigned long ndim, unsigned long nvar,
                     const CConfig *config, bool adjoint) {

  CheckNotInParallelRegion();

  /*--- Initializate the number of dimension and number of variables ---*/
  nPoint = npoint;
  nDim = ndim;
  nVar = nvar;

  /*--- Allocate fields common to all problems. Do not allocate fields
   that are specific to one solver, i.e. not common, in this class. ---*/
  Solution.resize(nPoint,nVar) = su2double(0.0);

  Solution_Old.resize(nPoint,nVar) = su2double(0.0);

  if (config->GetTime_Domain())
    Solution_time_n.resize(nPoint,nVar) = su2double(0.0);

  if (config->GetTime_Marching() != TIME_MARCHING::STEADY)
    Solution_time_n1.resize(nPoint,nVar) = su2double(0.0);

  /*--- User defined source terms ---*/
  if (config->GetPyCustomSource()) UserDefinedSource.resize(nPoint,nVar) = su2double(0.0);

  if (config->GetDiscrete_Adjoint()) {
    if (adjoint && config->GetMultizone_Problem())
      External.resize(nPoint,nVar) = su2double(0.0);

    if (!adjoint) {
      AD_InputIndex.resize(nPoint,nVar) = AD::GetPassiveIndex();
      AD_OutputIndex.resize(nPoint,nVar) = AD::GetPassiveIndex();
    }
  }

  if (config->GetMultizone_Problem())
    Solution_BGS_k.resize(nPoint,nVar) = su2double(0.0);
}
