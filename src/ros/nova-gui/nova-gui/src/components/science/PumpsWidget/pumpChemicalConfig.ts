// ============================================================================
// PUMP CHEMICAL CONFIGURATION
// Edit these values to customize chemical names, ml amounts, and position mappings
// ============================================================================

export interface Chemical {
  id: string;
  name: string;
  mlAmount: number;
}

/**
 * List of all chemicals used in the carousel system.
 * Each chemical has:
 * - id: unique identifier used in position mappings
 * - name: display name for the chemical
 * - mlAmount: amount in ml to dispense
 */
export const CHEMICALS: Chemical[] = [
  { id: "nile_red", name: "Nile Red", mlAmount: 1.0 },        // 1mL (1.9s)
  { id: "ninhydrin", name: "Ninhydrin", mlAmount: 1.75 },     // 1.75mL (3.325s)
  { id: "nadh", name: "NADH", mlAmount: 3.0 },                // 3mL (5.7s)
  { id: "molisch", name: "Molisch's", mlAmount: 1.75 },       // 1.75mL (3.325s)
  { id: "blank", name: "Blank", mlAmount: 3.0 },              // 3mL (5.7s)
];

/**
 * Inner ring: 15 positions (0-14, displayed as I1-I15)
 * Maps position index to chemical id, or null if no chemical assigned
 *
 * Nile Red: I1-I3, I7-I9 (positions 0-2, 6-8)
 * Ninhydrin: I4-I6, I13-I15 (positions 3-5, 12-14)
 * Empty/controls: I10-I12 (positions 9-11)
 */
export const INNER_RING_CHEMICALS: (string | null)[] = [
  "nile_red",   // Position 0 (I1 - Nile Red SL 1)
  "nile_red",   // Position 1 (I2 - Nile Red SL 2)
  "nile_red",   // Position 2 (I3 - Nile Red SL 3)
  "ninhydrin",  // Position 3 (I4 - Ninhydrin SR 1)
  "ninhydrin",  // Position 4 (I5 - Ninhydrin SR 2)
  "ninhydrin",  // Position 5 (I6 - Ninhydrin SR 3)
  "nile_red",   // Position 6 (I7 - Nile Red SR 1)
  "nile_red",   // Position 7 (I8 - Nile Red SR 2)
  "nile_red",   // Position 8 (I9 - Nile Red SR 3)
  null,         // Position 9 (I10 - Nile Red neg 1) - control
  null,         // Position 10 (I11 - Nile Red pos 2) - control
  null,         // Position 11 (I12 - empty)
  "ninhydrin",  // Position 12 (I13 - Ninhydrin SL 1)
  "ninhydrin",  // Position 13 (I14 - Ninhydrin SL 2)
  "ninhydrin",  // Position 14 (I15 - Ninhydrin SL 3)
];

/**
 * Outer ring: 24 positions (0-23, displayed as O1-O24)
 * Maps position index to chemical id, or null if no chemical assigned
 *
 * Molisch's: O1-O3, O8-O10 (positions 0-2, 7-9)
 * NADH: O4-O6, O11-O13 (positions 3-5, 10-12)
 * Blank: O7, O14 (positions 6, 13)
 * Controls/empty: O15-O24 (positions 14-23)
 */
export const OUTER_RING_CHEMICALS: (string | null)[] = [
  "molisch",    // Position 0 (O1 - Molish SL 1)
  "molisch",    // Position 1 (O2 - Molish SL 2)
  "molisch",    // Position 2 (O3 - Molish SL 3)
  "nadh",       // Position 3 (O4 - NADH SL 1)
  "nadh",       // Position 4 (O5 - NADH SL 2)
  "nadh",       // Position 5 (O6 - NADH SL 3)
  "blank",      // Position 6 (O7 - Blank SL)
  "molisch",    // Position 7 (O8 - Molish SR 1)
  "molisch",    // Position 8 (O9 - Molish SR 2)
  "molisch",    // Position 9 (O10 - Molish SR 3)
  "nadh",       // Position 10 (O11 - NADH SR 1)
  "nadh",       // Position 11 (O12 - NADH SR 2)
  "nadh",       // Position 12 (O13 - NADH SR 3)
  "blank",      // Position 13 (O14 - Blank SR)
  null,         // Position 14 (O15 - Molish neg 1) - control
  null,         // Position 15 (O16 - Molish pos 2) - control
  null,         // Position 16 (O17 - UV Fluor neg 1) - control
  null,         // Position 17 (O18 - UV Fluor pos 2) - control
  null,         // Position 18 (O19 - Ninhydrin neg 1) - control
  null,         // Position 19 (O20 - Ninhydrin pos 2) - control
  null,         // Position 20 (O21 - Blank) - control
  null,         // Position 21 (empty)
  null,         // Position 22 (empty)
  null,         // Position 23 (empty)
];

/**
 * Pump values that use ml-based timing (non-prime ring pumps only)
 * Prime variants are excluded - they use time-based input
 */
export const ML_BASED_PUMPS = ["shot_to_inner_pump", "shot_to_outer_pump"];

/**
 * Pump position offsets from the "top" cuvette position.
 * The pump physically dispenses to the cuvette at this offset.
 * Based on indicator dot positions in CarouselConfig.ts:
 * - Inner pump at cuvette position 5 (1-indexed) → offset 4 (0-indexed)
 * - Outer pump at cuvette position 6 (1-indexed) → offset 5 (0-indexed)
 */
export const INNER_PUMP_OFFSET = 5;  // Pump at position 5
export const OUTER_PUMP_OFFSET = 6;  // Pump at position 6
export const INNER_RING_SIZE = 15;
export const OUTER_RING_SIZE = 24;

/**
 * Calculate which cuvette is at the pump position given the current top cuvette
 */
export function getInnerPumpCuvette(topCuvette: number): number {
  return (topCuvette + INNER_PUMP_OFFSET) % INNER_RING_SIZE;
}

export function getOuterPumpCuvette(topCuvette: number): number {
  return (topCuvette + OUTER_PUMP_OFFSET) % OUTER_RING_SIZE;
}

/**
 * Check if a pump uses ml-based timing
 * Returns true only for exact matches (not /prime variants)
 */
export function isMlBasedPump(pumpValue: string): boolean {
  return ML_BASED_PUMPS.includes(pumpValue);
}

/**
 * Get chemical for an inner ring position
 * @param position 0-indexed position (0-14)
 */
export function getInnerRingChemical(position: number): Chemical | undefined {
  if (position < 0 || position >= INNER_RING_CHEMICALS.length) {
    return undefined;
  }
  const chemicalId = INNER_RING_CHEMICALS[position];
  if (!chemicalId) {
    return undefined;
  }
  return CHEMICALS.find((c) => c.id === chemicalId);
}

/**
 * Get chemical for an outer ring position
 * @param position 0-indexed position (0-23)
 */
export function getOuterRingChemical(position: number): Chemical | undefined {
  if (position < 0 || position >= OUTER_RING_CHEMICALS.length) {
    return undefined;
  }
  const chemicalId = OUTER_RING_CHEMICALS[position];
  if (!chemicalId) {
    return undefined;
  }
  return CHEMICALS.find((c) => c.id === chemicalId);
}
