/**
 * Complex number utilities for EPG simulation
 * Uses simple {real, imag} objects with helper functions
 */

export class Complex {
  /**
   * Create a complex number from real and imaginary parts
   * @param {number} real
   * @param {number} imag
   * @returns {{real: number, imag: number}}
   */
  static create(real, imag = 0) {
    return { real, imag };
  }

  /**
   * Complex zero
   * @returns {{real: number, imag: number}}
   */
  static zero() {
    return { real: 0, imag: 0 };
  }

  /**
   * Complex one
   * @returns {{real: number, imag: number}}
   */
  static one() {
    return { real: 1, imag: 0 };
  }

  /**
   * Imaginary unit i = 0 + 1i
   * @returns {{real: number, imag: number}}
   */
  static i() {
    return { real: 0, imag: 1 };
  }

  /**
   * Complex conjugate
   * @param {{real: number, imag: number}} c
   * @returns {{real: number, imag: number}}
   */
  static conjugate(c) {
    return { real: c.real, imag: -c.imag };
  }

  /**
   * Complex magnitude (absolute value)
   * @param {{real: number, imag: number}} c
   * @returns {number}
   */
  static magnitude(c) {
    return Math.sqrt(c.real * c.real + c.imag * c.imag);
  }

  /**
   * Complex phase (angle in radians)
   * @param {{real: number, imag: number}} c
   * @returns {number}
   */
  static phase(c) {
    return Math.atan2(c.imag, c.real);
  }

  /**
   * Complex addition
   * @param {{real: number, imag: number}} a
   * @param {{real: number, imag: number}} b
   * @returns {{real: number, imag: number}}
   */
  static add(a, b) {
    return { real: a.real + b.real, imag: a.imag + b.imag };
  }

  /**
   * Complex subtraction
   * @param {{real: number, imag: number}} a
   * @param {{real: number, imag: number}} b
   * @returns {{real: number, imag: number}}
   */
  static subtract(a, b) {
    return { real: a.real - b.real, imag: a.imag - b.imag };
  }

  /**
   * Complex multiplication
   * @param {{real: number, imag: number}} a
   * @param {{real: number, imag: number}} b
   * @returns {{real: number, imag: number}}
   */
  static multiply(a, b) {
    return {
      real: a.real * b.real - a.imag * b.imag,
      imag: a.real * b.imag + a.imag * b.real
    };
  }

  /**
   * Complex division
   * @param {{real: number, imag: number}} a
   * @param {{real: number, imag: number}} b
   * @returns {{real: number, imag: number}}
   */
  static divide(a, b) {
    const denom = b.real * b.real + b.imag * b.imag;
    return {
      real: (a.real * b.real + a.imag * b.imag) / denom,
      imag: (a.imag * b.real - a.real * b.imag) / denom
    };
  }

  /**
   * Scale complex number by real scalar
   * @param {{real: number, imag: number}} c
   * @param {number} scalar
   * @returns {{real: number, imag: number}}
   */
  static scale(c, scalar) {
    return { real: c.real * scalar, imag: c.imag * scalar };
  }

  /**
   * exp(c) = exp(a)(cos(b) + i*sin(b)) where c = a + i*b
   * @param {{real: number, imag: number}} c
   * @returns {{real: number, imag: number}}
   */
  static exp(c) {
    const expReal = Math.exp(c.real);
    return {
      real: expReal * Math.cos(c.imag),
      imag: expReal * Math.sin(c.imag)
    };
  }

  /**
   * String representation for debugging
   * @param {{real: number, imag: number}} c
   * @param {number} decimals
   * @returns {string}
   */
  static toString(c, decimals = 4) {
    const r = c.real.toFixed(decimals);
    const i = c.imag.toFixed(decimals);
    const sign = c.imag >= 0 ? '+' : '';
    return `(${r}${sign}${i}i)`;
  }
}
