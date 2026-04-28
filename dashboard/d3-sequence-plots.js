/**
 * d3-sequence-plots.js
 * SVG-based D3 plot utilities for sequence simulation dashboards.
 * Exposes window.D3SeqPlots with createLinePlot, createBarPlot, addMxyControlPoints.
 */
(function () {
    'use strict';

    const DARK = {
        bg:       '#1e1e2f',
        axisText: '#a0a0c0',
        title:    '#e0e0ff',
        grid:     'rgba(255,255,255,0.05)',
    };

    const DEFAULT_MARGIN = { top: 40, right: 20, bottom: 50, left: 60 };
    const LEGEND_RIGHT   = 130; // reserved px for legend column
    let plotClipCounter = 0;

    // ─── Helpers ────────────────────────────────────────────────────────────────

    function buildSVG(el, width, height, margin) {
        // Clear first so el.clientWidth isn't inflated by a previous SVG
        el.innerHTML = '';
        const svg = d3.select(el)
            .append('svg')
            .attr('width',  width)
            .attr('height', height)
            .style('background', DARK.bg)
            .style('display', 'block')
            .style('overflow', 'hidden')  // prevent legend text from expanding the container

        const g = svg.append('g')
            .attr('transform', `translate(${margin.left},${margin.top})`);

        const innerW = width  - margin.left - margin.right;
        const innerH = height - margin.top  - margin.bottom;

        return { svg, g, innerW, innerH };
    }

    function drawAxes(g, xScale, yScale, innerW, innerH, opts = {}) {
        // Gridlines
        g.append('g')
            .attr('class', 'grid-y')
            .call(d3.axisLeft(yScale)
                .tickSize(-innerW)
                .tickFormat(''))
            .selectAll('line')
            .attr('stroke', DARK.grid);
        g.select('.grid-y .domain').remove();

        g.append('g')
            .attr('class', 'grid-x')
            .attr('transform', `translate(0,${innerH})`)
            .call(d3.axisBottom(xScale)
                .tickSize(-innerH)
                .tickFormat(''))
            .selectAll('line')
            .attr('stroke', DARK.grid);
        g.select('.grid-x .domain').remove();

        // X axis
        const xAxisGenerator = d3.axisBottom(xScale)
            .tickFormat(opts.xTickFormat || null);
        if (opts.xTickValues) xAxisGenerator.tickValues(opts.xTickValues);
        else xAxisGenerator.ticks(opts.xTicks || 10);

        const xAxis = g.append('g')
            .attr('class', 'axis-x')
            .attr('transform', `translate(0,${innerH})`)
            .call(xAxisGenerator);
        xAxis.selectAll('text').attr('fill', DARK.axisText);
        xAxis.selectAll('line, path').attr('stroke', DARK.axisText);

        if (opts.xLabel) {
            g.append('text')
                .attr('x', innerW / 2)
                .attr('y', innerH + 42)
                .attr('text-anchor', 'middle')
                .attr('fill', DARK.title)
                .attr('font-size', 12)
                .text(opts.xLabel);
        }

        // Y axis
        const yAxis = g.append('g')
            .attr('class', 'axis-y')
            .call(d3.axisLeft(yScale).ticks(6).tickFormat(opts.yTickFormat || null));
        yAxis.selectAll('text').attr('fill', DARK.axisText);
        yAxis.selectAll('line, path').attr('stroke', DARK.axisText);

        if (opts.yLabel) {
            g.append('text')
                .attr('transform', 'rotate(-90)')
                .attr('x', -innerH / 2)
                .attr('y', -46)
                .attr('text-anchor', 'middle')
                .attr('fill', DARK.title)
                .attr('font-size', 12)
                .text(opts.yLabel);
        }

        // Title
        if (opts.title) {
            g.append('text')
                .attr('x', innerW / 2)
                .attr('y', -12)
                .attr('text-anchor', 'middle')
                .attr('fill', DARK.title)
                .attr('font-size', 13)
                .attr('font-weight', 'bold')
                .text(opts.title);
        }
    }

    // ─── createLinePlot ─────────────────────────────────────────────────────────

    /**
     * @param {HTMLElement} containerEl
     * @param {{
     *   width?, height?, margin?,
     *   xLabel?, yLabel?, title?,
     *   xMin?, xMax?, yMin?, yMax?,
     *   xType?,          // 'linear' (default) | 'band'
     *   xBandLabels?,    // string[] for band scale
     *   datasets,        // [{label, data, color, hidden, dots?}]
     *                    //   data: number[] (for band) or [{x,y}] (for linear)
     *   phaseBoxes?,     // [{xStart, xEnd, color, label}]
     *   showLegend?,     // default true
     * }}
     * @returns {{ update(datasets), svg, xScale, yScale }}
     */
    function createLinePlot(containerEl, opts) {
        // Clear before measuring so stale SVG doesn't inflate clientWidth
        containerEl.innerHTML = '';
        const showLegend = opts.showLegend !== false && opts.datasets && opts.datasets.length > 0;
        const width  = opts.width  || containerEl.clientWidth  || 500;
        const height = opts.height || containerEl.clientHeight || 260;
        const margin = opts.margin || {
            ...DEFAULT_MARGIN,
            right: showLegend ? LEGEND_RIGHT : DEFAULT_MARGIN.right,
        };

        const { svg, g, innerW, innerH } = buildSVG(containerEl, width, height, margin);
        const clipId = `d3-seq-clip-${++plotClipCounter}`;
        svg.append('defs')
            .append('clipPath')
            .attr('clipPathUnits', 'userSpaceOnUse')
            .attr('id', clipId)
            .append('rect')
            .attr('x', 0)
            .attr('y', 0)
            .attr('width', innerW)
            .attr('height', innerH);

        const xType = opts.xType || 'linear';

        // Determine data extents
        let xMin, xMax, yMin, yMax;
        function computeExtents(datasets) {
            let allX = [], allY = [];
            for (const ds of datasets) {
                if (!ds.data || ds.hidden) continue;
                if (xType === 'band') {
                    ds.data.forEach((v, i) => { allX.push(i); allY.push(v); });
                } else {
                    ds.data.forEach(d => { allX.push(d.x); allY.push(d.y); });
                }
            }
            return {
                xMin: allX.length ? d3.min(allX) : 0,
                xMax: allX.length ? d3.max(allX) : 1,
                yMin: allY.length ? d3.min(allY) : 0,
                yMax: allY.length ? d3.max(allY) : 1,
            };
        }

        let xScale, yScale;

        function buildScales(datasets) {
            const ext = computeExtents(datasets);
            if (xType === 'band') {
                const labels = opts.xBandLabels || datasets[0]?.data.map((_, i) => String(i + 1)) || [];
                xScale = d3.scaleBand()
                    .domain(labels)
                    .range([0, innerW])
                    .padding(0.1);
            } else {
                xScale = d3.scaleLinear()
                    .domain([opts.xMin ?? ext.xMin, opts.xMax ?? ext.xMax])
                    .range([0, innerW]);
            }
            const yPad = (opts.yMax !== undefined || opts.yMin !== undefined) ? 0 : 0.05 * Math.abs((ext.yMax - ext.yMin) || 1);
            yScale = d3.scaleLinear()
                .domain([opts.yMin ?? ext.yMin - yPad, opts.yMax ?? ext.yMax + yPad])
                .range([innerH, 0]);
        }

        let _currentDatasets = opts.datasets || [];

        buildScales(_currentDatasets);

        drawAxes(g, xScale, yScale, innerW, innerH, {
            xLabel: opts.xLabel,
            yLabel: opts.yLabel,
            title:  opts.title,
            xTicks: 10,
        });

        // Phase boxes (drawn before lines so lines sit on top)
        if (opts.phaseBoxes && xType === 'linear') {
            for (const box of opts.phaseBoxes) {
                g.append('rect')
                    .attr('x',      xScale(box.xStart))
                    .attr('y',      0)
                    .attr('width',  Math.max(0, xScale(box.xEnd) - xScale(box.xStart)))
                    .attr('height', innerH)
                    .attr('fill',   box.color)
                    .attr('pointer-events', 'none');
            }
        }

        const linesGroup = g.append('g')
            .attr('class', 'lines')
            .attr('clip-path', `url(#${clipId})`);

        function drawLines(datasets) {
            linesGroup.selectAll('*').remove();

            const lineGen = xType === 'linear'
                ? d3.line().x(d => xScale(d.x)).y(d => yScale(d.y)).defined(d => d.y != null)
                : d3.line().x((d, i, arr) => {
                        // For band-scale, we map index → mid of band
                        return xScale(opts.xBandLabels ? opts.xBandLabels[i] : String(i + 1)) + xScale.bandwidth() / 2;
                    }).y(d => yScale(d));

            for (const ds of datasets) {
                if (ds.hidden) continue;
                if (ds.type === 'scatter' || ds.dots) {
                    const data = xType === 'band'
                        ? ds.data.map((v, i) => ({ x: xScale(opts.xBandLabels ? opts.xBandLabels[i] : String(i + 1)) + xScale.bandwidth() / 2, y: yScale(v) }))
                        : ds.data.filter(d => d.y != null).map(d => ({ x: xScale(d.x), y: yScale(d.y) }));
                    linesGroup.selectAll(null)
                        .data(data)
                        .enter().append('circle')
                        .attr('cx', d => d.x)
                        .attr('cy', d => d.y)
                        .attr('r', 3)
                        .attr('fill', ds.color);
                } else {
                    const data = xType === 'band' ? ds.data : ds.data;
                    linesGroup.append('path')
                        .datum(data)
                        .attr('fill', 'none')
                        .attr('stroke', ds.color)
                        .attr('stroke-width', ds.strokeWidth || 2)
                        .attr('stroke-dasharray', ds.dashed ? '6,4' : null)
                        .attr('d', lineGen);
                }
            }
        }

        drawLines(_currentDatasets);

        // Legend
        if (showLegend) {
            const legendG = svg.append('g')
                .attr('transform', `translate(${margin.left + innerW + 6}, ${margin.top})`);
            opts.datasets.forEach((ds, i) => {
                const row = legendG.append('g')
                    .attr('transform', `translate(0,${i * 16})`)
                    .attr('opacity', ds.hidden ? 0.35 : 1)
                    .style('cursor', opts.onLegendClick ? 'pointer' : 'default');
                row.append('line')
                    .attr('x1', 0).attr('y1', 6).attr('x2', 14).attr('y2', 6)
                    .attr('stroke', ds.color).attr('stroke-width', 2)
                    .attr('stroke-dasharray', ds.dashed ? '4,3' : null);
                row.append('text')
                    .attr('x', 18).attr('y', 10)
                    .attr('fill', DARK.axisText)
                    .attr('font-size', 10)
                    .attr('text-decoration', ds.hidden ? 'line-through' : 'none')
                    .text(ds.label);
                if (opts.onLegendClick) {
                    row.on('click', () => opts.onLegendClick(ds.label, !ds.hidden));
                }
            });
        }

        // ─── Hover crosshair + tooltip ──────────────────────────────────────────
        if (xType === 'linear') {
            const hoverG = g.append('g').attr('class', 'hover-group').style('display', 'none');

            // Faint vertical crosshair line
            const crosshair = hoverG.append('line')
                .attr('y1', 0)
                .attr('y2', innerH)
                .attr('stroke', 'rgba(255,255,255,0.22)')
                .attr('stroke-width', 1)
                .attr('stroke-dasharray', '4,3')
                .attr('pointer-events', 'none');

            // Tooltip group
            const tooltipG = hoverG.append('g').attr('class', 'hover-tooltip');
            const tooltipBg = tooltipG.append('rect')
                .attr('rx', 4).attr('ry', 4)
                .attr('fill', 'rgba(20,20,40,0.88)')
                .attr('stroke', 'rgba(160,160,220,0.35)')
                .attr('stroke-width', 1);

            const bisect = d3.bisector(d => d.x).left;

            // Transparent overlay on top captures mouse events
            g.append('rect')
                .attr('class', 'hover-overlay')
                .attr('width', innerW)
                .attr('height', innerH)
                .attr('fill', 'none')
                .attr('pointer-events', 'all')
                .on('mousemove', function (event) {
                    const [mx] = d3.pointer(event);
                    const xVal = xScale.invert(mx);

                    const rows = [];
                    for (const ds of _currentDatasets) {
                        if (ds.hidden || !ds.data || ds.data.length < 1) continue;
                        const data = ds.data.filter(d => d.y != null);
                        if (!data.length) continue;
                        const idx = bisect(data, xVal);
                        let yVal;
                        if (idx === 0) {
                            yVal = data[0].y;
                        } else if (idx >= data.length) {
                            yVal = data[data.length - 1].y;
                        } else {
                            const d0 = data[idx - 1], d1 = data[idx];
                            const t = d1.x === d0.x ? 0 : (xVal - d0.x) / (d1.x - d0.x);
                            yVal = d0.y + t * (d1.y - d0.y);
                        }
                        rows.push({ label: ds.label, color: ds.color, value: yVal });
                    }

                    if (!rows.length) { hoverG.style('display', 'none'); return; }

                    crosshair.attr('x1', mx).attr('x2', mx);

                    tooltipG.selectAll('text.hover-row').remove();
                    const pad = 7, lineH = 15, textSize = 11;

                    tooltipG.append('text').attr('class', 'hover-row')
                        .attr('x', pad)
                        .attr('y', pad + textSize)
                        .attr('fill', DARK.axisText)
                        .attr('font-size', textSize)
                        .text(`x = ${xVal.toFixed(2)}`);

                    rows.forEach((row, i) => {
                        tooltipG.append('text').attr('class', 'hover-row')
                            .attr('x', pad + 8)
                            .attr('y', pad + textSize + (i + 1) * lineH + 2)
                            .attr('fill', row.color)
                            .attr('font-size', textSize)
                            .text(`${row.label}: ${row.value.toFixed(3)}`);
                    });

                    const totalRows = 1 + rows.length;
                    const tooltipH = pad * 2 + totalRows * lineH;
                    const maxChars = Math.max(
                        ('x = ' + xVal.toFixed(2)).length,
                        ...rows.map(r => r.label.length + 8)
                    );
                    const tooltipW = Math.max(90, maxChars * 7);
                    tooltipBg.attr('width', tooltipW).attr('height', tooltipH);

                    let tx = mx + 12;
                    if (tx + tooltipW > innerW) tx = mx - tooltipW - 12;
                    tooltipG.attr('transform', `translate(${tx},8)`);

                    hoverG.style('display', null);
                })
                .on('mouseleave', function () {
                    hoverG.style('display', 'none');
                });
        }

        return {
            svg,
            xScale,
            yScale,
            update(newDatasets) {
                _currentDatasets = newDatasets;
                buildScales(newDatasets);
                drawLines(newDatasets);
            },
        };
    }

    // ─── createBarPlot ──────────────────────────────────────────────────────────

    /**
     * @param {HTMLElement} containerEl
     * @param {{
     *   width?, height?, margin?,
     *   xLabels,         // string[]
     *   values,          // number[]
     *   colors,          // string | string[]
     *   yMin?, yMax?,
     *   yLabel?, xLabel?,
    *   xTickValues?,    // subset of xLabels to show on the axis
     *   draggable?,      // bool
     *   onDragEnd?,      // callback(newValues[])
     * }}
     * @returns {{ update(values, colors), enableDrag(bool) }}
     */
    function createBarPlot(containerEl, opts) {
        // Clear before measuring so stale SVG doesn't inflate clientWidth
        containerEl.innerHTML = '';
        const width  = opts.width  || containerEl.clientWidth  || 500;
        const height = opts.height || containerEl.clientHeight || 260;
        const margin = opts.margin || { ...DEFAULT_MARGIN };

        const { svg, g, innerW, innerH } = buildSVG(containerEl, width, height, margin);

        const xLabels = opts.xLabels || [];
        let values    = [...(opts.values || [])];
        const yMin    = opts.yMin ?? 0;
        const yMax    = opts.yMax ?? 190;

        const xScale = d3.scaleBand()
            .domain(xLabels)
            .range([0, innerW])
            .padding(0.15);

        const yScale = d3.scaleLinear()
            .domain([yMin, yMax])
            .range([innerH, 0]);

        drawAxes(g, xScale, yScale, innerW, innerH, {
            xLabel: opts.xLabel,
            yLabel: opts.yLabel,
            xTicks: Math.min(xLabels.length, 20),
            xTickValues: opts.xTickValues,
            yTickFormat: v => v + '°',
        });

        const barsGroup = g.append('g').attr('class', 'bars');
        const handlesGroup = g.append('g').attr('class', 'handles');

        function getColor(i) {
            if (Array.isArray(opts.colors)) return opts.colors[i] || opts.colors[0];
            return opts.colors || 'rgba(160,100,255,0.80)';
        }

        function drawBars(vals, colorsArg) {
            const bars = barsGroup.selectAll('rect.bar').data(vals);
            bars.enter().append('rect').attr('class', 'bar')
                .merge(bars)
                .attr('x',      (d, i) => xScale(xLabels[i]))
                .attr('y',      d => yScale(d))
                .attr('width',  xScale.bandwidth())
                .attr('height', d => Math.max(0, innerH - yScale(d)))
                .attr('fill',   (d, i) => Array.isArray(colorsArg) ? (colorsArg[i] || colorsArg[0]) : (colorsArg || getColor(i)));
            bars.exit().remove();
        }

        drawBars(values, opts.colors);

        let dragEnabled = !!opts.draggable;

        function setupDrag() {
            handlesGroup.selectAll('*').remove();
            if (!dragEnabled) return;

            const handles = handlesGroup.selectAll('circle.drag-handle').data(values);
            handles.enter().append('circle').attr('class', 'drag-handle')
                .merge(handles)
                .attr('cx',   (d, i) => xScale(xLabels[i]) + xScale.bandwidth() / 2)
                .attr('cy',   d => yScale(d))
                .attr('r',    6)
                .attr('fill', 'white')
                .attr('stroke', '#888')
                .attr('stroke-width', 1.5)
                .attr('cursor', 'ns-resize')
                .call(d3.drag()
                    .on('drag', function (event, d) {
                        const i = values.indexOf(d);
                        if (i < 0) return;
                        const newY = Math.max(0, Math.min(innerH, event.y));
                        const newVal = Math.round(yScale.invert(newY));
                        const clamped = Math.max(yMin, Math.min(yMax, newVal));
                        values[i] = clamped;
                        // Update bar and handle position immediately
                        d3.select(this)
                            .datum(clamped)
                            .attr('cy', yScale(clamped));
                        barsGroup.selectAll('rect.bar')
                            .filter((_, j) => j === i)
                            .attr('y', yScale(clamped))
                            .attr('height', Math.max(0, innerH - yScale(clamped)));
                    })
                    .on('end', function () {
                        if (opts.onDragEnd) opts.onDragEnd([...values]);
                    })
                );
            handles.exit().remove();
        }

        setupDrag();

        return {
            svg,
            xScale,
            yScale,
            update(newValues, newColors) {
                values = [...newValues];
                drawBars(values, newColors !== undefined ? newColors : opts.colors);
                setupDrag();
            },
            enableDrag(bool) {
                dragEnabled = bool;
                setupDrag();
            },
        };
    }

    // ─── addMxyControlPoints ─────────────────────────────────────────────────

    /**
     * Overlay draggable control points on an existing SVG from createLinePlot.
     * @param {SVGSVGElement} svgEl      — the raw <svg> DOM node
     * @param {{
     *   xScale,          // d3 scale (linear, echo index)
     *   yScale,          // d3 scale (Mxy)
     *   margin?,
     *   etl,             // total number of echoes (for snap)
     *   points,          // [{echoIndex, mxy}]  — 3–5 points
     *   onDragEnd,       // callback(newPoints[])
     * }}
     * @returns {{ update(points), remove() }}
     */
    function addMxyControlPoints(svgEl, opts) {
        const margin = opts.margin || { ...DEFAULT_MARGIN };
        const { xScale, yScale } = opts;
        let points = opts.points.map(p => ({ ...p }));

        const svg = d3.select(svgEl);
        const g = svg.append('g')
            .attr('class', 'mxy-ctrl-pts')
            .attr('transform', `translate(${margin.left},${margin.top})`);

        function draw(pts) {
            g.selectAll('circle.ctrl-pt').remove();
            g.selectAll('line.ctrl-line').remove();

            // Connect points with a thin dashed line
            if (pts.length > 1) {
                for (let i = 0; i < pts.length - 1; i++) {
                    g.append('line').attr('class', 'ctrl-line')
                        .attr('x1', xScale(pts[i].echoIndex))
                        .attr('y1', yScale(pts[i].mxy))
                        .attr('x2', xScale(pts[i + 1].echoIndex))
                        .attr('y2', yScale(pts[i + 1].mxy))
                        .attr('stroke', 'rgba(255,220,100,0.5)')
                        .attr('stroke-width', 1.5)
                        .attr('stroke-dasharray', '4,3');
                }
            }

            const circles = g.selectAll('circle.ctrl-pt').data(pts);
            circles.enter().append('circle').attr('class', 'ctrl-pt')
                .merge(circles)
                .attr('cx', d => xScale(d.echoIndex))
                .attr('cy', d => yScale(d.mxy))
                .attr('r', 8)
                .attr('fill', 'rgba(255,220,100,0.85)')
                .attr('stroke', '#fff')
                .attr('stroke-width', 1.5)
                .attr('cursor', 'pointer')
                .call(d3.drag()
                    .on('drag', function (event, d) {
                        const i = pts.indexOf(d);
                        if (i < 0) return;

                        // Snap x to nearest echo index
                        const rawEcho = Math.round(xScale.invert(event.x));
                        const clamped = Math.max(1, Math.min(opts.etl, rawEcho));
                        d.echoIndex = clamped;

                        // Clamp y to [0, 1.1]
                        const rawMxy = yScale.invert(event.y);
                        d.mxy = Math.max(0, Math.min(1.1, rawMxy));

                        draw(pts);
                    })
                    .on('end', function () {
                        if (opts.onDragEnd) opts.onDragEnd(pts.map(p => ({ ...p })));
                    })
                );
            circles.exit().remove();
        }

        draw(points);

        return {
            update(newPts) {
                points = newPts.map(p => ({ ...p }));
                draw(points);
            },
            remove() {
                g.remove();
            },
        };
    }

    // ─── addFAControlPoints ──────────────────────────────────────────────────────

    /**
    * Overlay draggable FA control nodes on a bar-plot SVG.
    * Nodes move freely in x (snaps to RF pulse index) and y (flip angle).
     * Linear interpolation between nodes is shown as a connecting line.
     *
     * @param {SVGSVGElement} svgEl
     * @param {{
     *   xScale,       // d3 band scale (from createBarPlot) — used to position nodes at band centres
     *   yScale,       // d3 linear scale  (flip angle)
     *   margin?,
    *   pulseCount,   // number of editable RF pulses (x snap range 1..pulseCount)
     *   xOffset?,     // number of prefixed bars before pulseIndex 1
    *   yMin?,        // default 0
     *   yMax?,        // default 180
    *   points,       // [{pulseIndex, fa}]
     *   onDragEnd,    // callback(newPoints[])
     * }}
     * @returns {{ update(points), remove() }}
     */
    function addFAControlPoints(svgEl, opts) {
        const margin = opts.margin || { ...DEFAULT_MARGIN };
        const { xScale, yScale } = opts;
        const xOffset = opts.xOffset || 0;
        const yMin = opts.yMin ?? 0;
        const yMax = opts.yMax ?? 180;
        let points = opts.points.map(p => ({ ...p }));

        // Map RF pulse index (1-based) to pixel x using the band scale's band centres.
        function pulseToPx(pulseIndex) {
            const domain = xScale.domain ? xScale.domain() : [];
            const label = domain[xOffset + pulseIndex - 1] ?? String(pulseIndex);
            const x = xScale(label);
            if (x === undefined) {
                return xScale(xOffset + pulseIndex) ?? 0;
            }
            return x + (xScale.bandwidth ? xScale.bandwidth() / 2 : 0);
        }

        const svg = d3.select(svgEl);
        const g = svg.append('g')
            .attr('class', 'fa-ctrl-pts')
            .attr('transform', `translate(${margin.left},${margin.top})`);

        function draw(pts) {
            g.selectAll('*').remove();
            const sorted = [...pts].sort((a, b) => a.pulseIndex - b.pulseIndex);

            // Linear interpolation preview line through all nodes
            if (sorted.length > 1) {
                for (let i = 0; i < sorted.length - 1; i++) {
                    g.append('line').attr('class', 'fa-ctrl-line')
                        .attr('x1', pulseToPx(sorted[i].pulseIndex))
                        .attr('y1', yScale(sorted[i].fa))
                        .attr('x2', pulseToPx(sorted[i + 1].pulseIndex))
                        .attr('y2', yScale(sorted[i + 1].fa))
                        .attr('stroke', 'rgba(255,180,50,0.8)')
                        .attr('stroke-width', 2)
                        .attr('stroke-dasharray', '5,3')
                        .attr('pointer-events', 'none');
                }
            }

            g.selectAll('circle.fa-ctrl-pt').data(pts)
                .enter().append('circle').attr('class', 'fa-ctrl-pt')
                .attr('cx', d => pulseToPx(d.pulseIndex))
                .attr('cy', d => yScale(d.fa))
                .attr('r', 9)
                .attr('fill', 'rgba(255,180,50,0.9)')
                .attr('stroke', '#fff')
                .attr('stroke-width', 1.5)
                .attr('cursor', 'move')
                .call(d3.drag()
                    .on('drag', function (event, d) {
                        const i = pts.indexOf(d);
                        if (i < 0) return;
                        // x: snap to nearest RF pulse index in [1, pulseCount]
                        let bestPulse = d.pulseIndex;
                        let bestDist = Infinity;
                        for (let pulseIndex = 1; pulseIndex <= opts.pulseCount; pulseIndex++) {
                            const px = pulseToPx(pulseIndex);
                            const dist = Math.abs(event.x - px);
                            if (dist < bestDist) { bestDist = dist; bestPulse = pulseIndex; }
                        }
                        d.pulseIndex = bestPulse;
                        // y: clamp to [yMin, yMax]
                        d.fa = Math.max(yMin, Math.min(yMax, yScale.invert(event.y)));
                        draw(pts);
                    })
                    .on('end', function () {
                        pts.sort((a, b) => a.pulseIndex - b.pulseIndex);
                        if (opts.onDragEnd) opts.onDragEnd(pts.map(p => ({ ...p })));
                    })
                );
        }

        draw(points);

        return {
            update(newPts) {
                points = newPts.map(p => ({ ...p }));
                draw(points);
            },
            remove() {
                g.remove();
            },
        };
    }

    // ─── Export ─────────────────────────────────────────────────────────────────

    window.D3SeqPlots = { createLinePlot, createBarPlot, addMxyControlPoints, addFAControlPoints };
})();
