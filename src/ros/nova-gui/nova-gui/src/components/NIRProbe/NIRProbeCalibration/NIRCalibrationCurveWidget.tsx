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

const generateRange = (start: number, end: number, count: number): number[] => {
  if (Number.isNaN(start) || Number.isNaN(end))
      return []
  const step = (end - start) / (count - 1);
  return Array.from({ length: count }, (_, i) => Math.round(start + i * step));
}

const NIRCalibrationCurveWidget: React.FC<NIRCalibrationCurveWidgetProps> = () => {
  const [calibrationModalIsOpen, setCalibrationModalIsOpen] = useState<boolean>(false)
  const [calibrationData, _] = useGenericStore<NIRProbeCalibrationData>("nirProbeCalibrationData");

  const xValues = useMemo(() => generateRange(calibrationData.xRange[0], calibrationData.xRange[1], GRANUALITY), [calibrationData])
  const yValues = useMemo(() => generateRange(calibrationData.yRange[0], calibrationData.yRange[1], GRANUALITY), [calibrationData])

  const zValues = useMemo(() => {
    return yValues.map(y => [...xValues.map(x => calibrationFunction(calibrationData.coefficients)(x, y))])
  }, [calibrationData])

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
          zValues={zValues}
          xValues={xValues}
          yValues={yValues}
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