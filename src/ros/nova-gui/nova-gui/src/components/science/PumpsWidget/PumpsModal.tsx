import {Button, Divider, Input, Modal, ModalBody, ModalContent, ModalFooter, ModalHeader,} from "@nextui-org/react";
import React, { useMemo, useState } from "react";
import { useGenericStore } from "../../../hooks/useGenericStore.ts";
import {PUMPS} from "./PumpsWidget.tsx";

// Non-prime ring pumps use ml-based timing, so they don't need default duration settings
// Prime variants still use time-based defaults
const ML_BASED_PUMP_VALUES = [
  "shot_to_inner_pump",
  "shot_to_outer_pump",
];

export interface PumpsModalProps {
  isOpen: boolean
  onOpenChange: (isOpen: boolean) => void
}

const PumpsModal: React.FC<PumpsModalProps> = ({isOpen, onOpenChange}: PumpsModalProps) => {
  const [defaultDurations, setDefaultDurations] = useGenericStore<Record<string, number>>("pumpDefaultDurations");

  // Filter out ml-based pumps - they use ml-based timing instead of default durations
  const timingBasedPumps = useMemo(() =>
    PUMPS.filter(pump => !ML_BASED_PUMP_VALUES.includes(pump.value)),
    []
  );

  // Converts default durations to a format that can be used by the Input component
  const getEditingDurations = (): Record<string, string> => {
    const initial: Record<string, string> = {};
    timingBasedPumps.forEach(pump => {
      initial[pump.value] = (defaultDurations[pump.value] ?? 10).toString();
    });
    return initial;
  }

  // Get time per ml values for editing
  const getEditingTimePerMl = (): { inner: string; outer: string } => ({
    inner: (defaultDurations.timePerMlInner ?? 1.9).toString(),
    outer: (defaultDurations.timePerMlOuter ?? 1.9).toString(),
  });

  const [editingDurations, setEditingDurations] = useState<Record<string, string>>(getEditingDurations());
  const [editingTimePerMl, setEditingTimePerMl] = useState(getEditingTimePerMl());

  const saveDefaultDurations = () => {
    const newDurations: Record<string, number> = {};
    timingBasedPumps.forEach(pump => {
      const val = Number(editingDurations[pump.value]);
      newDurations[pump.value] = isNaN(val) || val <= 0 ? 10 : val;
    });

    // Save time per ml values
    const innerVal = Number(editingTimePerMl.inner);
    const outerVal = Number(editingTimePerMl.outer);
    newDurations.timePerMlInner = isNaN(innerVal) || innerVal <= 0 ? 1.9 : innerVal;
    newDurations.timePerMlOuter = isNaN(outerVal) || outerVal <= 0 ? 1.9 : outerVal;

    setDefaultDurations(newDurations);
  };

  const handleEditingDurationChange = (pumpValue: string, value: string) => {
    setEditingDurations(prev => ({
      ...prev,
      [pumpValue]: value,
    }));
  };

  const handleTimePerMlChange = (ring: "inner" | "outer", value: string) => {
    setEditingTimePerMl(prev => ({
      ...prev,
      [ring]: value,
    }));
  };

  const resetState = () => {
    setEditingDurations(getEditingDurations());
    setEditingTimePerMl(getEditingTimePerMl());
  };

  return (
    <Modal
      isOpen={isOpen}
      onOpenChange={onOpenChange}
      className="dark text-foreground"
      size="lg"
    >
      <ModalContent>
        {(onClose) => (
          <>
            <ModalHeader>Pump Settings</ModalHeader>
            <ModalBody>
              {/* Time per ml section */}
              <div className="mb-2">
                <h4 className="text-sm font-semibold mb-2">Time per ml (for ring pumps)</h4>
                <div className="grid grid-cols-2 gap-3">
                  <Input
                    label="Inner Ring"
                    type="number"
                    step="0.1"
                    value={editingTimePerMl.inner}
                    onValueChange={(val) => handleTimePerMlChange("inner", val)}
                    endContent={
                      <div className="pointer-events-none flex items-center">
                        <span className="text-default-400 text-small">s/ml</span>
                      </div>
                    }
                  />
                  <Input
                    label="Outer Ring"
                    type="number"
                    step="0.1"
                    value={editingTimePerMl.outer}
                    onValueChange={(val) => handleTimePerMlChange("outer", val)}
                    endContent={
                      <div className="pointer-events-none flex items-center">
                        <span className="text-default-400 text-small">s/ml</span>
                      </div>
                    }
                  />
                </div>
              </div>

              <Divider className="my-2" />

              {/* Default durations section */}
              <div>
                <h4 className="text-sm font-semibold mb-2">Default Durations</h4>
                <div className="grid grid-cols-2 gap-3">
                  {timingBasedPumps.map((pump) => (
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
              </div>
            </ModalBody>
            <ModalFooter>
              <Button color="danger" variant="light" onPress={() => {
                resetState();
                onClose()
              }}>
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
