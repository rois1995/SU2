# Papers for SU2 anisotropic metric and Hessian improvements

Read the four papers already requested and the first five additional papers
before extending boundary recovery. The later groups cover metric construction,
gradation, validation and parallel remeshing. Each entry states the implementation
question it should help answer; inclusion does not imply that the published
algorithm is already implemented or that its assumptions hold for SU2.

PDF destination used for this list:
`/media/rausa/4TB/SU2_Versions/SU2_AdapNoExt/Papers`.
Filenames below are suggestions; existing download names are fine. Public reports
and author manuscripts are suitable when the publisher version is unavailable.

## Four papers already requested

1. **D. J. Mavriplis (2003), Revisiting the Least-squares Procedure for Gradient Reconstruction on Unstructured Meshes.** NASA/CR-2003-212683, NIA Report 2003-06.
   [NASA record](https://ntrs.nasa.gov/citations/20040070704) · [PDF](https://ntrs.nasa.gov/api/citations/20040070704/downloads/20040070704.pdf).
   Assess how curvature, stretching and weighting affect gradient recovery;
   distinguish node-centered from cell-centered results.
   Suggested filename: `Mavriplis_2003_LeastSquares_Gradient.pdf`.

2. **B. Diskin and J. L. Thomas (2012), Effects of Mesh Irregularities on Accuracy of Finite-Volume Discretization Schemes.** AIAA 2012-0609.
   [NASA record](https://ntrs.nasa.gov/citations/20120001451) · [PDF](https://ntrs.nasa.gov/api/citations/20120001451/downloads/20120001451.pdf).
   Compare quadratic fits and stencil expansion with our recovery, and separate
   derivative errors from errors in inviscid and viscous discretization.
   Suggested filename: `Diskin_Thomas_2012_MeshIrregularities.pdf`.

3. **H. Guo, Z. Zhang and R. Zhao (2014 preprint), Hessian Recovery for Finite Element Methods.** arXiv:1406.3108v2.
   [Paper](https://arxiv.org/abs/1406.3108) · [PDF](https://arxiv.org/pdf/1406.3108).
   Compare applying polynomial gradient recovery twice with differentiating a
   single scalar fit. Finite-element superconvergence guarantees require their
   own mesh and approximation assumptions.
   Suggested filename: `Guo_Zhang_Zhao_2014_HessianRecovery.pdf`.

4. **F. Alauzet and L. Frazza (2021), Feature-based and goal-oriented anisotropic mesh adaptation for RANS applications in aeronautics and aerospace.** Journal of Computational Physics 439, 110340.
   [DOI](https://doi.org/10.1016/j.jcp.2021.110340).
   Assess RANS sensor/error-estimate choices and the complete adaptation and flow
   convergence process, including physically meaningful output verification.
   Suggested filename: `Alauzet_Frazza_2021_RANS_Adaptation.pdf`.

## Additional papers to read first

5. **B. Diskin and J. L. Thomas (2008), Accuracy of Gradient Reconstruction on Grids with High Aspect Ratio.** NIA Report 2008-12.
   [NASA record](https://ntrs.nasa.gov/citations/20090007493) · [PDF](https://ntrs.nasa.gov/api/citations/20090007493/downloads/20090007493.pdf).
   Highest priority: compare directional higher-order terms and approximate
   mapping using a distance function with our Cartesian/SVD recovery. The report
   studies interior gradients in 2D, explicitly excluding boundary effects; it
   does not establish our partial cubic wall Hessian correction.
   Suggested filename: `Diskin_Thomas_2008_HighAspectRatio.pdf`.

6. **M.-G. Vallet, C.-M. Manole, J. Dompierre, S. Dufour and F. Guibault (2007), Numerical comparison of some Hessian recovery techniques.** International Journal for Numerical Methods in Engineering 72, 987–1007.
   [DOI](https://doi.org/10.1002/nme.2036).
   Compare recovery methods and their boundary behavior; use the analytic-function
   testing methodology to assess one-sided stencils and wall-edge failures.
   Suggested filename: `Vallet_et_al_2007_HessianComparison.pdf`.

7. **M. Picasso, F. Alauzet, H. Borouchaki and P.-L. George (2011), A Numerical Study of Some Hessian Recovery Techniques on Isotropic and Anisotropic Meshes.** SIAM Journal on Scientific Computing 33(3), 1058–1076.
   [DOI](https://doi.org/10.1137/100798715) · [public manuscript](https://www.ljll.fr/~frey/papers/morphing/Picasso%20M.%2C%20A%20numerical%20study%20of%20some%20Hessian%20recovery%20techniques%20on%20isotropic%20and%20anisotropic%20meshes.pdf).
   Assess topology-dependent recovery failures in 2D and 3D. Use the reported
   limitations to challenge our stencil acceptance and convergence tests.
   Suggested filename: `Picasso_et_al_2011_AnisotropicHessian.pdf`.

8. **L. Kamenski and W. Huang (2014), How a Nonconvergent Recovered Hessian Works in Mesh Adaptation.** SIAM Journal on Numerical Analysis 52(4), 1692–1708.
   [DOI](https://doi.org/10.1137/120898796) · [public preprint](https://arxiv.org/abs/1211.2877).
   Determine which aspects of Hessian quality matter for adaptation, rather than
   optimizing derivative accuracy alone. Its finite-element error bound is not
   a guarantee for SU2 RANS solutions.
   Suggested filename: `Kamenski_Huang_2014_NonconvergentHessian.pdf`.

9. **T. Michal, D. S. Kamenetskiy, D. Marcum, F. Alauzet, L. Frazza and A. Loseille (2018), Comparing Anisotropic Error Estimates for ONERA M6 Wing RANS Simulations.** AIAA 2018-0920.
   [DOI](https://doi.org/10.2514/6.2018-0920).
   Assess alternative RANS metric/error-estimate strategies on the same wing
   family as our real 3D fixture. Check equations and prerequisites before
   selecting sensors or goal-based extensions.
   Suggested filename: `Michal_et_al_2018_ONERAM6_ErrorEstimates.pdf`.

## Metric construction and gradation

10. **F. Alauzet (2010), Size gradation control of anisotropic meshes.** Finite Elements in Analysis and Design 46, 181–202.
    [DOI](https://doi.org/10.1016/j.finel.2009.06.028).
    Compare directional size transport and tensor intersection with the integrated
    gradation policy. Assess iteration cost and preservation of geometric BL
    constraints jointly with the agent owning that implementation.
    Suggested filename: `Alauzet_2010_AnisotropicGradation.pdf`.

11. **A. Loseille and F. Alauzet (2011), Continuous Mesh Framework Part I: Well-Posed Continuous Interpolation Error.** SIAM Journal on Numerical Analysis 49(1), 38–60.
    [DOI](https://doi.org/10.1137/090754078).
    Examine the link between interpolation error and metric density, orientation
    and stretching; check our complexity normalization against that framework.
    Suggested filename: `Loseille_Alauzet_2011_ContinuousMesh_I.pdf`.

12. **A. Loseille and F. Alauzet (2011), Continuous Mesh Framework Part II: Validations and Applications.** SIAM Journal on Numerical Analysis 49(1), 61–86.
    [DOI](https://doi.org/10.1137/10078654X).
    Examine practical metric construction and validation of the continuous model.
    Read with Part I when assessing error norms and target complexity.
    Suggested filename: `Loseille_Alauzet_2011_ContinuousMesh_II.pdf`.

## Validation and parallel adaptation

13. **M. C. Galbraith et al. (2020), Verification of Unstructured Grid Adaptation Components.** AIAA Journal 58(9), 3947–3962.
    [DOI](https://doi.org/10.2514/1.J058783) · [public journal manuscript](https://pages.saclay.inria.fr/frederic.alauzet/download/Galbraith_Verification%20of%20unstructured%20grid%20adaptation%20components.pdf).
    Assess component-isolation benchmarks, analytic boundary-layer problems and
    subsequent adaptation/flow convergence. This helps turn frozen-field checks
    into evidence about an accepted adaptation process.
    Suggested filename: `Galbraith_et_al_2020_AdaptationVerification.pdf`.

14. **C. Tsolakis, N. Chrisochoides, M. A. Park, A. Loseille and T. Michal (2021), Parallel Anisotropic Unstructured Grid Adaptation.** AIAA Journal 59(11), 4764–4776.
    [DOI](https://doi.org/10.2514/1.J060270) · [institutional download page](https://digitalcommons.odu.edu/computerscience_fac_pubs/201/).
    Compare parallel adaptation strategies and measured scaling behavior before
    changing partition interfaces, operation scheduling or load balancing.
    Suggested filename: `Tsolakis_et_al_2021_ParallelAdaptation.pdf`.

## Background for later extensions

15. **Z. Zhang and A. Naga (2005), A New Finite Element Gradient Recovery Method: Superconvergence Property.** SIAM Journal on Scientific Computing 26(4), 1192–1213.
    [DOI](https://doi.org/10.1137/S1064827503402837).
    Read for polynomial-preserving recovery foundations and their assumptions.
    Suggested filename: `Zhang_Naga_2005_PolynomialRecovery.pdf`.

16. **L. Frazza, A. Loseille and F. Alauzet (2018), Mesh Adaptation Strategies Using Wall Functions and Low-Reynolds Models.** AIAA 2018-4153.
    [DOI](https://doi.org/10.2514/6.2018-4153) · [authors' publication list](https://pages.saclay.inria.fr/frederic.alauzet/proceedings.html).
    Assess treatment of initially under-resolved near-wall turbulence before
    pursuing automatic first-height selection or changes to wall modeling.
    Suggested filename: `Frazza_et_al_2018_WallFunctions_Adaptation.pdf`.

17. **A. Loseille, F. Alauzet and V. Menier (2017), Unique cavity-based operator and hierarchical domain partitioning for fast parallel generation of anisotropic meshes.** Computer-Aided Design 85, 53–67.
    [DOI](https://doi.org/10.1016/j.cad.2016.09.008) · [public manuscript](https://pages.saclay.inria.fr/frederic.alauzet/download/Loseille_Unique%20cavity%20based%20operator%20and%20hierarchical%20domain%20partitioning%20for%20fast%20parallel%20generation%20of%20anisotropic%20meshes.pdf).
    Assess coordinated cavity operations and partition handling for future
    remesher robustness work. This is separate from the current Hessian changes.
    Suggested filename: `Loseille_et_al_2017_Cavity_Parallel.pdf`.

## Questions to answer before further recovery changes

- Which boundary stencils preserve quadratic polynomials, and when do additional
  tangential terms improve normal/mixed derivatives without hiding a rank defect?
- Does a wall-distance mapping outperform the current affine SVD scaling on
  curved layers, once the physical Hessian and mapping derivatives are included?
- Can published boundary or symmetry treatment address the unsupported wall
  edges without imposing incorrect conditions on pressure or turbulence sensors?
- Which recovery gains survive accepted adaptation and reduce flow/output error
  at a fixed vertex count and measured computation cost?

No paper in this list is claimed to validate our exact residual-based spectral
noise filter. Assess it against physical feature preservation and flow/output
error; keep the option disabled by default until those checks are satisfactory.
