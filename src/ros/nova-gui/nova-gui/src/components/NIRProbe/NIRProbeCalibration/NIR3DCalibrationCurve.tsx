import React from "react";
import Plot from 'react-plotly.js';

export interface NIR3DCalibrationCurveProps {
  data: number[][],
}

/**
 * 3D Visualisation of the Calibration Curve
 */
const NIR3DCalibrationCurve: React.FC<NIR3DCalibrationCurveProps> = ({ data }) => {
  return (
      <Plot
        data={[
          {
            z: data,
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
