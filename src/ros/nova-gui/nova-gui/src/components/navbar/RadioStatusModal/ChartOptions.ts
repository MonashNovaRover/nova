/*
 * Presets for chart styling
 * Author: Connor Macdougall
 * Modified by: Binuda Kalugalage
*/

import { ApexOptions } from "apexcharts";

export enum ChartStyle {
  Default,
  Signal,
  Received,
  Sent,
  Ping,
}

const getYAxisLabel = (style: ChartStyle): string => {
  switch (style) {
    case ChartStyle.Signal:
      return "Decibels (dB)";
    case ChartStyle.Received:
    case ChartStyle.Sent:
      return "Bandwidth (kbps)";
    case ChartStyle.Ping:
      return "Time (ms)";
    default:
      return "Value";
  }
};

export const ChartOptions = (style: ChartStyle, seriesName: string, numPoints: number): ApexOptions => {
  return {
    title: {
      text: seriesName,
      align: "center",
      floating: true,
      style: {
        fontSize: '18px',
        color: '#fff'
      },
    },
    stroke: {
      curve: "smooth"
    },
    chart: {
      animations: { enabled: false },
      type: 'line',
      background: "000",
      toolbar: {
        show: false
      }
    },
    tooltip: {
      theme: "dark",
      fixed: {
        offsetX: 10,
        offsetY: 10
      },
      x: {
        format: "HH:mm:ss",
      },
    },
    xaxis: {
      type: 'datetime',
      title: {
        text: `Last ${numPoints} seconds`,
        style: {
          fontSize: '14px',
          color: '#fff'
        }
      },
      labels: {
        show: true,
        datetimeUTC: false,
      }
    },
    yaxis: {
      min: undefined,
      max: undefined,
      forceNiceScale: true,
      title: {
        text: getYAxisLabel(style),
        style: {
          fontSize: '14px',
          color: '#fff'
        }
      },
      labels: {
        show: false,
      }
    },
    grid: {
      show: false
    },
    colors: ["#992F7B", "#C4841D"],
  };
};
