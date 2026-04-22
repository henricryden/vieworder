/**
 * brain-sim.js
 * Brain scan reconstruction: assembles measured k-space from view order + MPRAGE signals
 * + analytical phantom, then 2D IFFTs to produce a magnitude image.
 *
 * Depends on:
 *   window.AppState   (from app.js)  — .coords, .params
 *   window.MPRAGEModule (module script in index.html) — .getMPRAGESignals(params)
 *   BrainPhantom (brain-phantom.js)  — tissue k-space grids
 *   Chart.js (CDN)
 */

/* global BrainPhantom, Chart */

const BrainSim = (() => {
    const SVG_URL = '../phantoms/MedicalImagingVectorPhantoms/2D_axial_brain.svg';

    // Multi-worker pool for parallelized phantom computation
    let _workers = [];          // Array of Worker instances
    let _workerCount = 0;       // Number of active workers
    let _svgText = null;        // cached SVG text for grid display
    let _workersReady = 0;      // Count of workers that have completed SVG parsing
    let _svgInitPromise = null; // In-flight worker/SVG initialization promise
    let _scanInFlight = false;  // Prevent duplicate scan starts from concurrent handlers

    // Main-thread grid cache: key = `${Ny}x${Nz}` → grids object
    const _gridCache = new Map();

    // Pending compute promises per (Ny, Nz)
    const _pendingComputes = new Map();  // key = "${Ny}x${Nz}" → {resolve, reject, ...}

    // ── FFT (radix-2 Cooley-Tukey, in-place) ─────────────────────────────────

    function nextPoT(n) {
        let p = 1;
        while (p < n) p <<= 1;
        return p;
    }

    /**
     * In-place 1-D radix-2 FFT / IFFT.
     * re[], im[] must be Float64Arrays of length N (power-of-2).
     * inverse = true → IFFT with 1/N normalisation.
     */
    function fft1d(re, im, inverse) {
        const N = re.length;

        // Bit-reverse permutation
        let j = 0;
        for (let i = 1; i < N; i++) {
            let bit = N >> 1;
            while (j & bit) { j ^= bit; bit >>= 1; }
            j ^= bit;
            if (i < j) {
                let tmp = re[i]; re[i] = re[j]; re[j] = tmp;
                    tmp = im[i]; im[i] = im[j]; im[j] = tmp;
            }
        }

        // Butterfly passes
        const sign = inverse ? 1 : -1;
        for (let len = 2; len <= N; len <<= 1) {
            const half  = len >> 1;
            const angle = sign * 2 * Math.PI / len;
            const wRe   = Math.cos(angle);
            const wIm   = Math.sin(angle);

            for (let i = 0; i < N; i += len) {
                let curRe = 1, curIm = 0;
                for (let k = 0; k < half; k++) {
                    const uRe = re[i + k];
                    const uIm = im[i + k];
                    const vRe = re[i + k + half] * curRe - im[i + k + half] * curIm;
                    const vIm = re[i + k + half] * curIm + im[i + k + half] * curRe;

                    re[i + k]        = uRe + vRe;
                    im[i + k]        = uIm + vIm;
                    re[i + k + half] = uRe - vRe;
                    im[i + k + half] = uIm - vIm;

                    const nRe = curRe * wRe - curIm * wIm;
                    curIm = curRe * wIm + curIm * wRe;
                    curRe = nRe;
                }
            }
        }

        if (inverse) {
            for (let i = 0; i < N; i++) { re[i] /= N; im[i] /= N; }
        }
    }

    /**
     * 2-D IFFT of a centred k-space matrix.
     * Input: ksp_re[Ny*Nz], ksp_im[Ny*Nz] where DC is at (iy=Ny/2, iz=Nz/2).
     *        Flat index: iz * Ny + iy.
     * Returns: { img: Float32Array[PNy*PNz], width: PNy, height: PNz }
     *   where PNy, PNz are the next powers-of-two ≥ Ny, Nz.
     *   img contains the magnitude image, DC centred at (PNy/2, PNz/2).
     */
    function ifft2centered(ksp_re, ksp_im, Ny, Nz) {
        const t0 = performance.now();
        const PNy = nextPoT(Ny);
        const PNz = nextPoT(Nz);

        // Allocate zero-padded buffers (zero-fill gives sinc interpolation)
        const padRe = new Float64Array(PNy * PNz);
        const padIm = new Float64Array(PNy * PNz);

        // Zero-pad k-space symmetrically so zeros fall at high frequencies.
        // Input DC is at (iy = Ny/2, iz = Nz/2).
        // We map it to (0, 0) for the IFFT, placing zeros between positive and
        // negative frequency halves (not at the end, which would corrupt them).
        //
        //   iy  >= Ny/2  (DC + positive freqs)  → dst_iy = iy - Ny/2       (0 .. Ny/2-1)
        //   iy  <  Ny/2  (negative freqs)        → dst_iy = PNy - Ny/2 + iy (PNy-Ny/2 .. PNy-1)
        //   gap at dst_iy = Ny/2 .. PNy-Ny/2-1  ← zeros (high-frequency pad) ✓
        //
        // For power-of-two Ny (= PNy) the gap is empty and this is a plain fftshift.
        const Ny2 = Ny >> 1;
        const Nz2 = Nz >> 1;
        for (let iz = 0; iz < Nz; iz++) {
            const dst_iz = iz >= Nz2 ? iz - Nz2 : (PNz - Nz2 + iz);
            for (let iy = 0; iy < Ny; iy++) {
                const dst_iy = iy >= Ny2 ? iy - Ny2 : (PNy - Ny2 + iy);
                padRe[dst_iz * PNy + dst_iy] = ksp_re[iz * Ny + iy];
                padIm[dst_iz * PNy + dst_iy] = ksp_im[iz * Ny + iy];
            }
        }

        // Row-wise IFFT
        const rowRe = new Float64Array(PNy);
        const rowIm = new Float64Array(PNy);
        for (let iz = 0; iz < PNz; iz++) {
            const base = iz * PNy;
            rowRe.set(padRe.subarray(base, base + PNy));
            rowIm.set(padIm.subarray(base, base + PNy));
            fft1d(rowRe, rowIm, true);
            padRe.set(rowRe, base);
            padIm.set(rowIm, base);
        }

        // Column-wise IFFT
        const colRe = new Float64Array(PNz);
        const colIm = new Float64Array(PNz);
        for (let iy = 0; iy < PNy; iy++) {
            for (let iz = 0; iz < PNz; iz++) { colRe[iz] = padRe[iz * PNy + iy]; colIm[iz] = padIm[iz * PNy + iy]; }
            fft1d(colRe, colIm, true);
            for (let iz = 0; iz < PNz; iz++) { padRe[iz * PNy + iy] = colRe[iz]; padIm[iz * PNy + iy] = colIm[iz]; }
        }

        // fftshift output and compute magnitude
        const img = new Float32Array(PNy * PNz);
        for (let iz = 0; iz < PNz; iz++) {
            const iz_c = (iz + PNz / 2) % PNz;
            for (let iy = 0; iy < PNy; iy++) {
                const iy_c = (iy + PNy / 2) % PNy;
                const re   = padRe[iz_c * PNy + iy_c];
                const im   = padIm[iz_c * PNy + iy_c];
                img[iz * PNy + iy] = Math.sqrt(re * re + im * im);
            }
        }

        console.log(`[BrainSim] 2D IFFT ${Ny}×${Nz} → ${PNy}×${PNz} in ${(performance.now() - t0).toFixed(0)} ms`);
        return { img, width: PNy, height: PNz };
    }

    // ── Worker management ─────────────────────────────────────────────────────

    /**
     * Initialize a pool of workers for parallel phantom computation.
     * Uses navigator.hardwareConcurrency to determine pool size (defaults to 4).
     */
    function initWorkerPool() {
        if (_workers.length > 0) return;

        const poolSize = Math.max(2, Math.min(navigator.hardwareConcurrency || 4, 8));
        console.log(`[BrainSim] Initializing worker pool with ${poolSize} threads`);
        _workerCount = poolSize;

        for (let wid = 0; wid < poolSize; wid++) {
            const worker = new Worker('brain-phantom-worker.js');
            worker._brainReady = false;
            
            worker.onmessage = function (e) {
                const msg = e.data;

                if (msg.type === 'workerReady') {
                    worker._brainReady = true;
                    _workersReady = _workers.filter(w => w._brainReady === true).length;
                    console.log(`[BrainSim] Worker ${wid} ready (${_workersReady}/${poolSize})`);

                } else if (msg.type === 'progress') {
                    // Aggregate progress across all workers
                    const pending = _pendingComputes.values().next().value;
                    if (!pending) return;
                    pending.workerProgress[msg.workerId] = msg.value;
                    const avg = pending.workerProgress.reduce((a, b) => a + b, 0) / _workerCount;
                    const pct = Math.round(avg * 100);
                    const bar = document.getElementById('brain-progress-bar');
                    const label = document.getElementById('brain-progress-label');
                    if (bar) bar.style.width = pct + '%';
                    if (label) label.textContent = `Computing phantom grid… ${pct}%`;

                } else if (msg.type === 'gridChunkReady') {
                    const { key, izStart, izEnd, grids } = msg;
                    const pending = _pendingComputes.get(key);
                    if (!pending) return;

                    // Splice chunk into full assembled grids
                    const { Ny, assembledGrids } = pending;
                    const nRows = izEnd - izStart;
                    for (const tissueName of Object.keys(grids)) {
                        assembledGrids[tissueName].set(
                            grids[tissueName].subarray(0, nRows * Ny * 2),
                            izStart * Ny * 2
                        );
                    }

                    pending.rowsReceived += nRows;
                    if (pending.rowsReceived >= pending.Nz) {
                        _gridCache.set(key, assembledGrids);
                        console.log(`[BrainSim] Grid ${key} complete from ${_workerCount} workers`);
                        const progressContainer = document.getElementById('brain-progress-container');
                        if (progressContainer) progressContainer.style.display = 'none';
                        pending.resolve(assembledGrids);
                        _pendingComputes.delete(key);
                    }
                }
            };

            worker.onerror = function (e) {
                console.error(`[BrainSim] Worker ${wid} error:`, e);
                for (const pending of _pendingComputes.values()) {
                    if (!pending.hasErrored) {
                        pending.hasErrored = true;
                        pending.reject(e);
                    }
                }
            };

            _workers.push(worker);
        }
    }

    /**
     * Fetch brain.svg (once), parse it in main thread (DOMParser not available in Worker),
     * and send edge data to all workers.
     */
    async function ensureSVGParsed() {
        // Guard: skip if already initialised and all workers ready
        if (_workers.length > 0 && _workersReady === _workers.length) return;
        if (_svgInitPromise) return _svgInitPromise;

        _svgInitPromise = (async () => {
            initWorkerPool();

            if (!_svgText) {
                const t0 = performance.now();
                const resp = await fetch(SVG_URL);
                _svgText = await resp.text();
                console.log(`[BrainSim] Fetched brain.svg in ${(performance.now() - t0).toFixed(0)} ms`);
            }

            // Parse SVG in main thread (DOMParser is not available in Web Workers)
            const tParse = performance.now();
            const edgeData = BrainPhantom.parseSVG(_svgText);
            console.log(`[BrainSim] Parsed SVG in main thread in ${(performance.now() - tParse).toFixed(0)} ms`);

            _workersReady = 0;
            const readyWorkers = new Set();

            // Send parsed edge data to all workers
            for (let i = 0; i < _workers.length; i++) {
                _workers[i]._brainReady = false;
                _workers[i].postMessage({ type: 'setEdgeData', edgeData, workerId: i });
            }

            // Wait for all workers to acknowledge
            await new Promise((resolve, reject) => {
                const timeout = setTimeout(() => {
                    clearInterval(check);
                    reject(new Error(`Phantom workers failed to initialize (${readyWorkers.size}/${_workers.length} ready).`));
                }, 5000);
                const check = setInterval(() => {
                    for (let i = 0; i < _workers.length; i++) {
                        if (_workers[i] && _workers[i]._brainReady === true) readyWorkers.add(i);
                    }
                    _workersReady = readyWorkers.size;
                    if (_workersReady === _workers.length) {
                        clearTimeout(timeout);
                        clearInterval(check);
                        resolve();
                    }
                }, 20);
            });
        })();

        try {
            await _svgInitPromise;
        } finally {
            _svgInitPromise = null;
        }
    }

    /**
     * Ensure grid for (Ny, Nz) is computed and cached.
     * Distributes k-space rows across worker pool (true O(1/N) parallelism).
     * Returns a promise that resolves with the grid object.
     */
    async function ensureGrid(Ny, Nz) {
        const key = `${Ny}x${Nz}`;
        if (_gridCache.has(key)) {
            console.log(`[BrainSim] Main-thread grid cache hit: ${key}`);
            return _gridCache.get(key);
        }

        setStatus('Preparing phantom workers…');
        await ensureSVGParsed();

        setStatus(`Computing phantom grid ${Ny}×${Nz}…`);
        const progressContainer = document.getElementById('brain-progress-container');
        if (progressContainer) progressContainer.style.display = 'block';

        const tissues = window.PhantomState.tissues.map(t => t.name);
        const assembledGrids = {};
        for (const name of tissues) assembledGrids[name] = new Float32Array(Ny * Nz * 2);

        return new Promise((resolve, reject) => {
            _pendingComputes.set(key, {
                resolve, reject,
                Ny, Nz,
                assembledGrids,
                rowsReceived: 0,
                workerProgress: new Array(_workerCount).fill(0),
                hasErrored: false,
            });

            // Distribute rows evenly across workers
            const rowsPerWorker = Math.ceil(Nz / _workerCount);
            for (let wid = 0; wid < _workerCount; wid++) {
                const izStart = wid * rowsPerWorker;
                const izEnd = Math.min(izStart + rowsPerWorker, Nz);
                if (izStart >= Nz) break;
                _workers[wid].postMessage({ type: 'compute', key, Ny, Nz, izStart, izEnd, workerId: wid });
            }
        });
    }

    // ── Signal assembly ───────────────────────────────────────────────────────

    /**
     * Assemble measured k-space from view order coords, MPRAGE signals, and phantom grids.
     *
     * @param {Array}  coords      — from AppState.coords (each has .y_idx, .z_idx, .echo)
     * @param {Object} mprageSigs  — {tissueName: Float32Array[ETL*2]} (re, im interleaved) complex signals per echo
     * @param {Object} phantomGrid — {tissueName: Float32Array[Ny*Nz*2]} (re, im interleaved)
     * @param {number} Ny, Nz
     * @returns {[Float32Array, Float32Array]} [ksp_re, ksp_im] with DC at (iy=Ny/2, iz=Nz/2)
     */
    function assembleMeasuredKspace(coords, mprageSigs, phantomGrid, Ny, Nz) {
        const t0 = performance.now();
        const ksp_re = new Float32Array(Ny * Nz);
        const ksp_im = new Float32Array(Ny * Nz);

        const tissues = window.PhantomState.tissues.filter(t => t.enabled !== false).map(t => t.name);
        const PD_map  = Object.fromEntries(
            window.PhantomState.tissues.filter(t => t.enabled !== false).map(t => [t.name, t.PD])
        );

        for (const coord of coords) {
            const { y_idx, z_idx, echo } = coord;
            if (typeof echo !== 'number' || echo < 1) continue;

            const echoIdx = echo - 1;  // 0-based
            const kspIdx  = z_idx * Ny + y_idx;

            let sRe = 0, sIm = 0;
            for (const name of tissues) {
                const sigsComplex = mprageSigs[name];
                const grid = phantomGrid[name];
                if (!sigsComplex || !grid) continue;

                const PD  = PD_map[name] || 1.0;
                const gi  = kspIdx * 2;

                // Complex signal from EPG: Mxy = MxyRe + i*MxyIm
                const MxyRe = sigsComplex[echoIdx * 2] || 0;
                const MxyIm = sigsComplex[echoIdx * 2 + 1] || 0;
                
                // Grid point: gridVal = gridRe + i*gridIm
                const gridRe = grid[gi];
                const gridIm = grid[gi + 1];

                // Complex multiplication: PD * (MxyRe + i*MxyIm) * (gridRe + i*gridIm)
                // = PD * [(MxyRe*gridRe - MxyIm*gridIm) + i*(MxyRe*gridIm + MxyIm*gridRe)]
                sRe += PD * (MxyRe * gridRe - MxyIm * gridIm);
                sIm += PD * (MxyRe * gridIm + MxyIm * gridRe);
            }
            ksp_re[kspIdx] = sRe;
            ksp_im[kspIdx] = sIm;
        }

        console.log(`[BrainSim] k-space assembly in ${(performance.now() - t0).toFixed(0)} ms (${coords.length} coords)`);
        return [ksp_re, ksp_im];
    }

    // ── Rendering ─────────────────────────────────────────────────────────────

    function setStatus(text, isError = false) {
        const el = document.getElementById('brain-status');
        if (!el) return;
        el.textContent = text;
        el.className = 'brain-status' + (isError ? ' brain-status-error' : '');
    }

    function renderImage(canvas, img, width, height) {
        const t0 = performance.now();
        canvas.width  = width;
        canvas.height = height;
        const ctx = canvas.getContext('2d');
        const imageData = ctx.createImageData(width, height);
        const data = imageData.data;

        // Auto window-level: max 99th percentile (ignore bright artefacts at edges)
        const sorted = Float32Array.from(img).sort();
        const maxVal = sorted[Math.floor(sorted.length * 0.99)] || 1;

        for (let i = 0; i < width * height; i++) {
            const v = Math.min(255, Math.round((img[i] / maxVal) * 255));
            data[i * 4]     = v;  // R
            data[i * 4 + 1] = v;  // G
            data[i * 4 + 2] = v;  // B
            data[i * 4 + 3] = 255;
        }

        ctx.putImageData(imageData, 0, 0);
        console.log(`[BrainSim] Image rendered ${width}×${height} in ${(performance.now() - t0).toFixed(0)} ms`);
    }

    function renderMxyChart(mprageSigs, etl, sourceLabel = 'MPRAGE') {
        const el = document.getElementById('brain-mxy-chart');
        if (!el) return;

        const datasets = window.PhantomState.tissues.filter(t => t.enabled !== false).map(t => {
            const sigsComplex = mprageSigs[t.name];
            const data = [];
            if (sigsComplex) {
                for (let i = 0; i < etl; i++) {
                    const re = sigsComplex[i * 2] || 0;
                    const im = sigsComplex[i * 2 + 1] || 0;
                    data.push({ x: i + 1, y: Math.sqrt(re * re + im * im) });
                }
            } else {
                for (let i = 0; i < etl; i++) data.push({ x: i + 1, y: 0 });
            }
            return { label: t.label, data, color: t.plotColor || t.color };
        });

        D3SeqPlots.createLinePlot(el, {
            datasets,
            xType: 'linear', xMin: 1, xMax: etl,
            yMin: 0,
            xLabel: 'Echo index', yLabel: 'Mxy',
            title: `${sourceLabel} Mxy(echo) per tissue`,
            showLegend: true,
        });
    }

    // ── Reference image & diff ─────────────────────────────────────────────────

    // Saved reference: { img: Float32Array, width, height, label }
    let _reference = null;
    // Current magnitude image from the last run
    let _currentImg = null, _currentWidth = 0, _currentHeight = 0;

    /**
     * Render the per-pixel absolute difference between two magnitude Float32Arrays
     * onto the diff canvas. Both images must have the same dimensions.
     * Uses a hot colormap (black→red→yellow→white) to make small diffs visible.
     */
    function renderDiff(refImg, curImg, width, height) {
        const canvas = document.getElementById('brain-diff-canvas');
        if (!canvas) return;
        canvas.width  = width;
        canvas.height = height;
        const ctx = canvas.getContext('2d');
        const imageData = ctx.createImageData(width, height);
        const d = imageData.data;
        const N = width * height;

        // Build difference array
        const diff = new Float32Array(N);
        let maxDiff = 0;
        for (let i = 0; i < N; i++) {
            diff[i] = Math.abs(curImg[i] - refImg[i]);
            if (diff[i] > maxDiff) maxDiff = diff[i];
        }

        const scale = maxDiff > 0 ? 1 / maxDiff : 1;
        for (let i = 0; i < N; i++) {
            const t = diff[i] * scale;          // 0..1
            // Hot colormap: black(0)→red(.33)→yellow(.66)→white(1)
            d[i*4]   = Math.min(255, Math.round(t * 3 * 255));             // R
            d[i*4+1] = Math.min(255, Math.round(Math.max(0, t*3-1) * 255)); // G
            d[i*4+2] = Math.min(255, Math.round(Math.max(0, t*3-2) * 255)); // B
            d[i*4+3] = 255;
        }
        ctx.putImageData(imageData, 0, 0);

        const label = document.getElementById('brain-diff-label');
        if (label) label.textContent = `Max |Δ| = ${(maxDiff/255*100).toFixed(2)}%`;
    }

    function saveReference() {
        if (!_currentImg) return;
        const now = new Date();
        const timeStr = now.toLocaleTimeString('de-DE', { hour: '2-digit', minute: '2-digit', second: '2-digit' });
        _reference = {
            img:    Float32Array.from(_currentImg),
            width:  _currentWidth,
            height: _currentHeight,
            label:  timeStr,
        };

        // Show reference canvas
        const refCanvas = document.getElementById('brain-ref-canvas');
        if (refCanvas) renderImage(refCanvas, _reference.img, _reference.width, _reference.height);
        const refLabel = document.getElementById('brain-ref-label');
        if (refLabel) refLabel.textContent = `Saved at ${_reference.label}`;

        // Clear diff until next scan
        const diffCanvas = document.getElementById('brain-diff-canvas');
        if (diffCanvas) {
            const ctx = diffCanvas.getContext('2d');
            ctx.clearRect(0, 0, diffCanvas.width, diffCanvas.height);
        }
        const diffLabel = document.getElementById('brain-diff-label');
        if (diffLabel) diffLabel.textContent = 'Run another scan to see the diff.';

        document.getElementById('brain-ref-row').style.display = '';
        console.log('[BrainSim] Reference image saved.');
    }

    // ── Main entry point ──────────────────────────────────────────────────────

    /**
     * Run the full brain scan pipeline:
     *   1. Get view-order coords from AppState
     *   2. Run MPRAGE EPG simulation for all tissues
     *   3. Ensure phantom k-space grid for current Ny × Nz
     *   4. Assemble measured k-space
     *   5. 2D IFFT → magnitude image
     *   6. Render to canvas + Mxy chart
     */
    async function runScan() {
        const tTotal = performance.now();
        const canvas = document.getElementById('brain-image-canvas');
        const simBtn = document.getElementById('brain-simulate-btn');

        if (!canvas) { console.error('[BrainSim] Canvas not found'); return; }
        if (_scanInFlight) return;
        _scanInFlight = true;
        if (simBtn) simBtn.disabled = true;
        setStatus('Running scan simulation…');

        try {
            // 1. View-order parameters
            const coords = window.AppState && window.AppState.coords;
            const params = window.AppState && window.AppState.params;
            if (!coords || coords.length === 0) {
                setStatus('No view-order coordinates. Update Tab 1 first.', true);
                return;
            }
            const { ky: Ny, kz: Nz, etl: ETL } = params;

            // 2. Tissue signals — from whichever source the user selected
            const sigSource = window.BrainScanSignalSource || 'mprage';
            let sigs;
            if (sigSource === 'rare') {
                if (!window.RAREModule || !window.RAREModule.ready) {
                    setStatus('RARE module not ready — open Tab ③ first.', true);
                    return;
                }
                setStatus('Simulating RARE…');
                const t1 = performance.now();
                sigs = window.RAREModule.getRARESignals(params);
                console.log(`[BrainSim] RARE simulation in ${(performance.now() - t1).toFixed(0)} ms`, sigs);
                if (!sigs) { setStatus('RARE signals returned null — check console.', true); return; }
            } else if (sigSource === 'fspgr') {
                if (!window.FSPGRModule || !window.FSPGRModule.ready) {
                    setStatus('FSPGR module not ready — open Tab ③ first.', true);
                    return;
                }
                setStatus('Simulating FSPGR…');
                const t1 = performance.now();
                sigs = window.FSPGRModule.getFSPGRSignals(params);
                console.log(`[BrainSim] FSPGR simulation in ${(performance.now() - t1).toFixed(0)} ms`);
                if (!sigs) { setStatus('FSPGR signals returned null — check console.', true); return; }
            } else {
                if (!window.MPRAGEModule || !window.MPRAGEModule.ready) {
                    setStatus('MPRAGE module not ready — please wait.', true);
                    return;
                }
                setStatus('Simulating MPRAGE…');
                const t1 = performance.now();
                sigs = window.MPRAGEModule.getMPRAGESignals(params);
                console.log(`[BrainSim] MPRAGE simulation in ${(performance.now() - t1).toFixed(0)} ms`);
            }

            // 3. Phantom grid
            const grid = await ensureGrid(Ny, Nz);

            const progressContainer = document.getElementById('brain-progress-container');
            if (progressContainer) progressContainer.style.display = 'none';

            // 4. Assemble k-space
            setStatus('Assembling k-space…');
            const [ksp_re, ksp_im] = assembleMeasuredKspace(coords, sigs, grid, Ny, Nz);

            // 5. Reconstruct
            setStatus('Reconstructing image…');
            const { img, width, height } = ifft2centered(ksp_re, ksp_im, Ny, Nz);

            // 6. Render
            renderImage(canvas, img, width, height);
            const sourceLabel = sigSource === 'rare' ? 'RARE' : sigSource === 'fspgr' ? 'FSPGR' : 'MPRAGE';
            const titleEl = document.getElementById('brain-mxy-title');
            if (titleEl) titleEl.textContent = `${sourceLabel} Mxy(echo) per tissue — used in this scan`;
            renderMxyChart(sigs, ETL, sourceLabel);

            // Store current image for save/diff
            _currentImg    = img;
            _currentWidth  = width;
            _currentHeight = height;

            // Enable save button
            const saveBtn = document.getElementById('brain-save-btn');
            if (saveBtn) saveBtn.disabled = false;

            // If a reference exists with matching dimensions, render diff
            if (_reference && _reference.width === width && _reference.height === height) {
                renderDiff(_reference.img, img, width, height);
            } else if (_reference) {
                const diffLabel = document.getElementById('brain-diff-label');
                if (diffLabel) diffLabel.textContent =
                    `Dimensions changed (ref ${_reference.width}×${_reference.height} vs ${width}×${height}) — save a new reference.`;
            }

            const msg = `Done in ${(performance.now() - tTotal).toFixed(0)} ms (${Ny}×${Nz})`;
            setStatus(msg);
            console.log(`[BrainSim] Total scan pipeline: ${msg}`);

        } catch (err) {
            console.error('[BrainSim] Error in runScan:', err);
            setStatus('Error: ' + err.message, true);
        } finally {
            _scanInFlight = false;
            if (simBtn) simBtn.disabled = false;
        }
    }

    // ── Phantom change: invalidate grid cache + reinit workers ────────────
    window.addEventListener('phantomChanged', () => {
        console.log('[BrainSim] Phantom changed — clearing grid cache and reinitializing workers.');
        _gridCache.clear();
        for (const w of _workers) w.terminate();
        _workers = [];
        _workerCount = 0;
        _workersReady = 0;
        _svgInitPromise = null;
        // SVG text is still cached; workers will re-receive edge data on next ensureSVGParsed()
    });

    return { runScan, renderMxyChart, saveReference };
})();

window.BrainSim = BrainSim;

document.addEventListener('DOMContentLoaded', () => {
    const runBtn = document.getElementById('brain-simulate-btn');
    if (runBtn) {
        runBtn.addEventListener('click', (event) => {
            event.preventDefault();
            BrainSim.runScan();
        });
    }

    const saveBtn = document.getElementById('brain-save-btn');
    if (saveBtn) {
        saveBtn.addEventListener('click', (event) => {
            event.preventDefault();
            BrainSim.saveReference();
        });
    }
});
