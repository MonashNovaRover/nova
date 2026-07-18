import { useState } from "react";
import {
  Modal,
  ModalContent,
  ModalHeader,
  ModalBody,
  ModalFooter,
  Button,
  Input,
} from "@nextui-org/react";
import type { CalibrationOffsets } from "./potentiostatStorage.ts";

export interface ManualOffsetDialogProps {
  isOpen: boolean;
  onClose: () => void;
  channel: 1 | 2;
  currentOffsets: CalibrationOffsets | null;
  onSave: (voltage: number, current: number) => void;
}

export const ManualOffsetDialog = ({
  isOpen,
  onClose,
  channel,
  currentOffsets,
  onSave,
}: ManualOffsetDialogProps) => {
  const [voltageOffset, setVoltageOffset] = useState(
    currentOffsets?.voltageOffset.toString() || "0"
  );
  const [currentOffset, setCurrentOffset] = useState(
    currentOffsets?.currentOffset.toString() || "0"
  );

  // Track previous offsets to sync state when prop changes (React recommended pattern)
  const [prevOffsets, setPrevOffsets] = useState(currentOffsets);
  if (currentOffsets !== prevOffsets) {
    setPrevOffsets(currentOffsets);
    setVoltageOffset(currentOffsets?.voltageOffset.toString() || "0");
    setCurrentOffset(currentOffsets?.currentOffset.toString() || "0");
  }

  const handleSave = () => {
    const v = parseFloat(voltageOffset);
    const c = parseFloat(currentOffset);
    if (!isNaN(v) && !isNaN(c)) {
      onSave(v, c);
    }
  };

  const isValid = !isNaN(parseFloat(voltageOffset)) && !isNaN(parseFloat(currentOffset));

  return (
    <Modal isOpen={isOpen} onClose={onClose}>
      <ModalContent className="dark bg-content1">
        {(onClose) => (
          <>
            <ModalHeader>Set Manual Offset - {channel === 1 ? "Left" : "Right"}</ModalHeader>
            <ModalBody>
              <div className="flex flex-col gap-4">
                <Input
                  type="number"
                  label="Voltage Offset (V)"
                  value={voltageOffset}
                  onChange={(e) => setVoltageOffset(e.target.value)}
                  step="0.0001"
                  placeholder="0.0000"
                />
                <Input
                  type="number"
                  label="Current Offset (mA)"
                  value={currentOffset}
                  onChange={(e) => setCurrentOffset(e.target.value)}
                  step="0.0001"
                  placeholder="0.0000"
                />
                <p className="text-sm text-default-500">
                  Offsets are subtracted from raw measurements. Enter the values to shift the plot
                  through the origin (0,0).
                </p>
              </div>
            </ModalBody>
            <ModalFooter>
              <Button variant="light" onPress={onClose}>
                Cancel
              </Button>
              <Button color="primary" onPress={handleSave} isDisabled={!isValid}>
                Save Offset
              </Button>
            </ModalFooter>
          </>
        )}
      </ModalContent>
    </Modal>
  );
};
