import React from "react";
import Plot from 'react-plotly.js';

export interface NIR3DCalibrationCurveProps {
  xValues: number[],
  yValues: number[],
  zValues: number[][],
}

/**
 * 3D Visualisation of the Calibration Curve
 */
const NIR3DCalibrationCurve: React.FC<NIR3DCalibrationCurveProps> = ({ xValues, yValues, zValues }) => {
  return (
      <Plot
        data={[
          {
            x: xValues,
            y: yValues,
            z: zValues,
            type: 'surface',
          },
        ]}
        layout={
          {
            autosize: true,
            paper_bgcolor: "#18191a",
            plot_bgcolor: "#18191a",
            margin: {
              t: 0,
              b: 0,
              l: 0,
              r: 0,
            }
          }
        }
      />
  )
}

export default NIR3DCalibrationCurve;
