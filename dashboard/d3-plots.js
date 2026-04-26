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
    
    let shotColorbarCanvas = null;
    let echoColorbarCanvas = null;
    let shotColorbarCtx = null;
    let echoColorbarCtx = null;
    
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
    
    // Colorbar dimensions (horizontal)
    const colorbarWidth = 400;
    const colorbarHeight = 25;
    const colorbarMargin = { top: 12, right: 90, bottom: 12, left: 90 };
    
    // Tooltip element
    let tooltip = null;
    let selectedShotIndex = null; // zero-based
    let selectedEchoIndex = null; // one-based (echo values are 1..ETL)
    let showShotHighlight = true; // controlled by checkbox
    let showEchoHighlight = true; // controlled by checkbox
    
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
        
        // Create colorbar canvases
        shotColorbarCanvas = document.getElementById('colorbar-shot');
        echoColorbarCanvas = document.getElementById('colorbar-echo');
        
        if (shotColorbarCanvas) {
            shotColorbarCanvas.width = colorbarWidth + colorbarMargin.left + colorbarMargin.right;
            shotColorbarCanvas.height = colorbarHeight + colorbarMargin.top + colorbarMargin.bottom;
            shotColorbarCtx = shotColorbarCanvas.getContext('2d');
        }
        
        if (echoColorbarCanvas) {
            echoColorbarCanvas.width = colorbarWidth + colorbarMargin.left + colorbarMargin.right;
            echoColorbarCanvas.height = colorbarHeight + colorbarMargin.top + colorbarMargin.bottom;
            echoColorbarCtx = echoColorbarCanvas.getContext('2d');
        }
        
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
        console.log(`Creating color scale with max value: ${maxValue}, using ${useViridis ? 'viridis' : 'plasma'} palette`);
        return d3.scaleSequential(interpolator)
            .domain([0, maxValue]);
    }
    
    /**
     * Draw colorbar on canvas (horizontal) with position indicator
     */
    function drawColorbar(ctx, colorScale, leftLabel, rightLabel, selectedValue = null) {
        if (!ctx || !colorScale) return;
        
        const canvasWidth = colorbarWidth + colorbarMargin.left + colorbarMargin.right;
        const canvasHeight = colorbarHeight + colorbarMargin.top + colorbarMargin.bottom;
        
        // Clear canvas
        ctx.clearRect(0, 0, canvasWidth, canvasHeight);
        
        // Set background (neutral gray to match main plots)
        ctx.fillStyle = '#f3f3f3';
        ctx.fillRect(0, 0, canvasWidth, canvasHeight);
        
        // Draw gradient bar (horizontal)
        const barX = colorbarMargin.left;
        const barY = colorbarMargin.top;
        const barWidth = colorbarWidth;
        const barHeight = colorbarHeight;
        
        // Create horizontal gradient from left (min value) to right (max value)
        const domain = colorScale.domain();
        const maxVal = domain[1];
        const minVal = domain[0];
        
        for (let i = 0; i <= barWidth; i++) {
            // Map pixel position to data value (left = min, right = max)
            const t = i / barWidth;
            const value = minVal + t * (maxVal - minVal);
            
            ctx.fillStyle = colorScale(value);
            ctx.fillRect(barX + i, barY, 1, barHeight);
        }
        
        // Draw border around colorbar
        ctx.strokeStyle = 'black';
        ctx.lineWidth = 1;
        ctx.strokeRect(barX, barY, barWidth, barHeight);
        
        // Draw position indicator (triangles) if selectedValue is provided
        if (selectedValue !== null && selectedValue >= minVal && selectedValue <= maxVal) {
            const t = (selectedValue - minVal) / (maxVal - minVal);
            const indicatorX = barX + t * barWidth;
            
            ctx.fillStyle = 'red';
            ctx.strokeStyle = 'darkred';
            ctx.lineWidth = 1.5;
            
            // Draw triangle pointing down (▼) above the bar
            const triangleWidth = 10;
            indicatorY = barY - triangleWidth - 2;
            ctx.beginPath();
            ctx.moveTo(indicatorX, indicatorY + triangleWidth);  // apex at bottom
            ctx.lineTo(indicatorX - triangleWidth / 2, indicatorY + triangleWidth/4);  // base left
            ctx.lineTo(indicatorX + triangleWidth / 2, indicatorY + triangleWidth/4);  // base right
            ctx.closePath();
            ctx.fill();
            ctx.stroke();
        }
        
        // Draw labels
        ctx.fillStyle = 'black';
        ctx.font = 'bold 11px sans-serif';
        ctx.textAlign = 'center';
        
        // Left label (min value)
        const leftLabelX = barX - 5;
        const leftLines = leftLabel.split(' ');
        ctx.textAlign = 'right';
        leftLines.forEach((line, i) => {
            ctx.fillText(line, leftLabelX, barY + barHeight / 2 + (i - leftLines.length / 2 + 0.5) * 12);
        });
        
        // Right label (max value)
        const rightLabelX = barX + barWidth + 5;
        const rightLines = rightLabel.split(' ');
        ctx.textAlign = 'left';
        rightLines.forEach((line, i) => {
            ctx.fillText(line, rightLabelX, barY + barHeight / 2 + (i - rightLines.length / 2 + 0.5) * 12);
        });
    }
    
    /**
     * Draw title and axes on canvas
     */
    function drawAxes(ctx, xScale, yScale, title) {
        // Background is already set by drawPointsAsImage (or cleared for axes-only call)
        
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
     * Build a Uint8Array color LUT: index -> [r, g, b], size (maxVal+1)*3
     */
    function buildColorLUT(colorScale, maxVal) {
        const lut = new Uint8Array((maxVal + 1) * 3);
        for (let i = 0; i <= maxVal; i++) {
            const c = d3.color(colorScale(i));
            lut[i * 3]     = c.r;
            lut[i * 3 + 1] = c.g;
            lut[i * 3 + 2] = c.b;
        }
        return lut;
    }

    /**
     * Draw coords into ctx using an ImageData pixel buffer (fast path).
     * Axes must be drawn AFTER this call as putImageData overwrites everything.
     */
    function drawPointsAsImage(ctx, coords, xScale, yScale, lut, valueKey) {
        const imgData = ctx.createImageData(totalWidth, totalHeight);
        const data = imgData.data;

        // Fill background (#f3f3f3 = 243,243,243)
        for (let i = 0; i < data.length; i += 4) {
            data[i] = 243; data[i+1] = 243; data[i+2] = 243; data[i+3] = 255;
        }

        const bw = Math.max(1, Math.round(blockWidth));
        const bh = Math.max(1, Math.round(blockHeight));
        const halfBw = bw >> 1;
        const halfBh = bh >> 1;

        for (let ci = 0; ci < coords.length; ci++) {
            const coord = coords[ci];
            const cx = (xScale(coord.ky) + margin.left + 0.5 | 0) - halfBw;
            const cy = (yScale(coord.kz) + margin.top  + 0.5 | 0) - halfBh;
            const val = coord[valueKey] || 0;
            const li = val * 3;
            const r = lut[li], g = lut[li + 1], b = lut[li + 2];

            for (let py = cy; py < cy + bh; py++) {
                if (py < 0 || py >= totalHeight) continue;
                let idx = (py * totalWidth + cx) * 4;
                for (let px = cx; px < cx + bw; px++, idx += 4) {
                    if (px < 0 || px >= totalWidth) continue;
                    data[idx]   = r;
                    data[idx+1] = g;
                    data[idx+2] = b;
                    // data[idx+3] already 255
                }
            }
        }

        ctx.putImageData(imgData, 0, 0);
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

        // Build LUT and draw all points via ImageData (fast path)
        const shotLUT = buildColorLUT(shotColorScale, maxShot);
        drawPointsAsImage(shotCtx, coords, shotXScale, shotYScale, shotLUT, 'shot');

        // Draw axes on top of pixel data
        drawAxes(shotCtx, shotXScale, shotYScale, 'Phase Encoding Plan - Shot View');

        // Draw colorbar
        drawColorbar(shotColorbarCtx, shotColorScale, 'start of scan', 'end of scan', selectedShotIndex);

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

        // Build LUT and draw all points via ImageData (fast path)
        const echoLUT = buildColorLUT(echoColorScale, maxEcho);
        drawPointsAsImage(echoCtx, coords, echoXScale, echoYScale, echoLUT, 'echo');

        // Draw axes on top of pixel data
        drawAxes(echoCtx, echoXScale, echoYScale, 'Phase Encoding Plan - Echo View');

        // Draw colorbar with echo position indicator
        drawColorbar(echoColorbarCtx, echoColorScale, 'start of shot', 'end of shot', selectedEchoIndex);

        console.log('Echo plot rendered:', coords.length, 'points (Canvas)');
    }

    /**
     * Draw highlight overlays on both canvases for selected shot/echo
     */
    function drawHighlights() {
        // Re-render base plots to clear any previous highlight overlays
        if (shotCoords && shotCoords.length) plotByShotNumber(shotCoords);
        if (echoCoords && echoCoords.length) plotByEchoNumber(echoCoords);

        // Draw on shot canvas (only current selections and if enabled)
        if (shotCtx && shotCoords && (selectedShotIndex !== null || selectedEchoIndex !== null)) {
            shotCtx.save();
            shotCtx.translate(margin.left, margin.top);
            for (const coord of shotCoords) {
                const x = shotXScale(coord.ky) - blockWidth / 2;
                const y = shotYScale(coord.kz) - blockHeight / 2;

                if (showShotHighlight && selectedShotIndex !== null && coord.shot === selectedShotIndex) {
                    shotCtx.fillStyle = 'black';
                    shotCtx.globalAlpha = 1.0;
                    shotCtx.fillRect(x, y, blockWidth, blockHeight);
                }

                if (showEchoHighlight && selectedEchoIndex !== null && coord.echo === selectedEchoIndex) {
                    shotCtx.fillStyle = 'white';
                    shotCtx.globalAlpha = 1.0;
                    shotCtx.fillRect(x, y, blockWidth, blockHeight);
                }
            }
            shotCtx.restore();
        }

        // Draw on echo canvas (only current selections and if enabled)
        if (echoCtx && echoCoords && (selectedShotIndex !== null || selectedEchoIndex !== null)) {
            echoCtx.save();
            echoCtx.translate(margin.left, margin.top);
            for (const coord of echoCoords) {
                const x = echoXScale(coord.ky) - blockWidth / 2;
                const y = echoYScale(coord.kz) - blockHeight / 2;

                if (showShotHighlight && selectedShotIndex !== null && coord.shot === selectedShotIndex) {
                    echoCtx.fillStyle = 'black';
                    echoCtx.globalAlpha = 1.0;
                    echoCtx.fillRect(x, y, blockWidth, blockHeight);
                }

                if (showEchoHighlight && selectedEchoIndex !== null && coord.echo === selectedEchoIndex) {
                    echoCtx.fillStyle = 'white';
                    echoCtx.globalAlpha = 1.0;
                    echoCtx.fillRect(x, y, blockWidth, blockHeight);
                }
            }
            echoCtx.restore();
        }
    }

    function setSelectedShot(oneBasedShot) {
        if (oneBasedShot == null) {
            selectedShotIndex = null;
        } else {
            selectedShotIndex = Math.max(0, oneBasedShot - 1);
        }
    }

    function setSelectedEcho(echo) {
        if (echo == null) selectedEchoIndex = null;
        else selectedEchoIndex = echo;
    }
    
    function setShowShotHighlight(enabled) {
        showShotHighlight = enabled;
    }
    
    function setShowEchoHighlight(enabled) {
        showEchoHighlight = enabled;
    }
    
    /**
     * Export shot plot as SVG
     */
    function exportShotSVG() {
        if (!shotCoords || shotCoords.length === 0 || !shotColorScale) return;
        
        const svg = createSVGPlot(shotCoords, shotXScale, shotYScale, shotColorScale, 
                                   'Phase Encoding Plan - Shot View', 'shot', selectedShotIndex);
        downloadSVG(svg, 'shot-view.svg');
    }
    
    /**
     * Export echo plot as SVG
     */
    function exportEchoSVG() {
        if (!echoCoords || echoCoords.length === 0 || !echoColorScale) return;
        
        const svg = createSVGPlot(echoCoords, echoXScale, echoYScale, echoColorScale, 
                                   'Phase Encoding Plan - Echo View', 'echo', selectedEchoIndex);
        downloadSVG(svg, 'echo-view.svg');
    }
    
    /**
     * Create SVG string for a plot
     */
    function createSVGPlot(coords, xScale, yScale, colorScale, title, plotType, selectedValue) {
        const svgWidth = totalWidth;
        const svgHeight = totalHeight;
        
        let svg = `<svg xmlns="http://www.w3.org/2000/svg" width="${svgWidth}" height="${svgHeight}" viewBox="0 0 ${svgWidth} ${svgHeight}">`;
        
        // Background
        svg += `<rect width="${svgWidth}" height="${svgHeight}" fill="#f3f3f3"/>`;
        
        // Title
        svg += `<text x="${svgWidth/2}" y="25" text-anchor="middle" font-family="sans-serif" font-size="16" font-weight="bold" fill="black">${title}</text>`;
        
        // Main plot group with margins
        svg += `<g transform="translate(${margin.left},${margin.top})">`;
        
        // Grid
        const xTicks = xScale.ticks(10);
        const yTicks = yScale.ticks(10);
        
        for (const tick of xTicks) {
            const x = xScale(tick);
            svg += `<line x1="${x}" y1="0" x2="${x}" y2="${height}" stroke="rgba(0,0,0,0.1)" stroke-width="0.5"/>`;
        }
        for (const tick of yTicks) {
            const y = yScale(tick);
            svg += `<line x1="0" y1="${y}" x2="${width}" y2="${y}" stroke="rgba(0,0,0,0.1)" stroke-width="0.5"/>`;
        }
        
        // Data points
        for (const coord of coords) {
            const x = xScale(coord.ky) - blockWidth / 2;
            const y = yScale(coord.kz) - blockHeight / 2;
            const color = plotType === 'shot' ? colorScale(coord.shot || 0) : colorScale(coord.echo || 0);
            svg += `<rect x="${x}" y="${y}" width="${blockWidth}" height="${blockHeight}" fill="${color}" shape-rendering="crispEdges"/>`;
        }
        
        // Highlights
        if (plotType === 'shot' && showShotHighlight && selectedShotIndex !== null) {
            for (const coord of coords) {
                if (coord.shot === selectedShotIndex) {
                    const x = xScale(coord.ky) - blockWidth / 2;
                    const y = yScale(coord.kz) - blockHeight / 2;
                    svg += `<rect x="${x}" y="${y}" width="${blockWidth}" height="${blockHeight}" fill="black" shape-rendering="crispEdges"/>`;
                }
            }
        }
        if (showEchoHighlight && selectedValue !== null) {
            for (const coord of coords) {
                if (coord.echo === selectedValue) {
                    const x = xScale(coord.ky) - blockWidth / 2;
                    const y = yScale(coord.kz) - blockHeight / 2;
                    svg += `<rect x="${x}" y="${y}" width="${blockWidth}" height="${blockHeight}" fill="white" shape-rendering="crispEdges"/>`;
                }
            }
        }
        
        // Axes
        svg += `<line x1="0" y1="${height}" x2="${width}" y2="${height}" stroke="black" stroke-width="1"/>`;
        svg += `<line x1="0" y1="0" x2="0" y2="${height}" stroke="black" stroke-width="1"/>`;
        
        // X-axis ticks and labels
        for (const tick of xTicks) {
            const x = xScale(tick);
            svg += `<line x1="${x}" y1="${height}" x2="${x}" y2="${height + 5}" stroke="black" stroke-width="1"/>`;
            svg += `<text x="${x}" y="${height + 18}" text-anchor="middle" font-family="sans-serif" font-size="12" fill="black">${tick.toFixed(0)}</text>`;
        }
        
        // Y-axis ticks and labels
        for (const tick of yTicks) {
            const y = yScale(tick);
            svg += `<line x1="0" y1="${y}" x2="-5" y2="${y}" stroke="black" stroke-width="1"/>`;
            svg += `<text x="-8" y="${y}" text-anchor="end" dominant-baseline="middle" font-family="sans-serif" font-size="12" fill="black">${tick.toFixed(0)}</text>`;
        }
        
        // Axis labels
        svg += `<text x="${width/2}" y="${height + 40}" text-anchor="middle" font-family="sans-serif" font-size="14" fill="black">ky</text>`;
        svg += `<text x="-45" y="${height/2}" text-anchor="middle" transform="rotate(-90, -45, ${height/2})" font-family="sans-serif" font-size="14" fill="black">kz</text>`;
        
        svg += `</g>`;
        svg += `</svg>`;
        
        return svg;
    }
    
    /**
     * Download SVG as file
     */
    function downloadSVG(svgString, filename) {
        const blob = new Blob([svgString], { type: 'image/svg+xml' });
        const url = URL.createObjectURL(blob);
        const link = document.createElement('a');
        link.href = url;
        link.download = filename;
        document.body.appendChild(link);
        link.click();
        document.body.removeChild(link);
        URL.revokeObjectURL(url);
    }
    
    return {
        init,
        plotByShotNumber,
        plotByEchoNumber,
        drawHighlights,
        setSelectedShot,
        setSelectedEcho,
        setShowShotHighlight,
        setShowEchoHighlight,
        exportShotSVG,
        exportEchoSVG
    };
})();
