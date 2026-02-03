/**
 * Main Application Logic
 * Coordinates interactions between controls, utilities, and plots
 */

const App = (() => {
    let currentCoords = [];
    let currentParams = {
        etl: 96,
        ky: 128,
        kz: 128,
        centerEcho: 32,
        ordering: 'sequential',
        kyAccel: 1,
        kzAccel: 1,
        kzPF: 1.0,
        mtfDirection: 'ky',
        coverage: 'elliptical',
        useCaipirinha: true,
        calibrationSize: 32
    };
    
    /**
     * Initialize the application
     */
    function init() {
        D3Plots.init();
        setupEventListeners();
        // Render slider ticks for visible controls
        renderAllSliderTicks();
        updateControlAvailability();
        updatePlots();
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

        const ticks = document.createElement('div');
        ticks.className = 'slider-ticks';

        for (let i = 0; i <= steps; i++) {
            const span = document.createElement('span');
            const frac = steps > 0 ? (i / steps) : 0;
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
        sliders.forEach(s => renderSliderTicks(s));
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
            updatePlots();
        });
        
        // Center echo slider - calculate on drag end
        document.getElementById('center-echo-slider').addEventListener('change', (e) => {
            currentParams.centerEcho = parseInt(e.target.value);
            document.getElementById('center-echo-value').textContent = currentParams.centerEcho;
            updatePlots();
        });
        
        // Ordering select
        document.getElementById('ordering-select').addEventListener('change', (e) => {
            currentParams.ordering = e.target.value;

            // Update MTF Direction widget per ordering method
            const mtfControl = document.getElementById('mtf-control');
            const mtfSelect = document.getElementById('mtf-direction-select');

            if (e.target.value === 'sequential' || e.target.value === 'lcpo') {
                // Show ky/kz options
                mtfControl.style.display = '';
                mtfSelect.innerHTML = '<option value="ky">k<sub>y</sub></option><option value="kz">k<sub>z</sub></option>';
                mtfSelect.disabled = false;
                // Keep previously selected if applicable
                if (currentParams.mtfDirection !== 'ky' && currentParams.mtfDirection !== 'kz') {
                    currentParams.mtfDirection = 'ky';
                    mtfSelect.value = 'ky';
                } else {
                    mtfSelect.value = currentParams.mtfDirection;
                }
            } else if (e.target.value === 'cplo') {
                // CPLO only uses radial option 'kr'
                mtfControl.style.display = '';
                mtfSelect.innerHTML = '<option value="kr">k<sub>r</sub></option>';
                mtfSelect.value = 'kr';
                currentParams.mtfDirection = 'kr';
                mtfSelect.disabled = false;
            } else {
                // Chevron and CROC: hide widget
                mtfControl.style.display = 'none';
                // set to default
                currentParams.mtfDirection = 'ky';
            }

            // Update control availability when ordering changes
            updateControlAvailability();
            updatePlots();
        });
        
        // MTF Direction select
        document.getElementById('mtf-direction-select').addEventListener('change', (e) => {
            currentParams.mtfDirection = e.target.value;
            updatePlots();
        });
        
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
    }
    
    /**
     * Update all plots based on current parameters
     */
    function updatePlots() {
        // Render immediately (spinner disabled for fast updates)
        // Use setTimeout to yield to browser briefly for UI updates
        setTimeout(() => {
            // Generate coordinates with new parameters
            // Use separate Ry and Rz for ky and kz acceleration
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
            
            // Assign view ordering
            currentCoords = KSpaceUtils.assignViewOrdering(
                currentCoords,
                currentParams.etl,
                currentParams.ordering,
                currentParams.centerEcho,
                currentParams.mtfDirection
            );

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
                }
            }
            
            // Update plots
            D3Plots.plotByShotNumber(currentCoords);
            D3Plots.plotByEchoNumber(currentCoords);
            
            // Update info panel
            updateInfoPanel();
            
            // spinner disabled
        }, 0);
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
    
    return {
        init,
        getCurrentCoords,
        getCurrentParams
    };
})();

// Initialize when DOM is ready
document.addEventListener('DOMContentLoaded', () => {
    App.init();
});
