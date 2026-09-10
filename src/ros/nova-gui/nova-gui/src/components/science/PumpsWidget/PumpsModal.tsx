import {Button, Separator, Input, Modal, ModalBody, ModalDialog, ModalFooter, ModalHeader, useOverlayState} from "@heroui/react";
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
  const overlayState = useOverlayState({ isOpen, onOpenChange });
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
    <Modal state={overlayState}>
      <ModalDialog>
          <>
            <ModalHeader>Pump Settings</ModalHeader>
            <ModalBody>
              {/* Time per ml section */}
              <div className="mb-2">
                <h4 className="text-sm font-semibold mb-2">Time per ml (for ring pumps)</h4>
                <div className="grid grid-cols-2 gap-3">
                  <Input
                    aria-label="Inner Ring"
                    type="number"
                    step="0.1"
                    value={editingTimePerMl.inner}
                    onChange={(event) => handleTimePerMlChange("inner", event.target.value)}
                  />
                  <Input
                    aria-label="Outer Ring"
                    type="number"
                    step="0.1"
                    value={editingTimePerMl.outer}
                    onChange={(event) => handleTimePerMlChange("outer", event.target.value)}
                  />
                </div>
              </div>

              <Separator className="my-2" />

              {/* Default durations section */}
              <div>
                <h4 className="text-sm font-semibold mb-2">Default Durations</h4>
                <div className="grid grid-cols-2 gap-3">
                  {timingBasedPumps.map((pump) => (
                    <Input
                      key={pump.value}
                      aria-label={pump.display}
                      type="number"
                      value={editingDurations[pump.value] ?? ""}
                      onChange={(event) => handleEditingDurationChange(pump.value, event.target.value)}
                    />
                  ))}
                </div>
              </div>
            </ModalBody>
            <ModalFooter>
              <Button variant="danger" onPress={() => {
                resetState();
                overlayState.close()
              }}>
                Cancel
              </Button>
              <Button
                variant="primary"
                onPress={() => {
                  saveDefaultDurations();
                  overlayState.close();
                }}
              >
                Save
              </Button>
            </ModalFooter>
          </>
      </ModalDialog>
    </Modal>
  );
};

export default PumpsModal;
