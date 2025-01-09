export interface NIRProbeCalibrationData {
  coefficients: number[]
  zValues: number[][]
  xRange: number[]
  yRange: number[]
}

export const DEFAULT_NIR_PROBE_CALIBRATION_DATA: NIRProbeCalibrationData = {
  coefficients: [0.5, 6, 100],
  zValues: [],
  xRange: [0, 1300],
  yRange: [0, 1300],
}
