import {Button, Input, Modal, ModalBody, ModalContent, ModalHeader} from "@nextui-org/react";
import React, {useCallback, useMemo} from "react";
import {useGenericStore} from "../../../../hooks/useGenericStore.ts";
import {
  DEFAULT_NIR_PROBE_CALIBRATION_DATA,
  NIRProbeCalibrationData
} from "../../../../redux/models/genericStores/NIRProbeCalibrationData.ts";

export interface NIRSettingsModalProps {
  isOpen: boolean,
  setIsOpen: (v: boolean) => void,
}

/**
 * Settings modal for the NIR Calibration and Absorbance Function coefficients.
 * @param isOpen whether or not the modal is open
 * @param setIsOpen function to set the isOpen value
 * @constructor
 */
const NIRCalibrationSettingsModal: React.FC<NIRSettingsModalProps> = ({
  isOpen, setIsOpen
}) => {

  const [calibrationData, setCalibrationData] = useGenericStore<NIRProbeCalibrationData>("nirProbeCalibrationData");

  /* Coefficient Settings */

  const coefficientInputs = useMemo(() => {
    const setCoefficient = (i: number) => (newVal: string) => {
      setCalibrationData({
        ...calibrationData,
        coefficients: calibrationData.coefficients.map((v, index) => index == i ? parseFloat(newVal) : v)
      } as NIRProbeCalibrationData)
    }

    return calibrationData.coefficients.map((c, i) =>
      <Input
        key={`coefficient-${i}`}
        type="number"
        step="0.1"
        label={"Coefficient " + `${i + 1}`}
        value={formatPotentiallyNaNFloatString(c)}
        onValueChange={setCoefficient(i)}
      />)
  }, [calibrationData, setCalibrationData])

  const resetCoefficients = useCallback(() => {
    setCalibrationData({
      ...calibrationData,
      coefficients: DEFAULT_NIR_PROBE_CALIBRATION_DATA.coefficients,
    } as NIRProbeCalibrationData)
  }, [setCalibrationData, calibrationData])

  /* Offset settings */

  const offsetInputs = useMemo(() => {
    const setXOffset = (newVal: string) => {
      setCalibrationData({
        ...calibrationData,
        xOffset: parseFloat(newVal)
      } as NIRProbeCalibrationData)
    }

    const setYOffset = (newVal: string) => {
      setCalibrationData({
        ...calibrationData,
        yOffset: parseFloat(newVal)
      } as NIRProbeCalibrationData)
    }

    return [
      <Input
        key="x-offset"
        type="number"
        step="0.1"
        label="X-Offset"
        value={formatPotentiallyNaNFloatString(calibrationData.xOffset)}
        onValueChange={setXOffset}
      />,
      <Input
        key="y-offset"
        type="number"
        step="0.1"
        label="Y-Offset"
        value={formatPotentiallyNaNFloatString(calibrationData.yOffset)}
        onValueChange={setYOffset}
      />,
    ]
  }, [calibrationData, setCalibrationData])

  const resetOffsets = useCallback(() => {
    setCalibrationData({
      ...calibrationData,
      xOffset: DEFAULT_NIR_PROBE_CALIBRATION_DATA.xOffset,
      yOffset: DEFAULT_NIR_PROBE_CALIBRATION_DATA.yOffset,
    } as NIRProbeCalibrationData)
  }, [setCalibrationData, calibrationData])

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
          <span>Coefficients</span>
          <div className="grid grid-cols-3 gap-3">
            {coefficientInputs}
          </div>
          <Button onClick={resetCoefficients} className="mb-4">
            Reset Coefficients
          </Button>
          <span>Offsets</span>
          <div className="grid grid-cols-2 gap-3">
            {offsetInputs}
          </div>
          <Button onClick={resetOffsets} className="mb-4">
            Reset Offsets
          </Button>
        </ModalBody>
      </ModalContent>
    </Modal>
  )
}

const formatPotentiallyNaNFloatString = (value: number) => isNaN(value) ? "" : `${value}`;

export default NIRCalibrationSettingsModal;
