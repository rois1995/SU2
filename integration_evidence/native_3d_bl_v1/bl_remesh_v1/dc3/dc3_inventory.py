#!/usr/bin/env python3
"""DC-3 (BL_REMESH_PLAN rev 3): inventory of first-height tolerances in C++ and audit helpers.

Read-only scan. Every line that compares a wall height / altitude / first-cell height against a literal tolerance or
range is listed with its file:line, sha256 of the file, and a classification:
  T      -> must become the single ADAP_BL_FIRST_HEIGHT_TOL accessor (WP-B)
  KEEP   -> a distinct tolerance that WP-B must preserve (footprint, geometry, quadrature, ...)
  REVIEW -> matched by the pattern but not classified by the explicit table (must be zero for completeness)
The explicit table below is the reviewed classification; the scan proves nothing is missed by the pattern set.
"""
import hashlib, json, pathlib, re, sys

ROOT = pathlib.Path(__file__).resolve().parents[4]
CPP = sorted([*ROOT.glob('Common/src/adaptation/*.cpp'), *ROOT.glob('Common/include/adaptation/*.hpp'),
              *ROOT.glob('SU2_CFD/src/adaptation/*.cpp'), *ROOT.glob('SU2_CFD/include/adaptation/*.hpp'),
              ROOT/'SU2_CFD/src/solvers/CSolver.cpp', ROOT/'Common/src/CConfig.cpp'])
PY = sorted([*ROOT.glob('integration_evidence/*.py'), ROOT/'TestCases/adaptation/capability/capcheck.py'])

HEIGHT = re.compile(r'height|Height|altitude|Altitude|\bh0\b|first_height|firstHeight|relative_height')
TOL = re.compile(r'(?<![\w.])(1e-8|1e-08|1e-6|1e-06|1e-10|1e-9|1e-7|\.?0?\.5\b|2\.0?\b|\.98|1\.02|0\.9\b|1\.1\b|1e-3|1e-4)')
COMPARE = re.compile(r'[<>]=?|abs\(|fabs\(')

