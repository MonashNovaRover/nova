import {defaultCoefficients, defaultXRange, defaultYRange, defaultXOffset, defaultYOffset} from "../../../components/NIRProbe/NIRProbeCalibration/NIRCalibration.ts";

export interface NIRProbeCalibrationData {
  coefficients: number[]
  xOffset: number,
  yOffset: number,
  xRange: number[]
  yRange: number[]
}

export const DEFAULT_NIR_PROBE_CALIBRATION_DATA: NIRProbeCalibrationData = {
  coefficients: defaultCoefficients,
  xOffset: defaultXOffset,
  yOffset: defaultYOffset,
  xRange: defaultXRange,
  yRange: defaultYRange,
}
