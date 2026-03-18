import {useGenericStore} from "../../../../hooks/useGenericStore.ts";
import {NIRProbeCalibrationData} from "../../../../redux/models/genericStores/NIRProbeCalibrationData.ts";
import {NIRProbeReadingType} from "../SpaceResourcesSiteType.tsx";
import {useNIRSiteData} from "../useNIRSiteData.ts";
import {useMemo} from "react";

/**
 * Number of coefficients required for the calibration function
 */
export const COEFFICIENT_QUANTITY = 5;

export const defaultCoefficients = [
  1.59e-8,
  3.57,
  -82092.70,
  0.000289,
  3.2,
  
]

export const defaultXRange = [10000, 27000]
export const defaultYRange = [0, 27000]

export const defaultXOffset = 0;
export const defaultYOffset = 0;

// const absorbCoef = 3000

/**
 * Takes in the difference between reading and light blank and applies the offset and absorbance function
 * @param waterOffset offset of the raw water difference readings
 * @param iceOffset offset of the raw ice difference readings
 * @param type reading type (Water or Ice)
 * @param data nir probe reading
 */
export const absorbance = (waterOffset: number, iceOffset: number) => (type: NIRProbeReadingType, data: number) => {
  const offset = type === NIRProbeReadingType.PD1 ? waterOffset : iceOffset;
  return data + offset
  // return Math.log10(absorbCoef / (data + offset));
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

  return (
    c[0] * Math.abs(c[1] * x + y + c[2])**2 
    + c[3] * Math.abs(c[1] * x + y + c[2])
    + c[4]
  );
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
    const xList = readings[NIRProbeReadingType.PD1]
      .map(entry => entry.data)
    return xList.reduce((a, b) => a + b, 0) / Math.max(xList.length, 1)
  }, [readings])

  // average ice reading
  const averageY = useMemo(() => {
    const yList = readings[NIRProbeReadingType.PD2]
      .map(entry => entry.data)
    return yList.reduce((a,b) => a+b, 0) / Math.max(yList.length,1)
  }, [readings])

  const calibratedResult = calibrationFunc(averageX, averageY)

  // give within range average if no values
  if (readings[NIRProbeReadingType.PD1].length === 0 || readings[NIRProbeReadingType.PD2].length === 0) {
    return [defaultXRange[0], defaultYRange[0], 0]
  }

  return [averageX, averageY, calibratedResult]
}
