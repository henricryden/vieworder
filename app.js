/**
 * Main Application Logic
 * Coordinates interactions between controls, utilities, and plots
 */

const App = (() => {
    let currentCoords = [];
    let currentJumpMetrics = null;
    let currentMacroSearch = null;
    let updateTimer = null;
    let updateGeneration = 0;
    let currentParams = {
        etl: 144,
        ky: 256,
        kz: 256,
        centerEcho: 105,
        ordering: 'chevron',
        kyAccel: 1,
        kzAccel: 1,
        kzPF: 1.0,
        mtfDirection: 'ky',
        shotOrder: 'azimuthal',
        coverage: 'elliptical',
        useCaipirinha: true,
        calibrationSize: 32,
        macroWidth: 2
    };
    const viewOrderUrlSchema = {
        ky: { key: 'ky', type: 'int', min: 64, max: 320 },
        kz: { key: 'kz', type: 'int', min: 64, max: 320 },
        kyAccel: { key: 'ry', type: 'int', min: 1, max: 4 },
        kzAccel: { key: 'rz', type: 'int', min: 1, max: 4 },
        calibrationSize: { key: 'cal', type: 'int', min: 16, max: 64 },
        kzPF: { key: 'pf', type: 'float', min: 0.5, max: 1.0 },
        useCaipirinha: { key: 'caipi', type: 'bool' },
        coverage: { key: 'cov', type: 'enum', values: ['rectangular', 'elliptical'] },
        ordering: { key: 'method', type: 'enum', values: ['sequential', 'lcpo', 'cplo', 'chevron', 'croc'] },
        etl: { key: 'etl', type: 'int', min: 8, max: 256 },
        centerEcho: { key: 'center', type: 'int', min: 1, max: 256 },
        mtfDirection: { key: 'mtf', type: 'enum', values: ['ky', 'kz', 'kr'] },
        shotOrder: { key: 'shotOrder', type: 'enum', values: ['ky', 'kz', 'azimuthal'] },
        macroWidth: { key: 'macro', type: 'int', min: 1, max: 10 }
    };
    
    /**
     * Initialize the application
     */
    function init() {
        applyViewOrderParamsFromUrl();
        D3Plots.init();
        setupEventListeners();
        syncViewOrderControlsFromParams();

        // Render slider ticks for visible controls
        renderAllSliderTicks();
        updateControlAvailability();
        updatePlots();
    }

    function applyViewOrderParamsFromUrl() {
        if (!window.location || !window.location.search) return;
        const params = new URLSearchParams(window.location.search);
        for (const [paramName, spec] of Object.entries(viewOrderUrlSchema)) {
            if (!params.has(spec.key)) continue;
            const parsed = parseUrlParam(params.get(spec.key), spec);
            if (parsed !== null) currentParams[paramName] = parsed;
        }
        currentParams.centerEcho = clamp(currentParams.centerEcho, 1, currentParams.etl);
    }

    function parseUrlParam(raw, spec) {
        if (raw === null || raw === undefined) return null;
        if (spec.type === 'int') {
            const value = parseInt(raw, 10);
            if (!Number.isFinite(value)) return null;
            return clamp(value, spec.min, spec.max);
        }
        if (spec.type === 'float') {
            const value = parseFloat(raw);
            if (!Number.isFinite(value)) return null;
            return clamp(value, spec.min, spec.max);
        }
        if (spec.type === 'bool') {
            if (raw === '1' || raw === 'true') return true;
            if (raw === '0' || raw === 'false') return false;
            return null;
        }
        if (spec.type === 'enum') {
            return spec.values.includes(raw) ? raw : null;
        }
        return null;
    }

    function clamp(value, min, max) {
        return Math.max(min, Math.min(max, value));
    }

    function updateViewOrderUrl() {
        if (!window.history || !window.location) return;
        const url = new URL(window.location.href);
        url.searchParams.delete('opt');
        url.searchParams.delete('tol');
        for (const [paramName, spec] of Object.entries(viewOrderUrlSchema)) {
            let value = currentParams[paramName];
            if (spec.type === 'bool') value = value ? '1' : '0';
            url.searchParams.set(spec.key, String(value));
        }
        window.history.replaceState(null, '', url);
    }

    function syncViewOrderControlsFromParams() {
        const setValue = (id, value) => {
            const el = document.getElementById(id);
            if (el) el.value = value;
        };
        const setText = (id, value) => {
            const el = document.getElementById(id);
            if (el) el.textContent = value;
        };

        setValue('ky-slider', currentParams.ky);
        setText('ky-value', currentParams.ky);
        setValue('kz-slider', currentParams.kz);
        setText('kz-value', currentParams.kz);
        setValue('ky-accel-slider', currentParams.kyAccel);
        setText('ky-accel-value', currentParams.kyAccel);
        setValue('kz-accel-slider', currentParams.kzAccel);
        setText('kz-accel-value', currentParams.kzAccel);
        setValue('cal-size-slider', currentParams.calibrationSize);
        setText('cal-size-value', currentParams.calibrationSize);
        setValue('kz-pf-slider', currentParams.kzPF);
        setText('kz-pf-value', currentParams.kzPF.toFixed(2).replace(/\.00$/, '.0'));
        const caipiCheckbox = document.getElementById('caipi-checkbox');
        if (caipiCheckbox) caipiCheckbox.checked = currentParams.useCaipirinha;
        setValue('coverage-select', currentParams.coverage);
        setValue('ordering-select', currentParams.ordering);
        setValue('etl-slider', currentParams.etl);
        setText('etl-value', currentParams.etl);
        currentParams.centerEcho = clamp(currentParams.centerEcho, 1, currentParams.etl);
        const centerSlider = document.getElementById('center-echo-slider');
        if (centerSlider) {
            centerSlider.max = currentParams.etl;
            centerSlider.value = currentParams.centerEcho;
        }
        setText('center-echo-value', currentParams.centerEcho);
        syncCenterEchoTicks();
        syncOrderingControls();
        setValue('mtf-direction-select', currentParams.mtfDirection);
        setValue('shot-order-select', currentParams.shotOrder);
        setValue('macro-width-slider', currentParams.macroWidth);
        setText('macro-width-value', currentParams.macroWidth);
        updateMacroWidthBestLabel();
    }

    function syncCenterEchoTicks() {
        const centerDatalist = document.getElementById('center-echo-ticks');
        if (!centerDatalist) return;
        let opts = '';
        for (let i = 1; i <= currentParams.etl; i++) {
            opts += `<option value="${i}"></option>`;
        }
        centerDatalist.innerHTML = opts;
    }

    /**
     * Render tick bars for a single range input based on its min/max/step
     */
    function renderSliderTicks(input) {
        if (!input) return;
        // Remove existing ticks container in the parent if present
        const parent = input.parentNode;
        const existing = parent.querySelector('.slider-ticks');
        if (existing) existing.remove();

        const min = parseFloat(input.min || 0);
        const max = parseFloat(input.max || 0);
        const step = parseFloat(input.step || 1);
        if (isNaN(min) || isNaN(max) || isNaN(step) || max <= min) return;

        const steps = Math.round((max - min) / step);
        if (steps <= 0) return;
        const maxVisualTicks = Math.max(2, parseInt(input.dataset.maxTicks || '24', 10) || 24);
        const visualTickCount = Math.min(steps, maxVisualTicks);

        const ticks = document.createElement('div');
        ticks.className = 'slider-ticks';

        for (let i = 0; i <= visualTickCount; i++) {
            const span = document.createElement('span');
            const stepIndex = visualTickCount > 0 ? Math.round((i / visualTickCount) * steps) : 0;
            const frac = steps > 0 ? (stepIndex / steps) : 0;
            span.style.left = (frac * 100) + '%';
            ticks.appendChild(span);
        }

        // Append ticks as an absolutely-positioned child of the control-group
        parent.appendChild(ticks);

        // Position the ticks container over the slider using its bounding box
        positionTicks(ticks, input);
    }

    function renderAllSliderTicks() {
        const sliders = document.querySelectorAll('input.slider[type="range"]');
        sliders.forEach((slider) => {
            if (slider.getClientRects().length === 0 || slider.offsetWidth === 0) return;
            renderSliderTicks(slider);
        });
    }

    // Position ticks container exactly over the slider element
    function positionTicks(ticks, input) {
        if (!ticks || !input) return;
        const parentRect = input.parentNode.getBoundingClientRect();
        const inputRect = input.getBoundingClientRect();
        // Compute offsets relative to parent (which is positioned)
        // Account for slider thumb radius so ticks align with thumb center
        const thumbHalf = 13; // px - matches CSS thumb (20px + 3px border => ~26px total)
        const left = inputRect.left - parentRect.left + thumbHalf;
        const top = inputRect.top - parentRect.top + inputRect.height / 2;
        const width = Math.max(0, inputRect.width - 2 * thumbHalf);
        ticks.style.left = left + 'px';
        ticks.style.width = width + 'px';
        ticks.style.top = top + 'px';
        // Ensure each child span is placed correctly (they were set by percent)
        const spans = ticks.querySelectorAll('span');
        spans.forEach((sp) => {
            // nothing to change here; left already set as percent during creation
        });
    }

    // Reposition ticks on window resize to stay aligned
    window.addEventListener('resize', () => {
        const sliders = document.querySelectorAll('input.slider[type="range"]');
        sliders.forEach((s) => {
            const parent = s.parentNode;
            const ticks = parent.querySelector('.slider-ticks');
            if (ticks) positionTicks(ticks, s);
        });
    });

    /**
     * Enable/disable controls based on current parameters and ordering
     */
    function updateControlAvailability() {
        // Center echo: disable for Sequential method
        const centerGroup = document.getElementById('center-echo-control');
        const centerSlider = document.getElementById('center-echo-slider');
        if (currentParams.ordering === 'sequential') {
            centerSlider.disabled = true;
            if (centerGroup) centerGroup.classList.add('disabled');
        } else {
            centerSlider.disabled = false;
            if (centerGroup) centerGroup.classList.remove('disabled');
        }

        // Calibration region: disabled when both ky and kz acceleration are 1
        const calGroup = document.getElementById('cal-size-control');
        const calSlider = document.getElementById('cal-size-slider');
        const isAccelerated = currentParams.kyAccel > 1 || currentParams.kzAccel > 1;
        calSlider.disabled = !isAccelerated;
        if (!isAccelerated) {
            if (calGroup) calGroup.classList.add('disabled');
        } else {
            if (calGroup) calGroup.classList.remove('disabled');
        }

        // CAIPIRINHA: enable when either ky or kz acceleration > 1
        const caipiCheckbox = document.getElementById('caipi-checkbox');
        const caipiGroup = document.getElementById('caipi-control');
        if (caipiCheckbox) {
            caipiCheckbox.disabled = !isAccelerated;
            if (!isAccelerated) {
                if (caipiGroup) caipiGroup.classList.add('disabled');
            } else {
                if (caipiGroup) caipiGroup.classList.remove('disabled');
            }
        }

        const macroGroup = document.getElementById('macro-width-control');
        const macroSlider = document.getElementById('macro-width-slider');
        if (macroSlider) macroSlider.disabled = false;
        if (macroGroup) {
            macroGroup.classList.remove('disabled');
            macroGroup.style.display = '';
        }
    }

    function syncOrderingControls() {
        const mtfControl = document.getElementById('mtf-control');
        const mtfSelect = document.getElementById('mtf-direction-select');
        const shotOrderControl = document.getElementById('shot-order-control');
        const shotOrderSelect = document.getElementById('shot-order-select');
        if (!mtfControl || !mtfSelect || !shotOrderControl || !shotOrderSelect) return;

        if (currentParams.ordering === 'sequential' || currentParams.ordering === 'lcpo') {
            mtfControl.style.display = '';
            mtfSelect.innerHTML = '<option value="ky">k<sub>y</sub></option><option value="kz">k<sub>z</sub></option>';
            mtfSelect.disabled = false;
            if (currentParams.mtfDirection !== 'ky' && currentParams.mtfDirection !== 'kz') {
                currentParams.mtfDirection = 'ky';
            }
            mtfSelect.value = currentParams.mtfDirection;
            if (currentParams.ordering === 'sequential') {
                currentParams.shotOrder = currentParams.mtfDirection === 'ky' ? 'kz' : 'ky';
            }
        } else if (currentParams.ordering === 'cplo') {
            mtfControl.style.display = '';
            mtfSelect.innerHTML = '<option value="kr">k<sub>r</sub></option>';
            mtfSelect.value = 'kr';
            currentParams.mtfDirection = 'kr';
            mtfSelect.disabled = false;
        } else {
            mtfControl.style.display = 'none';
            currentParams.mtfDirection = 'ky';
        }

        if (currentParams.ordering === 'cplo') {
            shotOrderControl.style.display = 'inline-flex';
            shotOrderSelect.innerHTML = `
                    <option value="ky">k<sub>y</sub></option>
                    <option value="kz">k<sub>z</sub></option>
                    <option value="azimuthal">Azimuthal</option>
                `;
            if (!['ky', 'kz', 'azimuthal'].includes(currentParams.shotOrder)) {
                currentParams.shotOrder = 'azimuthal';
            }
            shotOrderSelect.value = currentParams.shotOrder;
        } else if (currentParams.ordering === 'lcpo') {
            shotOrderControl.style.display = 'inline-flex';
            shotOrderSelect.innerHTML = `
                    <option value="ky">k<sub>y</sub></option>
                    <option value="kz">k<sub>z</sub></option>
                `;
            if (currentParams.shotOrder === 'azimuthal') {
                currentParams.shotOrder = currentParams.mtfDirection === 'ky' ? 'kz' : 'ky';
            }
            shotOrderSelect.value = currentParams.shotOrder;
        } else {
            shotOrderControl.style.display = 'none';
        }
    }
    
    /**
     * Show/hide spinner
     */
    function showSpinner() {
        const spinner = document.getElementById('spinner');
        if (spinner) spinner.classList.remove('hidden');
    }
    
    function hideSpinner() {
        const spinner = document.getElementById('spinner');
        if (spinner) spinner.classList.add('hidden');
    }
    
    /**
     * Setup event listeners for all controls
     */
    function setupEventListeners() {
        // ETL slider - update display while dragging and update immediately
        document.getElementById('etl-slider').addEventListener('input', (e) => {
            const val = parseInt(e.target.value);
            document.getElementById('etl-value').textContent = val;
            currentParams.etl = val;
            // Update center echo slider max to match ETL
            document.getElementById('center-echo-slider').max = currentParams.etl;
            if (currentParams.centerEcho > currentParams.etl) {
                currentParams.centerEcho = currentParams.etl;
                document.getElementById('center-echo-slider').value = currentParams.etl;
                document.getElementById('center-echo-value').textContent = currentParams.etl;
            }
            // Update center-echo datalist ticks
            const centerDatalist = document.getElementById('center-echo-ticks');
            if (centerDatalist) {
                let opts = '';
                for (let i = 1; i <= currentParams.etl; i++) {
                    opts += `<option value="${i}"></option>`;
                }
                centerDatalist.innerHTML = opts;
                // Re-render ticks for the center-echo slider so visuals match new ETL
                const centerSlider = document.getElementById('center-echo-slider');
                if (centerSlider) {
                    renderSliderTicks(centerSlider);
                    const ticks = centerSlider.parentNode.querySelector('.slider-ticks');
                    if (ticks) positionTicks(ticks, centerSlider);
                }
            }
            updatePlots();
        });
        
        // ETL slider - calculate on drag end
        document.getElementById('etl-slider').addEventListener('change', (e) => {
            currentParams.etl = parseInt(e.target.value);
            document.getElementById('etl-value').textContent = currentParams.etl;
            
            // Update center echo slider max to match ETL
            document.getElementById('center-echo-slider').max = currentParams.etl;
            
            // Keep center echo in valid range [1, ETL]
            if (currentParams.centerEcho > currentParams.etl) {
                currentParams.centerEcho = currentParams.etl;
                document.getElementById('center-echo-slider').value = currentParams.etl;
                document.getElementById('center-echo-value').textContent = currentParams.etl;
            }
            // Update center-echo datalist ticks on change as well
            const centerDatalist = document.getElementById('center-echo-ticks');
            if (centerDatalist) {
                let opts = '';
                for (let i = 1; i <= currentParams.etl; i++) {
                    opts += `<option value="${i}"></option>`;
                }
                centerDatalist.innerHTML = opts;
                // Re-render ticks for the center-echo slider so visuals match new ETL
                const centerSlider = document.getElementById('center-echo-slider');
                if (centerSlider) {
                    renderSliderTicks(centerSlider);
                    const ticks = centerSlider.parentNode.querySelector('.slider-ticks');
                    if (ticks) positionTicks(ticks, centerSlider);
                }
            }
            updatePlots();
        });
        
        // k_y slider - update display while dragging and update immediately
        document.getElementById('ky-slider').addEventListener('input', (e) => {
            const val = parseInt(e.target.value);
            document.getElementById('ky-value').textContent = val;
            currentParams.ky = val;
            updatePlots();
        });
        
        // k_y slider - calculate on drag end
        document.getElementById('ky-slider').addEventListener('change', (e) => {
            currentParams.ky = parseInt(e.target.value);
            document.getElementById('ky-value').textContent = currentParams.ky;
            updatePlots();
        });
        
        // k_z slider - update display while dragging and update immediately
        document.getElementById('kz-slider').addEventListener('input', (e) => {
            const val = parseInt(e.target.value);
            document.getElementById('kz-value').textContent = val;
            currentParams.kz = val;
            updatePlots();
        });
        
        // k_z slider - calculate on drag end
        document.getElementById('kz-slider').addEventListener('change', (e) => {
            currentParams.kz = parseInt(e.target.value);
            document.getElementById('kz-value').textContent = currentParams.kz;
            updatePlots();
        });
        
        // Center echo slider - update display while dragging and update immediately
        document.getElementById('center-echo-slider').addEventListener('input', (e) => {
            const val = parseInt(e.target.value);
            document.getElementById('center-echo-value').textContent = val;
            currentParams.centerEcho = val;
            // Make echo highlight follow center echo while dragging
            const echoSelector = document.getElementById('echo-select-slider');
            const echoSelectorValue = document.getElementById('echo-select-value');
            if (echoSelector) {
                echoSelector.value = val;
                if (echoSelectorValue) echoSelectorValue.textContent = val;
                D3Plots.setSelectedEcho(val);
                D3Plots.drawHighlights();
            }
            updatePlots();
        });
        
        // Center echo slider - calculate on drag end
        document.getElementById('center-echo-slider').addEventListener('change', (e) => {
            currentParams.centerEcho = parseInt(e.target.value);
            document.getElementById('center-echo-value').textContent = currentParams.centerEcho;
            // Make echo highlight follow center echo on change as well
            const echoSelector = document.getElementById('echo-select-slider');
            const echoSelectorValue = document.getElementById('echo-select-value');
            if (echoSelector) {
                echoSelector.value = currentParams.centerEcho;
                if (echoSelectorValue) echoSelectorValue.textContent = currentParams.centerEcho;
                D3Plots.setSelectedEcho(currentParams.centerEcho);
                D3Plots.drawHighlights();
            }
            updatePlots();
        });
        
        // Ordering select
        document.getElementById('ordering-select').addEventListener('change', (e) => {
            currentParams.ordering = e.target.value;
            syncOrderingControls();
            
            // Update control availability when ordering changes
            updateControlAvailability();
            updatePlots();
        });
        
        // MTF Direction select
        document.getElementById('mtf-direction-select').addEventListener('change', (e) => {
            currentParams.mtfDirection = e.target.value;
            updatePlots();
        });
        
        // Shot Order select
        document.getElementById('shot-order-select').addEventListener('change', (e) => {
            currentParams.shotOrder = e.target.value;
            updatePlots();
        });

        const macroWidthSlider = document.getElementById('macro-width-slider');
        if (macroWidthSlider) {
            macroWidthSlider.addEventListener('input', (e) => {
                currentParams.macroWidth = parseInt(e.target.value);
                document.getElementById('macro-width-value').textContent = currentParams.macroWidth;
                updatePlots(250);
            });
            macroWidthSlider.addEventListener('change', (e) => {
                currentParams.macroWidth = parseInt(e.target.value);
                document.getElementById('macro-width-value').textContent = currentParams.macroWidth;
                updatePlots();
            });
        }

        
        // ky Acceleration slider - update display while dragging and update immediately
        document.getElementById('ky-accel-slider').addEventListener('input', (e) => {
            const val = parseInt(e.target.value);
            document.getElementById('ky-accel-value').textContent = val;
            currentParams.kyAccel = val;
            // Recompute control availability (will enable/disable calibration and CAIPI)
            updateControlAvailability();
            updatePlots();
        });
        
        // ky Acceleration slider - calculate on drag end
        document.getElementById('ky-accel-slider').addEventListener('change', (e) => {
            currentParams.kyAccel = parseInt(e.target.value);
            document.getElementById('ky-accel-value').textContent = currentParams.kyAccel;
            // Update control availability centrally
            updateControlAvailability();
            updatePlots();
        });
        
        // kz Acceleration slider - update display while dragging and update immediately
        document.getElementById('kz-accel-slider').addEventListener('input', (e) => {
            const val = parseInt(e.target.value);
            document.getElementById('kz-accel-value').textContent = val;
            currentParams.kzAccel = val;
            // Recompute control availability (will enable/disable calibration and CAIPI)
            updateControlAvailability();
            updatePlots();
        });
        
        // kz Acceleration slider - calculate on drag end
        document.getElementById('kz-accel-slider').addEventListener('change', (e) => {
            currentParams.kzAccel = parseInt(e.target.value);
            document.getElementById('kz-accel-value').textContent = currentParams.kzAccel;
            // Update control availability centrally
            updateControlAvailability();
            updatePlots();
        });
        
        // kz Partial Fourier slider - update display while dragging and update immediately
        document.getElementById('kz-pf-slider').addEventListener('input', (e) => {
            const val = parseFloat(e.target.value);
            document.getElementById('kz-pf-value').textContent = val.toFixed(2).replace(/\.00$/, '.0');
            currentParams.kzPF = val;
            updatePlots();
        });
        
        // kz Partial Fourier slider - calculate on drag end
        document.getElementById('kz-pf-slider').addEventListener('change', (e) => {
            currentParams.kzPF = parseFloat(e.target.value);
            document.getElementById('kz-pf-value').textContent = currentParams.kzPF.toFixed(1);
            updatePlots();
        });
        
        // Calibration region size slider - update display while dragging and update immediately
        document.getElementById('cal-size-slider').addEventListener('input', (e) => {
            const val = parseInt(e.target.value);
            document.getElementById('cal-size-value').textContent = val;
            currentParams.calibrationSize = val;
            updatePlots();
        });
        
        // Calibration region size slider - calculate on drag end
        document.getElementById('cal-size-slider').addEventListener('change', (e) => {
            currentParams.calibrationSize = parseInt(e.target.value);
            document.getElementById('cal-size-value').textContent = currentParams.calibrationSize;
            updatePlots();
        });
        
        // CAIPIRINHA checkbox
        document.getElementById('caipi-checkbox').addEventListener('change', (e) => {
            currentParams.useCaipirinha = e.target.checked;
            updatePlots();
        });

        // Coverage select (rectangular or elliptical)
        const coverageSelect = document.getElementById('coverage-select');
        if (coverageSelect) {
            coverageSelect.addEventListener('change', (e) => {
                currentParams.coverage = e.target.value;
                updatePlots();
            });
            // Initialize select to match currentParams
            coverageSelect.value = currentParams.coverage || 'elliptical';
        }

        // Shot/Echo selector sliders (under plots)
        const shotSelector = document.getElementById('shot-select-slider');
        const shotSelectorValue = document.getElementById('shot-select-value');
        const shotCheckbox = document.getElementById('shot-highlight-checkbox');
        
        if (shotCheckbox) {
            shotCheckbox.addEventListener('change', (e) => {
                D3Plots.setShowShotHighlight(e.target.checked);
                D3Plots.drawHighlights();
            });
        }

        const shotTrajectoryToggle = document.getElementById('shot-trajectory-toggle');
        if (shotTrajectoryToggle) {
            D3Plots.setShowShotTrajectory(shotTrajectoryToggle.checked);
            shotTrajectoryToggle.addEventListener('change', (e) => {
                D3Plots.setShowShotTrajectory(e.target.checked);
                D3Plots.drawHighlights();
            });
        }
        
        if (shotSelector) {
            shotSelector.addEventListener('input', (e) => {
                const v = parseInt(e.target.value);
                if (shotSelectorValue) shotSelectorValue.textContent = v;
                D3Plots.setSelectedShot(v);
                D3Plots.drawHighlights();
            });
            shotSelector.addEventListener('change', (e) => {
                const v = parseInt(e.target.value);
                if (shotSelectorValue) shotSelectorValue.textContent = v;
                D3Plots.setSelectedShot(v);
                D3Plots.drawHighlights();
            });
        }

        const echoSelector = document.getElementById('echo-select-slider');
        const echoSelectorValue = document.getElementById('echo-select-value');
        const echoCheckbox = document.getElementById('echo-highlight-checkbox');
        const echoTrajectoryToggle = document.getElementById('echo-trajectory-toggle');
        
        if (echoCheckbox) {
            echoCheckbox.addEventListener('change', (e) => {
                D3Plots.setShowEchoHighlight(e.target.checked);
                D3Plots.drawHighlights();
            });
        }

        if (echoTrajectoryToggle) {
            D3Plots.setShowEchoTrajectory(echoTrajectoryToggle.checked);
            echoTrajectoryToggle.addEventListener('change', (e) => {
                D3Plots.setShowEchoTrajectory(e.target.checked);
                D3Plots.drawHighlights();
            });
        }
        
        if (echoSelector) {
            echoSelector.addEventListener('input', (e) => {
                const v = parseInt(e.target.value);
                if (echoSelectorValue) echoSelectorValue.textContent = v;
                D3Plots.setSelectedEcho(v);
                D3Plots.drawHighlights();
            });
            echoSelector.addEventListener('change', (e) => {
                const v = parseInt(e.target.value);
                if (echoSelectorValue) echoSelectorValue.textContent = v;
                D3Plots.setSelectedEcho(v);
                D3Plots.drawHighlights();
            });
        }
        
        // Export buttons
        const exportShotBtn = document.getElementById('export-shot-svg');
        const exportEchoBtn = document.getElementById('export-echo-svg');
        
        if (exportShotBtn) {
            exportShotBtn.addEventListener('click', () => {
                D3Plots.exportShotSVG();
            });
        }
        
        if (exportEchoBtn) {
            exportEchoBtn.addEventListener('click', () => {
                D3Plots.exportEchoSVG();
            });
        }
        
        // JSON export button
        const exportJsonBtn = document.getElementById('export-json');
        if (exportJsonBtn) {
            exportJsonBtn.addEventListener('click', () => {
                exportJSON();
            });
        }
    }
    
    /**
     * Update all plots based on current parameters
     */
    function updatePlots(delay = 0) {
        updateGeneration++;
        const generation = updateGeneration;
        updateViewOrderUrl();
        if (updateTimer) {
            clearTimeout(updateTimer);
            updateTimer = null;
        }
        showSpinner();
        // Use setTimeout to yield to browser briefly for UI updates
        updateTimer = setTimeout(() => {
            updateTimer = null;
            if (generation !== updateGeneration) return;
            const t0 = performance.now();

            // Generate coordinates
            currentCoords = KSpaceUtils.generateCoordinates(
                currentParams.ky,
                currentParams.kz,
                currentParams.kyAccel,
                currentParams.kzAccel,
                currentParams.useCaipirinha,
                currentParams.coverage,
                currentParams.calibrationSize,
                currentParams.kzPF
            );
            const t1 = performance.now();

            // Assign view ordering
            currentCoords = KSpaceUtils.assignViewOrdering(
                currentCoords,
                currentParams.etl,
                currentParams.ordering,
                currentParams.centerEcho,
                currentParams.mtfDirection,
                currentParams.shotOrder,
                {
                    macroWidth: currentParams.macroWidth
                }
            );
            const t2 = performance.now();
            currentJumpMetrics = currentCoords.jumpMetrics || KSpaceUtils.calculateJumpMetrics(currentCoords);

            // If Sequential mode, derive the center echo from the centermost coordinate
            if (currentParams.ordering === 'sequential' && currentCoords.length > 0) {
                // Find the coordinate with smallest radius (centermost)
                let minIdx = 0;
                let minR = currentCoords[0].r;
                for (let i = 1; i < currentCoords.length; i++) {
                    if (currentCoords[i].r < minR) {
                        minR = currentCoords[i].r;
                        minIdx = i;
                    }
                }
                const centerCoord = currentCoords[minIdx];
                if (centerCoord && typeof centerCoord.echo === 'number') {
                    currentParams.centerEcho = centerCoord.echo;
                    // Update UI (slider + displayed value)
                    const centerSlider = document.getElementById('center-echo-slider');
                    const centerValue = document.getElementById('center-echo-value');
                    if (centerSlider) {
                        centerSlider.value = currentParams.centerEcho;
                    }
                    if (centerValue) {
                        centerValue.textContent = currentParams.centerEcho;
                    }
                    updateViewOrderUrl();
                }
            }
            currentMacroSearch = findBestMacroWidth();

            // Render plots
            D3Plots.plotByShotNumber(currentCoords);
            const t3 = performance.now();

            D3Plots.plotByEchoNumber(currentCoords);
            const t4 = performance.now();

            // Redraw highlights on top according to current selection
            D3Plots.drawHighlights();

            // Update info panel
            updateInfoPanel();

            // Update perf panel
            updatePerfPanel({
                gen:    t1 - t0,
                assign: t2 - t1,
                shot:   t3 - t2,
                echo:   t4 - t3,
            });
            updateMacroWidthBestLabel();
            hideSpinner();
        }, delay);
    }

    function makeCurrentCoordinates() {
        return KSpaceUtils.generateCoordinates(
            currentParams.ky,
            currentParams.kz,
            currentParams.kyAccel,
            currentParams.kzAccel,
            currentParams.useCaipirinha,
            currentParams.coverage,
            currentParams.calibrationSize,
            currentParams.kzPF
        );
    }

    function findBestMacroWidth() {
        const macroSlider = document.getElementById('macro-width-slider');
        const maxWidth = macroSlider ? parseInt(macroSlider.max, 10) : 40;
        return KSpaceUtils.findBestMacroWidthByRms(
            makeCurrentCoordinates,
            currentParams.etl,
            currentParams.ordering,
            currentParams.centerEcho,
            currentParams.mtfDirection,
            currentParams.shotOrder,
            1,
            maxWidth
        );
    }

    function updateMacroWidthBestLabel() {
        const best = document.getElementById('macro-width-best');
        if (!best) return;
        best.textContent = currentMacroSearch ? `(best ${currentMacroSearch.bestWidth})` : '';
    }

    /**
     * Display per-stage timing in the performance panel
     */
    function updatePerfPanel(times) {
        const total = times.gen + times.assign + times.shot + times.echo;
        const fmt = (ms) => ms.toFixed(1);
        const pct = (ms) => total > 0 ? (ms / total * 100).toFixed(0) + '%' : '—';

        const set = (id, val) => { const el = document.getElementById(id); if (el) el.textContent = val; };
        set('perf-gen',        fmt(times.gen));
        set('perf-gen-pct',    pct(times.gen));
        set('perf-assign',     fmt(times.assign));
        set('perf-assign-pct', pct(times.assign));
        set('perf-shot',       fmt(times.shot));
        set('perf-shot-pct',   pct(times.shot));
        set('perf-echo',       fmt(times.echo));
        set('perf-echo-pct',   pct(times.echo));
        set('perf-total',      fmt(total));
    }
    
    /**
     * Update information panel
     */
    function updateInfoPanel() {
        const totalCoords = currentCoords.length;
        const shots = new Set(currentCoords.map(c => c.shot));
        const numShots = shots.size;
        
        document.getElementById('info-total').textContent = totalCoords;
        document.getElementById('info-acceleration').textContent = numShots;
        updateJumpMetricPanel();
        
        // Update shot/echo selector sliders
        const shotSlider = document.getElementById('shot-select-slider');
        const shotValue = document.getElementById('shot-select-value');
        if (shotSlider) {
            shotSlider.max = Math.max(1, numShots);
            // Set to 1/3 of shots on first update
            if (shotSlider.value === '1' && numShots > 1) {
                shotSlider.value = Math.max(1, Math.round(numShots / 3));
            }
            if (parseInt(shotSlider.value) > shotSlider.max) shotSlider.value = shotSlider.max;
            if (shotValue) shotValue.textContent = shotSlider.value;
            // Rerender ticks for shot slider
            renderSliderTicks(shotSlider);
            const ticks = shotSlider.parentNode.querySelector('.slider-ticks');
            if (ticks) positionTicks(ticks, shotSlider);
            // Update highlight
            D3Plots.setSelectedShot(parseInt(shotSlider.value));
        }

        const echoSlider = document.getElementById('echo-select-slider');
        const echoValue = document.getElementById('echo-select-value');
        if (echoSlider) {
            echoSlider.max = Math.max(1, currentParams.etl);
            // Set to center echo on first update
            if (echoSlider.value === '1' && currentParams.centerEcho > 1) {
                echoSlider.value = currentParams.centerEcho;
            }
            if (parseInt(echoSlider.value) > echoSlider.max) echoSlider.value = echoSlider.max;
            if (echoValue) echoValue.textContent = echoSlider.value;
            // Rerender ticks for echo slider
            renderSliderTicks(echoSlider);
            const ticks2 = echoSlider.parentNode.querySelector('.slider-ticks');
            if (ticks2) positionTicks(ticks2, echoSlider);
            // Update highlight
            D3Plots.setSelectedEcho(parseInt(echoSlider.value));
        }
        
        // Redraw highlights with the updated selections
        D3Plots.drawHighlights();
    }

    function updateJumpMetricPanel() {
        const metrics = currentJumpMetrics || KSpaceUtils.calculateJumpMetrics(currentCoords);
        const fmtFloat = (value) => value.toFixed(2);
        const set = (id, val) => {
            const el = document.getElementById(id);
            if (el) el.textContent = val;
        };

        set('metric-rms-jump', fmtFloat(metrics.rmsJump));
    }
    
    /**
     * Get current coordinates (for debugging/export)
     */
    function getCurrentCoords() {
        return currentCoords;
    }
    
    /**
     * Get current parameters (for debugging/export)
     */
    function getCurrentParams() {
        return {...currentParams};
    }

    function getCurrentJumpMetrics() {
        return currentJumpMetrics ? {...currentJumpMetrics} : null;
    }
    
    /**
     * Export view order and metadata to JSON
     */
    function exportJSON() {
        // Calculate derived metadata
        const shots = new Set(currentCoords.map(c => c.shot));
        const numberOfShots = shots.size;
        const totalEncodes = currentCoords.length;
        const encodesPerShot = totalEncodes > 0 ? Math.ceil(totalEncodes / numberOfShots) : 0;
        const metrics = currentJumpMetrics || KSpaceUtils.calculateJumpMetrics(currentCoords);
        
        // Build metadata object with human-readable keys
        const metadata = {
            "Echo Train Length": currentParams.etl,
            "Matrix Size ky": currentParams.ky,
            "Matrix Size kz": currentParams.kz,
            "Number of Shots": numberOfShots,
            "Total Phase Encodes": totalEncodes,
            "Encodes per Shot": encodesPerShot,
            "ky Acceleration Factor": currentParams.kyAccel,
            "kz Acceleration Factor": currentParams.kzAccel,
            "kz Partial Fourier Factor": currentParams.kzPF,
            "Calibration Region Size": currentParams.calibrationSize,
            "CAIPIRINHA": currentParams.useCaipirinha,
            "k-space Coverage": currentParams.coverage,
            "View Ordering": currentParams.ordering,
            "Center Echo": currentParams.centerEcho,
            "MTF Direction": currentParams.mtfDirection,
            "Shot Order": currentParams.shotOrder,
            "Macro Width": currentParams.macroWidth,
            "RMS jump": metrics.rmsJump
        };
        
        // Build encodes array: sort by (shot asc, echo asc), project relevant fields
        const encodes = currentCoords
            .slice()
            .sort((a, b) => {
                if (a.shot !== b.shot) return a.shot - b.shot;
                return a.echo - b.echo;
            })
            .map(c => ({
                shot: c.shot,
                echo: c.echo,
                macroEcho: c.macroEcho,
                ky: c.ky,
                kz: c.kz
            }));
        
        // Build final JSON object
        const jsonData = {
            metadata,
            encodes
        };
        
        // Create blob and trigger download
        const jsonString = JSON.stringify(jsonData, null, 2);
        const blob = new Blob([jsonString], { type: 'application/json' });
        const url = URL.createObjectURL(blob);
        const link = document.createElement('a');
        link.href = url;
        link.download = 'view-order.json';
        document.body.appendChild(link);
        link.click();
        document.body.removeChild(link);
        URL.revokeObjectURL(url);
    }
    
    return {
        init,
        renderAllSliderTicks,
        getCurrentCoords,
        getCurrentParams,
        getCurrentJumpMetrics
    };
})();

window.App = App;

// Expose live accessors for the MPRAGE tab and external review tooling.
window.AppState = {
    get coords() { return App.getCurrentCoords(); },
    get params()  { return App.getCurrentParams(); },
    get jumpMetrics() { return App.getCurrentJumpMetrics(); },
};

// Initialize when DOM is ready
document.addEventListener('DOMContentLoaded', () => {
    App.init();
});
