import {useGenericStore} from "../../../hooks/useGenericStore.ts";
import {NIRProbeCalibrationData} from "../../../redux/models/genericStores/NIRProbeCalibrationData.ts";

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
export const calibrationFunction = (coef: number[]) => (x: number, y: number): number => {
  if (coef.length != COEFFICIENT_QUANTITY) {
    console.log("invalid number of c:", coef);
    return 0
  }

  const c = coef.map(v => Number.isNaN(v) ? 0 : v)

  // return //(c[0] * Math.log10(x+1)
    // + c[1] * Math.log10(y+1)
    return (c[2] * Math.log10(x + y)
    + c[5]);
}

/**
 * Calibration function with the current coefficients stored in the generic store.
 */
export const useCalibrationFunction = () => {
  const [calibrationData, _] = useGenericStore<NIRProbeCalibrationData>("nirProbeCalibrationData");
  return calibrationFunction(calibrationData.coefficients)
}
