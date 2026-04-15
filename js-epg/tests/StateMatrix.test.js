/**
 * Unit Tests for StateMatrix.js
 */

import { test } from 'node:test';
import { strict as assert } from 'node:assert';
import { StateMatrix } from '../src/StateMatrix.js';
import { Complex } from '../src/Complex.js';

test('StateMatrix - initialization', () => {
  const sm = new StateMatrix(2);
  assert.equal(sm.nState(), 2);
  assert.equal(sm.states().length, 5); // 2*2 + 1
});

test('StateMatrix - equilibrium F0', () => {
  const sm = new StateMatrix(1);
  const F0 = sm.F0();
  assert.equal(F0.real, 0);
  assert.equal(F0.imag, 0);
});

test('StateMatrix - equilibrium Z0', () => {
  const sm = new StateMatrix(1);
  const Z0 = sm.Z0();
  assert.equal(Z0.real, 1);
  assert.equal(Z0.imag, 0);
});

test('StateMatrix - reset', () => {
  const sm = new StateMatrix(1);
  
  // Modify center state
  const center = sm.center();
  center[0] = Complex.create(0.5, 0.5);
  center[2] = Complex.create(0.5, 0);
  
  // Reset and check
  sm.reset();
  const F0 = sm.F0();
  const Z0 = sm.Z0();
  assert.equal(F0.real, 0);
  assert.equal(F0.imag, 0);
  assert.equal(Z0.real, 1);
  assert.equal(Z0.imag, 0);
});

test('StateMatrix - spoil', () => {
  const sm = new StateMatrix(1);
  const center = sm.center();
  center[0] = Complex.create(0.5, 0.5);
  center[1] = Complex.create(0.3, 0.4);
  center[2] = Complex.create(0.8, 0);
  
  sm.spoil();
  
  assert.equal(sm.F0().real, 0);
  assert.equal(sm.F0().imag, 0);
  const center2 = sm.center();
  assert.equal(center2[1].real, 0);
  assert.equal(center2[1].imag, 0);
  // Z should be unchanged by spoil
  assert.equal(center2[2].real, 0.8);
});

test('StateMatrix - resize', () => {
  const sm = new StateMatrix(2);
  assert.equal(sm.nState(), 2);
  
  sm.resize(3);
  assert.equal(sm.nState(), 3);
  assert.equal(sm.states().length, 7); // 2*3 + 1
});

test('StateMatrix - clone', () => {
  const sm = new StateMatrix(1);
  const center = sm.center();
  center[0] = Complex.create(0.5, 0.5);
  
  const clone = sm.clone();
  assert.equal(clone.nState(), sm.nState());
  assert.equal(clone.F0().real, sm.F0().real);
  assert.equal(clone.F0().imag, sm.F0().imag);
  
  // Modify clone and verify original unchanged
  clone.center()[0] = Complex.create(0.1, 0.1);
  assert.equal(sm.F0().real, 0.5);
  assert.equal(clone.F0().real, 0.1);
});
