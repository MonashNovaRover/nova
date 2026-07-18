/**
 * NIR Probe threshold and calculation configuration
 *
 * PD1 measures thymine
 * PD2 measures ammonia
 *
 * Logic:
 * - Reading ABOVE threshold → Show concentration
 * - Reading BELOW threshold → Show "Not detected"
 */

// ============================================================================
// THRESHOLDS - Modify these values to change detection thresholds
// ============================================================================

/**
 * Threshold for PD1 (thymine detection)
 */
export function getPD1Threshold(): number {
  return 10000;
}

/**
 * Threshold for PD2 (ammonia detection)
 */
export function getPD2Threshold(): number {
  return 20000;
}

// ============================================================================
// CONCENTRATION CALCULATIONS - Modify these formulas as needed
// ============================================================================

/**
 * Converts PD1 reading to thymine concentration (μg/g)
 * @param reading - Raw PD1 reading value
 * @returns Concentration in μg/g
 */
export function calculateThymineConcentration(reading: number): number {
  // Placeholder formula: can be replaced with calibrated conversion
  // Example: linear conversion from reading range to concentration range
  const conversionFactor = 0.001; // Adjust this based on calibration
  return reading * conversionFactor;
}

/**
 * Converts PD2 reading to ammonia concentration (μg/g)
 * @param reading - Raw PD2 reading value
 * @returns Concentration in μg/g
 */
export function calculateAmmoniaConcentration(reading: number): number {
  // Placeholder formula: can be replaced with calibrated conversion
  // Example: linear conversion from reading range to concentration range
  const conversionFactor = 0.0015; // Adjust this based on calibration
  return reading * conversionFactor;
}

// ============================================================================
// RESULT DETERMINATION - Core logic for threshold-based detection
// ============================================================================

export interface NIRResult {
  detected: boolean;
  concentration?: number;
  displayText: string;
}

/**
 * Determines thymine detection result based on PD1 reading
 * @param reading - Average PD1 reading
 * @returns Result object with detection status and display text
 */
export function getThymineResult(reading: number): NIRResult {
  const threshold = getPD1Threshold();

  if (reading > threshold) {
    const concentration = calculateThymineConcentration(reading);
    return {
      detected: true,
      concentration,
      displayText: `${concentration.toFixed(2)} μg/g`
    };
  } else {
    return {
      detected: false,
      displayText: "Not detected"
    };
  }
}

/**
 * Determines ammonia detection result based on PD2 reading
 * @param reading - Average PD2 reading
 * @returns Result object with detection status and display text
 */
export function getAmmoniaResult(reading: number): NIRResult {
  const threshold = getPD2Threshold();

  if (reading > threshold) {
    const concentration = calculateAmmoniaConcentration(reading);
    return {
      detected: true,
      concentration,
      displayText: `${concentration.toFixed(2)} μg/g`
    };
  } else {
    return {
      detected: false,
      displayText: "Not detected"
    };
  }
}

// ============================================================================
// DISPLAY LABELS - Customize how results are labeled
// ============================================================================

/**
 * Gets the display name for PD1 measurement
 */
export function getPD1Label(): string {
  return "Thymine";
}

/**
 * Gets the display name for PD2 measurement
 */
export function getPD2Label(): string {
  return "Ammonia";
}
