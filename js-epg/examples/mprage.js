/**
 * MPRAGE Example - Magnetization-Prepared Rapid Gradient Echo
 * 
 * MPRAGE sequence structure:
 * 1. Inversion pulse (180°)
 * 2. Wait delay for T1 recovery (e.g., 400ms)
 * 3. N excitation pulses with small flip angle (e.g., 200 × 8°), each followed by gradient shift
 * 4. Recovery period (e.g., 1000ms)
 * 
 * This example simulates two tissue types with different T1/T2/M0 and shows
 * steady-state signal evolution.
 */

import {
  Complex,
  Sequence,
  StateMatrix,
  RFPulse,
  Relaxation,
  GradientShift,
  ADC,
  Spoiler
} from '../src/index.js';

// ─────────────────────────────────────────────────────────────────────────────
// Tissue Parameters
// ─────────────────────────────────────────────────────────────────────────────

const tissues = {
  grayMatter: {
    name: 'Gray Matter',
    M0: 1.0,      // Magnetization at equilibrium
    T1: 1200,     // Longitudinal relaxation (ms)
    T2: 100,      // Transverse relaxation (ms)
    color: '\x1b[36m'  // Cyan
  },
  whiteMatter: {
    name: 'White Matter',
    M0: 0.9,      // Slightly different M0
    T1: 600,      // Shorter T1
    T2: 60,       // Shorter T2
    color: '\x1b[35m'  // Magenta
  }
};

// ─────────────────────────────────────────────────────────────────────────────
// MPRAGE Parameters
// ─────────────────────────────────────────────────────────────────────────────

const mprage = {
  inversionFlip: 180,        // Inversion pulse (degrees)
  inversionPhase: 0,         // Inversion phase (degrees)
  inversionDelay: 400,       // TI: Time after inversion before excitation train (ms)
  excitationFlip: 8,         // Excitation flip angle (degrees)
  excitationPhase: 90,       // Excitation phase (degrees, usually y-axis)
  numExcitations: 200,       // Number of excitations in the train
  innerTR: 7,                // TR: Time between excitation pulses (ms)
  TE: null,                  // TE: Echo time (ms, default = innerTR/2)
  recoveryDelay: 1000,       // Time after excitation train before next inversion (ms)
  recordAllPulses: true      // Record ADC after every pulse (if false, record every Nth pulse)
};

// ─────────────────────────────────────────────────────────────────────────────
// Build MPRAGE Sequence
// ─────────────────────────────────────────────────────────────────────────────

function buildMPRAGESequence() {
  const seq = new Sequence(10);

  // Determine TE (default to TR/2)
  const TE = mprage.TE !== null ? mprage.TE : (mprage.innerTR / 2);

  // Step 1: Inversion pulse (180°)
  seq.add(new RFPulse(mprage.inversionFlip, mprage.inversionPhase));

  // Step 2: Inversion delay (TI) - wait for recovery
  seq.add(new Relaxation(
    mprage.inversionDelay,
    1000,  // Placeholder T1 (will be replaced per tissue)
    100    // Placeholder T2 (will be replaced per tissue)
  ));

  // Step 3: Excitation train
  for (let i = 0; i < mprage.numExcitations; i++) {
    // RF excitation pulse
    seq.add(new RFPulse(mprage.excitationFlip, mprage.excitationPhase));

    // Evolution until TE (dephasing during readout)
    seq.add(new Relaxation(TE, 1000, 100));

    // ADC at TE
    seq.add(new ADC(`TE_${i + 1}`));

    // Remaining time until TR (includes refocusing and recovery)
    const remainingTime = mprage.innerTR - TE;
    if (remainingTime > 0) {
      seq.add(new Relaxation(remainingTime, 1000, 100));
    }

    // Dephasing gradient for next excitation (except after last pulse in train,
    // but we include it anyway as it doesn't hurt for this simulation)
    seq.add(new GradientShift(1));
  }

  // Step 4: Final recovery period
  seq.add(new Relaxation(mprage.recoveryDelay, 1000, 100));

  return seq;
}

// ─────────────────────────────────────────────────────────────────────────────
// Simulate MPRAGE with custom T1/T2 for each tissue
// ─────────────────────────────────────────────────────────────────────────────

