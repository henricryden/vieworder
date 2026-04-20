/**
 * RFPulse Operator - RF pulse with flip angle and phase
 * Applies rotation: M = Rz(phi) @ Rx(alpha) @ Rz(-phi)
 */

import { Operator } from './Operator.js';
import { Complex } from '../Complex.js';

export class RFPulse extends Operator {
  /**
   * Create RF pulse operator
   * @param {number} alpha_deg - Flip angle in degrees
   * @param {number} phi_deg - RF phase in degrees
   */
  constructor(alpha_deg, phi_deg = 0) {
    super();
    this.alpha_deg = alpha_deg;
    this.phi_deg = phi_deg;

    // Pre-compute rotation matrix: M = Rz(phi) @ Rx(alpha) @ Rz(-phi)
    this.mat = this._computeRotationMatrix(alpha_deg, phi_deg);
  }

  /**
   * Convenience: RF pulse along x-axis (phi=0)
   * @param {number} alpha_deg
   * @returns {RFPulse}
   */
  static x(alpha_deg) {
    return new RFPulse(alpha_deg, 0);
  }

  /**
   * Convenience: RF pulse along y-axis (phi=90)
   * @param {number} alpha_deg
   * @returns {RFPulse}
   */
  static y(alpha_deg) {
    return new RFPulse(alpha_deg, 90);
  }

  /**
   * Apply RF pulse to all states in StateMatrix
   * @param {StateMatrix} stateMatrix
   */
  apply(stateMatrix) {
    const states = stateMatrix.states();
    for (let state of states) {
      const newState = this._matVecMult(this.mat, state);
      state[0] = newState[0];
      state[1] = newState[1];
      state[2] = newState[2];
    }
  }

  /**
   * Compute rotation matrix: Rz(phi) @ Rx(alpha) @ Rz(-phi)
   * @private
   * @param {number} alpha_deg
   * @param {number} phi_deg
   * @returns {Array<Array>}
   */
  _computeRotationMatrix(alpha_deg, phi_deg) {
    const alpha = alpha_deg * Math.PI / 180;
    const phi = phi_deg * Math.PI / 180;

    // Rx(alpha) rotation
    const ca2 = Math.cos(alpha / 2);
    const sa2 = Math.sin(alpha / 2);
    const sa = Math.sin(alpha);
    const ca = Math.cos(alpha);

    const Rx = [
      [Complex.create(ca2 * ca2), Complex.create(sa2 * sa2), Complex.scale(Complex.i(), -sa)],
      [Complex.create(sa2 * sa2), Complex.create(ca2 * ca2), Complex.scale(Complex.i(), sa)],
      [Complex.scale(Complex.i(), -sa / 2), Complex.scale(Complex.i(), sa / 2), Complex.create(ca)]
    ];

    // Rz(phi) rotation: diagonal matrix
    const Rz_phi = [
      [Complex.exp(Complex.scale(Complex.i(), phi)), Complex.zero(), Complex.zero()],
      [Complex.zero(), Complex.exp(Complex.scale(Complex.i(), -phi)), Complex.zero()],
      [Complex.zero(), Complex.zero(), Complex.one()]
    ];

    // Rz(-phi) rotation: diagonal matrix
    const Rz_neg_phi = [
      [Complex.exp(Complex.scale(Complex.i(), -phi)), Complex.zero(), Complex.zero()],
      [Complex.zero(), Complex.exp(Complex.scale(Complex.i(), phi)), Complex.zero()],
      [Complex.zero(), Complex.zero(), Complex.one()]
    ];

    // Multiply: Rz(phi) @ Rx(alpha)
    let temp = this._mat3Mul(Rz_phi, Rx);

    // Multiply: (Rz(phi) @ Rx(alpha)) @ Rz(-phi)
    return this._mat3Mul(temp, Rz_neg_phi);
  }

  /**
   * Multiply two 3x3 matrices
   * @private
   * @param {Array<Array>} a
   * @param {Array<Array>} b
   * @returns {Array<Array>}
   */
  _mat3Mul(a, b) {
    const result = [];
    for (let row = 0; row < 3; row++) {
      result[row] = [];
      for (let col = 0; col < 3; col++) {
        result[row][col] = Complex.zero();
        for (let k = 0; k < 3; k++) {
          result[row][col] = Complex.add(
            result[row][col],
            Complex.multiply(a[row][k], b[k][col])
          );
        }
      }
    }
    return result;
  }

  /**
   * Multiply 3x3 matrix by 3-element state vector
   * @private
   * @param {Array<Array>} mat
   * @param {Array} state
   * @returns {Array}
   */
  _matVecMult(mat, state) {
    const result = [Complex.zero(), Complex.zero(), Complex.zero()];
    for (let row = 0; row < 3; row++) {
      for (let col = 0; col < 3; col++) {
        result[row] = Complex.add(result[row], Complex.multiply(mat[row][col], state[col]));
      }
    }
    return result;
  }

  toString() {
    return `RFPulse(α=${this.alpha_deg}°, φ=${this.phi_deg}°)`;
  }
}
