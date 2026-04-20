/**
 * StateMatrix - Represents the Extended Phase Graph state
 * Stores F+(k), F-(k), Z(k) for k from -nState to +nState
 */

import { Complex } from './Complex.js';

export class StateMatrix {
  /**
   * Create a new EPG state matrix
   * @param {number} nState - initial number of phase states on each side of k=0
   */
  constructor(nState = 1) {
    this.nState_ = nState;
    this.states_ = [];
    this.reset();
  }

  /**
   * Reset to equilibrium: F0=0, Z0=1, all others zero
   */
  reset() {
    // Total size: 2*nState + 1 (symmetric around k=0)
    const size = 2 * this.nState_ + 1;
    this.states_ = [];

    for (let i = 0; i < size; i++) {
      this.states_.push([
        Complex.zero(),    // F+ initialized to 0
        Complex.zero(),    // F- initialized to 0
        i === this.nState_ ? Complex.one() : Complex.zero()  // Z=1 at k=0, else 0
      ]);
    }
  }

  /**
   * Get the number of phase states (on each side of k=0)
   * @returns {number}
   */
  nState() {
    return this.nState_;
  }

  /**
   * Get array of states
   * @returns {Array}
   */
  states() {
    return this.states_;
  }

  /**
   * Get the center state (k=0)
   * @returns {Array}
   */
  center() {
    return this.states_[this.nState_];
  }

  /**
   * Get F0 (signal at k=0)
   * @returns {{real: number, imag: number}}
   */
  F0() {
    return this.center()[0];
  }

  /**
   * Get Z0 (longitudinal magnetization at k=0)
   * @returns {{real: number, imag: number}}
   */
  Z0() {
    return this.center()[2];
  }

  /**
   * Spoil: zero all transverse states (F+ and F-)
   */
  spoil() {
    for (let state of this.states_) {
      state[0] = Complex.zero();
      state[1] = Complex.zero();
    }
  }

  /**
   * Resize state array to new nState
   * @param {number} newNState
   */
  resize(newNState) {
    if (newNState === this.nState_) {
      return;
    }

    if (newNState < this.nState_) {
      // Shrink - truncate extremes
      const diff = this.nState_ - newNState;
      this.states_.splice(0, diff);
      this.states_.splice(newNState * 2 + 1);
    } else {
      // Grow - pad with zero states
      const diff = newNState - this.nState_;
      for (let i = 0; i < diff; i++) {
        this.states_.unshift([Complex.zero(), Complex.zero(), Complex.zero()]);
        this.states_.push([Complex.zero(), Complex.zero(), Complex.zero()]);
      }
    }

    this.nState_ = newNState;
  }

  /**
   * Get state at k-offset
   * @param {number} offset - k value relative to k=0
   * @returns {Array|null}
   */
  getState(offset) {
    const idx = this.nState_ + offset;
    if (idx < 0 || idx >= this.states_.length) {
      return null;
    }
    return this.states_[idx];
  }

  /**
   * Deep copy of the state matrix
   * @returns {StateMatrix}
   */
  clone() {
    const clone = new StateMatrix(this.nState_);
    clone.states_ = this.states_.map(state => [
      { ...state[0] },
      { ...state[1] },
      { ...state[2] }
    ]);
    return clone;
  }

  /**
   * String representation for debugging
   * @returns {string}
   */
  toString() {
    let result = `StateMatrix(nState=${this.nState_}):\n`;
    result += `  F0 = ${Complex.toString(this.F0())}\n`;
    result += `  Z0 = ${Complex.toString(this.Z0())}\n`;
    return result;
  }
}
