/**
 * GradientShift Operator - Phase order shifting
 * Shifts phase coherence pathways in k-space
 */

import { Operator } from './Operator.js';
import { Complex } from '../Complex.js';

export class GradientShift extends Operator {
  /**
   * Create gradient shift operator
   * @param {number} k - Number of phase orders to shift
   */
  constructor(k) {
    super();
    this.k = k;
  }

  /**
   * Apply gradient shift to StateMatrix
   * @param {StateMatrix} stateMatrix
   */
  apply(stateMatrix) {
    if (this.k === 0) return;

    // Grow state matrix by |k|
    const absK = Math.abs(this.k);
    const oldNState = stateMatrix.nState();
    stateMatrix.resize(oldNState + absK);

    const states = stateMatrix.states();
    const sz = states.length;

    if (this.k > 0) {
      // Positive shift: F+ rightward (higher indices), F- leftward (lower indices)

      // F+: shift right by k
      // Copy from right to left to avoid overwriting
      for (let i = sz - 1; i >= this.k; i--) {
        states[i][0] = { ...states[i - this.k][0] };
      }
      // Zero-pad left side of F+
      for (let i = 0; i < this.k; i++) {
        states[i][0] = Complex.zero();
      }

      // F-: shift left by k
      for (let i = 0; i < sz - this.k; i++) {
        states[i][1] = { ...states[i + this.k][1] };
      }
      // Zero-pad right side of F-
      for (let i = sz - this.k; i < sz; i++) {
        states[i][1] = Complex.zero();
      }
    } else {
      // Negative shift: F+ leftward, F- rightward

      const k_abs = -this.k;

      // F+: shift left by k_abs
      for (let i = 0; i < sz - k_abs; i++) {
        states[i][0] = { ...states[i + k_abs][0] };
      }
      // Zero-pad right side of F+
      for (let i = sz - k_abs; i < sz; i++) {
        states[i][0] = Complex.zero();
      }

      // F-: shift right by k_abs
      for (let i = sz - 1; i >= k_abs; i--) {
        states[i][1] = { ...states[i - k_abs][1] };
      }
      // Zero-pad left side of F-
      for (let i = 0; i < k_abs; i++) {
        states[i][1] = Complex.zero();
      }
    }

    // Z states unchanged by gradient
  }

  toString() {
    return `GradientShift(k=${this.k})`;
  }
}
