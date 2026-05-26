import {
  Button, Input, Modal, ModalBody, ModalContent, ModalFooter, ModalHeader,
} from "@nextui-org/react";
import React, { useState } from "react";
import { useGenericStore } from "../../../hooks/useGenericStore.ts";
import { LitmusDipperConfig } from "./LitmusDipperWidget";

export interface LitmusDipperModalProps {
  isOpen: boolean;
  onOpenChange: (isOpen: boolean) => void;
}

const LitmusDipperModal: React.FC<LitmusDipperModalProps> = ({ isOpen, onOpenChange }) => {
  const [config, setConfig] = useGenericStore<LitmusDipperConfig>("litmusDipperConfig");

  const [editingDuration, setEditingDuration] = useState<string>(config?.defaultDuration?.toString() ?? "2");
  const [editingTwitchStep, setEditingTwitchStep] = useState<string>(config?.twitchStep?.toString() ?? "5");
  const [editingWaitDuration, setEditingWaitDuration] = useState<string>(config?.waitDuration?.toString() ?? "30");

  const save = () => {
    const durationVal = Number(editingDuration);
    const stepVal = Number(editingTwitchStep);
    const waitVal = Number(editingWaitDuration);

    setConfig({
      defaultDuration: isNaN(durationVal) || durationVal <= 0 ? 2 : durationVal,
      twitchStep: isNaN(stepVal) || stepVal <= 0 ? 5 : stepVal,
      waitDuration: isNaN(waitVal) || waitVal <= 0 ? 30 : waitVal,
    });
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
            <ModalHeader>Default Litmus Dipper Settings</ModalHeader>
            <ModalBody>
              <Input
                label="Dip Duration"
                type="number"
                value={editingDuration}
                onValueChange={setEditingDuration}
                endContent={
                  <div className="pointer-events-none flex items-center">
                    <span className="text-default-400 text-small">s</span>
                  </div>
                }
              />
              <Input
                label="Twitch Step"
                type="number"
                value={editingTwitchStep}
                onValueChange={setEditingTwitchStep}
                endContent={
                  <div className="pointer-events-none flex items-center">
                    <span className="text-default-400 text-small">°</span>
                  </div>
                }
              />
              <Input
                label="Wait Duration"
                type="number"
                value={editingWaitDuration}
                onValueChange={setEditingWaitDuration}
                endContent={
                  <div className="pointer-events-none flex items-center">
                    <span className="text-default-400 text-small">s</span>
                  </div>
                }
              />
            </ModalBody>
            <ModalFooter>
              <Button
                color="danger"
                variant="light"
                onPress={() => {
                  setEditingDuration(config?.defaultDuration?.toString() ?? "2");
                  setEditingTwitchStep(config?.twitchStep?.toString() ?? "5");
                  setEditingWaitDuration(config?.waitDuration?.toString() ?? "30");
                  onClose();
                }}
              >
                Cancel
              </Button>
              <Button
                color="primary"
                onPress={() => {
                  save();
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
export default LitmusDipperModal;