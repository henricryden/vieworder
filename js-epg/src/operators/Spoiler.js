/**
 * Spoiler Operator - Destroys transverse magnetization
 */

import { Operator } from './Operator.js';

export class Spoiler extends Operator {
  /**
   * Create spoiler operator
   */
  constructor() {
    super();
  }

  /**
   * Apply spoiler: zero all transverse magnetization (F+ and F-)
   * @param {StateMatrix} stateMatrix
   */
  apply(stateMatrix) {
    stateMatrix.spoil();
  }

  toString() {
    return 'Spoiler';
  }
}
