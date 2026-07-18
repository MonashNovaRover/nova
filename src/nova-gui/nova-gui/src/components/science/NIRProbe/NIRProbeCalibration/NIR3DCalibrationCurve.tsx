import React from "react";

// There doesn't exist a typescript definition for this package
// I tried and failed to create a module type dependency.
// eslint-disable-next-line @typescript-eslint/ban-ts-comment
// @ts-ignore
import Plotly from "plotly.js-cartesian-dist-min";
import createPlotlyComponent from "react-plotly.js/factory";

const Plot = createPlotlyComponent(Plotly);

export interface SurfacePlotData {
  x: number[];
  y: number[];
  z: number[][];
}

export interface ScatterPlotData {
  x: number[];
  y: number[];
  text: string[];
}

export interface NIRContourCalibrationCurveProps {
  surfaceData: SurfacePlotData;
  readingsScatterData: ScatterPlotData;
  averageScatterData: ScatterPlotData;
}

const NIRContourCalibrationCurve: React.FC<NIRContourCalibrationCurveProps> = ({
                                                                                 surfaceData,
                                                                                 readingsScatterData,
                                                                                 averageScatterData,
                                                                               }) => {
  return (
    <Plot
      data={[
        {
          ...surfaceData,
          type: "contour",
          name: "Calibration",
          colorscale: [
            [0.0, "rgb(24,40,68)"],
            [0.16, "rgb(61,59,114)"],
            [0.32, "rgb(111,77,150)"],
            [0.48, "rgb(162,98,169)"],
            [0.64, "rgb(205,126,175)"],
            [0.8, "rgb(231,164,182)"],
            [1, "rgb(243,206,201)"],
          ],
          contours: {
            coloring: "heatmap", // fills between contour lines
            showlabels: true,
          },
          colorbar: {
            title: { text: "Concentration" },
          },
        },

        {
          ...readingsScatterData,
          type: "scatter",
          mode: "markers",
          name: "Readings",
          marker: {
            color: "#f5a524",
            size: 8,
          },
        },

        {
          ...averageScatterData,
          type: "scatter",
          mode: "markers",
          name: "Average",
          marker: {
            color: "#22d3ee",
            size: 10,
          },
        },
      ]}
      layout={{
        autosize: true,
        height: 600,
        paper_bgcolor: "#18191a",
        plot_bgcolor: "#18191a",

        margin: {
          t: 0,
          b: 40,
          l: 40,
          r: 0,
        },

        xaxis: {
          title: { text: "Water" },
        },

        yaxis: {
          title: { text: "Ice" },
        },

        showlegend: true,
        legend: {
          x: 0,
          y: 1,
        },
      }}
    />
  );
};

export default NIRContourCalibrationCurve;