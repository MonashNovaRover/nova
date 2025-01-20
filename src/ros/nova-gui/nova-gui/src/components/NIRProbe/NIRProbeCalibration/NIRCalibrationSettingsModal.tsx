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
};

/**
 * Settings modal for the NIR Calibration Curve 3D visualisation.
 * @param isOpen whether or not the modal is open
 * @param setIsOpen function to set the isOpen value
 * @constructor
 */
const NIRCalibrationSettingsModal: React.FC<NIRCalibrationSettingsModalProps> = ({
  isOpen, setIsOpen
}) => {

  const [calibrationData, setCalibrationData] = useGenericStore<NIRProbeCalibrationData>("nirProbeCalibrationData");

  /* Coefficient Settings */

  const setCoefficient = (i: number) => (newVal: string) => {
    setCalibrationData({
      ...calibrationData,
      coefficients: calibrationData.coefficients.map((v, index) => index == i ? parseFloat(newVal) : v)
    } as NIRProbeCalibrationData)
  }

  const renderCoefficients = () => {
    return calibrationData.coefficients.map((c, i) => <Input
      type="number"
      step="0.1"
      label={"Coefficient " + `${i+1}`}
      value={formatPotentiallyNaNFloatString(c)}
      onValueChange={setCoefficient(i)}
    />)
  }

  const resetCoefficients = () => {
    setCalibrationData({
      ...calibrationData,
      coefficients: DEFAULT_NIR_PROBE_CALIBRATION_DATA.coefficients,
    } as NIRProbeCalibrationData)
  }

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
      <div className="grid grid-cols-2 gap-3">
        <Input
          label={"X Min"}
          value={formatPotentiallyNaNFloatString(calibrationData.xRange[0])}
          onValueChange={(v: string) => setXRange([parseFloat(v), calibrationData.xRange[1]])}
        />
        <Input
          label={"X Min"}
          value={formatPotentiallyNaNFloatString(calibrationData.xRange[1])}
          onValueChange={(v: string) => setXRange([calibrationData.xRange[0], parseFloat(v)])}
        />
      </div>,
      <Button onClick={() => setXRange(DEFAULT_NIR_PROBE_CALIBRATION_DATA.xRange)} className="mb-4">
        Reset X Range
      </Button>,
      <div className="grid grid-cols-2 gap-3">
        <Input
          label={"Y Min"}
          value={formatPotentiallyNaNFloatString(calibrationData.yRange[0])}
          onValueChange={(v: string) => setYRange([parseFloat(v), calibrationData.yRange[1]])}
        />
        <Input
          label={"Y Min"}
          value={formatPotentiallyNaNFloatString(calibrationData.yRange[1])}
          onValueChange={(v: string) => setYRange([calibrationData.yRange[0], parseFloat(v)])}
        />
      </div>,
      <Button onClick={() => setYRange(DEFAULT_NIR_PROBE_CALIBRATION_DATA.yRange)} className="mb-4">
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
        <ModalBody className="p-3 pt-0">
          <div className="grid grid-cols-3 gap-3">
            {renderCoefficients()}
          </div>
          <Button onClick={resetCoefficients} className="mb-4">
            Reset Coefficients
          </Button>
          {renderRanges()}
          <Button onClick={() => setCalibrationData(DEFAULT_NIR_PROBE_CALIBRATION_DATA)} className="bg-danger">
            Reset All
          </Button>
        </ModalBody>
      </ModalContent>
    </Modal>
  )
}

const formatPotentiallyNaNFloatString = (value: number) => isNaN(value) ? "" : `${value}`;

export default NIRCalibrationSettingsModal;
