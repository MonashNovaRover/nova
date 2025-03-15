import {useGenericStore} from "../../../hooks/useGenericStore.ts";
import {NIRProbeCalibrationData} from "../../../redux/models/genericStores/NIRProbeCalibrationData.ts";
import {NIRProbeReadingType} from "../SpaceResourcesSiteType.tsx";
import {useNIRSiteData} from "../useNIRSiteData.ts";
import {useMemo} from "react";

/**
 * Number of coefficients required for the calibration function
 */
export const COEFFICIENT_QUANTITY = 6;

const absorbCoef = 3000

/**
 * Takes in the difference between reading and light blank and applies the offset and absorbance function
 * @param waterOffset offset of the raw water difference readings
 * @param iceOffset offset of the raw ice difference readings
 * @param type reading type (Water or Ice)
 * @param data nir probe reading
 */
export const absorbance = (waterOffset: number, iceOffset: number) => (type: NIRProbeReadingType, data: number) => {
  const offset = type === NIRProbeReadingType.WATER ? waterOffset : iceOffset;
  return Math.log10(absorbCoef / (data + offset));
}

/**
 * Absorbance function with the current offsets stored in the generic store.
 */
export const useAbsorbance = () => {
  const [calibrationData, _] = useGenericStore<NIRProbeCalibrationData>("nirProbeCalibrationData");
  return absorbance(calibrationData.xOffset, calibrationData.yOffset);
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
    return 0;
  }

  const c = coef.map(v => Number.isNaN(v) ? 0 : v)

  return (c[2] * Math.log10(x + y)
  + c[5]);
}

/**
 * Calibration function with the current coefficients stored in the generic store.
 */
export const useCalibrationFunction = () => {
  const [calibrationData, _] = useGenericStore<NIRProbeCalibrationData>("nirProbeCalibrationData");
  return calibrationFunction(calibrationData.coefficients);
}

/**
 * Calculates the average reading
 * returns an array containing:
 *  - averageX
 *  - averageY
 *  - calibratedResult
 */
export const useAverageReading = (): [number, number, number] => {
  const [readings, _] = useNIRSiteData()
  const calibrationFunc = useCalibrationFunction()

  // average water reading
  const averageX = useMemo(() => {
    const xList = readings[NIRProbeReadingType.WATER]
      .map(entry => entry.data)
    return xList.reduce((a, b) => a + b, 0) / Math.max(xList.length, 1)
  }, [readings])

  // average ice reading
  const averageY = useMemo(() => {
    const yList = readings[NIRProbeReadingType.ICE]
      .map(entry => entry.data)
    return yList.reduce((a,b) => a+b, 0) / Math.max(yList.length,1)
  }, [readings])

  const calibratedResult = calibrationFunc(averageX, averageY)

  return [averageX, averageY, calibratedResult]
}
