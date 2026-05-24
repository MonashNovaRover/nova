import { useMemo } from "react";
import { useCarouselPosition } from "../CarouselWidget/CarouselPositionContext";
import { useGenericStore } from "../../../hooks/useGenericStore";
import {
  Chemical,
  isMlBasedPump,
  getInnerRingChemical,
  getOuterRingChemical,
  getInnerPumpCuvette,
  getOuterPumpCuvette,
} from "./pumpChemicalConfig";

export interface PumpMlTimingResult {
  /** The chemical at the pump position (offset from top cuvette), if applicable */
  chemical: Chemical | undefined;
  /** The calculated duration in seconds (ml × time_per_ml), if applicable */
  calculatedDuration: number | undefined;
  /** Whether this pump uses ml-based timing (true for non-prime ring pumps) */
  usesMlTiming: boolean;
  /** Time per ml for inner ring in seconds */
  timePerMlInner: number;
  /** Time per ml for outer ring in seconds */
  timePerMlOuter: number;
  /** Function to update time per ml values */
  setTimePerMl: (ring: "inner" | "outer", value: number) => void;
  /** Which ring this pump targets, if applicable */
  ring: "inner" | "outer" | undefined;
}

/**
 * Hook for calculating ml-based pump timing based on carousel position.
 *
 * For non-prime ring pumps (shot_to_inner_pump, shot_to_outer_pump):
 * - Calculates which cuvette is at the pump position (offset from top)
 * - Looks up the chemical for that cuvette
 * - Calculates duration as: ml_amount × time_per_ml
 *
 * For prime variants and other pumps:
 * - Returns usesMlTiming: false
 * - calculatedDuration is undefined
 */
export function usePumpMlTiming(pumpValue: string): PumpMlTimingResult {
  const carouselPosition = useCarouselPosition();
  const [defaultDurations, setDefaultDurations] = useGenericStore<Record<string, number>>("pumpDefaultDurations");

  // Get time per ml values from store (with defaults)
  const timePerMlInner = defaultDurations.timePerMlInner ?? 1.9;
  const timePerMlOuter = defaultDurations.timePerMlOuter ?? 1.9;

  // Function to update time per ml
  const setTimePerMl = (ring: "inner" | "outer", value: number) => {
    const key = ring === "inner" ? "timePerMlInner" : "timePerMlOuter";
    setDefaultDurations({
      ...defaultDurations,
      [key]: value,
    });
  };

  // Determine which ring this pump targets
  const ring = useMemo((): "inner" | "outer" | undefined => {
    if (pumpValue === "shot_to_inner_pump") return "inner";
    if (pumpValue === "shot_to_outer_pump") return "outer";
    return undefined;
  }, [pumpValue]);

  // Check if this pump uses ml-based timing
  const usesMlTiming = isMlBasedPump(pumpValue);

  // Get chemical based on ring and pump position (not top position)
  // The pump is at a fixed offset from the top cuvette position
  const chemical = useMemo((): Chemical | undefined => {
    if (!usesMlTiming || !carouselPosition) {
      return undefined;
    }

    if (ring === "inner") {
      const pumpCuvette = getInnerPumpCuvette(carouselPosition.innerCuvette);
      return getInnerRingChemical(pumpCuvette);
    } else if (ring === "outer") {
      const pumpCuvette = getOuterPumpCuvette(carouselPosition.outerCuvette);
      return getOuterRingChemical(pumpCuvette);
    }
    return undefined;
  }, [usesMlTiming, ring, carouselPosition]);

  // Calculate duration
  const calculatedDuration = useMemo((): number | undefined => {
    if (!usesMlTiming || !chemical) {
      return undefined;
    }

    const timePerMl = ring === "inner" ? timePerMlInner : timePerMlOuter;
    return chemical.mlAmount * timePerMl;
  }, [usesMlTiming, chemical, ring, timePerMlInner, timePerMlOuter]);

  return {
    chemical,
    calculatedDuration,
    usesMlTiming,
    timePerMlInner,
    timePerMlOuter,
    setTimePerMl,
    ring,
  };
}
