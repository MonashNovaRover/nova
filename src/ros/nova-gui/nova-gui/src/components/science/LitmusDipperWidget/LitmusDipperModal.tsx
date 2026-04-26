import {
  Button, Input, Modal, ModalBody, ModalContent, ModalFooter, ModalHeader,
} from "@nextui-org/react";
import React, { useState } from "react";
import { useGenericStore } from "../../../hooks/useGenericStore.ts";

export interface LitmusDipperModalProps {
  isOpen: boolean;
  onOpenChange: (isOpen: boolean) => void;
}

const LitmusDipperModal: React.FC<LitmusDipperModalProps> = ({ isOpen, onOpenChange }) => {
  const [defaultDuration, setDefaultDuration] = useGenericStore<number>("litmusDipperDefaultDuration");
  const [twitchStep, setTwitchStep] = useGenericStore<number>("litmusDipperTwitchStep");

  const [editingDuration, setEditingDuration] = useState<string>(defaultDuration?.toString() ?? "2");
  const [editingTwitchStep, setEditingTwitchStep] = useState<string>(twitchStep?.toString() ?? "5");

  const save = () => {
    const val = Number(editingDuration);
    setDefaultDuration(isNaN(val) || val <= 0 ? 2 : val);

    const stepVal = Number(editingTwitchStep);
    setTwitchStep(isNaN(stepVal) || stepVal <= 0 ? 5 : stepVal);
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
            </ModalBody>
            <ModalFooter>
              <Button
                color="danger"
                variant="light"
                onPress={() => {
                  setEditingDuration(defaultDuration?.toString() ?? "2");
                  setEditingTwitchStep(twitchStep?.toString() ?? "5");
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