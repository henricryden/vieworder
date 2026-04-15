/**
 * MPRAGE Variations - Demonstrates how to modify sequence parameters
 * 
 * Shows different TR and TE combinations to understand their effects on contrast
 */

import {
  Complex,
  Sequence,
  StateMatrix,
  RFPulse,
  Relaxation,
  GradientShift,
  ADC
} from '../src/index.js';

// Tissue parameters
const grayMatter = {
  name: 'Gray Matter',
  T1: 1200,
  T2: 100
};

const whiteMatter = {
  name: 'White Matter',
  T1: 600,
  T2: 60
};

/**
 * Build MPRAGE sequence with configurable parameters
 */
function buildMPRAGE(config) {
  const seq = new Sequence(15);

  const {
    ti = 400,           // Inversion time (ms)
    numPulses = 50,     // Fewer for demo (normally 200)
    innerTR = 7,        // TR between pulses
    TE = null,          // Echo time (defaults to TR/2)
    flipAngle = 8,      // Excitation flip angle
  } = config;

  const actualTE = TE !== null ? TE : (innerTR / 2);

  // Inversion pulse
  seq.add(new RFPulse(180, 0));

  // Inversion delay
  seq.add(new Relaxation(ti, 1000, 100)); // Placeholder T1/T2

  // Excitation train
  for (let i = 0; i < numPulses; i++) {
    seq.add(new RFPulse(flipAngle, 90));
    seq.add(new Relaxation(actualTE, 1000, 100));
    seq.add(new ADC(`pulse_${i + 1}`));
    
    const remainingTR = innerTR - actualTE;
    if (remainingTR > 0) {
      seq.add(new Relaxation(remainingTR, 1000, 100));
    }
    seq.add(new GradientShift(1));
  }

  // Recovery
  seq.add(new Relaxation(1000, 1000, 100));

  return seq;
}

/**
 * Clone sequence with tissue-specific T1/T2
 */
function cloneWithTissue(seq, tissue) {
  const newSeq = new Sequence(seq.initialNState);
  for (let op of seq.operators) {
    if (op instanceof Relaxation) {
      newSeq.add(new Relaxation(op.tau, tissue.T1, tissue.T2, op.g));
    } else {
      newSeq.add(op);
    }
  }
  return newSeq;
}

/**
 * Simulate sequence reaching steady state
 */
function simulateSteadyState(seq, tissue, numCycles = 2) {
  let state = null;
  let result = null;

  for (let cycle = 0; cycle < numCycles; cycle++) {
    const tissueSeq = cloneWithTissue(seq, tissue);
    result = tissueSeq.simulate({ state, nState: 20 });
    state = result.finalState;
  }

  return result;
}

/**
 * Format complex number nicely
 */
function formatSignal(c) {
  const mag = Math.sqrt(c.real * c.real + c.imag * c.imag);
  return mag.toFixed(6);
}

// ─────────────────────────────────────────────────────────────────────────────
// Main: Compare different TR/TE combinations
// ─────────────────────────────────────────────────────────────────────────────

console.log('\n' + '═'.repeat(70));
console.log('MPRAGE Sequence Variations - TR and TE Impact on Signal');
console.log('═'.repeat(70) + '\n');

const variations = [
  { name: 'Short TR, short TE', config: { innerTR: 5, TE: 2.5 } },
  { name: 'Short TR, long TE', config: { innerTR: 5, TE: 4.0 } },
  { name: 'Medium TR, medium TE', config: { innerTR: 7, TE: 3.5 } },
  { name: 'Long TR, long TE', config: { innerTR: 10, TE: 5.0 } },
  { name: 'Long TR, short TE', config: { innerTR: 10, TE: 3.0 } },
];

// Build base sequences for each variation
const baseSequences = variations.map(v => ({
  name: v.name,
  config: v.config,
  seq: buildMPRAGE({ ...v.config, numPulses: 50 })
}));

// Run simulations
const results = baseSequences.map(base => {
  const gmResult = simulateSteadyState(base.seq, grayMatter, 2);
  const wmResult = simulateSteadyState(base.seq, whiteMatter, 2);

  const gmSigs = gmResult.signals.map(s => Math.sqrt(s.real ** 2 + s.imag ** 2));
  const wmSigs = wmResult.signals.map(s => Math.sqrt(s.real ** 2 + s.imag ** 2));

  const gmAvg = gmSigs.reduce((a, b) => a + b, 0) / gmSigs.length;
  const wmAvg = wmSigs.reduce((a, b) => a + b, 0) / wmSigs.length;

  return {
    name: base.name,
    TR: base.config.innerTR,
    TE: base.config.TE || (base.config.innerTR / 2),
    gmSignal: gmAvg,
    wmSignal: wmAvg,
    contrast: gmAvg > 0 && wmAvg > 0 ? Math.abs(gmAvg - wmAvg) : 0,
    ratio: gmAvg > 0 && wmAvg > 0 ? (gmAvg / wmAvg) : 0
  };
});

// Display table
console.log('Steady-State Signal Comparison (Cycle 2):\n');
console.log('Configuration'.padEnd(25) + 
            'TR(ms)'.padEnd(8) + 
            'TE(ms)'.padEnd(8) +
            'GM Signal'.padEnd(12) +
            'WM Signal'.padEnd(12) +
            'Contrast'.padEnd(12) +
            'Ratio\n');
console.log('─'.repeat(75));

for (const r of results) {
  console.log(
    r.name.padEnd(25) +
    r.TR.toFixed(1).padEnd(8) +
    r.TE.toFixed(1).padEnd(8) +
    r.gmSignal.toFixed(5).padEnd(12) +
    r.wmSignal.toFixed(5).padEnd(12) +
    r.contrast.toFixed(5).padEnd(12) +
    r.ratio.toFixed(4)
  );
}

// Find best contrast
const maxContrastIdx = results.reduce((maxIdx, r, i) => 
  r.contrast > results[maxIdx].contrast ? i : maxIdx, 0);

console.log('\n' + '─'.repeat(75));
console.log(`\n💡 Best tissue contrast: ${results[maxContrastIdx].name}`);
console.log(`   Contrast difference: ${results[maxContrastIdx].contrast.toFixed(5)}`);
console.log(`   Ratio (GM/WM): ${results[maxContrastIdx].ratio.toFixed(4)}`);

console.log('\n💡 Notes:');
console.log('  - Shorter TR favors T1 contrast');
console.log('  - Longer TE favors T2 contrast');
console.log('  - MPRAGE is primarily T1-weighted (due to short TR, inversion)');
console.log('  - Adjust TR and TE based on desired tissue differentiation');

console.log('\n');
