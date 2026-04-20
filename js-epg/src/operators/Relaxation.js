/**
 * Relaxation Operator - T1/T2 decay and recovery
 * Applies: F+ *= exp(-tau/T2 + 2πi*g*tau)
 *          F- *= conj(exp(-tau/T2 + 2πi*g*tau))
 *          Z  *= exp(-tau/T1) and then Z += (1 - exp(-tau/T1))
 */

import { Operator } from './Operator.js';
import { Complex } from '../Complex.js';

export class Relaxation extends Operator {
  /**
   * Create relaxation operator
   * @param {number} tau - Evolution time in milliseconds
   * @param {number} T1 - Longitudinal relaxation time in milliseconds
   * @param {number} T2 - Transverse relaxation time in milliseconds
   * @param {number} g - Off-resonance frequency in kHz (default 0)
   */
  constructor(tau, T1, T2, g = 0) {
    super();
    this.tau = tau;
    this.T1 = T1;
    this.T2 = T2;
    this.g = g;

    this._precompute();
  }

  /**
   * Pre-compute decay factors
   * @private
   */
  _precompute() {
    // Transverse:
    // rT = tau/T2 + 2π*i*g*tau  (g in kHz, tau in ms)
    const rT = Complex.create(this.tau / this.T2, 2 * Math.PI * this.g * this.tau);
    this.eFp = Complex.exp(Complex.scale(rT, -1)); // exp(-rT)
    this.eFm = Complex.conjugate(this.eFp);  // conj(exp(-rT))

    // Longitudinal:
    // eL = exp(-tau/T1)
    this.eL = Math.exp(-this.tau / this.T1);

    // Recovery term:
    // recovery = 1 - exp(-tau/T1)
    this.recovery = 1 - this.eL;
  }

  /**
   * Apply relaxation to all states in StateMatrix
   * @param {StateMatrix} stateMatrix
   */
  apply(stateMatrix) {
    const states = stateMatrix.states();

    // Apply to all states
    for (let state of states) {
      // Transverse decay with phase evolution
      state[0] = Complex.multiply(state[0], this.eFp);
      state[1] = Complex.multiply(state[1], this.eFm);
      // Longitudinal decay
      state[2] = Complex.scale(state[2], this.eL);
    }

    // Add longitudinal recovery to k=0 only
    const center = stateMatrix.center();
    center[2] = Complex.add(center[2], Complex.create(this.recovery, 0));
  }

  toString() {
    return `Relaxation(τ=${this.tau}ms, T1=${this.T1}ms, T2=${this.T2}ms, g=${this.g}kHz)`;
  }
}
