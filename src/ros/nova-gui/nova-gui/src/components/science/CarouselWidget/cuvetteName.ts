/**
 * Pure logic for determining cuvette names based on carousel position.
 * Decoupled from React/context - can be tested independently.
 */

import { OUTER_CUVETTE_NAMES, INNER_CUVETTE_NAMES } from "./CarouselConfig.ts";

/**
 * Returns the appropriate cuvette name based on current carousel positions.
 *
 * Rules:
 * - Inner at position 12 (index 11) AND outer NOT at 22-24 → outer name
 * - Outer at positions 22-24 (indices 21-23) AND inner NOT at 12 → inner name
 * - Both conditions met OR neither met → undefined (empty)
 *
 * @param innerCuvette - 0-indexed inner ring position
 * @param outerCuvette - 0-indexed outer ring position
 * @returns The cuvette name to use, or undefined if no name applies
 */
export function getCuvetteName(
  innerCuvette: number,
  outerCuvette: number
): string | undefined {
  const innerAt12 = innerCuvette === 11;
  const outerAt22_24 = outerCuvette >= 21 && outerCuvette <= 23;

  // Both or neither → empty
  if (innerAt12 === outerAt22_24) {
    return "";
  }

  // Inner at 12 → use outer name
  if (innerAt12) {
    return OUTER_CUVETTE_NAMES[outerCuvette];
  }

  // Outer at 22-24 → use inner name
  if (outerAt22_24) {
    return INNER_CUVETTE_NAMES[innerCuvette];
  }

  return undefined;
}
