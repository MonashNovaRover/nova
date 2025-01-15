import React, {useEffect, useState} from "react";
import {
  Button,
  Card,
  CardBody,
  CardHeader,
} from "@nextui-org/react";
import {MoreHorizontal} from "react-feather";
import NIR3DCalibrationCurve from "./NIR3DCalibrationCurve.tsx";
import NIRCalibrationSettingsModal from "./NIRCalibrationSettingsModal.tsx";
import {useGenericStore} from "../../../hooks/useGenericStore.ts";
import {NIRProbeCalibrationData} from "../../../redux/models/genericStores/NIRProbeCalibrationData.ts";
import {calibrationFunction} from "./NIRCalibration.ts";

const GRANUALITY = 24

export interface NIRCalibrationCurveWidgetProps {
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

const generateRange = (start: number, end: number, count: number): number[] => {
  const step = (end - start) / (count - 1);
  return Array.from({ length: count }, (_, i) => Math.round(start + i * step));
}

const NIRCalibrationCurveWidget: React.FC<NIRCalibrationCurveWidgetProps> = () => {
  const [calibrationModalIsOpen, setCalibrationModalIsOpen] = useState<boolean>(false)
  const [calibrationData, setCalibrationData] = useGenericStore<NIRProbeCalibrationData>("nirProbeCalibrationData");

  // update xValues and yValues when the x and y range changes.
  useEffect(() => {
    setCalibrationData({
      ...calibrationData,
      xValues: generateRange(calibrationData.xRange[0], calibrationData.xRange[1], GRANUALITY),
      yValues: generateRange(calibrationData.yRange[0], calibrationData.yRange[1], GRANUALITY),
    } as NIRProbeCalibrationData)
  }, [calibrationData.yRange, calibrationData.xRange]);

  // update the z values when anything else changes
  useEffect(() => {
    setCalibrationData({
      ...calibrationData,
      zValues: calibrationData.yValues.map(y => [...calibrationData.xValues.map(x => calibrationFunction(calibrationData.coefficients)(x, y))]),
    } as NIRProbeCalibrationData)
    console.log("zvalues updated", calibrationData.zValues)
  }, [calibrationData.xValues, calibrationData.yValues, calibrationData.coefficients]);

  return (
    <Card>
      <CardHeader className="pb-0 flex flex-row justify-center">
        <div className="grow">NIR Calibration Curve</div>
        <Button
          variant={"light"}
          isIconOnly
          onPress={() => setCalibrationModalIsOpen(true)}
        >
          <MoreHorizontal/>
        </Button>
      </CardHeader>
      <CardBody>
        <NIR3DCalibrationCurve
          {...calibrationData}
        />
      </CardBody>
      <NIRCalibrationSettingsModal
        isOpen={calibrationModalIsOpen}
        setIsOpen={setCalibrationModalIsOpen}
      />
    </Card>
  )
}

export default NIRCalibrationCurveWidget;