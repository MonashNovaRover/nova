import {Button, Input, Modal, ModalBody, ModalContent, ModalHeader} from "@nextui-org/react";
import React from "react";
import {useGenericStore} from "../../../hooks/useGenericStore.ts";
import {
  DEFAULT_NIR_PROBE_CALIBRATION_DATA,
  NIRProbeCalibrationData
} from "../../../redux/models/genericStores/NIRProbeCalibrationData.ts";

export interface NIRCalibrationSettingsModalProps {
  isOpen: boolean,
  setIsOpen: (v: boolean) => void,
}

/**
 * Settings modal for the NIR Calibration Curve 3D visualisation.
 * @param isOpen whether or not the modal is open
 * @param setIsOpen function to set the isOpen value
 * @constructor
 */
const NIR3DCurveSettingsModal: React.FC<NIRCalibrationSettingsModalProps> = ({
  isOpen, setIsOpen
}) => {

  const [calibrationData, setCalibrationData] = useGenericStore<NIRProbeCalibrationData>("nirProbeCalibrationData");

  /* Range Settings */

  const setXRange = (range: number[]) => {
    setCalibrationData({
      ...calibrationData,
      xRange: range,
    } as NIRProbeCalibrationData)
  }
  const setYRange = (range: number[]) => {
    setCalibrationData({
      ...calibrationData,
      yRange: range,
    } as NIRProbeCalibrationData)
  }

  const renderRanges = () => {
    return [
      <span key="xrange-title">X Range</span>,
      <div key="XRange-input" className="grid grid-cols-2 gap-3">
        <Input
          label="X Min"
          value={formatPotentiallyNaNFloatString(calibrationData.xRange[0])}
          onValueChange={(v: string) => setXRange([parseFloat(v), calibrationData.xRange[1]])}
        />
        <Input
          label="X Max"
          value={formatPotentiallyNaNFloatString(calibrationData.xRange[1])}
          onValueChange={(v: string) => setXRange([calibrationData.xRange[0], parseFloat(v)])}
        />
      </div>,
      <Button onClick={() => setXRange(DEFAULT_NIR_PROBE_CALIBRATION_DATA.xRange)} className="mb-4" key="reset-x-range">
        Reset X Range
      </Button>,
      <span key="yrange-title">Y Range</span>,
      <div className="grid grid-cols-2 gap-3" key="YRange-input">
        <Input
          label="Y Min"
          value={formatPotentiallyNaNFloatString(calibrationData.yRange[0])}
          onValueChange={(v: string) => setYRange([parseFloat(v), calibrationData.yRange[1]])}
        />
        <Input
          label="Y Max"
          value={formatPotentiallyNaNFloatString(calibrationData.yRange[1])}
          onValueChange={(v: string) => setYRange([calibrationData.yRange[0], parseFloat(v)])}
        />
      </div>,
      <Button onClick={() => setYRange(DEFAULT_NIR_PROBE_CALIBRATION_DATA.yRange)} className="mb-4" key="reset-y-range">
        Reset Y Range
      </Button>,
    ]
  }

  return (
    <Modal
      size="2xl"
      className="dark text-foreground"
      isOpen={isOpen}
      onClose={() => setIsOpen(false)}
    >
      <ModalContent>
        <ModalHeader className="flex flex-col gap-1">
          NIR Calibration Settings
        </ModalHeader>
        <ModalBody>
          {renderRanges()}
        </ModalBody>
      </ModalContent>
    </Modal>
  )
}

const formatPotentiallyNaNFloatString = (value: number) => isNaN(value) ? "" : `${value}`;

export default NIR3DCurveSettingsModal;
