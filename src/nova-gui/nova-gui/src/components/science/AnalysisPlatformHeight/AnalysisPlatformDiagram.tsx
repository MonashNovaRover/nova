import ReactApexChart from "react-apexcharts";
import {ApexOptions} from "apexcharts";

export interface AnalysisArmDiagramProps {
  // Percentage the analysis arm is up (100 = top)
  percent: number // Integer between [0, 100]
  // Target percentage (100 = top), out of range = no target e.g. -100
  target: number // Integer between [0, 100]
  // Distance from analysis arm to ground (float) in mm
  bottomDistance: number
  // Distance from top in mm.
  topDistance: number
}

/**
 * Diagram providing visualisation of the current state of the analysis arm.
 * @param props
 * @constructor
 */
const AnalysisArmDiagram: React.FC<AnalysisArmDiagramProps> = ({percent, target, bottomDistance, topDistance}: AnalysisArmDiagramProps) => {

  const series = [
    {
      type: "boxPlot" as const,
      data: [
        {
          x: "Sample",
          y: [-5, percent-5, percent+5, percent+5, 105], // min, Q1, median, Q3, max
        },
      ],
    },
  ];

  const options: ApexOptions = {
    chart: {
      type: "boxPlot",
      // height: 350,
      toolbar: { show: false },
      zoom: { enabled: false },
      selection: { enabled: false },
      animations: { enabled: false },
    },
    stroke: {
      width: 1,
      colors: ["#FFFFFF"],
    },
    tooltip: {
      enabled: false,
    },
    states: {
      hover: {
        filter: {
          type: "none",
        },
      },
      active: {
        filter: {
          type: "none",
        },
      },
    },
    dataLabels: {
      enabled: false,
    },

    markers: {
      hover: {
        size: 0,
      },
    },
    plotOptions: {
      boxPlot: {
        colors: {
          upper: "#CD7EAF",
          lower: "#CD7EAF",
        },
      },
    },
    grid: {
      borderColor: "#3E3F47", // grid line colour
      yaxis: {
        lines: {
          show: true,
        },
      },
    },
    xaxis: {
      type: "category",
    },
    yaxis: {
      min: -5,
      max: 105,
      labels: {
        formatter: (value) => {
          if (value === 105) {
            return "0";
          }
          else if (value === -5) {
            return 'max'
          }
          return "";
        },
      },
    },
    annotations: {
      yaxis: [
        {
          y: target, // exact Y value to attach to
          label: {
            text: "Target",
            borderWidth: 0,
            style: {
              background: "#26262a",
              color: "#edecee",
              fontSize: "14px",
            },
          },
        },
        {
          y: -4,
          label: {
            text: `${bottomDistance.toFixed(2)} mm`,
            offsetX: 32,
            borderWidth: 0,
            style: {
              background: "transparent",
              color: "#666",
              fontSize: "16px",
            },
          },
          borderColor: "transparent",
        },
        {
          y: 96,
          label: {
            text: `${topDistance.toFixed(2)} mm`,
            offsetX: 32,
            borderWidth: 0,
            style: {
              background: "transparent",
              color: "#666",
              fontSize: "16px",
            },
          },
          borderColor: "transparent",
        },
      ],
    },
  };

  return (
    <ReactApexChart
      options={options}
      series={series}
      type="boxPlot"
      height={350}
    />
  );
}

export default AnalysisArmDiagram