import {Button, Input, Modal, ModalBody, ModalContent, ModalHeader} from "@nextui-org/react";
import React from "react";
import {useGenericStore} from "../../../hooks/useGenericStore.ts";
import {
  DEFAULT_NIR_PROBE_CALIBRATION_DATA,
  NIRProbeCalibrationData
} from "../../../redux/models/genericStores/NIRProbeCalibrationData.ts";

const ROW_SIZE = 3;

export interface NIRCalibrationSettingsModalProps {
  isOpen: boolean,
  setIsOpen: (v: boolean) => void,
};

const NIRCalibrationSettingsModal: React.FC<NIRCalibrationSettingsModalProps> = ({
  isOpen, setIsOpen
}) => {

  const [calibrationData, setCalibrationData] = useGenericStore<NIRProbeCalibrationData>("nirProbeCalibrationData");

  const setCoefficient = (i: number) => (newVal: string) => {
    setCalibrationData({
      ...calibrationData,
      coefficients: calibrationData.coefficients.map((v, index) => index == i ? parseFloat(newVal) : v)
    } as NIRProbeCalibrationData)
  }

  const renderCoefficients = () => {
    const inputs = calibrationData.coefficients.map((c, i) => <Input
      label={"Coefficient " + `${i+1}`}
      value={formatPotentiallyNaNFloatString(c)}
      onValueChange={setCoefficient(i)}
    />)

    return Array.from(
      { length: Math.ceil(inputs.length / ROW_SIZE) },
      (_, i) => <div className="flex flex-row gap-3">
        {inputs.slice(i * ROW_SIZE, i * ROW_SIZE + ROW_SIZE)}
      </div>
    );
  }

  const resetCoefficients = () => {
    setCalibrationData({
      ...calibrationData,
      coefficients: DEFAULT_NIR_PROBE_CALIBRATION_DATA.coefficients,
    } as NIRProbeCalibrationData)
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
          {renderCoefficients()}
          <Button onClick={resetCoefficients}>
            Reset Coefficients
          </Button>
        </ModalBody>
      </ModalContent>
    </Modal>
  )
}

const formatPotentiallyNaNFloatString = (value: number) => isNaN(value) ? "" : `${value}`;

export default NIRCalibrationSettingsModal;
