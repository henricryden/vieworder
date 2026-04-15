/**
 * Unit Tests for Complex.js
 */

import { test } from 'node:test';
import { strict as assert } from 'node:assert';
import { Complex } from '../src/Complex.js';

test('Complex - creation', () => {
  const c = Complex.create(3, 4);
  assert.equal(c.real, 3);
  assert.equal(c.imag, 4);
});

test('Complex - magnitude', () => {
  const c = Complex.create(3, 4);
  const mag = Complex.magnitude(c);
  assert.equal(mag, 5); // 3-4-5 triangle
});

test('Complex - conjugate', () => {
  const c = Complex.create(3, 4);
  const conj = Complex.conjugate(c);
  assert.equal(conj.real, 3);
  assert.equal(conj.imag, -4);
});

test('Complex - add', () => {
  const a = Complex.create(1, 2);
  const b = Complex.create(3, 4);
  const result = Complex.add(a, b);
  assert.equal(result.real, 4);
  assert.equal(result.imag, 6);
});

test('Complex - multiply', () => {
  const a = Complex.create(1, 2);
  const b = Complex.create(3, 4);
  // (1+2i)(3+4i) = 3 + 4i + 6i + 8i² = 3 + 10i - 8 = -5 + 10i
  const result = Complex.multiply(a, b);
  assert.equal(result.real, -5);
  assert.equal(result.imag, 10);
});

test('Complex - exp(0)', () => {
  const c = Complex.create(0, 0);
  const result = Complex.exp(c);
  assert.equal(result.real, 1);
  assert.equal(result.imag, 0);
});

test('Complex - exp with imaginary unit', () => {
  // exp(i*π) = cos(π) + i*sin(π) ≈ -1
  const c = Complex.create(0, Math.PI);
  const result = Complex.exp(c);
  assert.equal(Math.abs(result.real + 1) < 1e-10, true);
  assert.equal(Math.abs(result.imag) < 1e-10, true);
});

test('Complex - i() imaginary unit', () => {
  const i = Complex.i();
  assert.equal(i.real, 0);
  assert.equal(i.imag, 1);
});

test('Complex - scale', () => {
  const c = Complex.create(2, 3);
  const scaled = Complex.scale(c, 2);
  assert.equal(scaled.real, 4);
  assert.equal(scaled.imag, 6);
});
