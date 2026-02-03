/**
 * Canvas-based Plots Module
 * High-performance k-space visualization using Canvas 2D
 */

const D3Plots = (() => {
    let shotCanvas = null;
    let echoCanvas = null;
    let shotCtx = null;
    let echoCtx = null;
    let shotXScale = null;
    let shotYScale = null;
    let echoXScale = null;
    let echoYScale = null;
    
    let shotCoords = [];
    let echoCoords = [];
    let shotColorScale = null;
    let echoColorScale = null;
    let blockWidth = 0;
    let blockHeight = 0;
    
    const margin = { top: 40, right: 20, bottom: 50, left: 60 };
    const width = 600 - margin.left - margin.right;
    const height = 600 - margin.top - margin.bottom;
    const totalWidth = width + margin.left + margin.right;
    const totalHeight = height + margin.top + margin.bottom;
    
    // Tooltip element
    let tooltip = null;
    
    /**
     * Initialize plot containers with Canvas elements
     */
    function init() {
        // Clear existing plots
        const shotDiv = document.getElementById('plot-by-shot');
        const echoDiv = document.getElementById('plot-by-echo');
        
        if (shotDiv) shotDiv.innerHTML = '';
        if (echoDiv) echoDiv.innerHTML = '';
        
        // Create canvas for shot plot
        shotCanvas = document.createElement('canvas');
        shotCanvas.width = totalWidth;
        shotCanvas.height = totalHeight;
        shotCanvas.style.cursor = 'crosshair';
        shotDiv.appendChild(shotCanvas);
        shotCtx = shotCanvas.getContext('2d');
        
        // Create canvas for echo plot
        echoCanvas = document.createElement('canvas');
        echoCanvas.width = totalWidth;
        echoCanvas.height = totalHeight;
        echoCanvas.style.cursor = 'crosshair';
        echoDiv.appendChild(echoCanvas);
        echoCtx = echoCanvas.getContext('2d');
        
        // Create tooltip
        if (!tooltip) {
            tooltip = document.createElement('div');
            tooltip.className = 'canvas-tooltip';
            tooltip.style.cssText = `
                position: absolute;
                visibility: hidden;
                background-color: white;
                border: 1px solid #ddd;
                border-radius: 4px;
                padding: 8px;
                font-size: 12px;
                pointer-events: none;
                z-index: 1000;
            `;
            document.body.appendChild(tooltip);
        }
        
        // Add mouse move listeners for tooltips
        shotCanvas.addEventListener('mousemove', (e) => handleMouseMove(e, shotCanvas, shotCoords, shotXScale, shotYScale));
        echoCanvas.addEventListener('mousemove', (e) => handleMouseMove(e, echoCanvas, echoCoords, echoXScale, echoYScale));
        
        shotCanvas.addEventListener('mouseout', () => tooltip.style.visibility = 'hidden');
        echoCanvas.addEventListener('mouseout', () => tooltip.style.visibility = 'hidden');
    }
    
    /**
     * Mouse move handler for tooltip display
     */
    function handleMouseMove(event, canvas, coords, xScale, yScale) {
        if (!coords.length || !xScale || !yScale) return;
        
        const rect = canvas.getBoundingClientRect();
        const mouseX = event.clientX - rect.left - margin.left;
        const mouseY = event.clientY - rect.top - margin.top;
        
        // Convert pixel to data coordinates
        const dataKy = xScale.invert(mouseX);
        const dataKz = yScale.invert(mouseY);
        
        // Find closest point within threshold
        const threshold = Math.max(blockWidth, blockHeight) / 2;
        let closest = null;
        let minDist = threshold;
        
        for (const coord of coords) {
            const px = xScale(coord.ky);
            const py = yScale(coord.kz);
            const dist = Math.sqrt((px - mouseX) ** 2 + (py - mouseY) ** 2);
            
            if (dist < minDist) {
                minDist = dist;
                closest = coord;
            }
        }
        
        if (closest) {
            tooltip.style.visibility = 'visible';
            tooltip.innerHTML = `
                <strong>${closest.isCalibration ? 'Calibration' : 'Acquisition'}</strong><br/>
                Shot: ${closest.shot || 0}<br/>
                Echo: ${closest.echo || 0}<br/>
                ky: ${closest.ky}<br/>
                kz: ${closest.kz}<br/>
                r: ${closest.r.toFixed(3)}<br/>
                φ: ${(closest.phi * 180 / Math.PI).toFixed(1)}°
            `;
            tooltip.style.top = (event.pageY - 10) + 'px';
            tooltip.style.left = (event.pageX + 10) + 'px';
        } else {
            tooltip.style.visibility = 'hidden';
        }
    }
    
    /**
     * Calculate axis ranges with padding
     */
    function calculateRanges(coords) {
        // Use full matrix size range, not just the data range
        // This ensures consistent scaling even with partial Fourier
        let full_ky_max = 0, full_kz_max = 0;
        
        for (const c of coords) {
            full_ky_max = Math.max(full_ky_max, Math.abs(c.ky));
            full_kz_max = Math.max(full_kz_max, Math.abs(c.kz));
        }
        
        // Also check calibration region for full range
        for (const c of coords) {
            if (c.isCalibration) {
                full_ky_max = Math.max(full_ky_max, Math.abs(c.ky));
                full_kz_max = Math.max(full_kz_max, Math.abs(c.kz));
            }
        }
        
        // Apply 10% padding
        const padding_level = 0.1;
        const ky_padding = padding_level * full_ky_max;
        const kz_padding = padding_level * full_kz_max;
        
        return {
            ky: [-full_ky_max - ky_padding, full_ky_max + ky_padding],
            kz: [-full_kz_max - kz_padding, full_kz_max + kz_padding]
        };
    }
    
    /**
     * Create color scale for shots/echoes
     */
    function createColorScale(maxValue, useViridis = true) {
        const interpolator = useViridis ? d3.interpolateViridis : d3.interpolatePlasma;
        return d3.scaleSequential(interpolator)
            .domain([0, maxValue]);
    }
    
    /**
     * Draw title and axes on canvas
     */
    function drawAxes(ctx, xScale, yScale, title) {
        // Clear canvas
        ctx.clearRect(0, 0, totalWidth, totalHeight);
        
        // Set background
        ctx.fillStyle = 'white';
        ctx.fillRect(0, 0, totalWidth, totalHeight);
        
        // Draw title
        ctx.fillStyle = 'black';
        ctx.font = 'bold 16px sans-serif';
        ctx.textAlign = 'center';
        ctx.fillText(title, totalWidth / 2, 25);
        
        // Save state before transforming
        ctx.save();
        ctx.translate(margin.left, margin.top);
        
        // Draw axes
        ctx.strokeStyle = 'black';
        ctx.lineWidth = 1;
        ctx.font = '12px sans-serif';
        
        // X-axis
        ctx.beginPath();
        ctx.moveTo(0, height);
        ctx.lineTo(width, height);
        ctx.stroke();
        
        // Y-axis
        ctx.beginPath();
        ctx.moveTo(0, 0);
        ctx.lineTo(0, height);
        ctx.stroke();
        
        // X-axis ticks and labels
        const xTicks = xScale.ticks(10);
        ctx.textAlign = 'center';
        ctx.textBaseline = 'top';
        for (const tick of xTicks) {
            const x = xScale(tick);
            ctx.beginPath();
            ctx.moveTo(x, height);
            ctx.lineTo(x, height + 5);
            ctx.stroke();
            ctx.fillText(tick.toFixed(0), x, height + 8);
        }
        
        // Y-axis ticks and labels
        const yTicks = yScale.ticks(10);
        ctx.textAlign = 'right';
        ctx.textBaseline = 'middle';
        for (const tick of yTicks) {
            const y = yScale(tick);
            ctx.beginPath();
            ctx.moveTo(0, y);
            ctx.lineTo(-5, y);
            ctx.stroke();
            ctx.fillText(tick.toFixed(0), -8, y);
        }
        
        // Axis labels
        ctx.font = '14px sans-serif';
        ctx.textAlign = 'center';
        ctx.textBaseline = 'top';
        ctx.fillText('ky', width / 2, height + 30);
        
        ctx.save();
        ctx.translate(-45, height / 2);
        ctx.rotate(-Math.PI / 2);
        ctx.textAlign = 'center';
        ctx.textBaseline = 'top';
        ctx.fillText('kz', 0, 0);
        ctx.restore();
        
        // Draw grid
        ctx.strokeStyle = 'rgba(0, 0, 0, 0.1)';
        ctx.lineWidth = 0.5;
        for (const tick of xTicks) {
            const x = xScale(tick);
            ctx.beginPath();
            ctx.moveTo(x, 0);
            ctx.lineTo(x, height);
            ctx.stroke();
        }
        for (const tick of yTicks) {
            const y = yScale(tick);
            ctx.beginPath();
            ctx.moveTo(0, y);
            ctx.lineTo(width, y);
            ctx.stroke();
        }
        
        ctx.restore();
    }
    
    /**
     * Plot phase encodes by shot number
     */
    function plotByShotNumber(coords) {
        if (!shotCtx || coords.length === 0) return;
        
        shotCoords = coords;
        
        // Calculate ranges
        const ranges = calculateRanges(coords);
        
        // Create scales
        shotXScale = d3.scaleLinear()
            .domain(ranges.ky)
            .range([0, width]);
        
        shotYScale = d3.scaleLinear()
            .domain(ranges.kz)
            .range([height, 0]);  // Invert y-axis
        
        // Get max shot number for color scale
        let maxShot = 0;
        for (const c of coords) {
            if (c.shot > maxShot) maxShot = c.shot;
        }
        shotColorScale = createColorScale(maxShot, false);  // Use magma
        
        // Calculate block size
        blockWidth = Math.abs(shotXScale(1) - shotXScale(0));
        blockHeight = Math.abs(shotYScale(1) - shotYScale(0));
        
        // Draw axes
        drawAxes(shotCtx, shotXScale, shotYScale, 'Phase Encoding Plan - Shot View');
        
        // Draw points
        shotCtx.save();
        shotCtx.translate(margin.left, margin.top);
        
        for (const coord of coords) {
            const x = shotXScale(coord.ky) - blockWidth / 2;
            const y = shotYScale(coord.kz) - blockHeight / 2;
            const color = shotColorScale(coord.shot || 0);
            
            shotCtx.fillStyle = color;
            shotCtx.globalAlpha = 0.8;
            shotCtx.fillRect(x, y, blockWidth, blockHeight);
        }
        
        shotCtx.restore();
        console.log('Shot plot rendered:', coords.length, 'points (Canvas)');
    }
    
    /**
     * Plot phase encodes by echo number
     */
    function plotByEchoNumber(coords) {
        if (!echoCtx || coords.length === 0) return;
        
        echoCoords = coords;
        
        // Calculate ranges
        const ranges = calculateRanges(coords);
        
        // Create scales
        echoXScale = d3.scaleLinear()
            .domain(ranges.ky)
            .range([0, width]);
        
        echoYScale = d3.scaleLinear()
            .domain(ranges.kz)
            .range([height, 0]);  // Invert y-axis
        
        // Get max echo number for color scale
        let maxEcho = 0;
        for (const c of coords) {
            if (c.echo > maxEcho) maxEcho = c.echo;
        }
        echoColorScale = createColorScale(maxEcho, true);  // Use viridis
        
        // Calculate block size
        blockWidth = Math.abs(echoXScale(1) - echoXScale(0));
        blockHeight = Math.abs(echoYScale(1) - echoYScale(0));
        
        // Draw axes
        drawAxes(echoCtx, echoXScale, echoYScale, 'Phase Encoding Plan - Echo View');
        
        // Draw points
        echoCtx.save();
        echoCtx.translate(margin.left, margin.top);
        
        for (const coord of coords) {
            const x = echoXScale(coord.ky) - blockWidth / 2;
            const y = echoYScale(coord.kz) - blockHeight / 2;
            const color = echoColorScale(coord.echo || 0);
            
            echoCtx.fillStyle = color;
            echoCtx.globalAlpha = 0.8;
            echoCtx.fillRect(x, y, blockWidth, blockHeight);
        }
        
        echoCtx.restore();
        console.log('Echo plot rendered:', coords.length, 'points (Canvas)');
    }
    
    return {
        init,
        plotByShotNumber,
        plotByEchoNumber
    };
})();
