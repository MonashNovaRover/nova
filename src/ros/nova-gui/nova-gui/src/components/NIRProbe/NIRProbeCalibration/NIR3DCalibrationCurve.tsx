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
  readingsScatterData: Scatter3DPlotData,
  averageScatterData: Scatter3DPlotData,
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
  text: string[],
}

/**
 * 3D Visualisation of the Calibration Curve
 */
const NIR3DCalibrationCurve: React.FC<NIR3DCalibrationCurveProps> = ({ surfaceData, readingsScatterData, averageScatterData }) => {
  return (
    <Plot
      data={[
        // docs for surface struct type: https://plotly.com/javascript/reference/surface/
        {
          ...surfaceData,
          type: 'surface',
          name: 'Calibration',
          colorscale: [
            [0.0, 'rgb(24,40,68)'],
            [0.16, 'rgb(61,59,114)'],
            [0.32, 'rgb(111,77,150)'],
            [0.48, 'rgb(162,98,169)'],
            [0.64, 'rgb(205,126,175)'],
            [0.80 , 'rgb(231,164,182)'],
            [1, 'rgb(243,206,201)'],
          ],
        },
        // docs for scatter 3d struct type: https://plotly.com/javascript/reference/scatter3d/
        {
          ...readingsScatterData,
          type: 'scatter3d',
          mode: 'markers',
          name: 'Readings',
          marker: {
            color: '#3c1c88',
          },
        },
        {
          ...averageScatterData,
          type: 'scatter3d',
          mode: 'markers',
          name: 'Average',
          marker: {
            color: '#0b8399'
          }
        },
      ]}
      layout={
        {
          autosize: true,
          height: 600,
          paper_bgcolor: "#18191a",
          plot_bgcolor: "#18191a",
          margin: {
            t: 0,
            b: 0,
            l: 0,
            r: 0,
          },
          showlegend: true,
          legend: {
            x: 0,
            y: 0,
          },
        }
      }
    />
  )
}

export default NIR3DCalibrationCurve;