function simulateTissue(tissue, sequenceTemplate, numCycles = 2) {
  const results = [];

  // Clone template sequence structure
  function cloneSequenceWithTissueParams(template, T1, T2) {
    const seq = new Sequence(template.initialNState);
    for (let op of template.operators) {
      if (op instanceof Relaxation) {
        // Create new Relaxation with tissue-specific T1/T2
        seq.add(new Relaxation(op.tau, T1, T2, op.g));
      } else {
        seq.add(op);
      }
    }
    return seq;
  }

  let currentState = null;

  for (let cycle = 0; cycle < numCycles; cycle++) {
    // Build sequence with tissue parameters
    const customSeq = cloneSequenceWithTissueParams(
      sequenceTemplate,
      tissue.T1,
      tissue.T2
    );

    // Simulate
    const result = customSeq.simulate({
      state: currentState,
      nState: 20
    });

    results.push({
      cycle,
      signals: result.signals,
      labels: result.labels,
      finalState: result.finalState
    });

    // Use final state as initial state for next cycle (steady state approach)
    currentState = result.finalState;
  }

  return results;
}

// ─────────────────────────────────────────────────────────────────────────────
// Analyze and Display Results
// ─────────────────────────────────────────────────────────────────────────────

function formatComplexSignal(c, decimals = 6) {
  const mag = Math.sqrt(c.real * c.real + c.imag * c.imag);
  const phase = Math.atan2(c.imag, c.real) * 180 / Math.PI;
  return `mag=${mag.toFixed(decimals)}, phase=${phase.toFixed(1)}°`;
}

function displayResults(tissue, results) {
  console.log(`\n${tissue.color}═══════════════════════════════════════════════════════`);
  console.log(`MPRAGE Simulation: ${tissue.name}`);
  console.log(`T1=${tissue.T1}ms, T2=${tissue.T2}ms`);
  console.log(`═══════════════════════════════════════════════════════\x1b[0m\n`);

  for (const cycleResult of results) {
    console.log(`${tissue.color}Cycle ${cycleResult.cycle + 1}:${'\x1b[0m'}`);

    // Show final state magnetization
    const finalMz = cycleResult.finalState.Z0();
    const finalMzMag = Complex.magnitude(finalMz);
    console.log(`  Final Mz (at k=0): ${formatComplexSignal(finalMz)}`);
    console.log(`  Final F0 (signal):  ${formatComplexSignal(cycleResult.finalState.F0())}`);

    // Show signal statistics
    if (cycleResult.signals.length > 0) {
      const numSignals = cycleResult.signals.length;
      const magnitudes = cycleResult.signals.map(s => Complex.magnitude(s));
      const avgMag = magnitudes.reduce((a, b) => a + b, 0) / magnitudes.length;
      const maxMag = Math.max(...magnitudes);
      const minMag = Math.min(...magnitudes);

      console.log(`  Signal Statistics (${numSignals} ADC points):`);
      console.log(`    Min: ${minMag.toFixed(6)}, Max: ${maxMag.toFixed(6)}, Avg: ${avgMag.toFixed(6)}`);

      // Show first 5 and last 5 signals
      console.log(`    First 5 signals:`);
      for (let i = 0; i < Math.min(5, numSignals); i++) {
        const signal = cycleResult.signals[i];
        console.log(`      ${cycleResult.labels[i]}: ${formatComplexSignal(signal)}`);
      }

      if (numSignals > 10) {
        console.log(`    ... (${numSignals - 10} more) ...`);
        console.log(`    Last 5 signals:`);
        for (let i = Math.max(0, numSignals - 5); i < numSignals; i++) {
          const signal = cycleResult.signals[i];
          console.log(`      ${cycleResult.labels[i]}: ${formatComplexSignal(signal)}`);
        }
      } else if (numSignals > 5) {
        for (let i = 5; i < numSignals; i++) {
          const signal = cycleResult.signals[i];
          console.log(`      ${cycleResult.labels[i]}: ${formatComplexSignal(signal)}`);
        }
      }
      console.log('');
    }
  }
}

// ─────────────────────────────────────────────────────────────────────────────
// Main
// ─────────────────────────────────────────────────────────────────────────────

console.log('\n\x1b[1m╔══════════════════════════════════════════════════════════╗');
console.log('║        MPRAGE MRI Pulse Sequence Simulator (js-epg)      ║');
console.log('╚══════════════════════════════════════════════════════════╝\x1b[0m');

