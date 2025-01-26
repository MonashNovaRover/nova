import React, {useMemo, useState} from "react";
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

/**
 * Generates a range of values
 * Array will be empty if start or end is NaN
 * @param start start point of range
 * @param end end point of range
 * @param count number of points within range
 */
const generateRange = (start: number, end: number, count: number): number[] => {
  if (Number.isNaN(start) || Number.isNaN(end))
      return []
  const step = (end - start) / (count - 1);
  return Array.from({ length: count }, (_, i) => Math.round(start + i * step));
}

/**
 * Widget containing the NIR Probe Calibration Curve
 * Contains a 3d graph and settings modal
 * @constructor
 */
const NIRCalibrationCurveWidget: React.FC<NIRCalibrationCurveWidgetProps> = () => {
  const [calibrationModalIsOpen, setCalibrationModalIsOpen] = useState<boolean>(false)
  const [calibrationData, _] = useGenericStore<NIRProbeCalibrationData>("nirProbeCalibrationData");

  /* Plotting readings on the calibration curve */

  // const [readings, ,] = useNIRSiteData()
  // const scatterData = useMemo(() => ({
  //   x: readings.map
  // } as Scatter3DPlotData), [readings])

  /* Generating points on the calibration curve */

  const xValues = useMemo(() => generateRange(calibrationData.xRange[0], calibrationData.xRange[1], GRANUALITY), [calibrationData])
  const yValues = useMemo(() => generateRange(calibrationData.yRange[0], calibrationData.yRange[1], GRANUALITY), [calibrationData])

  // z values is a 2d array where each value is generated from the corresponding x and y values
  const zValues = useMemo(() => {
    return yValues.map(y => [...xValues.map(x => calibrationFunction(calibrationData.coefficients)(x, y))])
  }, [calibrationData, xValues, yValues])

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
          surfaceData={{x: xValues, y: yValues, z: zValues}}
          scatterData={{x: [], y: [], z: []}}
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