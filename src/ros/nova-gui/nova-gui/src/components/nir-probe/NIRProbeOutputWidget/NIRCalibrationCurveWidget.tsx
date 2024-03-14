import React, {useCallback, useState} from "react";
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
import {ISpaceResourcesFile} from "./NIRProbeWidget.tsx";
import SpaceResourceSiteType from "../SpaceResourcesSiteType.tsx";
import {useLocalStorage} from "../hooks/useLocalStorage.ts";
import NIRCalibrationSettingsTable from "./NIRCalibrationSettingsTable.tsx";
import {MoreHorizontal} from "react-feather";

export interface NIRCalibrationCurveWidgetProps {
  files: {[key: string] : ISpaceResourcesFile},
  type: SpaceResourceSiteType
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

const EMPTY_CALIBRATION_DATA: NIRCalibrationData = {
  points: [],
  yIntercept: 0,
  gradient: 0,
  chemBlankDifference: 0
}


const NIRCalibrationCurveWidget: React.FC<NIRCalibrationCurveWidgetProps> = ({files, type}) => {

  const [calibrationData, setCalibrationData] = useLocalStorage<NIRCalibrationData>(
    type === SpaceResourceSiteType.WATER ? "nir-calibration-water" : "nir-calibration-ilmenite",
    EMPTY_CALIBRATION_DATA,
    [type]
  );

  const [calibrationModalIsOpen, setCalibrationModalIsOpen] = useState<boolean>(false)

  const typeName = type === SpaceResourceSiteType.WATER ? "Water" : "Ilmenite"

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
      strokeColors: "#18181b"
    },
    grid: {
      borderColor: "#3f3f46"
    },

  };

  const relevantSiteFiles = Object.entries(files)
    .filter(([,file]) => file.type === type)

  // This function maps differences to predicted concentrations
  const calibrationFunction = useCallback((rawValue: number) => {
    return (rawValue - calibrationData.yIntercept) / calibrationData.gradient ;
  }, [calibrationData.gradient, calibrationData.yIntercept])




  const maxCalibrationDifference = calibrationData.points.map(v => v.difference).reduce((a, b) => Math.max(a,b), 0);


  const absorbance = useCallback((rawDifference: number) => {
    return Math.log10(maxCalibrationDifference / rawDifference);
  }, [calibrationData])

  const fileSeries = relevantSiteFiles
    .map(([filename, file] : [string, ISpaceResourcesFile]) => ({
      name: filename,
      data: file.entries.map(
        ({difference, concentration}) =>
          [concentration ?? calibrationFunction(absorbance(difference)), absorbance(difference)]
      ),
      type: "scatter"
    }))

  const calibrationPoints = calibrationData.points.map(({difference, concentration}) => [concentration, absorbance(difference)]);

  const xMaxFileSeries = fileSeries
    .flatMap(({data}) => data.map(v => v[0]))
    .reduce((a, b) => Math.max(a,b), 0);
  const xMaxCalibration = calibrationPoints.map(v => v[0]).reduce((a, b) => Math.max(a,b), 0);
  const xMax = Math.max(xMaxFileSeries, xMaxCalibration);

  const series : ApexAxisChartSeries | ApexNonAxisChartSeries = [
    {
      name: "Fit",
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
    ...fileSeries
  ];

  return (
    <Card>
      <CardHeader className="pb-0 flex flex-row justify-center">
        <div className="grow">{typeName} Calibration Curve</div>
        <Button
          variant={"light"}
          isIconOnly
          onClick={() => setCalibrationModalIsOpen(true)}
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