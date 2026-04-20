/**
 * Base Operator class
 * All pulse sequence operators extend this
 */

export class Operator {
  /**
   * Apply this operator to a StateMatrix
   * @abstract
   * @param {StateMatrix} stateMatrix
   */
  apply(stateMatrix) {
    throw new Error('apply() must be implemented by subclass');
  }

  /**
   * Get a human-readable name for this operator
   * @returns {string}
   */
  name() {
    return this.constructor.name;
  }

  /**
   * Get a string representation for debugging
   * @abstract
   * @returns {string}
   */
  toString() {
    return this.name();
  }
}
