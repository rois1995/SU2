# Fixed-face diagnostic before multi-point reconstruction

Input: joint_runtime_v4/mpi1/cylinder_axis1_m1_s0_rejected.json.
For the constant isotropic diagnostic field, a triangle with area A and summed
squared edge lengths S bounds the quality of every attached tetrahedron:
q_max = 6 (2/9)^(1/3) (A/S)^(2/3). The optimum places its apex over the centroid
at height sqrt(2S/9). This follows by maximizing
12(A h)^(2/3)/(4S/3 + 3h^2). It is an upper bound, not a feasible simultaneous
placement for all faces of a cavity.

At each base vertex the scaled Jacobian is at most sqrt(2) sin(theta), where theta
is its triangle angle. Thus sqrt(2) min_vertex sin(theta) is a further upper bound.
All 160 saved physical triangles have q_max >=0.209609 and this J upper bound
>=0.079040. None is individually ruled out by q>=0.20/J>=0.05. This does not prove
a jointly feasible placement or completed field coverage/conservation/MPI gates.
It motivates testing multiple interior apices before changing the physical surface.

Two-apex proposal: partition the cavity skin into two disks, connect each disk to
its own interior point, and bridge each interface-ring edge with a tetrahedron
containing both apices. Preserve the original oriented outer skin except the
declared projected split. Validate topology, positive orientation, exact overlap/
contact and actual frozen metric before any production publication.
