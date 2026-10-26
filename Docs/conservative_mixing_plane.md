# Conservative axial mixing planes

Set `CONSERVATIVE_MIXINGPLANE= YES` in the common multizone configuration to enable
shared mean **numerical** fluxes at the Giles mixing interfaces. The default is
`NO`. Both zones connected by an interface must use the same setting. The former
private `SU2_MIXING_CONSERVATION` environment variable is no longer used; old study
executables retain their original behavior and source pins.

The supported scope is steady direct compressible Euler/Navier–Stokes/RANS flow,
three-dimensional axial turbomachinery about the z axis, with one OpenMP thread
per MPI rank. Mixing markers must use Giles conditions. The origin must lie on
the z axis, faces must be axial with positive radius and zero normal grid speed,
and connected zones must share density, velocity and pressure reference scales.
At least two strictly ordered radial sample planes and positive blade counts are
required. Forward/reverse differentiation, continuous/discrete adjoints, other
axes/offsets, unsteady flow and multiple OpenMP threads are rejected. General
reverse-flow robustness has not been established.

## Flux construction

For a frozen geometry, let `G_i` be a span-integrated outward raw numerical flux
in mass/axial-momentum/axial-torque/radial-momentum/total-energy coordinates. The
shared target is `C_i = (G_i - M_ij G_j)/2`, where the donor map uses conservative
normalized-annular-area overlap and blade-count scaling. The residual correction
redistributes `C_i - G_i` using normalized interface dual-face areas. Targets are
lagged across each zone solve and applied on the finest grid only. The first
assembly initializes raw fluxes before a shared target is available.

This makes the paired, pitch-scaled **interface flux ledger** balance to roundoff.
It does not make reconstructed physical fluxes equal, guarantee a converged
solution or improve turbine efficiency. The existing sparse local Jacobian is
retained as an approximate linearization; the experimental mean-flux Jacobian is
not included. Turbulence-variable transfer is unchanged and is not made
conservative by this option.

## Strong wall constraints and reactions

An interface can intersect a no-slip wall. As in the existing solver, the strong
wall condition subsequently replaces the momentum equations at those nodes; the
interface correction must not be re-added after that operation.

If `L` is the interface contribution before projection and `Q` is the strong-wall
row projection, its contribution to the wall constraint reaction is
`R_wall = Q L - L`. The constrained equation budget is therefore
`Q L = L + R_wall`. Mass and energy are not cleared by the stationary adiabatic
no-slip momentum constraint. Axial momentum and axial torque at shared nodes can
be carried by this reaction rather than by the remaining flow equations. A full
physical wall-force balance must also include the other residual contributions.

The optional private flux diagnostics record `L` (convective minus viscous plus
coupling), **before strong-wall constraints**. Their roundoff balance must not be
reported as a measured whole-domain/post-constraint momentum or wall-force
balance. The registered wall-reaction test inserts the production cylindrical
correction on a partitioned box face, calls the actual Navier–Stokes wall assembly,
and checks both its projection and the resulting reaction budget. It does not
pretend that the reaction is zero.

## Validation and limits

The completed Aachen three-zone, equal-pitch, eight-rank study verifies recorded
mean numerical mass/axial-momentum/energy/torque balance to approximately
`1.1e-13 percent` in the checked late window. Existing steady MG2 runs establish
finite histories, not general accuracy or a speedup. Pure helper tests cover
unequal blade counts, nonmatching radial samples, constant flux density,
redistribution and invalid inputs; those helper tests do not establish end-to-end
CFD accuracy for every unequal-pitch geometry.

The normal Meson suite registers the standalone conservative mean-flux test with
assertions enabled. Catch tests tagged `[MixingConservation]` cover the
configuration default/explicit switch and the real strong-wall projection.
Further support requires targeted validation rather than assuming that the
restricted Aachen result applies to general mixing-plane configurations.
