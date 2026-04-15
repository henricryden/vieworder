/**
 * brain-phantom-worker.js
 * Web Worker for parallel phantom grid computation.
 * Each worker receives a row range [izStart, izEnd) and computes k-space for all tissues.
 *
 * Messages received:
 *   { type: 'setEdgeData', edgeData, workerId, totalWorkers }
 *     → replies { type: 'workerReady' }
 *
 *   { type: 'compute', key, Ny, Nz, izStart, izEnd, workerId }
 *     → progress: { type: 'progress', workerId, value: 0..1 }
 *     → result:   { type: 'gridChunkReady', key, izStart, izEnd, grids }
 *                 (grids: {tissueName: Float32Array[nRows*Ny*2]}, buffers transferred)
 */

'use strict';

importScripts('brain-phantom.js');

let _workerId = 0;

self.onmessage = function (e) {
    const msg = e.data;

    if (msg.type === 'setEdgeData') {
        _workerId = msg.workerId || 0;
        BrainPhantom.setEdgeData(msg.edgeData);
        self.postMessage({ type: 'workerReady' });

    } else if (msg.type === 'compute') {
        const { key, Ny, Nz, izStart, izEnd } = msg;
        _workerId = msg.workerId || _workerId;

        const grids = BrainPhantom.computePhantomGridChunk(Ny, Nz, izStart, izEnd, (frac) => {
            self.postMessage({ type: 'progress', workerId: _workerId, value: frac });
        });

        const transferables = Object.values(grids).map(a => a.buffer);
        self.postMessage({ type: 'gridChunkReady', key, izStart, izEnd, grids }, transferables);
    }
};