console.log(`\n${'\x1b[33m'}Sequence Parameters:${'\x1b[0m'}`);
console.log(`  Inversion: ${mprage.inversionFlip}° @ φ=${mprage.inversionPhase}°`);
console.log(`  Inversion delay (TI): ${mprage.inversionDelay}ms`);
console.log(`  Excitations: ${mprage.numExcitations} × ${mprage.excitationFlip}° @ φ=${mprage.excitationPhase}°`);
console.log(`  Inner TR (between pulses): ${mprage.innerTR}ms`);
const TE = mprage.TE !== null ? mprage.TE : (mprage.innerTR / 2);
console.log(`  TE (echo time): ${TE}ms${mprage.TE === null ? ' (default = TR/2)' : ''}`);
console.log(`  Recovery delay: ${mprage.recoveryDelay}ms`);
console.log(`  Total ADCs: ${mprage.numExcitations}`);

// Build template sequence
const mpragSeq = buildMPRAGESequence();
console.log(`\n${'\x1b[33m'}Sequence structure: ${mpragSeq.operators.length} operators\x1b[0m`);

// Simulate both tissues
console.log(`\n${'\x1b[33m'}Running simulations...${'\x1b[0m'}`);

const gmResults = simulateTissue(tissues.grayMatter, mpragSeq, 2);
const wmResults = simulateTissue(tissues.whiteMatter, mpragSeq, 2);

// Display results
displayResults(tissues.grayMatter, gmResults);
displayResults(tissues.whiteMatter, wmResults);

// ─────────────────────────────────────────────────────────────────────────────
// Comparison: Steady State Signals
// ─────────────────────────────────────────────────────────────────────────────

console.log(`\n${'\x1b[1m'}Steady-State Signal Comparison (Cycle 2):${'\x1b[0m'}`);
if (gmResults.length > 1 && wmResults.length > 1) {
  const gmSteady = gmResults[1];
  const wmSteady = wmResults[1];

  console.log(`\n${tissues.grayMatter.color}Gray Matter:${'\x1b[0m'}`);
  if (gmSteady.signals.length > 0) {
    const mags = gmSteady.signals.map(s => Complex.magnitude(s));
    const avgMag = mags.reduce((a, b) => a + b, 0) / mags.length;
    const maxMag = Math.max(...mags);
    const minMag = Math.min(...mags);
    console.log(`  Signal range: ${minMag.toFixed(6)} to ${maxMag.toFixed(6)}`);
    console.log(`  Mean signal: ${avgMag.toFixed(6)}`);
    console.log(`  First ADC: ${formatComplexSignal(gmSteady.signals[0])}`);
    console.log(`  Last ADC:  ${formatComplexSignal(gmSteady.signals[gmSteady.signals.length - 1])}`);
  }

  console.log(`\n${tissues.whiteMatter.color}White Matter:${'\x1b[0m'}`);
  if (wmSteady.signals.length > 0) {
    const mags = wmSteady.signals.map(s => Complex.magnitude(s));
    const avgMag = mags.reduce((a, b) => a + b, 0) / mags.length;
    const maxMag = Math.max(...mags);
    const minMag = Math.min(...mags);
    console.log(`  Signal range: ${minMag.toFixed(6)} to ${maxMag.toFixed(6)}`);
    console.log(`  Mean signal: ${avgMag.toFixed(6)}`);
    console.log(`  First ADC: ${formatComplexSignal(wmSteady.signals[0])}`);
    console.log(`  Last ADC:  ${formatComplexSignal(wmSteady.signals[wmSteady.signals.length - 1])}`);
  }

  // Signal ratio comparison
  if (gmSteady.signals.length > 0 && wmSteady.signals.length > 0) {
    const gmMags = gmSteady.signals.map(s => Complex.magnitude(s));
    const wmMags = wmSteady.signals.map(s => Complex.magnitude(s));
    const gmAvg = gmMags.reduce((a, b) => a + b, 0) / gmMags.length;
    const wmAvg = wmMags.reduce((a, b) => a + b, 0) / wmMags.length;
    
    console.log(`\n  ${'\x1b[1m'}Mean signal ratio (GM/WM): ${(gmAvg / wmAvg).toFixed(4)}${'\x1b[0m'}`);
    console.log(`  This contrast determines the image tissue differentiation`);
  }
}

console.log('\n');