# Reviewed classification: (relative path, line) -> (class, note). Lines are those of the scanned revision.
TABLE = {
    ('Common/src/adaptation/CNativeBoundary3D.cpp', 369): ('T', '3D CheckFirstHeight height_failures error>1e-8 (all 3D callers)'),
    ('Common/src/adaptation/CNativeBoundary3D.cpp', 361): ('KEEP', 'footprint-resolution tolerance (barycentric rounding guard)'),
    ('Common/include/adaptation/CNativeBoundary2D.hpp', 273): ('T', '2D boundary first-row height check'),
    ('Common/include/adaptation/CNativeBoundary2D.hpp', 572): ('T', '2D layer op height check'),
    ('Common/include/adaptation/CNativeEngine2D.hpp', 191): ('T', '2D HEIGHT demand: add excess over tol, not >1e-8'),
    ('Common/include/adaptation/CNativeEngine2D.hpp', 653): ('T', '2D final missed-height count'),
    ('Common/include/adaptation/CNativeEngine2D.hpp', 851): ('T', '2D bind validity (<1e-8 -> <=tol)'),
    ('Common/src/adaptation/CNativeRemesher.cpp', 257): ('T', '2D remesher report height flag'),
    ('Common/src/adaptation/CNativeRemesher.cpp', 344): ('T', '2D protected_cell marking'),
    ('Common/src/adaptation/CNativeRemesher.cpp', 345): ('T', '2D protected_cell marking (continuation)'),
    ('Common/src/adaptation/CNativeRemesher.cpp', 430): ('T', 'contract text "relative error<=1e-8"'),
    ('Common/src/adaptation/CNativeRemesher.cpp', 516): ('T', '2D final missedHeight'),
    ('integration_evidence/run_native_airfoil_audits.py', 56): ('T', 'airfoil audit first_height <=1e-8'),
    ('integration_evidence/run_native_opposing_audits.py', 66): ('T', 'opposing audit max_relative_height_error<=1e-8'),
    ('TestCases/adaptation/capability/capcheck.py', 405): ('KEEP', 'metric-field hmin check (0.5*h0 floor of the METRIC, not the first row)'),
    ('TestCases/adaptation/capability/capcheck.py', 34): ('T', 'capability BL cell height window [0.5,2]h0 -> T'),
    ('Common/include/adaptation/CNativeSupport3D.hpp', 227): ('OTHER', 'mixture PDF guard'),
    ('Common/src/adaptation/CNativeBoundary3D.cpp', 2538): ('OTHER', 'move fraction halving'),
    ('Common/src/adaptation/CNativeEngine3D.cpp', 67): ('KEEP', 'geometry ceiling min(configured,.5 h0)'),
    ('Common/src/adaptation/CNativeRegion3D.cpp', 977): ('OTHER', 'edge length screen'),
    ('SU2_CFD/src/adaptation/CBoundaryLayerMetric.cpp', 602): ('OTHER', 'fade start full=max(h0,.9 thickness)'),
    ('SU2_CFD/src/adaptation/CBoundaryLayerMetric.cpp', 868): ('KEEP', 'analytic chord epsilon (geometry)'),
    ('SU2_CFD/src/adaptation/CBoundaryLayerMetric.cpp', 1011): ('WPA', '2D/3D BL corner limit replaced by D1 (WP-A)'),
    ('SU2_CFD/src/adaptation/CBoundaryLayerMetric.cpp', 2366): ('KEEP', 'quadrature distance levels'),
    ('SU2_CFD/src/adaptation/CBoundaryLayerMetric.cpp', 2387): ('KEEP', 'quadrature distance roundoff'),
    ('SU2_CFD/include/adaptation/CBoundaryLayerRemesher.hpp', 91): ('MMG-REPORT', 'MMG BL path report counter'),
    ('SU2_CFD/include/adaptation/CBoundaryLayerRemesher.hpp', 107): ('MMG-REPORT', 'MMG BL path report counter [0.5,2]h0'),
    ('SU2_CFD/src/adaptation/CBoundaryLayerRemesher.cpp', 321): ('MMG-REPORT', 'MMG BL path report'),
    ('SU2_CFD/src/adaptation/CBoundaryLayerRemesher.cpp', 856): ('MMG-REPORT', 'MMG BL nearest-height report below .5h0'),
    ('SU2_CFD/src/adaptation/CBoundaryLayerRemesher.cpp', 857): ('MMG-REPORT', 'MMG BL nearest-height report above 2h0'),
    ('SU2_CFD/src/adaptation/CBoundaryLayerRemesher.cpp', 919): ('MMG-REPORT', 'MMG BL corner-face report [0.5,2]'),
    ('SU2_CFD/src/adaptation/CBoundaryLayerRemesher.cpp', 1097): ('OTHER', 'MMG hmin parameter'),
    ('SU2_CFD/src/adaptation/CBoundaryLayerRemesher.cpp', 1101): ('OTHER', 'MMG hausd parameter'),
    ('SU2_CFD/src/adaptation/CBoundaryLayerRemesher.cpp', 1389): ('MMG-REPORT', 'MMG report text'),
    ('SU2_CFD/src/adaptation/CBoundaryLayerRemesher.cpp', 1394): ('MMG-REPORT', 'MMG report text'),
    ('SU2_CFD/src/adaptation/CBoundaryLayerRemesher.cpp', 1414): ('MMG-REPORT', 'MMG report text'),
    ('integration_evidence/audit_engine_scaling.py', 79): ('T-HELPER', 'independent completion heightError<=1e-8'),
    ('integration_evidence/audit_native_unsteady.py', 123): ('T-HELPER', 'height_error<1e-8 (strict <, also inclusive-bound mismatch)'),
    ('integration_evidence/audit_rae_unsteady.py', 38): ('T-HELPER', 'max(errors)<=1e-8'),
    ('integration_evidence/check_native_audit_inputs.py', 17): ('T-HELPER', 'max_relative_height_error<=1e-8'),
    ('integration_evidence/check_native_cross_grid_completion_v31.py', 60): ('T-HELPER', 'RANS h<=1e-8'),
    ('integration_evidence/check_native_integration_completion.py', 19): ('T-HELPER', 'max_relative_height_error<=1e-8'),
    ('integration_evidence/check_native_metric_integration.py', 60): ('T-HELPER', 'RANS h<=1e-8'),
    ('integration_evidence/inspect_rae2822_solution.py', 53): ('T-HELPER', 'height_error<=1e-8'),
    ('integration_evidence/run_native_bl_audits.py', 69): ('T', 'BL audit max_relative_height_error<=1e-8 (named in WP-B)'),
    ('Common/include/adaptation/CNativeBoundary2D.hpp', 437): ('OTHER', '2D apex placement at h0 (construction, not a tolerance)'),
    ('Common/include/adaptation/CNativeSupport3D.hpp', 114): ('OTHER', 'support component profile'),
    ('Common/src/adaptation/CNativeBoundary3D.cpp', 1864): ('OTHER', 'placement fraction halving'),
    ('Common/src/adaptation/CNativeBoundary3D.cpp', 2118): ('KEEP', 'geometry ceiling min(surface_tolerance,.5 h0)'),
    ('Common/src/adaptation/CNativeBoundary3D.cpp', 2552): ('OTHER', 'repair loss progress'),
    ('Common/src/adaptation/CNativeBoundary3D.cpp', 2687): ('OTHER', 'repair loss progress'),
    ('Common/src/adaptation/CNativeEngine3D.cpp', 771): ('OTHER', 'row depth formula'),
    ('Common/src/adaptation/CNativeRegion3D.cpp', 283): ('OTHER', 'region geometry'),
    ('Common/src/adaptation/CNativeRegion3D.cpp', 321): ('OTHER', 'bisection tolerance'),
    ('Common/src/adaptation/CNativeRegion3D.cpp', 323): ('OTHER', 'bisection midpoint'),
    ('Common/src/adaptation/CNativeRegion3D.cpp', 866): ('OTHER', 'lattice offset'),
    ('SU2_CFD/include/adaptation/CBoundaryLayerRemesher.hpp', 96): ('MMG-REPORT', 'MMG report comment'),
    ('SU2_CFD/include/adaptation/CBoundaryLayerRemesher.hpp', 115): ('MMG-REPORT', 'MMG report comment [0.5,2]h0'),
    ('SU2_CFD/include/adaptation/CReferenceWall.hpp', 264): ('OTHER', 'corner growth'),
    ('SU2_CFD/src/adaptation/CBoundaryLayerMetric.cpp', 1091): ('OTHER', 'hn law (WP-A input)'),
    ('SU2_CFD/src/adaptation/CBoundaryLayerMetric.cpp', 1110): ('KEEP', 'analytic chord epsilon (geometry)'),
    ('SU2_CFD/src/adaptation/CBoundaryLayerMetric.cpp', 2432): ('KEEP', 'quadrature allowance'),
    ('SU2_CFD/src/adaptation/CBoundaryLayerMetric.cpp', 2658): ('OTHER', 'quadrature midpoint'),
    ('SU2_CFD/src/adaptation/CBoundaryLayerMetric.cpp', 2660): ('KEEP', 'quadrature distance tolerance (plan: :2655)'),
    ('SU2_CFD/src/adaptation/CBoundaryLayerRemesher.cpp', 1209): ('OTHER', 'MMG failure midpoint'),
    ('TestCases/adaptation/capability/capcheck.py', 1010): ('OTHER', 'self-test'),
    ('integration_evidence/audit_engine_scaling.py', 81): ('OTHER', 'independent value agreement'),
    ('integration_evidence/check_native_cross_grid_completion_v31.py', 62): ('OTHER', 'q/length gates'),
    ('integration_evidence/check_native_metric_integration.py', 62): ('OTHER', 'q/length gates'),
    ('integration_evidence/investigate_rae2822_bl_metric.py', 13): ('OTHER', 'analysis formula'),
    ('integration_evidence/run_native_airfoil_audits.py', 61): ('KEEP', 'parameter deviation'),
    ('integration_evidence/run_native_bl_audits.py', 63): ('OTHER', 'quantiles'),
    ('integration_evidence/run_native_opposing_audits.py', 60): ('OTHER', 'quantiles'),
    ('integration_evidence/run_native_final_output_audits.py', 86): ('T-HELPER', 'final output audit <=1e-8'),
}


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def scan(paths):
    rows = []
    for path in paths:
        if not path.exists():
            continue
        rel = str(path.relative_to(ROOT))
        lines = path.read_text(errors='replace').splitlines()
        for i, text in enumerate(lines, 1):
            window = ' '.join(lines[max(0, i-6):i])  # literal/height may sit on a nearby line
            if not TOL.search(text) or not (COMPARE.search(text) or COMPARE.search(window)):
                continue
            if not (HEIGHT.search(text) or HEIGHT.search(window)):
                continue
            cls, note = TABLE.get((rel, i), ('REVIEW', ''))
            rows.append({'file': rel, 'line': i, 'class': cls, 'note': note, 'text': text.strip()[:220]})
    return rows


