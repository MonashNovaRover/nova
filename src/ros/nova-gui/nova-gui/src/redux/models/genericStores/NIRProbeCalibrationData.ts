export interface NIRProbeCalibrationData {
  coefficients: number[]
  xOffset: number,
  yOffset: number,
  xRange: number[]
  yRange: number[]
}

export const DEFAULT_NIR_PROBE_CALIBRATION_DATA: NIRProbeCalibrationData = {
  coefficients: [0.5, 6, 100, 24, 4.5, 10],
  xOffset: 300,
  yOffset: -300,
  xRange: [0, 1300],
  yRange: [0, 1300],
}
