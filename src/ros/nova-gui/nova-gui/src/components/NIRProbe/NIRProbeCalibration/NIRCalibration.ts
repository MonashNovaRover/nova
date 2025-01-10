/**
 * Number of coefficients required for the calibration function
 */
export const COEFFICIENT_QUANTITY = 6

/**
 * calibration function
 * @param c c for each
 * @param x difference in absorption for water readings
 * @param y difference in absorption for ice readings
 */
export const calibrationFunction = (c: number[]) => (x: number, y: number): number => {
  if (c.length != COEFFICIENT_QUANTITY) {
    console.log("invalid number of c:", c);
    return 0
  }

  return (c[0] * Math.log10(x+1)
    + c[1] * Math.log10(y+1)
    // + c[2] * Math.log10(x + y)
    // + c[3] * Math.log10(x - y)
    + c[5]) % 100
}
