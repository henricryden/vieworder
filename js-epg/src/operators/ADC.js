/**
 * ADC Operator - Readout/data acquisition marker
 * Doesn't modify state, just records F0
 */

import { Operator } from './Operator.js';

export class ADC extends Operator {
  /**
   * Create ADC operator
   * @param {string} label - Optional label for this readout
   */
  constructor(label = '') {
    super();
    this.label = label;
  }

  /**
   * ADC doesn't modify state (read-only)
   * @param {StateMatrix} stateMatrix
   */
  apply(stateMatrix) {
    // ADC is read-only - just records F0
    // The Sequence will handle recording the signal
  }

  toString() {
    return `ADC${this.label ? `(${this.label})` : ''}`;
  }
}
