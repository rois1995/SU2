"""Visualization-only density sampling through saved native surface bindings."""
import json
import math
import struct
import sys
from collections import Counter
from pathlib import Path

R = Path(__file__).resolve().parent
sys.path.insert(0, str(R.parents[3] / "TestCases/adaptation/capability"))
import capcheck

from assess_surface_transaction470 import sha as _sha

ATLAS_SCHEMA = 0x4E41544956454138


def _interpolate(values, weights):
    assert len(values) == len(weights) and len(values) in (1, 2, 3)
    assert all(math.isfinite(float(x)) for x in (*values, *weights))
    assert all(0 <= w <= 1 for w in weights) and math.isclose(sum(weights), 1.0, abs_tol=1e-14)
    result = sum(float(x) * float(w) for x, w in zip(values, weights))
    assert min(values) - 1e-14 <= result <= max(values) + 1e-14
    return result


def _self_test():
    assert _interpolate([1, 3], [.75, .25]) == 1.5
    assert _interpolate([1, 2, 4], [.5, .25, .25]) == 2


def _native_reference(path, points):
    with Path(path).open() as stream:
        magic, version = stream.readline().split()
        assert magic == "SU2_NATIVE_REFERENCE_3D" and int(version) == 9
        header = list(map(float, stream.readline().split()))
        markers, nfacets, analytic, affine, errors, npoints, nsurface, geom_version, revision, curves, atlas, tol = header
        markers, nfacets, npoints = int(markers), int(nfacets), int(npoints)
        assert analytic == 0 and affine >= 0 and geom_version > 0 and tol > 0
        names = [json.loads(stream.readline()) for _ in range(markers)]
        facets, original = {}, {}
        edge_set = set()
        for _ in range(nfacets):
            row = stream.readline().split()
            assert len(row) == 14
            fid, marker = map(int, row[:2])
            assert fid not in facets and 0 <= marker < markers
            verts = []
            for j in range(3):
                node = int(row[2 + 4*j])
                xyz = tuple(map(float, row[3 + 4*j:6 + 4*j]))
                assert len(xyz) == 3 and all(math.isfinite(x) for x in xyz)
                assert node < len(points) and tuple(points[node]) == xyz
                assert original.setdefault(node, xyz) == xyz
                verts.append(node)
            facets[fid] = tuple(verts)
            edge_set.update(tuple(sorted((verts[j], verts[(j+1) % 3]))) for j in range(3))
    assert len(facets) == nfacets and len(original) == npoints
    return facets, sorted(edge_set), names, int(geom_version), len(original)


def surface_seed_density(state, nodes):
    """Return {current boundary node ID: seed rho}; geometry weights are for display only."""
    _self_test()
    seed = R / "m6_bl_inputs/rans_metric_entry364_v1"
    provenance = json.loads((seed / "provenance.json").read_text())
    qualification = json.loads((seed / "qualification.json").read_text())
    restart = seed / "baseline_sa_seed.dat"
    seed_sha = provenance["files"][restart.name]["sha256"]
    assert _sha(restart) == seed_sha == qualification["sha256"]
    points, fields, _ = capcheck.read_restart(restart)
    rho = fields["Density"]
    assert len(rho) == len(points) and bool((rho > 0).all())
    assert all(math.isfinite(float(x)) for x in rho)
    assert [float(rho.min()), float(rho.max())] == qualification["density_range"]

    ref = state["prepared_reference"]
    assert _sha(ref["path"]) == ref["sha256"] and Path(ref["path"]).stat().st_size == ref["bytes"]
    facets, edges, _, geometry_version, original_vertex_checks = _native_reference(ref["path"], points)
    param = state["parameters"]
    assert _sha(param["path"]) == param["sha256"]
    data = Path(param["path"]).read_bytes()
    magic, schema, version, epoch, npnt, nface, policy, atlas = struct.unpack_from("<8Q", data)
    tolerance = struct.unpack_from("<d", data, 64)[0]
    assert magic == ATLAS_SCHEMA and schema == 9 and version == geometry_version
    assert len(data) == 72 + npnt * 72 + nface * 32 + atlas
    bindings = {}
    pos = 72
    for _ in range(npnt):
        node, x, y, z, feature, binding_version, dim, u, v = struct.unpack_from("<QdddQQQdd", data, pos)
        pos += 72
        assert node not in bindings and binding_version == version
        bindings[node] = ((x, y, z), feature, dim, u, v)

    result, dims = {}, Counter()
    adjustments = 0
    for node, (xyz, _tensor) in nodes.items():
        assert node in bindings, f"surface node {node} has no saved binding"
        pxyz, feature, dim, u, v = bindings[node]
        assert tuple(xyz) == pxyz
        if dim == 0:
            source_ids, weights = (feature,), (1.0,)
            assert u == v == 0
        elif dim == 1:
            assert feature < len(edges) and v == 0
            source_ids, weights = edges[feature], (1.0-u, u)
        elif dim == 2:
            assert feature in facets
            source_ids, weights = facets[feature], (1.0-u-v, u, v)
        else:
            raise ValueError(f"unsupported binding dimension {dim} for node {node}")
        w = list(weights)
        assert all(math.isfinite(a) and -1e-12 <= a <= 1+1e-12 for a in w), (node, w)
        clipped = [min(1.0, max(0.0, a)) for a in w]
        total = sum(clipped)
        assert total > 0
        normalized = [a / total for a in clipped]
        adjustments += int(normalized != w)
        assert all(i < len(rho) for i in source_ids)
        result[node] = _interpolate([rho[i] for i in source_ids], normalized)
        dims[dim] += 1
    assert len(result) == len(nodes)
    metadata = dict(field="Density_seed_reference_interp", units="raw saved restart Density units",
        scope="Parametric visualization sampling of the saved pre-transfer CFD seed; not an adapted CFD solution or conservative transfer.",
        source_restart=str(restart.resolve()), source_restart_sha256=seed_sha,
        seed_provenance_sha256=_sha(seed / "provenance.json"), seed_qualification_sha256=_sha(seed / "qualification.json"),
        restart_reader_sha256=_sha(capcheck.__file__),
        native_reference=str(Path(ref["path"]).resolve()), native_reference_sha256=ref["sha256"],
        parameters=str(Path(param["path"]).resolve()), parameters_sha256=param["sha256"],
        restart_points=len(points), boundary_points=len(result), binding_dimension_counts=dict(sorted(dims.items())),
        exact_original_nodes_checks=original_vertex_checks, exact_vertex_boundary_bindings=dims[0],
        roundoff_weight_adjustments=adjustments,
        density_range=[float(min(result.values())), float(max(result.values()))],
        reference_edges=len(edges), reference_facets=len(facets), tolerance=tolerance)
    return result, metadata
