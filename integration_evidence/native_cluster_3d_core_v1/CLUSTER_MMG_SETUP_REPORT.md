# Cluster MMG dependency and SGE search-path correction — 2026-10-11

Repository: /media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated.
Branch: codex/native-3d-core; predecessor1ba501f9dfc97b17688186cf56cca0c49f152818.
Status: package checks PASS; remote MMG build/load and numerical runs pending.

The user's build-native regeneration failed in meson.build at find_library('mmg').
This branch searches mmg_root/lib and includes mmg_root/include, expecting
mmg/libmmg.h plus the combined library with both MMG2D/3D entry points. The
default root is /usr/local. Enabling enable-mmg without the cluster installation
prefix therefore did not find its library.

Read-only inspection of downloaded ClusterResults/reuse_matrix_582358/checkpoint.json
and ClusterResults/jobs/581950_actual_bl_to_euler/campaign.json found enable-mmg=false
and mmg_root=/usr/local. These older native campaigns did not prove MMG installation
or link/load availability. They remain untouched.

The user supplied /home/aero/arausa/global/Libs/mmg-5.8.0/install as the cluster MMG
prefix. README now configures -Dmmg_root explicitly and adds its lib directory
to LD_LIBRARY_PATH; it documents separate SCOTCH prefixes when actually required.
This prefix comes from user input, not a locally verified remote filesystem.
Do not disable MMG and call the required MMG regression completed.

Submission now passes PATH and, when defined, LD_LIBRARY_PATH by variable name
with qsub -v. The actual SGE script already prepends its declared GCC directories
and retains inherited search paths. The fix preserves login MPI/Python/library
paths without exporting the full environment or changing numerical settings.
Prepared bundle identities/gates remain immutable: prepare in a new output folder
after updating if the old bundle froze the prior submission recipe.

package_checks_v10 passes18 lightweight checks (0.773s unittest/1.087s wrapper):
mocked qsub with/without LD_LIBRARY_PATH, no full-environment export, unchanged
serialized job/gate/partial-receipt behavior, and the actual Bash SGE launcher
using a harmless fake Python probe. The probe confirms inherited PATH and MMG/MPI
library paths survive GCC prepending into the job process and retains normal exit
status. Python AST, shell syntax, owned diff checks and source/artifact pins pass.
No compiler, MPI, CFD or real scheduler job ran. No C++/driver/2D/MMG/remeshing/
auditor source or target changed, and no actual remote library link/load is claimed.

Goal1 remains ACTIVE. Communication tracing found the existing World counters
also omit failure elections, direct passive reductions/index construction,
communicator operations and external/vendor work; complete MPI wait/traffic
calibration still needs implementation and runtime evidence. This turn prioritized
repairing the user's concrete cluster build/submission prerequisite. Native MPI
qualification, repeated SGE timing/limits and memory calibration remain pending.
