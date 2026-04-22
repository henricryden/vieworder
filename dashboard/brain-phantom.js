/**
 * brain-phantom.js
 * Analytical k-space computation from brain.svg polygon vertices.
 * Implements the 2D polygon Fourier transform formula (Green's theorem / Gach 2008).
 * Designed to run in a Web Worker (no DOM dependencies beyond DOMParser for SVG parsing).
 *
 * Tissues included (3T, from MedicalImagingVectorPhantoms/2D_axial_brain.toml):
 *   gray, white, CSF, adipose, bonemarrow, muscle, cortical, blood
 */

/* global self */

const BrainPhantom = (() => {
    const FOV_Y = 200;  // phantom FOV in ky direction [mm] — increased to reduce aliasing
    const FOV_Z = 200;  // phantom FOV in kz direction [mm] — increased to reduce aliasing

    // Tissue fill hex → display name and 3T relaxation parameters
    // Colors from phantoms/MedicalImagingVectorPhantoms/2D_axial_brain.toml
    const TISSUE_MAP = {
        '#625e42': { name: 'gray',       T1: 1450, T2: 100,  PD: 1.0,  color: '#625e42' },
        '#a09c7f': { name: 'white',      T1: 830,  T2: 69,   PD: 0.92, color: '#a09c7f' },
        '#c6c5b2': { name: 'CSF',        T1: 4160, T2: 2100, PD: 1.0,  color: '#c6c5b2' },
        '#b6985f': { name: 'adipose',    T1: 370,  T2: 130,  PD: 1.0,  color: '#b6985f' },
        '#a78f23': { name: 'bonemarrow', T1: 898,  T2: 34,   PD: 1.0,  color: '#a78f23' },
        '#500f05': { name: 'muscle',     T1: 1400, T2: 50,   PD: 1.0,  color: '#500f05' },
        '#504010': { name: 'cortical',   T1: 900,  T2: 0.5,  PD: 0.2,  color: '#504010' },
        '#990000': { name: 'blood',      T1: 1650, T2: 150,  PD: 1.0,  color: '#990000' },
    };

    // Precomputed per-tissue edge arrays: Float64Array of [Ly, Lz, rcy, rcz] quads
    let _edgeData = null;

    // ── Tissue map ────────────────────────────────────────────────────────────

    /**
     * Build a { [colorHex]: {name, T1, T2, PD, color} } map.
     * Reads from window.PhantomState when available (main thread with phantom-state.js loaded);
     * falls back to the static TISSUE_MAP for worker contexts.
     */
    function getTissueMap() {
        if (typeof window !== 'undefined' && window.PhantomState && window.PhantomState.tissues) {
            const map = {};
            for (const t of window.PhantomState.tissues) {
                map[t.color] = { name: t.name, T1: t.T1, T2: t.T2, PD: t.PD, color: t.color };
            }
            return map;
        }
        return TISSUE_MAP;
    }

    // Cache: key = `${Ny}x${Nz}` → {tissueName: Float32Array[Ny*Nz*2]} (interleaved re,im)
    const _cache = new Map();

    // ── Numerics ──────────────────────────────────────────────────────────────

    function sinc(x) {
        if (Math.abs(x) < 1e-10) return 1.0;
        const px = Math.PI * x;
        return Math.sin(px) / px;
    }

    /**
     * Compute k-space at (ky, kz) [cycles/mm] for one tissue via the edge array.
     * Returns [re, im].
     * At k=0 returns the polygon area (shoelace via edges).
     */
    function kspaceForEdges(edges, ky, kz) {
        const N4 = edges.length;  // N_edges × 4
        if (N4 === 0) return [0, 0];

        if (Math.abs(ky) < 1e-12 && Math.abs(kz) < 1e-12) {
            // K(0,0) = signed area: 0.5 * Σ_e (rcy * Lz - rcz * Ly)
            let area = 0;
            for (let n = 0; n < N4; n += 4) {
                area += edges[n + 2] * edges[n + 1] - edges[n + 3] * edges[n];
            }
            return [area * 0.5, 0];
        }

        const k2 = ky * ky + kz * kz;
        const TWO_PI = 2 * Math.PI;
        let sumRe = 0, sumIm = 0;

        for (let n = 0; n < N4; n += 4) {
            const Ly  = edges[n];
            const Lz  = edges[n + 1];
            const rcy = edges[n + 2];
            const rcz = edges[n + 3];

            const kDotL   = ky * Ly + kz * Lz;            // k · L
            const kCrossL = ky * Lz - kz * Ly;            // (k × L)_z
            const s = sinc(kDotL);                        // sinc(k·L)
            const phase = TWO_PI * (ky * rcy + kz * rcz); // 2π k·rc

            // contrib = kCrossL * s * exp(-i·phase)
            sumRe += kCrossL * s * Math.cos(phase);
            sumIm -= kCrossL * s * Math.sin(phase);
        }

        // K = (i / (2π k²)) × (sumRe + i·sumIm)
        //   = (-sumIm + i·sumRe) / (2π k²)
        const pf = 1.0 / (TWO_PI * k2);
        return [-sumIm * pf, sumRe * pf];
    }

    // ── SVG parsing ──────────────────────────────────────────────────────────

    /**
     * Parse the `d` attribute of an SVG path into an array of subpaths.
     * Each subpath is an array of [y_mm, z_mm] vertices.
     * `scale` is applied to raw coordinates (0.26458333 for internal Inkscape units, or 1.0).
     * Handles M, m, L, l, H, h, V, v, Z, z.
     * Bezier (C, S, Q, T, A) are skipped with a one-time console warning.
     */
    function parseSVGPath(d, scale) {
        const subpaths = [];

        // Tokenize: command letters and floating-point numbers (including negatives)
        // SVG numbers: optional sign, digits, optional decimal, optional exponent
        const tokenRe = /([MmLlZzHhVvCcSsQqTtAa])|(-?(?:\d+\.?\d*|\.\d+)(?:[eE][+-]?\d+)?)/g;
        const tokens = [];
        let tm;
        while ((tm = tokenRe.exec(d)) !== null) {
            if (tm[1]) tokens.push(tm[1]);
            else       tokens.push(parseFloat(tm[2]));
        }

        let ti = 0;
        let cx = 0, cy = 0, startX = 0, startY = 0;
        let current = [];
        let cmd = null;
        let warned = false;

        function peek()   { return ti < tokens.length ? tokens[ti] : null; }
        function isCmd(t) { return typeof t === 'string'; }
        function readN()  {
            const t = peek();
            if (t !== null && !isCmd(t)) { ti++; return t; }
            return null;
        }

        while (ti < tokens.length) {
            const t = peek();
            if (isCmd(t)) { cmd = t; ti++; }
            // else: implicit repetition of last command

            if (!cmd) { ti++; continue; }

            const upper = cmd.toUpperCase();
            const rel   = (cmd !== cmd.toUpperCase()) && upper !== 'Z';

            if (upper === 'M') {
                const x = readN(), y = readN();
                if (x === null) break;
                cx = rel ? cx + x : x;
                cy = rel ? cy + y : y;
                startX = cx; startY = cy;
                if (current.length >= 3) subpaths.push(current);
                current = [[cx * scale, cy * scale]];
                // After M/m, subsequent coord pairs are L/l
                cmd = rel ? 'l' : 'L';

            } else if (upper === 'L') {
                const x = readN(), y = readN();
                if (x === null) break;
                cx = rel ? cx + x : x;
                cy = rel ? cy + y : y;
                current.push([cx * scale, cy * scale]);

            } else if (upper === 'H') {
                const x = readN();
                if (x === null) break;
                cx = rel ? cx + x : x;
                current.push([cx * scale, cy * scale]);

            } else if (upper === 'V') {
                const y = readN();
                if (y === null) break;
                cy = rel ? cy + y : y;
                current.push([cx * scale, cy * scale]);

            } else if (upper === 'Z') {
                if (current.length >= 3) subpaths.push(current);
                current = [];
                cx = startX; cy = startY;
                cmd = null;  // no implicit repetition after Z

            } else {
                // Bezier / arc: skip arguments
                if (!warned) {
                    console.warn('[BrainPhantom] skipping unsupported SVG path command:', upper);
                    warned = true;
                }
                const argCount = { C: 6, S: 4, Q: 4, T: 2, A: 7 };
                const n = argCount[upper] || 2;
                let skipped = 0;
                while (skipped < n) { if (readN() === null) break; skipped++; }
            }
        }

        if (current.length >= 3) subpaths.push(current);
        return subpaths;
    }

    /**
     * Parse the raw SVG XML string and extract per-tissue edge arrays.
     * Uses DOMParser (available in Workers in all modern browsers).
     * Returns {tissueName: Float64Array[N_edges*4]} where each quad is [Ly, Lz, rcy, rcz].
     */
    function buildEdgeData(svgText) {
        const parser = new DOMParser();
        const doc = parser.parseFromString(svgText, 'image/svg+xml');
        const paths = doc.querySelectorAll('path');

        // Compute center offset from viewBox so phantom is centered at (0,0).
        // For old SVGs centered at origin the offset is 0; for new SVGs with
        // top-left origin (viewBox "minX minY width height") this shifts to center.
        let offsetY = 0, offsetZ = 0;
        const svgEl = doc.querySelector('svg');
        if (svgEl) {
            const vb = svgEl.getAttribute('viewBox');
            if (vb) {
                const parts = vb.trim().split(/[\s,]+/);
                if (parts.length === 4) {
                    offsetY = parseFloat(parts[0]) + parseFloat(parts[2]) / 2;
                    offsetZ = parseFloat(parts[1]) + parseFloat(parts[3]) / 2;
                }
            }
        }

        const tissueMap = getTissueMap();
        const tissuePolygons = {};
        for (const info of Object.values(tissueMap)) {
            tissuePolygons[info.name] = [];
        }

        for (const path of paths) {
            const style = path.getAttribute('style') || '';
            const fm = style.match(/fill:(#[0-9a-f]+)/);
            if (!fm) continue;
            const info = tissueMap[fm[1]];
            if (!info) continue;

            const hasScale = (path.getAttribute('transform') || '').includes('scale');
            const scale = hasScale ? 0.26458333 : 1.0;

            const d = path.getAttribute('d') || '';
            const subpaths = parseSVGPath(d, scale);
            for (const verts of subpaths) {
                if (verts.length >= 3) tissuePolygons[info.name].push(verts);
            }
        }

        const result = {};
        for (const [name, polygons] of Object.entries(tissuePolygons)) {
            const quads = [];
            for (const verts of polygons) {
                const N = verts.length;
                for (let j = 0; j < N; j++) {
                    const p1 = verts[j];
                    const p2 = verts[(j + 1) % N];
                    const p1y = p1[0] - offsetY;
                    const p1z = p1[1] - offsetZ;
                    const p2y = p2[0] - offsetY;
                    const p2z = p2[1] - offsetZ;
                    quads.push(p2y - p1y);             // Ly
                    quads.push(p2z - p1z);             // Lz
                    quads.push((p1y + p2y) * 0.5);    // rcy
                    quads.push((p1z + p2z) * 0.5);    // rcz
                }
            }
            result[name] = new Float64Array(quads);
        }
        return result;
    }

    // ── Grid computation ──────────────────────────────────────────────────────

    /**
     * Compute the k-space grid for all tissues at Ny × Nz resolution.
     * k_y axis: (iy - Ny/2) / FOV_Y  for iy = 0..Ny-1  [cycles/mm]
     * k_z axis: (iz - Nz/2) / FOV_Z  for iz = 0..Nz-1  [cycles/mm]
     * DC is at (iy = Ny/2, iz = Nz/2).
     *
     * Returns { tissueName: Float32Array[Ny*Nz*2] } (interleaved re, im).
     * Caches result; only recomputes if (Ny, Nz) pair changes.
     *
     * @param {number} Ny
     * @param {number} Nz
     * @param {function(number):void} [progressCb] - called with fraction 0..1
     */
    function computePhantomGrid(Ny, Nz, progressCb) {
        const key = `${Ny}x${Nz}`;
        if (_cache.has(key)) {
            console.log(`[BrainPhantom] Cache hit for ${key} — skipping recompute.`);
            return _cache.get(key);
        }

        if (!_edgeData) throw new Error('[BrainPhantom] parseSVG() must be called first.');

        console.log(`[BrainPhantom] Computing ${key} phantom grid…`);
        const t0 = performance.now();

        const tissues = Object.keys(_edgeData);
        const nT = tissues.length;

        // Build k-space axes
        const kyAxis = new Float64Array(Ny);
        const kzAxis = new Float64Array(Nz);
        for (let iy = 0; iy < Ny; iy++) kyAxis[iy] = (iy - Ny / 2) / FOV_Y;
        for (let iz = 0; iz < Nz; iz++) kzAxis[iz] = (iz - Nz / 2) / FOV_Z;

        // Allocate output: [re, im] interleaved, indexed by iz*Ny + iy
        const result = {};
        for (const name of tissues) result[name] = new Float32Array(Ny * Nz * 2);

        const total = Ny * Nz;
        const reportEvery = Math.max(1, Math.floor(total / 100));
        let done = 0;

        // Individual tissue timings
        const tissueT0 = {};
        for (const name of tissues) tissueT0[name] = 0;

        for (let iz = 0; iz < Nz; iz++) {
            const kz = kzAxis[iz];
            for (let iy = 0; iy < Ny; iy++) {
                const ky  = kyAxis[iy];
                const idx = (iz * Ny + iy) * 2;

                for (let ti = 0; ti < nT; ti++) {
                    const name  = tissues[ti];
                    const edges = _edgeData[name];
                    const [re, im] = kspaceForEdges(edges, ky, kz);
                    result[name][idx]     = re;
                    result[name][idx + 1] = im;
                }

                done++;
                if (progressCb && done % reportEvery === 0) {
                    progressCb(done / total);
                }
            }
        }

        const elapsedMs = performance.now() - t0;
        console.log(`[BrainPhantom] Done ${key} in ${elapsedMs.toFixed(0)} ms.`);
        if (progressCb) progressCb(1.0);

        _cache.set(key, result);
        return result;
    }

    /**
     * Compute k-space for a horizontal slice of rows iz in [izStart, izEnd).
     * Used by workers for parallel computation — each worker handles a row range.
     * Returns {tissueName: Float32Array[nRows*Ny*2]} (re,im interleaved).
     * Flat index within result: (iz - izStart)*Ny + iy.
     */
    function computePhantomGridChunk(Ny, Nz, izStart, izEnd, progressCb) {
        if (!_edgeData) throw new Error('[BrainPhantom] parseSVG() must be called first.');

        const kyAxis = new Float64Array(Ny);
        const kzAxis = new Float64Array(Nz);
        for (let iy = 0; iy < Ny; iy++) kyAxis[iy] = (iy - Ny / 2) / FOV_Y;
        for (let iz = 0; iz < Nz; iz++) kzAxis[iz] = (iz - Nz / 2) / FOV_Z;

        const nRows = izEnd - izStart;
        const tissues = Object.keys(_edgeData);
        const nT = tissues.length;

        const result = {};
        for (const name of tissues) result[name] = new Float32Array(nRows * Ny * 2);

        const total = nRows * Ny;
        const reportEvery = Math.max(1, Math.floor(total / 50));
        let done = 0;

        for (let iz = izStart; iz < izEnd; iz++) {
            const kz = kzAxis[iz];
            const rowOff = (iz - izStart) * Ny;
            for (let iy = 0; iy < Ny; iy++) {
                const ky  = kyAxis[iy];
                const idx = (rowOff + iy) * 2;

                for (let ti = 0; ti < nT; ti++) {
                    const name  = tissues[ti];
                    const edges = _edgeData[name];
                    const [re, im] = kspaceForEdges(edges, ky, kz);
                    result[name][idx]     = re;
                    result[name][idx + 1] = im;
                }

                done++;
                if (progressCb && done % reportEvery === 0) progressCb(done / total);
            }
        }

        if (progressCb) progressCb(1.0);
        return result;
    }

    // ── Public API ────────────────────────────────────────────────────────────

    return {
        get TISSUE_MAP() { return getTissueMap(); },
        FOV_Y,
        FOV_Z,

        /**
         * Parse brain.svg text, build edge lookup tables.
         * Must be called before computePhantomGrid().
         */
        parseSVG(svgText) {
            const t0 = performance.now();
            _edgeData = buildEdgeData(svgText);
            const elapsed = (performance.now() - t0).toFixed(0);
            const info = Object.entries(_edgeData)
                .map(([n, e]) => `${n}: ${e.length / 4} edges`)
                .join(', ');
            console.log(`[BrainPhantom] SVG parsed in ${elapsed} ms. ${info}`);
            return _edgeData;
        },

        computePhantomGrid,
        computePhantomGridChunk,

        /** True if SVG has been parsed */
        get ready() { return _edgeData !== null; },

        /** Check if a given (Ny, Nz) grid is cached */
        isCached(Ny, Nz) { return _cache.has(`${Ny}x${Nz}`); },

        /** The cached grid for (Ny,Nz), or null */
        getGrid(Ny, Nz) { return _cache.get(`${Ny}x${Nz}`) || null; },

        /**
         * Set edge data (used by Web Worker after receiving parsed data from main thread).
         * Allows worker to use pre-parsed edge data without DOMParser.
         */
        setEdgeData(edgeData) {
            _edgeData = edgeData;
            console.log(`[BrainPhantom] Edge data set (${Object.keys(_edgeData).length} tissues)`);
        },
    };
})();