def main():
    rows = scan(CPP) + scan(PY)
    callers = []
    for path in CPP:
        rel = str(path.relative_to(ROOT))
        for i, text in enumerate(path.read_text(errors='replace').splitlines(), 1):
            if 'CheckFirstHeight(' in text:
                callers.append({'file': rel, 'line': i, 'text': text.strip()[:160]})
    # Explicit-table lines that the pattern missed are listed too (pattern blind spots are part of completeness).
    seen = {(r['file'], r['line']) for r in rows}
    missed = [{'file': f, 'line': l, 'class': c, 'note': n} for (f, l), (c, n) in TABLE.items() if (f, l) not in seen]
    result = {
        'scope': 'DC-3: first-height tolerance inventory (C++ adaptation sources, CSolver, CConfig, top-level '
                 'integration_evidence/*.py, capcheck.py). Read-only.',
        'files_sha256': {str(p.relative_to(ROOT)): sha(p) for p in CPP + PY if p.exists()},
        'matches': rows,
        'table_lines_not_matched_by_pattern': missed,
        'check_first_height_call_sites': callers,
        'counts': {c: sum(r['class'] == c for r in rows) for c in ('T', 'T-HELPER', 'KEEP', 'WPA', 'MMG-REPORT', 'OTHER', 'REVIEW')},
    }
    json.dump(result, sys.stdout, indent=1)


if __name__ == '__main__':
    main()
