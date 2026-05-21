/**
 * Configuration for UV-Vis chemicals and their measurement types
 */

export enum MeasurementType {
  INTENSITY = "intensity",
  ABSORBANCE = "absorbance"
}

export const CHEMICALS = [
  "Nile Red",
  "Ninhydrin",
  "Molish",
  "UV Fluor"
] as const;

export type Chemical = typeof CHEMICALS[number];

/**
 * Maps each chemical to its measurement type
 * - INTENSITY: Raw fluorescence/luminance (blank should NOT be applied)
 * - ABSORBANCE: Requires blank correction (blank should be applied)
 */
export const CHEMICAL_MEASUREMENT_TYPE: Record<Chemical, MeasurementType> = {
  "Nile Red": MeasurementType.INTENSITY,
  "Ninhydrin": MeasurementType.ABSORBANCE,
  "Molish": MeasurementType.ABSORBANCE,
  "UV Fluor": MeasurementType.INTENSITY,
};

/**
 * Detects the chemical from a spectrum name
 * @param name - Spectrum name (e.g., "I1 - Nile Red S1 1")
 * @returns The detected chemical or null if not found
 */
export function detectChemicalFromName(name: string): Chemical | null {
  for (const chemical of CHEMICALS) {
    if (name.includes(chemical)) {
      return chemical;
    }
  }
  return null;
}

/**
 * Determines if a blank should be applied for the given spectrum name
 * @param name - Spectrum name
 * @returns true if blank should be applied (absorbance), false for intensity
 */
export function shouldApplyBlank(name: string): boolean {
  const chemical = detectChemicalFromName(name);
  if (!chemical) return false;
  return CHEMICAL_MEASUREMENT_TYPE[chemical] === MeasurementType.ABSORBANCE;
}

/**
 * Gets the measurement type for a given spectrum name
 * @param name - Spectrum name
 * @returns The measurement type or INTENSITY as default
 */
export function getMeasurementType(name: string): MeasurementType {
  const chemical = detectChemicalFromName(name);
  if (!chemical) return MeasurementType.INTENSITY;
  return CHEMICAL_MEASUREMENT_TYPE[chemical];
}
