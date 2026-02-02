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
        updatePlots();
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
        // ETL slider - update display while dragging
        document.getElementById('etl-slider').addEventListener('input', (e) => {
            document.getElementById('etl-value').textContent = e.target.value;
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
        
        // k_y slider - update display while dragging
        document.getElementById('ky-slider').addEventListener('input', (e) => {
            document.getElementById('ky-value').textContent = e.target.value;
        });
        
        // k_y slider - calculate on drag end
        document.getElementById('ky-slider').addEventListener('change', (e) => {
            currentParams.ky = parseInt(e.target.value);
            document.getElementById('ky-value').textContent = currentParams.ky;
            updatePlots();
        });
        
        // k_z slider - update display while dragging
        document.getElementById('kz-slider').addEventListener('input', (e) => {
            document.getElementById('kz-value').textContent = e.target.value;
        });
        
        // k_z slider - calculate on drag end
        document.getElementById('kz-slider').addEventListener('change', (e) => {
            currentParams.kz = parseInt(e.target.value);
            document.getElementById('kz-value').textContent = currentParams.kz;
            updatePlots();
        });
        
        // Center echo slider - update display while dragging
        document.getElementById('center-echo-slider').addEventListener('input', (e) => {
            document.getElementById('center-echo-value').textContent = e.target.value;
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
            
            // Enable/disable MTF Direction based on ordering method
            const mtfDirectionSelect = document.getElementById('mtf-direction-select');
            const isSequentialOrLCPO = e.target.value === 'sequential' || e.target.value === 'lcpo';
            mtfDirectionSelect.disabled = !isSequentialOrLCPO;
            
            updatePlots();
        });
        
        // MTF Direction select
        document.getElementById('mtf-direction-select').addEventListener('change', (e) => {
            currentParams.mtfDirection = e.target.value;
            updatePlots();
        });
        
        // ky Acceleration slider - update display while dragging
        document.getElementById('ky-accel-slider').addEventListener('input', (e) => {
            document.getElementById('ky-accel-value').textContent = e.target.value;
        });
        
        // ky Acceleration slider - calculate on drag end
        document.getElementById('ky-accel-slider').addEventListener('change', (e) => {
            currentParams.kyAccel = parseInt(e.target.value);
            document.getElementById('ky-accel-value').textContent = currentParams.kyAccel;
            
            // Enable/disable calibration and CAIPIRINHA controls based on acceleration
            const isAccelerated = currentParams.kyAccel > 1 || currentParams.kzAccel > 1;
            document.getElementById('cal-size-slider').disabled = !isAccelerated;
            document.getElementById('caipi-checkbox').disabled = !isAccelerated;
            
            updatePlots();
        });
        
        // kz Acceleration slider - update display while dragging
        document.getElementById('kz-accel-slider').addEventListener('input', (e) => {
            document.getElementById('kz-accel-value').textContent = e.target.value;
        });
        
        // kz Acceleration slider - calculate on drag end
        document.getElementById('kz-accel-slider').addEventListener('change', (e) => {
            currentParams.kzAccel = parseInt(e.target.value);
            document.getElementById('kz-accel-value').textContent = currentParams.kzAccel;
            
            // Enable/disable calibration and CAIPIRINHA controls based on acceleration
            const isAccelerated = currentParams.kyAccel > 1 || currentParams.kzAccel > 1;
            document.getElementById('cal-size-slider').disabled = !isAccelerated;
            document.getElementById('caipi-checkbox').disabled = !isAccelerated;
            
            updatePlots();
        });
        
        // kz Partial Fourier slider - update display while dragging
        document.getElementById('kz-pf-slider').addEventListener('input', (e) => {
            document.getElementById('kz-pf-value').textContent = parseFloat(e.target.value).toFixed(1);
        });
        
        // kz Partial Fourier slider - calculate on drag end
        document.getElementById('kz-pf-slider').addEventListener('change', (e) => {
            currentParams.kzPF = parseFloat(e.target.value);
            document.getElementById('kz-pf-value').textContent = currentParams.kzPF.toFixed(1);
            updatePlots();
        });
        
        // Calibration region size slider - update display while dragging
        document.getElementById('cal-size-slider').addEventListener('input', (e) => {
            document.getElementById('cal-size-value').textContent = e.target.value;
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
        showSpinner();
        
        // Use setTimeout to ensure spinner displays before heavy computation
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
            
            hideSpinner();
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
