import React, {useMemo, useState} from "react";
import {Button, Card, CardBody, CardHeader,} from "@nextui-org/react";
import {MoreHorizontal} from "react-feather";
import NIR3DCalibrationCurve, {Scatter3DPlotData} from "./NIR3DCalibrationCurve.tsx";
import NIR3DCurveSettingsModal from "./NIR3DCurveSettingsModal.tsx";
import {useGenericStore} from "../../../../hooks/useGenericStore.ts";
import {NIRProbeCalibrationData} from "../../../../redux/models/genericStores/NIRProbeCalibrationData.ts";
import {useAverageReading, useCalibrationFunction} from "./NIRCalibration.ts";
import {useNIRSiteData} from "../useNIRSiteData.ts";
import {ISpaceResourcesEntry, NIRProbeReadingType} from "../SpaceResourcesSiteType.tsx";

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
  const calibrationFunc = useCalibrationFunction()
  const [readings, ,] = useNIRSiteData()
  const [averageX, averageY, calibratedResult] = useAverageReading()

  /* Plotting readings on the calibration curve */

  const readingsScatterData = useMemo(() => {
    const xAndAllY = readings[NIRProbeReadingType.PD1]
      .map(v => [v.data, readings[NIRProbeReadingType.PD2].filter(val => val.label === v.label)] as [number, ISpaceResourcesEntry[]])
      .filter(arr => arr[1].length > 0)
    const labels = xAndAllY
      .map(arr => arr[1][0].label)
    const valuesScatter = xAndAllY
      .map(arr => [arr[0], arr[1][0].data])
    const zValuesScatter = valuesScatter
      .map(v => v[0] !== undefined && v[1] !== undefined ? calibrationFunc(v[0], v[1]) : 0)

    return {x: valuesScatter.map(v => v[0]), y: valuesScatter.map(v => v[1]), z: zValuesScatter, text: labels} as Scatter3DPlotData
  }, [readings, calibrationFunc])

  /* Generating points on the calibration curve */

  const xValuesSurface = useMemo(() => generateRange(calibrationData.xRange[0], calibrationData.xRange[1], GRANUALITY), [calibrationData])
  const yValuesSurface = useMemo(() => generateRange(calibrationData.yRange[0], calibrationData.yRange[1], GRANUALITY), [calibrationData])

  // z values is a 2d array where each value is generated from the corresponding x and y values
  const zValuesSurface = useMemo(() => {
    return yValuesSurface.map(y => [...xValuesSurface.map(x => calibrationFunc(x, y))])
  }, [xValuesSurface, yValuesSurface, calibrationFunc])

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
          surfaceData={{x: xValuesSurface, y: yValuesSurface, z: zValuesSurface}}
          readingsScatterData={readingsScatterData}
          averageScatterData={{x: [averageX], y: [averageY], z: [calibratedResult], text: []}}
        />
      </CardBody>
      <NIR3DCurveSettingsModal
        isOpen={calibrationModalIsOpen}
        setIsOpen={setCalibrationModalIsOpen}
      />
    </Card>
  )
}

export default NIRCalibrationCurveWidget;