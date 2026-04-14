import {
  Button,
  Input,
  Modal,
  ModalBody,
  ModalContent,
  ModalFooter,
  ModalHeader,
} from "@nextui-org/react";
import React, { useEffect, useState } from "react";
import { useGenericStore } from "../../../hooks/useGenericStore.ts";
import {PUMPS} from "./PumpsWidget.tsx";

export interface PumpsModalProps {
  isOpen: boolean
  onOpenChange: (isOpen: boolean) => void
}


const PumpsModal: React.FC<PumpsModalProps> = ({isOpen, onOpenChange}: PumpsModalProps) => {
  const [defaultDurations, setDefaultDurations] = useGenericStore<Record<string, number>>("pumpDefaultDurations");
  const [editingDurations, setEditingDurations] = useState<Record<string, string>>({});

  // Initialize editing durations when modal opens
  useEffect(() => {
    if (isOpen) {
      const initial: Record<string, string> = {};
      PUMPS.forEach(pump => {
        initial[pump.value] = (defaultDurations[pump.value] ?? 10).toString();
      });
      setEditingDurations(initial);
    }
  }, [isOpen, defaultDurations]);

  const saveDefaultDurations = () => {
    const newDurations: Record<string, number> = {};
    PUMPS.forEach(pump => {
      const val = Number(editingDurations[pump.value]);
      newDurations[pump.value] = isNaN(val) || val <= 0 ? 10 : val;
    });
    setDefaultDurations(newDurations);
  };

  const handleEditingDurationChange = (pumpValue: string, value: string) => {
    setEditingDurations(prev => ({
      ...prev,
      [pumpValue]: value,
    }));
  };

  return (
    <Modal
      isOpen={isOpen}
      onOpenChange={onOpenChange}
      className="dark text-foreground"
    >
      <ModalContent>
        {(onClose) => (
          <>
            <ModalHeader>Default Pump Durations</ModalHeader>
            <ModalBody>
              <div className="grid grid-cols-2 gap-3">
                {PUMPS.map((pump) => (
                  <Input
                    key={pump.value}
                    label={pump.display}
                    type="number"
                    value={editingDurations[pump.value] ?? ""}
                    onValueChange={(val) => handleEditingDurationChange(pump.value, val)}
                    endContent={
                      <div className="pointer-events-none flex items-center">
                        <span className="text-default-400 text-small">s</span>
                      </div>
                    }
                  />
                ))}
              </div>
            </ModalBody>
            <ModalFooter>
              <Button color="danger" variant="light" onPress={onClose}>
                Cancel
              </Button>
              <Button
                color="primary"
                onPress={() => {
                  saveDefaultDurations();
                  onClose();
                }}
              >
                Save
              </Button>
            </ModalFooter>
          </>
        )}
      </ModalContent>
    </Modal>
  );
};

export default PumpsModal;
