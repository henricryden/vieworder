/**
 * Unit Tests for Sequence.js
 */

import { test } from 'node:test';
import { strict as assert } from 'node:assert';
import { Sequence } from '../src/Sequence.js';
import { StateMatrix } from '../src/StateMatrix.js';
import { Complex } from '../src/Complex.js';
import { RFPulse } from '../src/operators/RFPulse.js';
import { Relaxation } from '../src/operators/Relaxation.js';
import { GradientShift } from '../src/operators/GradientShift.js';
import { ADC } from '../src/operators/ADC.js';

test('Sequence - builder pattern', () => {
  const seq = new Sequence(5);
  
  const result = seq
    .add(new RFPulse(90, 90))
    .add(new ADC('test'));
  
  assert.equal(seq === result, true); // Returns this
  assert.equal(seq.operators.length, 2);
});

test('Sequence - simulate 90° pulse', () => {
  const seq = new Sequence(1);
  seq
    .add(new RFPulse(90, 90))
    .add(new ADC());
  
  const result = seq.simulate();
  
  assert.equal(result.signals.length, 1);
  
  const signal = result.signals[0];
  const mag = Complex.magnitude(signal);
  
  // Should be ≈ 1 (i becomes -i magnitude 1)
  assert.equal(Math.abs(mag - 1) < 1e-10, true);
});

test('Sequence - simulate with initial state', () => {
  const initialState = new StateMatrix(1);
  
  // Invert the initial state
  new RFPulse(180, 0).apply(initialState);
  
  assert.equal(Math.abs(initialState.Z0().real + 1) < 1e-10, true);
  
  // Now try to recover
  const seq = new Sequence(1);
  seq.add(new Relaxation(1000, 1000, 100));
  
  const result = seq.simulate({ state: initialState });
  
  // Z0 should have recovered somewhat
  const Z0_final = result.finalState.Z0().real;
  assert.equal(Z0_final > -1, true); // Better than -1
  assert.equal(Z0_final < 1, true);  // Not fully recovered yet
});

test('Sequence - multiple ADCs', () => {
  const seq = new Sequence(5);
  seq
    .add(new RFPulse(90, 90))
    .add(new Relaxation(10, 1000, 100))
    .add(new ADC('t=0ms'))
    .add(new Relaxation(10, 1000, 100))
    .add(new ADC('t=10ms'))
    .add(new Relaxation(10, 1000, 100))
    .add(new ADC('t=20ms'));
  
  const result = seq.simulate();
  
  assert.equal(result.signals.length, 3);
  assert.equal(result.labels.length, 3);
  
  // Signal should decay over time
  const mag0 = Complex.magnitude(result.signals[0]);
  const mag1 = Complex.magnitude(result.signals[1]);
  const mag2 = Complex.magnitude(result.signals[2]);
  
  assert.equal(mag0 >= mag1, true);
  assert.equal(mag1 >= mag2, true);
});

test('Sequence - auto nState detection', () => {
  const seq = new Sequence(1);
  seq
    .add(new RFPulse(90, 90))
    .add(new GradientShift(3))  // Gradient shift should increase nState
    .add(new ADC());
  
  const result = seq.simulate(); // Auto-detect nState
  
  // Should complete without error
  assert.equal(result.signals.length, 1);
});

test('Sequence - spin echo pattern', () => {
  const seq = new Sequence(5);
  
  // Classic spin echo
  seq
    .add(new RFPulse(90, 90))      // 90° excitation
    .add(new GradientShift(1))      // Dephase gradient
    .add(new Relaxation(5, 1000, 100))  // Evolution
    .add(new RFPulse(180, 0))       // 180° refocusing
    .add(new GradientShift(1))      // Rephase gradient
    .add(new ADC('echo'));          // Read echo
  
  const result = seq.simulate();
  
  assert.equal(result.signals.length, 1);
  
  // Echo signal should be significant
  const echoDec = Complex.magnitude(result.signals[0]);
  assert.equal(echoDec > 0.01, true);
});

test('Sequence - return finalState', () => {
  const seq = new Sequence(1);
  seq
    .add(new RFPulse(90, 90))
    .add(new Relaxation(100, 1000, 100));
  
  const result = seq.simulate();
  
  assert.equal(result.finalState !== undefined, true);
  assert.equal(result.finalState instanceof StateMatrix, true);
});

test('Sequence - toString', () => {
  const seq = new Sequence(1);
  seq
    .add(new RFPulse(90, 90))
    .add(new ADC());
  
  const str = seq.toString();
  
  assert.equal(str.includes('Sequence with 2 operators'), true);
  assert.equal(str.includes('RFPulse'), true);
  assert.equal(str.includes('ADC'), true);
});
