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
        updateControlAvailability();
        updatePlots();
    }

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
