import {useGenericStore} from "../../../hooks/useGenericStore.ts";
import {NIRProbeCalibrationData} from "../../../redux/models/genericStores/NIRProbeCalibrationData.ts";
import {NIRProbeReadingType} from "../SpaceResourcesSiteType.tsx";

/**
 * Number of coefficients required for the calibration function
 */
export const COEFFICIENT_QUANTITY = 6;

// TODO make part of calibration data
export const WATER_OFFSET = 300;
export const ICE_OFFSET = 250;

const absorbCoef = 3000

/**
 * Takes in the difference between reading and light blank and applies the offset and absorbance function
 * @param type reading type (Water or Ice)
 * @param data nir probe reading
 */
export const absorbance = (type: NIRProbeReadingType, data: number) => {
  const offset = type === NIRProbeReadingType.WATER ? WATER_OFFSET : ICE_OFFSET;
  return Math.log10(absorbCoef / (data + offset));
}


/**
 * calibration function
 * @param coef c for each
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
