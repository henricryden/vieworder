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
     * @returns {Array} Coordinates with shot and echo assignments
     */
    function assignViewOrdering(coords, etl, ordering, centerEcho, mtfDirection = 'ky') {
        if (coords.length === 0) return coords;
        
        // shotOrder is orthogonal to mtfDirection
        const shotOrder = mtfDirection === 'ky' ? 'kz' : 'ky';
        
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
        
        return coords;
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
        const echoGroups = {};
        for (let i = 0; i < indices.length; i++) {
            const idx = indices[i];
            const echo = coords[idx].echo;
            if (!echoGroups[echo]) {
                echoGroups[echo] = [];
            }
            echoGroups[echo].push(idx);
        }
        
        const echoKeys = Object.keys(echoGroups).map(Number);
        if (echoKeys.length === 0) return; // Safety check
        
        const maxEcho = Math.max(...echoKeys);
        const echoPointers = {};
        for (let e = 1; e <= maxEcho; e++) {
            echoPointers[e] = 0;
        }
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
        // This is what allows axis_ratio to shift which coordinate appears central
        const maxKy = Math.max(...coords.map(c => Math.abs(c.ky)));
        const y0 = maxKy;  // Edge of k-space in ky direction
        const z0 = 0;     // Center in kz direction
        
        // Helper function: calculate chevron ellipse radius
        function setChevronRadius(axisRatio) {
            // Calculate theta for each coordinate relative to ellipse origin (y0, z0)
            // Normalize to [0, 2π] for continuous sweep without wrapping
            for (let i = 0; i < numCoords; i++) {
                const ky_dist = coords[i].ky - y0;
                const kz_dist = coords[i].kz - z0;
                // Match C implementation: theta = atan2(y_dist, z_dist)
                let theta = Math.atan2(ky_dist, kz_dist);
                // Convert from [-π, π] to [0, 2π]
                if (theta < 0) theta += 2 * Math.PI;
                coords[i].theta = theta;
            }
            
            // Calculate chevron radius using Manhattan-like metric to match C:
            // user1 = |y - y0| + |z - z0| * axisRatio
            for (let i = 0; i < numCoords; i++) {
                const ky_dist = coords[i].ky - y0;
                const kz_dist = coords[i].kz - z0;
                coords[i].ellipDist = Math.abs(ky_dist) + Math.abs(kz_dist) * axisRatio;
            }
        }
        
        // Helper function: find center echo at origin
        function findCenterEcho() {
            let minDist = Infinity;
            let centerEchoFound = -1;
            for (let i = 0; i < numCoords; i++) {
                const dist = Math.sqrt(coords[i].ky * coords[i].ky + coords[i].kz * coords[i].kz);
                if (dist < minDist) {
                    minDist = dist;
                    centerEchoFound = coords[i].echo;
                }
            }
            return centerEchoFound;
        }
        
        // Binary search for axis ratio
        let leftRatio = 0.005;
        let rightRatio = 100;  // Reduced from 500 to search more reasonable range
        let axisRatio = 1.0;
        let curCenterEcho = -1;
        let foundCenter = false;
        const maxIter = 20;
        
        console.log(`Chevron: Desired center echo ${centerEcho}`);
        
        for (let iter = 0; iter < maxIter && !foundCenter; iter++) {
            axisRatio = (leftRatio + rightRatio) / 2;
            
            // Set ellipse radius
            setChevronRadius(axisRatio);
            
            // Sort by ellipse radius
            indices.sort((a, b) => coords[a].ellipDist - coords[b].ellipDist);
            
            // Assign echoes - use numShots (like C implementation) for proper grouping
            for (let i = 0; i < numCoords; i++) {
                const idx = indices[i];
                coords[idx].echo = ((i / numShots) | 0) + 1;
            }
            
            // Find center echo
            curCenterEcho = findCenterEcho();
            
            console.log(`Chevron iter ${iter}: axisRatio=${axisRatio.toFixed(3)}, centerEcho=${curCenterEcho}, want=${centerEcho}`);
            
            if (curCenterEcho === centerEcho) {
                foundCenter = true;
            } else if (curCenterEcho > centerEcho) {
                // Center echo too high - follow C implementation: move left bound up
                leftRatio = axisRatio;
            } else {
                // Center echo too low - move right bound down
                rightRatio = axisRatio;
            }
        }
        
        // Fine-tuning: if binary search found exact match, increment slightly
        if (foundCenter && axisRatio < 10) {
            let axisRatioFineTune = axisRatio;
            for (let fineIter = 0; fineIter < 100; fineIter++) {
                axisRatioFineTune *= 1.01;  // Incrementally increase by 1%
                
                setChevronRadius(axisRatioFineTune);
                
                // Sort by ellipse radius
                indices.sort((a, b) => coords[a].ellipDist - coords[b].ellipDist);
                
                // Assign echoes - use numShots
                for (let i = 0; i < numCoords; i++) {
                    const idx = indices[i];
                    coords[idx].echo = ((i / numShots) | 0) + 1;
                }
                
                curCenterEcho = findCenterEcho();
                
                if (curCenterEcho === centerEcho) {
                    axisRatio = axisRatioFineTune;
                } else {
                    break;
                }
            }
        }
        
        // Final assignment with best axis ratio
        setChevronRadius(axisRatio);
        indices.sort((a, b) => coords[a].ellipDist - coords[b].ellipDist);
        
        for (let i = 0; i < numCoords; i++) {
            const idx = indices[i];
            coords[idx].echo = ((i / numShots) | 0) + 1;
        }
        
        curCenterEcho = findCenterEcho();
        console.log(`Chevron final: axisRatio=${axisRatio.toFixed(3)}, centerEcho=${curCenterEcho}`);
        // If we couldn't find an exact center (foundCenter == false), try reversed center
        if (!foundCenter) {
            const reversedCenterEcho = etl + 1 - centerEcho; // 1-based reversed center
            console.log(`Chevron: trying reversed center echo ${reversedCenterEcho}`);
            // Binary search for reversed target
            let leftR = 0.005;
            let rightR = 100;
            let axisR = axisRatio;
            let reversedFound = false;
            let curRevCenter = -1;
            for (let iter = 0; iter < maxIter && !reversedFound; iter++) {
                axisR = (leftR + rightR) / 2;
                setChevronRadius(axisR);
                indices.sort((a, b) => coords[a].ellipDist - coords[b].ellipDist);
                for (let i = 0; i < numCoords; i++) {
                    const idx = indices[i];
                    coords[idx].echo = ((i / numShots) | 0) + 1;
                }
                curRevCenter = findCenterEcho();
                console.log(`Chevron rev iter ${iter}: axisRatio=${axisR.toFixed(3)}, centerEcho=${curRevCenter}, want=${reversedCenterEcho}`);
                if (curRevCenter === reversedCenterEcho) {
                    reversedFound = true;
                } else if (curRevCenter > reversedCenterEcho) {
                    leftR = axisR;
                } else {
                    rightR = axisR;
                }
            }

            if (reversedFound) {
                // We found a reversed solution: reverse all echo assignments
                console.log(`Chevron: reversing echo assignments for reversed center ${reversedCenterEcho}`);
                for (let i = 0; i < numCoords; i++) {
                    coords[i].echo = etl + 1 - coords[i].echo;
                }
                foundCenter = true; // behave as if we found a solution
            }
        }

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
        sortIndicesByEchoThen(indices, coords, (a, b) => coords[a][shotOrder] - coords[b][shotOrder]);
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
        
        // Step 4: Sort by echo, then phi, then radius for T_R spoke pattern
        sortIndicesByEchoThen(indices, coords, (a, b) => {
            const phiA = coords[a].phi;
            const phiB = coords[b].phi;
            if (Math.abs(phiA - phiB) > 0.01) return phiA - phiB;
            return coords[a].r - coords[b].r;
        });
        
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
        const maxKy = Math.max(...coords.map(c => Math.abs(c.ky)));
        const maxKz = Math.max(...coords.map(c => Math.abs(c.kz)));
        
        if (largestSector === NORTH) endKy = maxKy;       // NORTH: positive ky
        else if (largestSector === EAST) endKz = maxKz;   // EAST: positive kz
        else if (largestSector === SOUTH) endKy = -maxKy; // SOUTH: negative ky
        else if (largestSector === WEST) endKz = -maxKz;  // WEST: negative kz
        
        // Normalization constants
        const normKy = maxKy || 1;
        const normKz = maxKz || 1;
        
        // Helper function to test a center position with given aspect ratio
        function tryCenter(offsetKy, offsetKz, kyRatio, kzRatio) {
            // Calculate elliptical distance from offset center (normalized)
            for (let i = 0; i < numCoords; i++) {
                const ky_dist = Math.abs(coords[i].ky - offsetKy) / normKy;
                const kz_dist = Math.abs(coords[i].kz - offsetKz) / normKz;
                coords[i].ellipDist = Math.sqrt(ky_dist * ky_dist * kyRatio * kyRatio + kz_dist * kz_dist * kzRatio * kzRatio);
            }
            
            // Sort by elliptical distance
            indices.sort((a, b) => coords[a].ellipDist - coords[b].ellipDist);
            
            // Assign echoes based on sorted position
            for (let i = 0; i < numCoords; i++) {
                const idx = indices[i];
                coords[idx].echo = ((i / numShots) | 0) + 1;
            }
            
            // Find which echo contains k-space center (ky=0, kz=0)
            let minDistToOrigin = Infinity;
            let centerEchoFound = -1;
            for (let i = 0; i < numCoords; i++) {
                const dist = Math.sqrt(coords[i].ky * coords[i].ky + coords[i].kz * coords[i].kz);
                if (dist < minDistToOrigin) {
                    minDistToOrigin = dist;
                    centerEchoFound = coords[i].echo;
                }
            }
            
            return centerEchoFound;
        }
        
        // Binary search along the line from origin to edge
        let centerKy = 0, centerKz = 0;
        let kyRatio = 1.0, kzRatio = 1.0;
        let foundCenter = false;
        let curCenterEcho = -1;
        const maxIter = 10;
        
        for (let iter = 0; iter < maxIter; iter++) {
            centerKy = (startKy + endKy) / 2;
            centerKz = (startKz + endKz) / 2;
            
            curCenterEcho = tryCenter(centerKy, centerKz, kyRatio, kzRatio);
            
            console.log(`CROC iter ${iter}: offset=[${centerKy.toFixed(1)}, ${centerKz.toFixed(1)}], found echo=${curCenterEcho}, want=${centerEcho}`);
            
            if (curCenterEcho === centerEcho) {
                foundCenter = true;
                break;
            } else if (curCenterEcho > centerEcho) {
                // Current center echo is too high - move closer to origin
                endKy = centerKy;
                endKz = centerKz;
            } else {
                // Current center echo is too low - move away from origin (toward edge)
                startKy = centerKy;
                startKz = centerKz;
            }
        }
        
        // If binary search failed, adjust ellipse eccentricity
        if (!foundCenter) {
            console.log(`CROC: Binary search failed, trying ellipse adjustment. Final echo was ${curCenterEcho}`);
            
            // Determine which axis to adjust based on sector
            // For SOUTH/NORTH: adjust kz axis (perpendicular to sector direction)
            // For EAST/WEST: adjust ky axis (perpendicular to sector direction)
            // If found echo is too LOW: compress perpendicular axis (ratio < 1.0)
            // If found echo is too HIGH: stretch perpendicular axis (ratio > 1.0)
            let ratioStep = curCenterEcho < centerEcho ? 0.8 : 1.2;
            
            // Try adjusting ratio up to 20 steps
            for (let iter = 0; iter < 20 && !foundCenter; iter++) {
                if (largestSector === SOUTH || largestSector === NORTH) {
                    // SOUTH or NORTH - adjust kz axis (perpendicular)
                    kzRatio *= ratioStep;
                } else {
                    // EAST or WEST - adjust ky axis (perpendicular)
                    kyRatio *= ratioStep;
                }
                
                if (kyRatio < 0.1 || kyRatio > 10.0 || kzRatio < 0.1 || kzRatio > 10.0) break;
                
                curCenterEcho = tryCenter(centerKy, centerKz, kyRatio, kzRatio);
                
                console.log(`CROC ellipse iter ${iter}: kyRatio=${kyRatio.toFixed(2)}, kzRatio=${kzRatio.toFixed(2)}, found echo=${curCenterEcho}`);
                
                if (curCenterEcho === centerEcho) {
                    foundCenter = true;
                    break;
                }
            }
        }
        
        // If still not found, try reversed center echo
        if (!foundCenter) {
            console.log(`CROC: Ellipse adjustment failed, trying reversed center echo ${etl - 1 - centerEcho}`);
            
            const reversedCenterEcho = etl - 1 - centerEcho;
            kyRatio = 1.0;
            kzRatio = 1.0;
            let reversedFound = false;
            
            // Binary search again with reversed center
            startKy = 0;
            startKz = 0;
            endKy = 0;
            endKz = 0;
            
            if (largestSector === NORTH) endKy = maxKy;
            else if (largestSector === EAST) endKz = maxKz;
            else if (largestSector === SOUTH) endKy = -maxKy;
            else if (largestSector === WEST) endKz = -maxKz;
            
            for (let iter = 0; iter < maxIter; iter++) {
                centerKy = (startKy + endKy) / 2;
                centerKz = (startKz + endKz) / 2;
                
                curCenterEcho = tryCenter(centerKy, centerKz, kyRatio, kzRatio);
                
                console.log(`CROC reversed iter ${iter}: offset=[${centerKy.toFixed(1)}, ${centerKz.toFixed(1)}], found echo=${curCenterEcho}, want=${reversedCenterEcho}`);
                
                if (curCenterEcho === reversedCenterEcho) {
                    reversedFound = true;
                    console.log(`CROC: Found solution for reversed center echo ${reversedCenterEcho}`);
                    break;
                } else if (curCenterEcho > reversedCenterEcho) {
                    endKy = centerKy;
                    endKz = centerKz;
                } else {
                    startKy = centerKy;
                    startKz = centerKz;
                }
            }
            
            if (reversedFound) {
                // Reverse all echo assignments: echo -> (etl + 1 - echo) for 1-indexed
                console.log(`CROC: Reversing all echo assignments`);
                for (let i = 0; i < numCoords; i++) {
                    coords[i].echo = etl + 1 - coords[i].echo;
                }
                
                // Verify the center echo after reversal
                let verifyMinDist = Infinity;
                let verifyEcho = -1;
                for (let i = 0; i < numCoords; i++) {
                    const dist = Math.sqrt(coords[i].ky * coords[i].ky + coords[i].kz * coords[i].kz);
                    if (dist < verifyMinDist) {
                        verifyMinDist = dist;
                        verifyEcho = coords[i].echo;
                    }
                }
                console.log(`CROC: After reversal, center echo is ${verifyEcho}`);
                foundCenter = true;
            } else {
                console.log(`CROC: Could not find a reversed solution`);
            }
        }
        
        console.log(`CROC final: center=[${centerKy.toFixed(1)}, ${centerKz.toFixed(1)}], ratios=[${kyRatio.toFixed(2)}, ${kzRatio.toFixed(2)}], wanted=${centerEcho}`);
        
        // Only call tryCenter to finalize if we haven't done reversal
        if (!foundCenter || foundCenter === false) {
            tryCenter(centerKy, centerKz, kyRatio, kzRatio);
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
        calculateAcceleration
    };
})();
