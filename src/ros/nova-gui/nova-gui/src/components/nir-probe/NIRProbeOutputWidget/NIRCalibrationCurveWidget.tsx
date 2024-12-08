import React, {useState} from "react";
import Chart from "react-apexcharts";
import {
  Button,
  Card,
  CardBody,
  CardHeader,
  Modal,
  ModalBody,
  ModalContent,
  ModalHeader,
} from "@nextui-org/react";
import {ISpaceResourcesFile, SpaceResourcesSiteType} from "../SpaceResourcesSiteType.tsx";
import NIRCalibrationSettingsTable from "./NIRCalibrationSettingsTable.tsx";
import {MoreHorizontal} from "react-feather";
import {SiteDataState} from "../../../redux/models/genericStores/SiteDataState.ts";

export interface NIRCalibrationCurveWidgetProps {
  files: SiteDataState,
  type: SpaceResourcesSiteType,
  absorbance: (v: number) => number,
  calibrationFunction: (v: number) => number,
  calibrationData: NIRCalibrationData,
  setCalibrationData: (newValue: NIRCalibrationData) => void,
}

export interface NIRCalibrationPoint {
  difference: number,
  concentration: number
}

export interface NIRCalibrationData {
  points: NIRCalibrationPoint[]
  yIntercept: number,
  gradient: number,
  chemBlankDifference: number,
}

export const EMPTY_CALIBRATION_DATA: NIRCalibrationData = {
  points: [],
  yIntercept: 0,
  gradient: 0,
  chemBlankDifference: 0
}

export const SITE_GRAPH_COLOURS = [
  "#f43f5e",
  "#f59e0b",
  "#0ea5e9",
  "#8b5cf6",
]

const NIRCalibrationCurveWidget: React.FC<NIRCalibrationCurveWidgetProps> = ({
  files, type, calibrationFunction, absorbance, calibrationData, setCalibrationData
}) => {
  const [calibrationModalIsOpen, setCalibrationModalIsOpen] = useState<boolean>(false)

  const typeName = type === SpaceResourcesSiteType.WATER ? "Water" : "Ilmenite"

  const options : ApexCharts.ApexOptions = {
    chart: {
      id: "basic-bar",
      type: 'line',
      foreColor: '#ccc',
    },
    xaxis: {
      tickAmount: 1,
      labels: {
        formatter: function(val) {
          return parseFloat(val).toFixed(1)
        }
      },
      title: {
        text: 'Concentration (%)'
      }
    },
    yaxis: {
      labels: {
        formatter: function(val) {
          return val.toFixed(2)
        }
      },
      title: {
        text: 'Absorbance'
      }
    },
    tooltip: {
      theme: 'dark'
    },
    stroke: {
      width: [5, 5, ...Object.entries(files).map(() => 0)],
      curve: 'straight',
      dashArray: [10, 0, ...Object.entries(files).map(() => 0)],
    },
    markers: {
      size: [0, 3, ...Object.entries(files).map(() => 5)],
      strokeColors: "#18181b",

    },
    grid: {
      borderColor: "#3f3f46"
    },

  };

  const relevantSiteFiles = Object.entries(files)
    .map(([filename, file], i) =>
      [filename, file, SITE_GRAPH_COLOURS[i]] as [string, ISpaceResourcesFile, string])
    .filter(([,file,]) => file.type === type)

  const fileSeries = relevantSiteFiles
    .map(([filename, file, color] : [string, ISpaceResourcesFile, string]) => ({
      name: filename,
      data: file.entries.map(
        ({difference, concentration}) =>
          [concentration ?? calibrationFunction(absorbance(difference)), absorbance(difference)]
      ),
      type: "scatter",
      color: color
    }))

  const fileAverageSeries = relevantSiteFiles
    .map(([filename, file, color] : [string, ISpaceResourcesFile, string]) => ({
      name: `${filename}-average`,
      data: [[
        file.entries.map(v => v.concentration ?? calibrationFunction(absorbance(v.difference))).reduce((a,b) => a+b, 0) / file.entries.length,
        file.entries.map(v => absorbance(v.difference)).reduce((a,b) => a+b, 0) / file.entries.length,
      ]],
      type: "scatter",
      color: color,
    }))

  const calibrationPoints = calibrationData.points.map(({difference, concentration}) => [concentration, absorbance(difference)]);

  const xMaxFileSeries = fileSeries
    .flatMap(({data}) => data.map(v => v[0]))
    .reduce((a, b) => Math.max(a,b), 0);
  const xMaxCalibration = calibrationPoints.map(v => v[0]).reduce((a, b) => Math.max(a,b), 0);
  const xMax = Math.max(xMaxFileSeries, xMaxCalibration);

  const series : ApexAxisChartSeries | ApexNonAxisChartSeries = [
    {
      name: "",
      data: [[0, calibrationData.yIntercept], [xMax, calibrationData.gradient*xMax + calibrationData.yIntercept]],
      color: "#10b98188",
      type: "line"
    },
    {
      name: "Calibration Data",
      data: calibrationPoints,
      color: "#10b981",
      type: "scatter"
    },
    ...fileSeries,
    ...fileAverageSeries
  ];

  return (
    <Card>
      <CardHeader className="pb-0 flex flex-row justify-center">
        <div className="grow">NIR {typeName} Calibration Curve</div>
        <Button
          variant={"light"}
          isIconOnly
          onPress={() => setCalibrationModalIsOpen(true)}
        >
          <MoreHorizontal/>
        </Button>
      </CardHeader>
      <CardBody>
        <Chart options={options}
               series={series}
        />
      </CardBody>

      <Modal
        size="2xl"
        className="dark text-foreground"
        isOpen={calibrationModalIsOpen}
        onClose={() => setCalibrationModalIsOpen(false)}
      >
        <ModalContent>
          {() => (
            <>
              <ModalHeader className="flex flex-col gap-1">
                NIR {typeName} Calibration Settings
              </ModalHeader>
              <ModalBody className="p-3 pt-0">
                <NIRCalibrationSettingsTable setData={setCalibrationData} data={calibrationData}/>
              </ModalBody>
            </>
          )}
        </ModalContent>
      </Modal>
    </Card>
  )
}

export default NIRCalibrationCurveWidget;