/*
 * Presets for chart styling
 * Author: Connor Macdougall
 * Modified by: Binuda Kalugalage
*/

import { ApexOptions } from "apexcharts";

export enum ChartStyle {
  Temperature,
}

const getYAxisLabel = (
  style: ChartStyle
): string => {
  switch (style) {
    case ChartStyle.Temperature:
      return "Temperature (°C)";
    default:
      return "Value";
  }
};

export const ChartOptions = (
  style: ChartStyle,
  seriesName: string
): ApexOptions => {
  return {
    title: {
      text: seriesName,
      align: "center",
      floating: true,
      style: {
        fontSize: "18px",
        color: "#fff",
      },
    },
    stroke: {
      curve: "smooth",
    },
    chart: {
      animations: {
        enabled: false,
      },
      type: "line",
      background: "000",
      toolbar: {
        show: false,
      },
      zoom: {
        enabled: false,
      },
    },
    tooltip: {
      theme: "dark",
      fixed: {
        offsetX: 10,
        offsetY: 10,
      },
      shared: true,
      intersect: false,
      x: {
        format: "HH:mm:ss",
      },
    },
    xaxis: {
      type: "datetime",
      labels: {
        show: true,
        datetimeUTC: false,
      },
      title: {
        text: "Time (s)",
        style: {
          fontSize: "14px",
          color: "#fff",
        },
      },
    },
    yaxis: {
      min: (min) =>
        Math.floor(min - 10),
      max: (max) =>
        Math.ceil(max + 10),
      forceNiceScale: true,
      title: {
        text: getYAxisLabel(style),
        style: {
          fontSize: "14px",
          color: "#fff",
        },
      },
      labels: {
        show: true,
        style: {
          colors: "#fff",
        },
      },
    },
    grid: {
      show: false,
    },
    colors: ["#992F7B", "#C4841D"],
  };
};