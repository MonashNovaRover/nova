import React from "react";

// There doesn't exist a typescript definition for this package
// I tried and failed to create a module type dependency.
// eslint-disable-next-line @typescript-eslint/ban-ts-comment
// @ts-ignore
import Plotly from "plotly.js-gl3d-dist-min";
import createPlotlyComponent from 'react-plotly.js/factory';
const Plot = createPlotlyComponent(Plotly)
export interface NIR3DCalibrationCurveProps {
  surfaceData: SurfacePlotData,
  scatterData: Scatter3DPlotData,
}

// Data required to plot a react-plotly surface plot
export interface SurfacePlotData {
  x: number[],
  y: number[],
  z: number[][],
}

// Data required to plot a react-plotly surface 3d plot
export interface Scatter3DPlotData {
  x: number[],
  y: number[],
  z: number[],
}

/**
 * 3D Visualisation of the Calibration Curve
 */
const NIR3DCalibrationCurve: React.FC<NIR3DCalibrationCurveProps> = ({ surfaceData, scatterData }) => {
  return (
    <Plot
      data={[
        // docs for surface struct type: https://plotly.com/javascript/reference/surface/
        {
          ...surfaceData,
          type: 'surface',
        },
        // docs for scatter 3d struct type: https://plotly.com/javascript/reference/scatter3d/
        {
          ...scatterData,
          type: 'scatter3d',
        }
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
