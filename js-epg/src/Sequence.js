/**
 * Sequence - Orchestrates pulse sequence simulation
 * Uses builder pattern for intuitive API
 */

import { StateMatrix } from './StateMatrix.js';
import { ADC } from './operators/ADC.js';
import { GradientShift } from './operators/GradientShift.js';

export class Sequence {
  /**
   * Create a new pulse sequence
   * @param {number} initialNState - Initial number of phase states (default 1)
   */
  constructor(initialNState = 1) {
    this.operators = [];
    this.initialNState = initialNState;
  }

  /**
   * Add an operator to the sequence (builder pattern)
   * @param {Operator} operator
   * @returns {Sequence} this (for chaining)
   */
  add(operator) {
    this.operators.push(operator);
    return this;
  }

  /**
   * Add multiple operators to the sequence
   * @param {Array<Operator>} operators
   * @returns {Sequence} this (for chaining)
   */
  addAll(operators) {
    this.operators.push(...operators);
    return this;
  }

  /**
   * Simulate the sequence with given initial magnetization
   * @param {{state: StateMatrix, nState: number}} options
   * @returns {{signals: Array, labels: Array, stateHistory: Array}}
   */
  simulate(options = {}) {
    const { state: initialState, nState: providedNState } = options;

    // Determine nState
    let nState = providedNState || this.initialNState;

    // Auto-detect nState from gradient shifts if not provided
    if (!providedNState) {
      for (let op of this.operators) {
        if (op instanceof GradientShift) {
          nState = Math.max(nState, nState + Math.abs(op.k));
        }
      }
    }

    // Initialize state matrix
    let stateMatrix = initialState || new StateMatrix(nState);

    // Storage for results
    const signals = [];
    const labels = [];
    const stateHistory = [];

    // Process each operator
    for (let operator of this.operators) {
      // Apply operator
      operator.apply(stateMatrix);

      // If it's an ADC, record the signal
      if (operator instanceof ADC) {
        signals.push({ ...stateMatrix.F0() }); // Copy F0
        labels.push(operator.label);
        stateHistory.push(stateMatrix.clone());
      }
    }

    return {
      signals,
      labels,
      stateHistory,
      finalState: stateMatrix.clone()
    };
  }

  toString() {
    let result = `Sequence with ${this.operators.length} operators:\n`;
    for (let i = 0; i < this.operators.length; i++) {
      result += `  ${i}: ${this.operators[i].toString()}\n`;
    }
    return result;
  }
}
