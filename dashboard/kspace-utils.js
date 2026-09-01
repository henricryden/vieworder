/**
 * K-Space Utilities Module
 * Handles coordinate generation and view ordering logic
 */

const KSpaceUtils = (() => {
    /**
     * Generate 3D k-space coordinates with acceleration and coverage options
     * @param {number} Ny - y matrix size
     * @param {number} Nz - z matrix size
     * @param {number} R - acceleration factor (Ry = Rz = R)
     * @param {boolean} useCaipirinha - whether to apply CAIPIRINHA shifts
     * @param {string} coverage - 'rectangular' or 'elliptical'
     * @param {number} calSize - calibration region size (default: 128)
     * @returns {Array} Array of {ky, kz, r, phi} coordinates
     */
    function generateCoordinates(Ny, Nz, Ry = 1, Rz = 1, useCaipirinha = true, coverage = 'rectangular', calSize = 128, kzPF = 1.0) {
        // Pre-allocate array with estimated maximum size
        const maxCoords = Math.ceil((Ny * Nz) / (Ry * Rz)) + (calSize * calSize);
        const coords = new Array(maxCoords);
        let coordIndex = 0;
        
        const center_y = Ny / 2.0;
        const center_z = Nz / 2.0;
        const y_max = Ny / 2.0;
        const z_max = Nz / 2.0;
        const epsilon = 1e-6;
        
        // Calibration region (same in y and z)
        const cal_y = Math.min(calSize, Ny);
        const cal_z = Math.min(calSize, Nz);
        const y_cal_low = Math.floor(center_y - cal_y / 2.0);
        const y_cal_hi = y_cal_low + cal_y;
        const z_cal_low = Math.floor(center_z - cal_z / 2.0);
        const z_cal_hi = z_cal_low + cal_z;
        
        // Partial Fourier: kzPF ranges from 0.5 (keep 50% centered) to 1.0 (full)
        // Mapping (matching C logic): kept_fraction = 0.5 + nover_z / Nz
        // => nover_z = (kzPF - 0.5) * Nz
        let nover_z = Math.round((kzPF - 0.5) * Nz);
        if (nover_z < 0) nover_z = 0;
        if (nover_z > Math.floor(Nz / 2)) nover_z = Math.floor(Nz / 2);
        const pf_zmode = kzPF < 1.0 ? 'LATE' : 'NO';  // Default to removing high kz (LATE)
        
        // CAIPIRINHA shift (if enabled)
        const dy = useCaipirinha ? 1 : 0;
        const step_y = Ny - 1;
        
        // Generate accelerated coordinates
        let offset = 0;
        for (let row = 0; row < Nz; row += Rz) {
            const z = row;
            
            // Apply partial Fourier filtering
            if (pf_zmode === 'LATE' && z >= (Nz / 2 + nover_z)) {
                // Skip high kz lines
                offset += dy;
                offset = offset % (step_y + 1);
                continue;
            }
            
            for (let col = 0; col < Ny; col += Ry) {
                let y = col + offset;
                
                // Apply modulo for CAIPIRINHA wrapping
                if (y >= Ny) {
                    y = y % Ny;
                }
                
                // Normalize to [-1, 1]
                const y1 = Ny > 1 ? (y - Ny / 2 + 0.5) / y_max : 0.0;
                const z1 = Nz > 1 ? (z - Nz / 2 + 0.5) / z_max : 0.0;
                
                // Calculate radial and angular coordinates
                const r = Math.sqrt(y1 * y1 + z1 * z1);
                const phi = Math.atan2(z1, y1);
                
                // Check elliptical coverage
                if (coverage === 'elliptical' && r > 1.0 + epsilon) {
                    continue;
                }
                
                // Skip if in calibration region when accelerated (match C code logic)
                if ((Ry > 1 || Rz > 1) && 
                    (y >= y_cal_low && y < y_cal_hi) &&
                    (z >= z_cal_low && z < z_cal_hi)) {
                    // For elliptical cal coverage, skip points inside the ellipse
                    if (coverage === 'elliptical') {
                        const y_dist = (y + 0.5 - Ny / 2.0);
                        const z_dist = (z + 0.5 - Nz / 2.0);
                        const l_y = y_dist / (cal_y / 2.0);
                        const l_z = z_dist / (cal_z / 2.0);
                        const l_r = l_y * l_y + l_z * l_z;
                        if (l_r < 1.0 + epsilon) continue;  // Skip if inside ellipse
                    } else {
                        // For rectangular cal coverage, skip all points in cal region
                        continue;
                    }
                }
                
                coords[coordIndex++] = {
                    ky: (y - Ny / 2) | 0,
                    kz: (z - Nz / 2) | 0,
                    r: r,
                    phi: phi,
                    y_idx: y | 0,
                    z_idx: z | 0,
                    shot: -1,
                    echo: -1,
                    isCalibration: false
                };
            }
            
            // Update offset for CAIPIRINHA pattern
            if (dy > 0) {
                offset += dy;
                offset = offset % (step_y + 1);
            }
        }
        
        // Add calibration region
        if (Ry > 1 || Rz > 1) {
            for (let y = y_cal_low; y < y_cal_hi; y++) {
                for (let z = z_cal_low; z < z_cal_hi; z++) {
                    if (z < 0 || z >= Nz || y < 0 || y >= Ny) continue;
                    
                    // Normalize to [-1, 1]
                    const y1 = Ny > 1 ? (y - Ny / 2 + 0.5) / y_max : 0.0;
                    const z1 = Nz > 1 ? (z - Nz / 2 + 0.5) / z_max : 0.0;
                    
                    const r = Math.sqrt(y1 * y1 + z1 * z1);
                    const phi = Math.atan2(z1, y1);
                    
                    // Check elliptical coverage for cal region
                    if (coverage === 'elliptical') {
                        const y_dist = (y + 0.5 - Ny / 2.0);
                        const z_dist = (z + 0.5 - Nz / 2.0);
                        const l_y = y_dist / (cal_y / 2.0);
                        const l_z = z_dist / (cal_z / 2.0);
                        const l_r = l_y * l_y + l_z * l_z;
                        if (l_r > 1.0 + epsilon) continue;
                    }
                    
                    coords[coordIndex++] = {
                        ky: (y - Ny / 2) | 0,
                        kz: (z - Nz / 2) | 0,
                        r: r,
                        phi: phi,
                        y_idx: y | 0,
                        z_idx: z | 0,
                        shot: -1,
                        echo: -1,
                        isCalibration: true
                    };
                }
            }
        }
        
        // Trim array to actual size
        coords.length = coordIndex;
        return coords;
    }
    
    /**
     * Assign view ordering to phase encodes
     * @param {Array} coords - Array of coordinate objects
     * @param {number} etl - Echo Train Length
     * @param {string} ordering - Ordering type: 'sequential', 'chevron', 'lcpo', 'cplo', 'croc'
     * @param {number} centerEcho - Center echo position (1 to etl)
     * @param {string} mtfDirection - MTF direction: 'ky' or 'kz'
     * @param {string} shotOrderParam - Shot ordering: 'ky', 'kz', or 'azimuthal' (for CPLO/LCPO)
     * @returns {Array} Coordinates with shot and echo assignments
     */
    function assignViewOrdering(coords, etl, ordering, centerEcho, mtfDirection = 'ky', shotOrderParam = null, macroOptions = null) {
        if (coords.length === 0) return coords;
        
        // shotOrder is orthogonal to mtfDirection (or user-specified for LCPO/CPLO)
        const shotOrder = shotOrderParam || (mtfDirection === 'ky' ? 'kz' : 'ky');
        
        switch(ordering) {
            case 'sequential':
                sequentialOrdering(coords, etl, centerEcho, shotOrder);
                break;
            case 'chevron':
                chevronOrdering(coords, etl, centerEcho, shotOrder);
                break;
            case 'lcpo':
                lcpoOrdering(coords, etl, centerEcho, shotOrder);
                break;
            case 'cplo':
                cploOrdering(coords, etl, centerEcho, shotOrder);
                break;
            case 'croc':
                crocOrdering(coords, etl, centerEcho, shotOrder);
                break;
            default:
                sequentialOrdering(coords, etl, centerEcho, shotOrder);
        }

        preserveBaseEcho(coords);
        const macroWidth = parseMacroWidth(macroOptions);
        if (macroWidth === -1) {
            const search = assignBestMacroWidthByRms(coords, etl, centerEcho, ordering, shotOrder, 1, 5);
            coords.jumpMetrics = search.bestRmsJumpMetrics;
            coords.macroSearch = search;
        } else {
            const maxMacroWidth = Math.max(1, macroWidth);
            applyMacroCoarseFineAssignment(coords, etl, centerEcho, maxMacroWidth, ordering, shotOrder);
            coords.jumpMetrics = calculateJumpMetrics(coords);
            coords.macroSearch = null;
        }
        
        return coords;
    }

    function parseMacroWidth(macroOptions) {
        if (!macroOptions || macroOptions.macroWidth === undefined || macroOptions.macroWidth === null) return 2;
        const value = Math.floor(Number(macroOptions.macroWidth));
        if (!Number.isFinite(value)) return 2;
        return value;
    }

    function findBestMacroWidthByRms(generateCoords, etl, ordering, centerEcho, mtfDirection = 'ky', shotOrderParam = null, minWidth = 1, maxWidth = 5) {
        const start = Math.max(1, Math.floor(Number(minWidth) || 1));
        const stop = Math.max(start, Math.floor(Number(maxWidth) || start));
        let bestWidth = start;
        let bestRmsJump = Infinity;
        let worstWidth = start;
        let worstRmsJump = -Infinity;
        const evaluatedWidths = [];
        const rmsByWidth = [];

        for (let width = start; width <= stop; width++) {
            const candidate = generateCoords();
            assignViewOrdering(candidate, etl, ordering, centerEcho, mtfDirection, shotOrderParam, { macroWidth: width });
            const rmsJump = candidate.jumpMetrics ? candidate.jumpMetrics.rmsJump : calculateJumpMetrics(candidate).rmsJump;
            evaluatedWidths.push(width);
            rmsByWidth.push({ width, rmsJump });
            if (rmsJump < bestRmsJump) {
                bestWidth = width;
                bestRmsJump = rmsJump;
            }
            if (rmsJump > worstRmsJump) {
                worstWidth = width;
                worstRmsJump = rmsJump;
            }
        }

        return {
            bestWidth,
            bestRmsJump,
            worstWidth,
            worstRmsJump,
            searchMin: start,
            searchMax: stop,
            evaluatedWidths,
            rmsByWidth
        };
    }
    
    /**
     * Helper sort functions to reduce code duplication
     */
    
    // Sort indices by coordinate radius (low to high)
    function sortIndicesByRadius(indices, coords) {
        return indices.sort((a, b) => {
            return coords[a].r - coords[b].r;
        });
    }
    
    // Sort indices by azimuthal angle (phi)
    function sortIndicesByPhi(indices, coords) {
        return indices.sort((a, b) => {
            return coords[a].phi - coords[b].phi;
        });
    }
    
    // Sort indices by a specific coordinate axis (ky or kz), respecting sign
    function sortIndicesByAxis(indices, coords, axis, abs) {
        return indices.sort((a, b) => {
            if (abs) {
                return Math.abs(coords[a][axis]) - Math.abs(coords[b][axis]);
            }
            return coords[a][axis] - coords[b][axis];
        });
    }
    
    // Sort indices by echo, then by secondary sort function
    function sortIndicesByEchoThen(indices, coords, secondarySort) {
        return indices.sort((a, b) => {
            const echoA = coords[a].echo;
            const echoB = coords[b].echo;
            if (echoA !== echoB) return echoA - echoB;
            return secondarySort(a, b);
        });
    }
    
    // Secondary sort by theta (angle) then radius
    function sortByThetaThenRadius(a, b) {
        // This is the T_R order used in C code
        const coords_a = arguments[2]; // We'll pass coords as third arg when used
        const coords_b = arguments[3];
        // Note: This needs coords passed, will be used differently
        return 0; // Placeholder
    }
    
    // Assign shots where each shot gets one coordinate from each echo
    function assignShotsPerEcho(coords, indices, numShots) {
        // First pass: find maxEcho without spread operator
        let maxEcho = 0;
        for (let i = 0; i < indices.length; i++) {
            const echo = coords[indices[i]].echo;
            if (echo > maxEcho) maxEcho = echo;
        }
        if (maxEcho === 0) return; // Safety check

        // Use plain arrays instead of plain objects for faster indexed access
        const echoGroups = new Array(maxEcho + 1).fill(null);
        for (let i = 0; i < indices.length; i++) {
            const idx = indices[i];
            const echo = coords[idx].echo;
            if (!echoGroups[echo]) echoGroups[echo] = [];
            echoGroups[echo].push(idx);
        }

        const echoPointers = new Int32Array(maxEcho + 1); // zero-initialised
        // Cycle through shots and echoes repeatedly until all coords are assigned.
        // This handles uneven echo-group sizes and ensures no coordinate remains unassigned.
        let remaining = 0;
        for (let e = 1; e <= maxEcho; e++) {
            if (echoGroups[e]) remaining += echoGroups[e].length;
        }

        let pos = 0;
        while (remaining > 0) {
            for (let echo = 1; echo <= maxEcho && remaining > 0; echo++) {
                if (echoGroups[echo] && echoPointers[echo] < echoGroups[echo].length) {
                    const idx = echoGroups[echo][echoPointers[echo]];
                    coords[idx].shot = pos;
                    coords[idx].adjustedEcho = coords[idx].echo;
                    echoPointers[echo]++;
                    remaining--;
                }
            }
            pos = (pos + 1) % Math.max(1, numShots);
        }
    }

    function preserveBaseEcho(coords) {
        for (let i = 0; i < coords.length; i++) {
            coords[i].baseEcho = coords[i].echo;
            coords[i].baseShot = coords[i].shot;
            coords[i].baseRadarAngle = getRadarAngle(coords[i]);
            coords[i].adjustedEcho = coords[i].echo;
        }
    }

    function applyMacroCoarseFineAssignment(coords, etl, centerEcho, macroWidth, ordering, shotOrder) {
        const numShots = getNumShots(coords);
        if (macroWidth <= 1) return;
        const macroSegments = buildMacroSegments(etl, macroWidth);

        if (ordering === 'chevron') {
            assignChevronCoarseMacroEchoes(coords, centerEcho, macroSegments, numShots);
        } else if (ordering === 'croc') {
            assignCrocCoarseMacroEchoes(coords, centerEcho, macroSegments, numShots);
        } else {
            assignFallbackCoarseMacroEchoes(coords, macroSegments, numShots, ordering, shotOrder);
        }

        for (let i = 0; i < coords.length; i++) {
            coords[i].macroOrderKey = getMacroOrderKey(coords[i], ordering, shotOrder);
        }

        for (let i = 0; i < macroSegments.length; i++) {
            assignMacroSegmentPaths(coords, macroSegments[i], numShots);
        }
    }

    function assignBestMacroWidthByRms(coords, etl, centerEcho, ordering, shotOrder, minWidth, maxWidth) {
        const start = Math.max(1, Math.floor(Number(minWidth) || 1));
        const stop = Math.max(start, Math.floor(Number(maxWidth) || start));
        let bestCandidate = null;
        let bestWidth = start;
        let bestRmsJump = Infinity;
        let bestRmsJumpMetrics = null;
        let worstWidth = start;
        let worstRmsJump = -Infinity;
        const evaluatedWidths = [];
        const rmsByWidth = [];

        for (let width = start; width <= stop; width++) {
            const candidate = coords.map((coord) => ({ ...coord }));
            applyMacroCoarseFineAssignment(candidate, etl, centerEcho, width, ordering, shotOrder);
            const metrics = calculateJumpMetrics(candidate);
            evaluatedWidths.push(width);
            rmsByWidth.push({ width, rmsJump: metrics.rmsJump });
            if (metrics.rmsJump < bestRmsJump) {
                bestCandidate = candidate;
                bestWidth = width;
                bestRmsJump = metrics.rmsJump;
                bestRmsJumpMetrics = metrics;
            }
            if (metrics.rmsJump > worstRmsJump) {
                worstWidth = width;
                worstRmsJump = metrics.rmsJump;
            }
        }

        if (bestCandidate) {
            for (let i = 0; i < coords.length; i++) {
                Object.assign(coords[i], bestCandidate[i]);
            }
        }

        return {
            bestWidth,
            bestRmsJump,
            bestRmsJumpMetrics,
            worstWidth,
            worstRmsJump,
            searchMin: start,
            searchMax: stop,
            evaluatedWidths,
            rmsByWidth
        };
    }

    function buildMacroSegments(etl, maxMacroWidth) {
        const maxWidth = Math.max(1, Math.floor(Number(maxMacroWidth) || 1));
        const segments = [];
        let startEcho = 1;
        if (maxWidth <= 1) {
            while (startEcho <= etl) {
                segments.push({
                    macroEcho: startEcho,
                    startEcho,
                    endEcho: startEcho,
                    width: 1,
                    maxMacroWidth: 1
                });
                startEcho++;
            }
            return segments;
        }

        while (startEcho <= etl) {
            const progress = (startEcho - 1) / Math.max(1, etl - 1);
            let width = Math.round(2 + progress * (maxWidth - 2));
            width = Math.max(2, Math.min(maxWidth, width));
            const endEcho = Math.min(etl, startEcho + width - 1);
            segments.push({
                macroEcho: segments.length + 1,
                startEcho,
                endEcho,
                width: endEcho - startEcho + 1,
                maxMacroWidth: maxWidth
            });
            startEcho = endEcho + 1;
        }
        return segments;
    }

    function macroEchoFromRank(rank, macroSegments, numShots) {
        let remaining = Math.max(0, rank);
        for (let i = 0; i < macroSegments.length; i++) {
            const capacity = numShots * macroSegments[i].width;
            if (remaining < capacity) return macroSegments[i].macroEcho;
            remaining -= capacity;
        }
        return macroSegments.length > 0 ? macroSegments[macroSegments.length - 1].macroEcho : 1;
    }

    function assignMacroEchoesBySortedIndices(coords, indices, macroSegments, numShots) {
        for (let rank = 0; rank < indices.length; rank++) {
            const coord = coords[indices[rank]];
            const segment = macroSegments[macroEchoFromRank(rank, macroSegments, numShots) - 1];
            coord.macroEcho = segment.macroEcho;
            coord.macroStartEcho = segment.startEcho;
            coord.macroEndEcho = segment.endEcho;
            coord.macroWidth = segment.width;
            coord.macroMaxWidth = segment.maxMacroWidth;
        }
    }

    function coarseCenterEcho(centerEcho, macroSegments) {
        for (let i = 0; i < macroSegments.length; i++) {
            const segment = macroSegments[i];
            if (centerEcho >= segment.startEcho && centerEcho <= segment.endEcho) return segment.macroEcho;
        }
        return macroSegments.length > 0 ? macroSegments[macroSegments.length - 1].macroEcho : 1;
    }

    function nearestOriginIndex(coords) {
        let nearestIdx = 0;
        let minOriginSq = coords[0].ky * coords[0].ky + coords[0].kz * coords[0].kz;
        for (let i = 1; i < coords.length; i++) {
            const sq = coords[i].ky * coords[i].ky + coords[i].kz * coords[i].kz;
            if (sq < minOriginSq) {
                minOriginSq = sq;
                nearestIdx = i;
            }
        }
        return nearestIdx;
    }

    function assignChevronCoarseMacroEchoes(coords, centerEcho, macroSegments, numShots) {
        const numCoords = coords.length;
        const indices = Array.from({ length: numCoords }, (_, i) => i);
        let maxKy = 0;
        for (let i = 0; i < numCoords; i++) {
            const a = Math.abs(coords[i].ky);
            if (a > maxKy) maxKy = a;
        }
        const y0 = maxKy;
        const z0 = 0;
        for (let i = 0; i < numCoords; i++) {
            let theta = Math.atan2(coords[i].ky - y0, coords[i].kz - z0);
            if (theta < 0) theta += 2 * Math.PI;
            coords[i].theta = theta;
        }

        const nearestIdx = nearestOriginIndex(coords);
        const kyDistFromY0 = new Float64Array(numCoords);
        const absKzArr = new Float64Array(numCoords);
        for (let i = 0; i < numCoords; i++) {
            kyDistFromY0[i] = Math.abs(coords[i].ky - y0);
            absKzArr[i] = Math.abs(coords[i].kz);
        }

        function setRadius(axisRatio) {
            for (let i = 0; i < numCoords; i++) {
                coords[i].ellipDist = kyDistFromY0[i] + absKzArr[i] * axisRatio;
            }
        }

        function macroEchoOfNearest() {
            const nearestDist = coords[nearestIdx].ellipDist;
            let rank = 0;
            for (let i = 0; i < numCoords; i++) {
                if (coords[i].ellipDist < nearestDist) rank++;
            }
            return macroEchoFromRank(rank, macroSegments, numShots);
        }

        const targetMacroCenter = coarseCenterEcho(centerEcho, macroSegments);
        const macroCount = macroSegments.length;
        let leftRatio = 0.005;
        let rightRatio = 100;
        let axisRatio = Math.max(0.005, 2 * Math.PI * targetMacroCenter / Math.max(1, macroCount));
        let foundCenter = false;

        setRadius(axisRatio);
        let currentMacroCenter = macroEchoOfNearest();
        if (currentMacroCenter === targetMacroCenter) {
            foundCenter = true;
        } else if (currentMacroCenter > targetMacroCenter) {
            leftRatio = axisRatio;
        } else {
            rightRatio = axisRatio;
        }

        for (let iter = 0; iter < 60 && !foundCenter; iter++) {
            axisRatio = (leftRatio + rightRatio) / 2;
            setRadius(axisRatio);
            currentMacroCenter = macroEchoOfNearest();
            if (currentMacroCenter === targetMacroCenter) {
                foundCenter = true;
            } else if (currentMacroCenter > targetMacroCenter) {
                leftRatio = axisRatio;
            } else {
                rightRatio = axisRatio;
            }
        }

        setRadius(axisRatio);
        indices.sort((a, b) => coords[a].ellipDist - coords[b].ellipDist);
        assignMacroEchoesBySortedIndices(coords, indices, macroSegments, numShots);
    }

    function assignCrocCoarseMacroEchoes(coords, centerEcho, macroSegments, numShots) {
        const numCoords = coords.length;
        const indices = Array.from({ length: numCoords }, (_, i) => i);
        const NORTH = 0, EAST = 1, SOUTH = 2, WEST = 3;

        const numCoordsPerSector = [0, 0, 0, 0];
        for (let i = 0; i < numCoords; i++) {
            coords[i].theta = Math.atan2(coords[i].ky + 0.5, coords[i].kz + 0.5);
            const ky = coords[i].ky + 0.5;
            const kz = coords[i].kz + 0.5;
            const absKy = Math.abs(ky);
            const absKz = Math.abs(kz);
            if (absKy >= absKz) {
                numCoordsPerSector[ky > 0 ? NORTH : SOUTH]++;
            } else {
                numCoordsPerSector[kz > 0 ? EAST : WEST]++;
            }
        }

        let largestSector = 0;
        for (let i = 1; i < 4; i++) {
            if (numCoordsPerSector[i] >= numCoordsPerSector[largestSector]) largestSector = i;
        }

        let maxKy = 0;
        let maxKz = 0;
        for (let i = 0; i < numCoords; i++) {
            const ak = Math.abs(coords[i].ky);
            const az = Math.abs(coords[i].kz);
            if (ak > maxKy) maxKy = ak;
            if (az > maxKz) maxKz = az;
        }
        maxKy = maxKy || 1;
        maxKz = maxKz || 1;

        let startKy = 0;
        let startKz = 0;
        let endKy = 0;
        let endKz = 0;
        if (largestSector === NORTH) endKy = maxKy;
        else if (largestSector === EAST) endKz = maxKz;
        else if (largestSector === SOUTH) endKy = -maxKy;
        else if (largestSector === WEST) endKz = -maxKz;

        const nearestIdx = nearestOriginIndex(coords);
        const kyN = new Float64Array(numCoords);
        const kzN = new Float64Array(numCoords);
        for (let i = 0; i < numCoords; i++) {
            kyN[i] = coords[i].ky / maxKy;
            kzN[i] = coords[i].kz / maxKz;
        }
        const nearestKyN = kyN[nearestIdx];
        const nearestKzN = kzN[nearestIdx];

        function macroEchoOfNearest(offsetKyN, offsetKzN, kyRatio, kzRatio) {
            const dky = nearestKyN - offsetKyN;
            const dkz = nearestKzN - offsetKzN;
            const nearestSq = dky * dky * kyRatio * kyRatio + dkz * dkz * kzRatio * kzRatio;
            let rank = 0;
            for (let i = 0; i < numCoords; i++) {
                const dk = kyN[i] - offsetKyN;
                const dz = kzN[i] - offsetKzN;
                if (dk * dk * kyRatio * kyRatio + dz * dz * kzRatio * kzRatio < nearestSq) rank++;
            }
            return macroEchoFromRank(rank, macroSegments, numShots);
        }

        function setDistances(offsetKy, offsetKz, kyRatio, kzRatio) {
            const offsetKyN = offsetKy / maxKy;
            const offsetKzN = offsetKz / maxKz;
            for (let i = 0; i < numCoords; i++) {
                const dk = kyN[i] - offsetKyN;
                const dz = kzN[i] - offsetKzN;
                coords[i].ellipDist = dk * dk * kyRatio * kyRatio + dz * dz * kzRatio * kzRatio;
            }
        }

        const targetMacroCenter = coarseCenterEcho(centerEcho, macroSegments);
        let centerKy = 0;
        let centerKz = 0;
        let kyRatio = 1.0;
        let kzRatio = 1.0;
        let foundCenter = false;
        let currentMacroCenter = -1;

        for (let iter = 0; iter < 30; iter++) {
            centerKy = (startKy + endKy) / 2;
            centerKz = (startKz + endKz) / 2;
            currentMacroCenter = macroEchoOfNearest(centerKy / maxKy, centerKz / maxKz, kyRatio, kzRatio);
            if (currentMacroCenter === targetMacroCenter) {
                foundCenter = true;
                break;
            } else if (currentMacroCenter > targetMacroCenter) {
                endKy = centerKy;
                endKz = centerKz;
            } else {
                startKy = centerKy;
                startKz = centerKz;
            }
        }

        if (!foundCenter) {
            let ratioStep = currentMacroCenter < targetMacroCenter ? 0.8 : 1.2;
            const offsetKyN = centerKy / maxKy;
            const offsetKzN = centerKz / maxKz;
            for (let iter = 0; iter < 40 && !foundCenter; iter++) {
                if (largestSector === SOUTH || largestSector === NORTH) {
                    kzRatio *= ratioStep;
                } else {
                    kyRatio *= ratioStep;
                }
                if (kyRatio < 0.05 || kyRatio > 20.0 || kzRatio < 0.05 || kzRatio > 20.0) break;
                currentMacroCenter = macroEchoOfNearest(offsetKyN, offsetKzN, kyRatio, kzRatio);
                if (currentMacroCenter === targetMacroCenter) {
                    foundCenter = true;
                } else if ((currentMacroCenter < targetMacroCenter) !== (ratioStep < 1)) {
                    ratioStep = 1 + (1 - ratioStep) * 0.5;
                }
            }
        }

        setDistances(centerKy, centerKz, kyRatio, kzRatio);
        indices.sort((a, b) => coords[a].ellipDist - coords[b].ellipDist);
        assignMacroEchoesBySortedIndices(coords, indices, macroSegments, numShots);

        for (let i = 0; i < numCoords; i++) {
            let atan_x = 0;
            let atan_y = 0;
            const y = coords[i].ky - centerKy;
            const z = coords[i].kz - centerKz;
            if (largestSector === WEST) {
                atan_x = -z;
                atan_y = -y;
            } else if (largestSector === SOUTH) {
                atan_x = -y;
                atan_y = z;
            } else if (largestSector === EAST) {
                atan_x = z;
                atan_y = y;
            } else {
                atan_x = y;
                atan_y = -z;
            }
            let angle = Math.atan2(atan_y, atan_x);
            if (angle < 0) angle += 2 * Math.PI;
            coords[i].crocAngle = angle;
        }
    }

    function assignFallbackCoarseMacroEchoes(coords, macroSegments, numShots, ordering, shotOrder) {
        const indices = Array.from({ length: coords.length }, (_, i) => i);
        for (let i = 0; i < coords.length; i++) {
            coords[i].macroOrderKey = getMacroOrderKey(coords[i], ordering, shotOrder);
        }
        indices.sort((a, b) => {
            if (coords[a].macroOrderKey !== coords[b].macroOrderKey) return coords[a].macroOrderKey - coords[b].macroOrderKey;
            if (coords[a].baseEcho !== coords[b].baseEcho) return coords[a].baseEcho - coords[b].baseEcho;
            return (coords[a].baseShot || 0) - (coords[b].baseShot || 0);
        });
        assignMacroEchoesBySortedIndices(coords, indices, macroSegments, numShots);
    }

    function assignMacroSegmentPaths(coords, macroSegment, numShots) {
        const startEcho = macroSegment.startEcho;
        const endEcho = macroSegment.endEcho;
        const segment = [];
        for (let i = 0; i < coords.length; i++) {
            if (coords[i].macroEcho === macroSegment.macroEcho) segment.push(coords[i]);
        }
        segment.sort((a, b) => {
            if (a.macroOrderKey !== b.macroOrderKey) return a.macroOrderKey - b.macroOrderKey;
            if (a.baseEcho !== b.baseEcho) return a.baseEcho - b.baseEcho;
            return (a.baseShot || 0) - (b.baseShot || 0);
        });

        const lanes = new Array(numShots);
        let pos = 0;
        for (let lane = 0; lane < numShots; lane++) {
            lanes[lane] = [];
            for (let echo = startEcho; echo <= endEcho && pos < segment.length; echo++, pos++) {
                lanes[lane].push(segment[pos]);
            }
        }

        for (let lane = 0; lane < numShots; lane++) {
            const shot = lane;
            const laneCoords = lanes[lane];
            for (let i = 0; i < laneCoords.length; i++) {
                const coord = laneCoords[i];
                coord.shot = shot;
                coord.echo = startEcho + i;
                coord.adjustedEcho = coord.echo;
            }
        }
    }

    function getMacroOrderKey(coord, ordering, shotOrder) {
        if (ordering === 'chevron' && Number.isFinite(coord.theta)) return normalizeAngle(coord.theta);
        if (ordering === 'croc' && Number.isFinite(coord.crocAngle)) return normalizeAngle(coord.crocAngle);
        if ((shotOrder === 'ky' || shotOrder === 'kz') && Number.isFinite(coord[shotOrder])) return coord[shotOrder];
        if (Number.isFinite(coord.crocAngle)) return normalizeAngle(coord.crocAngle);
        if (Number.isFinite(coord.theta)) return normalizeAngle(coord.theta);
        if (Number.isFinite(coord.phi)) return normalizeAngle(coord.phi);
        return 0;
    }

    function getNumShots(coords) {
        let maxShot = -1;
        for (let i = 0; i < coords.length; i++) {
            if (coords[i].shot > maxShot) maxShot = coords[i].shot;
        }
        return Math.max(1, maxShot + 1);
    }

    function getRadarAngle(coord) {
        const angle = Number.isFinite(coord.crocAngle) ? coord.crocAngle :
            Number.isFinite(coord.theta) ? coord.theta :
            Number.isFinite(coord.phi) ? coord.phi : 0;
        return normalizeAngle(angle);
    }

    function normalizeAngle(angle) {
        const twoPi = 2 * Math.PI;
        let a = angle % twoPi;
        if (a < 0) a += twoPi;
        return a;
    }

    function calculateJumpMetrics(coords) {
        const byShot = new Map();
        for (let i = 0; i < coords.length; i++) {
            const coord = coords[i];
            if (!byShot.has(coord.shot)) byShot.set(coord.shot, []);
            byShot.get(coord.shot).push(coord);
        }

        let totalSquaredJump = 0;
        let transitionCount = 0;

        for (const shotCoords of byShot.values()) {
            shotCoords.sort((a, b) => {
                if (a.echo !== b.echo) return a.echo - b.echo;
                return (a.baseEcho || 0) - (b.baseEcho || 0);
            });

            for (let i = 1; i < shotCoords.length; i++) {
                const prev = shotCoords[i - 1];
                const cur = shotCoords[i];
                const dy = cur.ky - prev.ky;
                const dz = cur.kz - prev.kz;
                const d2 = dy * dy + dz * dz;
                totalSquaredJump += d2;
                transitionCount++;
            }
        }

        return {
            transitionCount,
            rmsJump: transitionCount > 0 ? Math.sqrt(totalSquaredJump / transitionCount) : 0
        };
    }
    
    /**
     * Sequential ordering (line-by-line)
     * Matches C code ks_generate_peplan_from_kcoords with LINEAR_SWEEP:
     * 1) Sort by mtfDirection (considering sign)
     * 2) Assign echo 1-ETL sequentially along MTF direction
     * 3) Reorganize into (readout, shot) loops - each readout gets all shots
     * 4) Assign shots and echo based on cycle position
     */
    function sequentialOrdering(coords, etl, centerEcho, shotOrder) {
        const numCoords = coords.length;
        if (numCoords === 0) return;
        
        // Determine mtfDirection (opposite of shotOrder)
        const mtfDirection = shotOrder === 'ky' ? 'kz' : 'ky';
        
        // Calculate shots and encodes per shot
        const numShots = Math.ceil(numCoords / etl);
        const encodesPerShot = Math.ceil(numCoords / numShots);
        
        // Step 1: Sort by mtfDirection (orthogonal to shotOrder)
        const indices = Array.from({length: numCoords}, (_, i) => i);
        sortIndicesByAxis(indices, coords, mtfDirection, false);
            
        // Step 2: Assign echo values sequentially - each echo gets numShots coords
        for (let i = 0; i < numCoords; i++) {
            const idx = indices[i];
            const echoNum = ((i / numShots) | 0) + 1;
            coords[idx].echo = echoNum;
        }
        sortIndicesByEchoThen(indices, coords, (a, b) => coords[a][shotOrder] - coords[b][shotOrder]);
        
        // Step 4: Assign shots - each shot gets one coord from each echo
        assignShotsPerEcho(coords, indices, numShots);
    }
    
    /**
     * Chevron ordering (alternating up-down from center)
     */
    function chevronOrdering(coords, etl, centerEcho, shotOrder) {
        const numCoords = coords.length;
        if (numCoords === 0) return;
        
        // Calculate shots and encodes per shot
        const numShots = Math.ceil(numCoords / etl);
        const encodesPerShot = Math.ceil(numCoords / numShots);
        
        const indices = Array.from({length: numCoords}, (_, i) => i);
        
        // Ellipse origin parameters - at the edge of k-space in ky direction
        let maxKy = 0;
        for (let i = 0; i < numCoords; i++) {
            const a = Math.abs(coords[i].ky);
            if (a > maxKy) maxKy = a;
        }
        const y0 = maxKy;  // Edge of k-space in ky direction
        const z0 = 0;      // Center in kz direction

        // Pre-compute theta once — depends only on static ky/kz, not on axisRatio
        for (let i = 0; i < numCoords; i++) {
            let theta = Math.atan2(coords[i].ky - y0, coords[i].kz - z0);
            if (theta < 0) theta += 2 * Math.PI;
            coords[i].theta = theta;
        }

        // Pre-compute nearest-to-origin index — invariant across binary search
        let nearestIdx = 0;
        let minOriginSq = coords[0].ky * coords[0].ky + coords[0].kz * coords[0].kz;
        for (let i = 1; i < numCoords; i++) {
            const sq = coords[i].ky * coords[i].ky + coords[i].kz * coords[i].kz;
            if (sq < minOriginSq) { minOriginSq = sq; nearestIdx = i; }
        }

        // Pre-compute per-coord constants for setChevronRadius (avoids repeated Math.abs + property access)
        const kyDistFromY0 = new Float64Array(numCoords);
        const absKzArr     = new Float64Array(numCoords);
        for (let i = 0; i < numCoords; i++) {
            kyDistFromY0[i] = Math.abs(coords[i].ky - y0);
            absKzArr[i]     = Math.abs(coords[i].kz); // z0 = 0
        }

        // Only update ellipDist per iteration (theta is precomputed above)
        function setChevronRadius(axisRatio) {
            for (let i = 0; i < numCoords; i++) {
                coords[i].ellipDist = kyDistFromY0[i] + absKzArr[i] * axisRatio;
            }
        }

        // O(n) rank scan — no sort needed during binary search.
        // Returns the echo that nearestIdx would be assigned at the current ellipDist values.
        function echoOfNearestNoSort() {
            const nearestDist = coords[nearestIdx].ellipDist;
            let rank = 0;
            for (let i = 0; i < numCoords; i++) {
                if (coords[i].ellipDist < nearestDist) rank++;
            }
            return (rank / numShots | 0) + 1;
        }

        // Binary search for axis ratio — O(n) per iteration instead of O(n log n)
        let leftRatio = 0.005;
        let rightRatio = 100;
        let axisRatio = 1.0;
        let curCenterEcho = -1;
        let foundCenter = false;
        const maxIter = 60; // cheap to run more iterations now

        console.log(`Chevron: Desired center echo ${centerEcho}`);

        // Better first candidate: axisRatio ≈ 2π * centerEcho / etl
        axisRatio = 2 * Math.PI * centerEcho / etl;
        setChevronRadius(axisRatio);
        curCenterEcho = echoOfNearestNoSort();
        console.log(`Chevron first candidate: axisRatio=${axisRatio.toFixed(3)}, centerEcho=${curCenterEcho}, want=${centerEcho}`);
        if (curCenterEcho === centerEcho) {
            foundCenter = true;
        } else if (curCenterEcho > centerEcho) {
            leftRatio = axisRatio;
        } else {
            rightRatio = axisRatio;
        }

        for (let iter = 0; iter < maxIter && !foundCenter; iter++) {
            axisRatio = (leftRatio + rightRatio) / 2;
            setChevronRadius(axisRatio);
            curCenterEcho = echoOfNearestNoSort();

            console.log(`Chevron iter ${iter}: axisRatio=${axisRatio.toFixed(3)}, centerEcho=${curCenterEcho}, want=${centerEcho}`);

            if (curCenterEcho === centerEcho) {
                foundCenter = true;
            } else if (curCenterEcho > centerEcho) {
                leftRatio = axisRatio;
            } else {
                rightRatio = axisRatio;
            }
        }

        // If we couldn't find an exact center, try reversed center
        if (!foundCenter) {
            const reversedCenterEcho = etl + 1 - centerEcho;
            console.log(`Chevron: trying reversed center echo ${reversedCenterEcho}`);
            let leftR = 0.005;
            let rightR = 100;
            let axisR = axisRatio;
            let reversedFound = false;
            for (let iter = 0; iter < maxIter && !reversedFound; iter++) {
                axisR = (leftR + rightR) / 2;
                setChevronRadius(axisR);
                const curRevCenter = echoOfNearestNoSort();
                console.log(`Chevron rev iter ${iter}: axisRatio=${axisR.toFixed(3)}, centerEcho=${curRevCenter}, want=${reversedCenterEcho}`);
                if (curRevCenter === reversedCenterEcho) {
                    reversedFound = true;
                    axisRatio = axisR;
                } else if (curRevCenter > reversedCenterEcho) {
                    leftR = axisR;
                } else {
                    rightR = axisR;
                }
            }

            if (reversedFound) {
                // Do the single final sort + echo assignment, then reverse
                setChevronRadius(axisRatio);
                indices.sort((a, b) => coords[a].ellipDist - coords[b].ellipDist);
                for (let i = 0; i < numCoords; i++) {
                    coords[indices[i]].echo = ((i / numShots) | 0) + 1;
                }
                for (let i = 0; i < numCoords; i++) {
                    coords[i].echo = etl + 1 - coords[i].echo;
                }
                console.log(`Chevron: reversed echo assignments for center ${reversedCenterEcho}`);
                foundCenter = true;
            }
        }

        // Final sort + echo assignment (skipped above only when reversedFound handled it)
        if (!foundCenter || coords[nearestIdx].echo === -1) {
            setChevronRadius(axisRatio);
            indices.sort((a, b) => coords[a].ellipDist - coords[b].ellipDist);
            for (let i = 0; i < numCoords; i++) {
                coords[indices[i]].echo = ((i / numShots) | 0) + 1;
            }
        }

        console.log(`Chevron final: axisRatio=${axisRatio.toFixed(3)}, centerEcho=${coords[nearestIdx].echo}`);

        // Step 3: Sort by echo first, then theta (azimuthal angle) for radar sweep
        sortIndicesByEchoThen(indices, coords, (a, b) => coords[a].theta - coords[b].theta);
        
        // Step 4: Assign shots - each shot gets one coord from each echo
        assignShotsPerEcho(coords, indices, numShots);
    }
    
    /**
     * Low-to-High Peripheral Ordering (LCPO)
     * Linear center, pivot outer
     */
    function lcpoOrdering(coords, etl, centerEcho, shotOrder) {
        const numCoords = coords.length;
        if (numCoords === 0) return;
        
        // Determine orthogonal direction to shotOrder
        const mtfDirection = shotOrder === 'ky' ? 'kz' : 'ky';
        
        // Calculate shots and encodes per shot
        const numShots = Math.ceil(numCoords / etl);
        const encodesPerShot = Math.ceil(numCoords / numShots);
        
        // Calculate number of linear center segments
        let numLinearCenter;
        if (centerEcho <= encodesPerShot / 2) {
            numLinearCenter = 2 * centerEcho - (encodesPerShot % 2);
        } else {
            numLinearCenter = 2 * (encodesPerShot - centerEcho) + (encodesPerShot % 2);
        }
        numLinearCenter = Math.max(1, Math.min(numLinearCenter, encodesPerShot));
        
        const indices = Array.from({length: numCoords}, (_, i) => i);
        
        // Initialize echo as -1 (unassigned) for all coords
        for (let i = 0; i < numCoords; i++) {
            coords[i].echo = -1;
        }
        
        // Step 1: Sort by abs(mtfDirection) to get center-most coords first
        sortIndicesByAxis(indices, coords, mtfDirection, true);
        
        // Take only the center-most numShots * numLinearCenter coords
        const coordsToProcess = Math.min(numCoords, numShots * numLinearCenter);
        const centerIndices = indices.slice(0, coordsToProcess);
        
        // Step 1a: Sort these center coords linearly along mtfDirection (signed)
        centerIndices.sort((a, b) => coords[a][mtfDirection] - coords[b][mtfDirection]);
        
        // Step 1b: Assign echoes [1, numLinearCenter] linearly
        for (let i = 0; i < centerIndices.length; i++) {
            const idx = centerIndices[i];
            const echoNum = ((i / numShots) | 0) + 1;
            coords[idx].echo = echoNum;
        }
        
        // Step 2: Sort ALL coords by abs(mtfDirection)
        sortIndicesByAxis(indices, coords, mtfDirection, true);
        
        // Step 3: Assign remaining echoes using integer division
        // Collect unassigned peripheral coords and assign echoes numLinearCenter+1 through encodesPerShot
        let peripheralPos = 0;
        for (let i = 0; i < numCoords; i++) {
            const idx = indices[i];
            if (coords[idx].echo === -1) {
                const echoNum = numLinearCenter + ((peripheralPos / numShots) | 0) + 1;
                coords[idx].echo = echoNum;
                peripheralPos++;
            }
        }
        
        // Step 4: Sort by echo, then shotOrder, and assign shots
        if (shotOrder === 'ky' || shotOrder === 'kz') {
            sortIndicesByEchoThen(indices, coords, (a, b) => coords[a][shotOrder] - coords[b][shotOrder]);
        } else {
            // Default fallback to ky if invalid
            sortIndicesByEchoThen(indices, coords, (a, b) => coords[a].ky - coords[b].ky);
        }
        assignShotsPerEcho(coords, indices, numShots);
    }
    
    /**
     * Pivot Center, Linear Outer (PCLO/CPLO)
     * Uses permutation formula to achieve arbitrary center echo
     */
    function cploOrdering(coords, etl, centerEcho, shotOrder) {
        const numCoords = coords.length;
        if (numCoords === 0) return;
        
        // Calculate shots and encodes per shot
        const numShots = Math.ceil(numCoords / etl);
        const encodesPerShot = Math.ceil(numCoords / numShots);
        
        // Create indices array
        const indices = Array.from({length: numCoords}, (_, i) => i);
        
        // Step 1: Sort by radius (center to periphery) using helper
        sortIndicesByRadius(indices, coords);
        
        // Step 2: Build PCLO permutation table
        // N_cp = c + (c mod 2) - number of central rings
        let numCentral = centerEcho + (centerEcho % 2);
        if (centerEcho > encodesPerShot / 2) {
            numCentral = encodesPerShot - (encodesPerShot - centerEcho) - ((encodesPerShot - centerEcho) % 2);
        }
        numCentral = Math.max(1, Math.min(numCentral, encodesPerShot));
        
        // Build permutation table based on PCLO formula
        const permutation = new Array(encodesPerShot + 1);
        for (let i = 1; i <= encodesPerShot; i++) {
            if (i <= numCentral / 2) {
                permutation[i] = centerEcho - 2 * i + 2;
            } else if (i <= numCentral) {
                permutation[i] = 2 * i - centerEcho - 1;
            } else {
                permutation[i] = i;
            }
        }
        
        // Step 3: Apply permutation - assign echo based on position in radius-sorted order
        // Each echo gets numShots coordinates
        for (let i = 0; i < numCoords; i++) {
            const idx = indices[i];
            const echoPosition = ((i / numShots) | 0) + 1;  // Which echo slot (1-indexed)
            coords[idx].echo = Math.max(1, Math.min(permutation[echoPosition], encodesPerShot));
        }
        
        // Step 4: Sort by echo, then by shotOrder (azimuthal/ky/kz)
        if (shotOrder === 'azimuthal') {
            // Sort by phi (azimuthal), then radius for T_R spoke pattern
            sortIndicesByEchoThen(indices, coords, (a, b) => {
                const phiA = coords[a].phi;
                const phiB = coords[b].phi;
                if (Math.abs(phiA - phiB) > 0.01) return phiA - phiB;
                return coords[a].r - coords[b].r;
            });
        } else if (shotOrder === 'ky' || shotOrder === 'kz') {
            // Sort by specified axis
            sortIndicesByEchoThen(indices, coords, (a, b) => coords[a][shotOrder] - coords[b][shotOrder]);
        } else {
            // Default to azimuthal if invalid
            sortIndicesByEchoThen(indices, coords, (a, b) => {
                const phiA = coords[a].phi;
                const phiB = coords[b].phi;
                if (Math.abs(phiA - phiB) > 0.01) return phiA - phiB;
                return coords[a].r - coords[b].r;
            });
        }
        
        // Step 5: Assign shots - each shot gets one coord from each echo
        assignShotsPerEcho(coords, indices, numShots);
    }
    
    /**
     * Concentric Rings with Offset Center (CROC)
     * Uses elliptical distances from an offset center to achieve arbitrary center echo
     */
    function crocOrdering(coords, etl, centerEcho, shotOrder) {
        const numCoords = coords.length;
        if (numCoords === 0) return;
        
        // Calculate shots and encodes per shot
        const numShots = Math.ceil(numCoords / etl);
        const encodesPerShot = Math.ceil(numCoords / numShots);
        
        const indices = Array.from({length: numCoords}, (_, i) => i);
        
        // Determine mtfDirection
        const mtfDirection = shotOrder === 'ky' ? 'kz' : 'ky';
        
        // Step 1: Find the largest sector (quadrant with most coordinates)
        // Calculate theta for each coord: atan2(ky, kz) like in C code
        for (let i = 0; i < numCoords; i++) {
            coords[i].theta = Math.atan2(coords[i].ky + 0.5, coords[i].kz + 0.5);
        }
        
        // Count coordinates directly in each of 4 quadrants without sorting
        // to avoid wrap-around issues
        const numCoordsPerSector = [0, 0, 0, 0];
        const NORTH = 0, EAST = 1, SOUTH = 2, WEST = 3;
        
        for (let i = 0; i < numCoords; i++) {
            const ky = coords[i].ky + 0.5;
            const kz = coords[i].kz + 0.5;
            
            // Determine quadrant based on signs
            // NORTH: ky > 0 (regardless of kz)
            // SOUTH: ky < 0 (regardless of kz)
            // EAST: kz > 0 (regardless of ky)
            // WEST: kz < 0 (regardless of ky)
            // For priority: if both signs differ, prioritize by which is larger in magnitude
            
            const absKy = Math.abs(ky);
            const absKz = Math.abs(kz);
            
            if (absKy >= absKz) {
                // ky-dominated
                numCoordsPerSector[ky > 0 ? NORTH : SOUTH]++;
            } else {
                // kz-dominated
                numCoordsPerSector[kz > 0 ? EAST : WEST]++;
            }
        }
        
        console.log(`CROC sectors [NORTH, EAST, SOUTH, WEST]: [${numCoordsPerSector.join(', ')}]`);
        
        // Find largest sector
        let largestSector = 0;
        for (let i = 1; i < 4; i++) {
            if (numCoordsPerSector[i] >= numCoordsPerSector[largestSector]) {
                largestSector = i;
            }
        }
        
        console.log(`CROC largest sector: ${['NORTH','EAST','SOUTH','WEST'][largestSector]} with ${numCoordsPerSector[largestSector]} coords`);
        
        // Step 2: Binary search for offset center position along sector midline
        // Start at origin (k-space center), end at edge in the direction of largest sector
        let startKy = 0, startKz = 0;
        let endKy = 0, endKz = 0;
        
        // Set end point at edge based on sector direction
        let maxKy = 0;
        let maxKz = 0;
        for (let i = 0; i < numCoords; i++) {
            const ak = Math.abs(coords[i].ky);
            const az = Math.abs(coords[i].kz);
            if (ak > maxKy) maxKy = ak;
            if (az > maxKz) maxKz = az;
        }
        
        if (largestSector === NORTH) endKy = maxKy;       // NORTH: positive ky
        else if (largestSector === EAST) endKz = maxKz;   // EAST: positive kz
        else if (largestSector === SOUTH) endKy = -maxKy; // SOUTH: negative ky
        else if (largestSector === WEST) endKz = -maxKz;  // WEST: negative kz
        
        // Normalization constants
        const normKy = maxKy || 1;
        const normKz = maxKz || 1;

        // Pre-compute nearest-to-origin index — invariant across all search calls
        let nearestCrocIdx = 0;
        let minCrocSq = coords[0].ky * coords[0].ky + coords[0].kz * coords[0].kz;
        for (let i = 1; i < numCoords; i++) {
            const sq = coords[i].ky * coords[i].ky + coords[i].kz * coords[i].kz;
            if (sq < minCrocSq) { minCrocSq = sq; nearestCrocIdx = i; }
        }

        // Pre-compute normalised coords as typed arrays — avoids repeated division + property lookup
        const kyN = new Float64Array(numCoords);
        const kzN = new Float64Array(numCoords);
        for (let i = 0; i < numCoords; i++) {
            kyN[i] = coords[i].ky / normKy;
            kzN[i] = coords[i].kz / normKz;
        }
        // Pre-compute nearest-to-origin normalised values for rank scan
        const nearestKyN = kyN[nearestCrocIdx];
        const nearestKzN = kzN[nearestCrocIdx];

        // O(n) rank scan — no sort needed during search.
        // Uses squared distance (monotone w.r.t. sqrt) to avoid Math.sqrt.
        function rankOfNearest(offsetKyN, offsetKzN, kyRatio, kzRatio) {
            const dky = nearestKyN - offsetKyN;
            const dkz = nearestKzN - offsetKzN;
            const nearestSq = dky * dky * kyRatio * kyRatio + dkz * dkz * kzRatio * kzRatio;
            let rank = 0;
            for (let i = 0; i < numCoords; i++) {
                const dk = kyN[i] - offsetKyN;
                const dz = kzN[i] - offsetKzN;
                if (dk * dk * kyRatio * kyRatio + dz * dz * kzRatio * kzRatio < nearestSq) rank++;
            }
            return (rank / numShots | 0) + 1;
        }

        // Called once at the end: writes ellipDist, sorts, assigns echo
        function finalize(offsetKy, offsetKz, kyRatio, kzRatio) {
            const offsetKyN = offsetKy / normKy;
            const offsetKzN = offsetKz / normKz;
            for (let i = 0; i < numCoords; i++) {
                const dk = kyN[i] - offsetKyN;
                const dz = kzN[i] - offsetKzN;
                coords[i].ellipDist = dk * dk * kyRatio * kyRatio + dz * dz * kzRatio * kzRatio;
            }
            indices.sort((a, b) => coords[a].ellipDist - coords[b].ellipDist);
            for (let i = 0; i < numCoords; i++) {
                coords[indices[i]].echo = ((i / numShots) | 0) + 1;
            }
        }

        // Helper: normalised offset from world coords
        const toN = (ky, kz) => [ky / normKy, kz / normKz];

        // Binary search along the line from origin to edge — O(n) per iteration
        let centerKy = 0, centerKz = 0;
        let kyRatio = 1.0, kzRatio = 1.0;
        let foundCenter = false;
        let curCenterEcho = -1;
        const maxIter = 30;

        for (let iter = 0; iter < maxIter; iter++) {
            centerKy = (startKy + endKy) / 2;
            centerKz = (startKz + endKz) / 2;
            const [oKyN, oKzN] = toN(centerKy, centerKz);

            curCenterEcho = rankOfNearest(oKyN, oKzN, kyRatio, kzRatio);

            console.log(`CROC iter ${iter}: offset=[${centerKy.toFixed(1)}, ${centerKz.toFixed(1)}], found echo=${curCenterEcho}, want=${centerEcho}`);

            if (curCenterEcho === centerEcho) {
                foundCenter = true;
                break;
            } else if (curCenterEcho > centerEcho) {
                endKy = centerKy; endKz = centerKz;
            } else {
                startKy = centerKy; startKz = centerKz;
            }
        }

        // If binary search failed, adjust ellipse eccentricity — O(n) per step
        if (!foundCenter) {
            console.log(`CROC: Binary search failed, trying ellipse adjustment. Final echo was ${curCenterEcho}`);
            let ratioStep = curCenterEcho < centerEcho ? 0.8 : 1.2;
            const [oKyN, oKzN] = toN(centerKy, centerKz);

            for (let iter = 0; iter < 40 && !foundCenter; iter++) {
                if (largestSector === SOUTH || largestSector === NORTH) {
                    kzRatio *= ratioStep;
                } else {
                    kyRatio *= ratioStep;
                }
                if (kyRatio < 0.05 || kyRatio > 20.0 || kzRatio < 0.05 || kzRatio > 20.0) break;

                curCenterEcho = rankOfNearest(oKyN, oKzN, kyRatio, kzRatio);
                console.log(`CROC ellipse iter ${iter}: kyRatio=${kyRatio.toFixed(2)}, kzRatio=${kzRatio.toFixed(2)}, found echo=${curCenterEcho}`);

                if (curCenterEcho === centerEcho) {
                    foundCenter = true;
                } else if ((curCenterEcho < centerEcho) !== (ratioStep < 1)) {
                    // Overshot — flip step direction and halve it
                    ratioStep = 1 + (1 - ratioStep) * 0.5;
                }
            }
        }

        // If still not found, try reversed center echo — O(n) per iteration
        if (!foundCenter) {
            const reversedCenterEcho = etl - 1 - centerEcho;
            console.log(`CROC: Ellipse adjustment failed, trying reversed center echo ${reversedCenterEcho}`);
            kyRatio = 1.0; kzRatio = 1.0;
            let reversedFound = false;
            startKy = 0; startKz = 0; endKy = 0; endKz = 0;
            if (largestSector === NORTH) endKy = maxKy;
            else if (largestSector === EAST) endKz = maxKz;
            else if (largestSector === SOUTH) endKy = -maxKy;
            else endKz = -maxKz;

            for (let iter = 0; iter < maxIter; iter++) {
                centerKy = (startKy + endKy) / 2;
                centerKz = (startKz + endKz) / 2;
                const [oKyN, oKzN] = toN(centerKy, centerKz);
                curCenterEcho = rankOfNearest(oKyN, oKzN, kyRatio, kzRatio);
                console.log(`CROC reversed iter ${iter}: offset=[${centerKy.toFixed(1)}, ${centerKz.toFixed(1)}], found echo=${curCenterEcho}, want=${reversedCenterEcho}`);
                if (curCenterEcho === reversedCenterEcho) {
                    reversedFound = true;
                    break;
                } else if (curCenterEcho > reversedCenterEcho) {
                    endKy = centerKy; endKz = centerKz;
                } else {
                    startKy = centerKy; startKz = centerKz;
                }
            }

            if (reversedFound) {
                finalize(centerKy, centerKz, kyRatio, kzRatio);
                for (let i = 0; i < numCoords; i++) {
                    coords[i].echo = etl + 1 - coords[i].echo;
                }
                console.log(`CROC: After reversal, center echo is ${coords[nearestCrocIdx].echo}`);
                foundCenter = true;
            } else {
                console.log(`CROC: Could not find a reversed solution`);
            }
        }

        console.log(`CROC final: center=[${centerKy.toFixed(1)}, ${centerKz.toFixed(1)}], ratios=[${kyRatio.toFixed(2)}, ${kzRatio.toFixed(2)}], wanted=${centerEcho}`);

        // Single final sort + echo assignment (skipped when reversal already did it)
        if (!foundCenter || coords[nearestCrocIdx].echo === -1) {
            finalize(centerKy, centerKz, kyRatio, kzRatio);
        }
        
        // Step 3: Calculate angle from offset center for shot ordering
        for (let i = 0; i < numCoords; i++) {
            let atan_x = 0, atan_y = 0;
            const y = coords[i].ky - centerKy;
            const z = coords[i].kz - centerKz;
            
            if (largestSector === 3) {        // WEST
                atan_x = -z;
                atan_y = -y;
            } else if (largestSector === 2) {  // SOUTH
                atan_x = -y;
                atan_y = z;
            } else if (largestSector === 1) {  // EAST
                atan_x = z;
                atan_y = y;
            } else {                           // NORTH (0)
                atan_x = y;
                atan_y = -z;
            }
            
            let angle = Math.atan2(atan_y, atan_x);
            if (angle < 0) angle += 2 * Math.PI;
            coords[i].crocAngle = angle;
        }
        
        // Step 4: Sort by echo, then angle for radar sweep
        sortIndicesByEchoThen(indices, coords, (a, b) => coords[a].crocAngle - coords[b].crocAngle);
        
        // Step 5: Special handling for first echo to avoid singularity
        const firstEchoIndices = [];
        for (let i = 0; i < numCoords; i++) {
            const idx = indices[i];
            if (coords[idx].echo === 1) {
                firstEchoIndices.push(idx);
            }
        }
        
        // Sort first echo by perpendicular direction from offset center
        firstEchoIndices.sort((a, b) => {
            let sortVal_a = 0, sortVal_b = 0;
            const y_a = coords[a].ky - centerKy;
            const z_a = coords[a].kz - centerKz;
            const y_b = coords[b].ky - centerKy;
            const z_b = coords[b].kz - centerKz;
            
            if (largestSector === 3) {        // WEST
                sortVal_a = y_a;
                sortVal_b = y_b;
            } else if (largestSector === 1) { // EAST
                sortVal_a = -y_a;
                sortVal_b = -y_b;
            } else if (largestSector === 2) { // SOUTH
                sortVal_a = -z_a;
                sortVal_b = -z_b;
            } else {                          // NORTH
                sortVal_a = z_a;
                sortVal_b = z_b;
            }
            return sortVal_a - sortVal_b;
        });
        
        // Rebuild indices array with sorted first echo
        const newIndices = [...firstEchoIndices];
        for (let i = 0; i < numCoords; i++) {
            const idx = indices[i];
            if (coords[idx].echo !== 1) {
                newIndices.push(idx);
            }
        }
        
        // Step 5: Assign shots
        assignShotsPerEcho(coords, newIndices, numShots);
    }
    
    /**
     * Calculate acceleration factor (simplified)
     */
    function calculateAcceleration(totalCoords, etl, numShots) {
        return (totalCoords / (etl * numShots)).toFixed(2);
    }
    
    return {
        generateCoordinates,
        assignViewOrdering,
        findBestMacroWidthByRms,
        calculateJumpMetrics,
        calculateAcceleration
    };
})();
