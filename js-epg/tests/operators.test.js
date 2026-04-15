/**
 * Unit Tests for Operators
 */

import { test } from 'node:test';
import { strict as assert } from 'node:assert';
import { StateMatrix } from '../src/StateMatrix.js';
import { Complex } from '../src/Complex.js';
import { RFPulse } from '../src/operators/RFPulse.js';
import { Relaxation } from '../src/operators/Relaxation.js';
import { GradientShift } from '../src/operators/GradientShift.js';
import { Spoiler } from '../src/operators/Spoiler.js';

// ─────────────────────────────────────────────────────────────────────────────
// RFPulse Tests
// ─────────────────────────────────────────────────────────────────────────────

test('RFPulse - 90° along x-axis', () => {
  const sm = new StateMatrix(1);
  const pulse = new RFPulse(90, 0);  // phi=0 means x-axis
  pulse.apply(sm);
  
  const F0 = sm.F0();
  const Z0 = sm.Z0();
  
  // After 90° pulse along x: F0 should be ≈ -i
  assert.equal(Math.abs(F0.real) < 1e-10, true); // ~0
  assert.equal(Math.abs(F0.imag + 1) < 1e-10, true); // ~-1
  
  // Z0 should be ≈ 0
  assert.equal(Math.abs(Z0.real) < 1e-10, true);
});

test('RFPulse - 180° inversion', () => {
  const sm = new StateMatrix(1);
  const pulse = new RFPulse(180, 0);
  pulse.apply(sm);
  
  const F0 = sm.F0();
  const Z0 = sm.Z0();
  
  // After 180° pulse: F0 should stay ~0
  assert.equal(Math.abs(F0.real) < 1e-10, true);
  
  // Z0 should be ≈ -1 (inversion)
  assert.equal(Math.abs(Z0.real + 1) < 1e-10, true);
});

test('RFPulse - convenience Rx(90)', () => {
  const sm1 = new StateMatrix(1);
  const sm2 = new StateMatrix(1);
  
  const pulse1 = RFPulse.x(90);
  const pulse2 = new RFPulse(90, 0);
  
  pulse1.apply(sm1);
  pulse2.apply(sm2);
  
  const F0_1 = sm1.F0();
  const F0_2 = sm2.F0();
  
  assert.equal(Math.abs(F0_1.real - F0_2.real) < 1e-10, true);
  assert.equal(Math.abs(F0_1.imag - F0_2.imag) < 1e-10, true);
});

// ─────────────────────────────────────────────────────────────────────────────
// Relaxation Tests
// ─────────────────────────────────────────────────────────────────────────────

test('Relaxation - T2 decay', () => {
  const sm = new StateMatrix(1);
  
  // Create F0 = 1 (set by 90° pulse)
  const pulse90 = new RFPulse(90, 90);
  pulse90.apply(sm);
  
  const F0_before = Complex.magnitude(sm.F0());
  
  // Apply relaxation
  const T1 = 1000;
  const T2 = 100;
  const tau = 50; // 50ms
  const relax = new Relaxation(tau, T1, T2, 0);
  relax.apply(sm);
  
  const F0_after = Complex.magnitude(sm.F0());
  
  // Expected decay: exp(-tau/T2) = exp(-50/100) ≈ 0.606
  const expected = F0_before * Math.exp(-tau / T2);
  
  assert.equal(Math.abs(F0_after - expected) < 1e-10, true);
});

test('Relaxation - T1 recovery', () => {
  const sm = new StateMatrix(1);
  
  // Invert (Z0 = -1)
  const invert = new RFPulse(180, 0);
  invert.apply(sm);
  
  assert.equal(Math.abs(sm.Z0().real + 1) < 1e-10, true);
  
  // Apply relaxation
  const T1 = 1000;
  const T2 = 100;
  const tau = 100; // 100ms
  const relax = new Relaxation(tau, T1, T2, 0);
  relax.apply(sm);
  
  const Z0 = sm.Z0().real;
  
  // Expected: Z = -1 * exp(-tau/T1) + (1 - exp(-tau/T1))
  //         = -exp(-tau/T1) + 1 - exp(-tau/T1)
  //         = 1 - 2*exp(-tau/T1)
  const expected = 1 - 2 * Math.exp(-tau / T1);
  
  assert.equal(Math.abs(Z0 - expected) < 1e-10, true);
});

// ─────────────────────────────────────────────────────────────────────────────
// GradientShift Tests
// ─────────────────────────────────────────────────────────────────────────────

test('GradientShift - positive shift', () => {
  const sm = new StateMatrix(2);
  
  // Set F+ at k=0
  sm.center()[0] = Complex.create(0.5, 0);
  
  // Apply shift
  const shift = new GradientShift(1);
  shift.apply(sm);
  
  // After shift by +1, nState should grow
  assert.equal(sm.nState(), 3);
  
  // F+ should have moved to higher indices
  const newF0 = sm.F0();
  assert.equal(newF0.real, 0); // F+ at k=0 should be zeroed
});

test('GradientShift - no shift (k=0)', () => {
  const sm = new StateMatrix(1);
  sm.center()[0] = Complex.create(0.5, 0.5);
  
  const shift = new GradientShift(0);
  shift.apply(sm);
  
  // Should be unchanged
  assert.equal(sm.F0().real, 0.5);
  assert.equal(sm.F0().imag, 0.5);
});

// ─────────────────────────────────────────────────────────────────────────────
// Spoiler Tests
// ─────────────────────────────────────────────────────────────────────────────

test('Spoiler - zeros transverse states', () => {
  const sm = new StateMatrix(1);
  
  // Create transverse magnetization with 90° pulse along x
  const pulse = new RFPulse(90, 0);
  pulse.apply(sm);
  
  assert.equal(Math.abs(sm.F0().imag + 1) < 1e-10, true); // F0 ≈ -i (non-zero)
  
  // Apply spoiler
  const spoil = new Spoiler();
  spoil.apply(sm);
  
  // F0 should be zero
  assert.equal(sm.F0().real, 0);
  assert.equal(sm.F0().imag, 0);
  
  // Z0 should be unchanged (≈0 after 90°)
  assert.equal(Math.abs(sm.Z0().real) < 1e-10, true);
});

// ─────────────────────────────────────────────────────────────────────────────
// Multi-operator sequences
// ─────────────────────────────────────────────────────────────────────────────

test('Sequence - spin echo', () => {
  const sm = new StateMatrix(5);
  
  // 90° along x
  new RFPulse(90, 0).apply(sm);
  let F0 = Complex.magnitude(sm.F0());
  assert.equal(Math.abs(F0 - 1) < 1e-10, true);
  
  // Dephase with relaxation
  new GradientShift(1).apply(sm);
  new Relaxation(5, 1000, 100).apply(sm);  // T2 decay
  
  // 180°
  new RFPulse(180, 0).apply(sm);
  
  // Rephase with relaxation
  new GradientShift(1).apply(sm);
  new Relaxation(5, 1000, 100).apply(sm);  // More T2 decay
  
  // Signal should be non-zero but less than 1 due to T2 decay
  // Total T2 decay: exp(-10/100) ≈ 0.9048
  F0 = Complex.magnitude(sm.F0());
  assert.equal(F0 > 0.9, true);  
  assert.equal(F0 < 1.0, true);
});
