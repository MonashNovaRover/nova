import ReactApexChart from "react-apexcharts";
import {ApexOptions} from "apexcharts";

export interface AnalysisArmDiagramProps {
  // Percentage the analysis arm is up (100 = top)
  percent: number // Integer between [0, 100]
  // Target percentage (100 = top)
  target: number // Integer between [0, 100]
}

/**
 * Diagram providing visualisation of the current state of the analysis arm.
 * @param props
 * @constructor
 */
const AnalysisArmDiagram: React.FC<AnalysisArmDiagramProps> = ({percent, target}: AnalysisArmDiagramProps) => {

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
            borderColor: "#666",
            style: {
              color: "#000",
              fontSize: "12px",
            },
          },
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