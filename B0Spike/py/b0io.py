"""Reader of the B0 spike mesh format (.b0in)."""
import numpy as np, struct

def read(path):
    with open(path, 'rb') as f:
        assert f.read(8)[:7] == b'B0MESH1'
        dim, nv, ne, nmk = struct.unpack('<4Q', f.read(32))
        prm = struct.unpack('<5d', f.read(40)); iprm = struct.unpack('<2i', f.read(8))
        nm = dim * (dim + 1) // 2
        X = np.frombuffer(f.read(8 * nv * dim), '<f8').reshape(nv, dim)
        M = np.frombuffer(f.read(8 * nv * nm), '<f8').reshape(nv, nm)
        T = np.frombuffer(f.read(8 * ne * (dim + 1)), '<u8').reshape(ne, dim + 1).astype(np.int64)
        Tref = np.frombuffer(f.read(4 * ne), '<i4')
        markers = []
        for _ in range(nmk):
            ln, ref, nf = struct.unpack('<3Q', f.read(24))
            name = f.read(ln).decode()
            F = np.frombuffer(f.read(8 * nf * dim), '<u8').reshape(nf, dim).astype(np.int64)
            markers.append((name, ref, F))
    return dict(dim=dim, X=X, M=M, T=T, Tref=Tref, markers=markers,
                params=dict(hmin=prm[0], hmax=prm[1], hgrad=prm[2], hausd=prm[3], angle=prm[4], surface=iprm[0], bl=iprm[1]))

def full(M, dim):
    n = M.shape[0]; A = np.zeros((n, dim, dim)); k = 0
    for i in range(dim):
        for j in range(i, dim):
            A[:, i, j] = M[:, k]; A[:, j, i] = M[:, k]; k += 1
    return A

def logAR(M, dim):
    w = np.linalg.eigvalsh(full(M, dim))
    return 0.5 * np.log10(w[:, -1] / w[:, 0])
