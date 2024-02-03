import {
  Button,
  Input,
  Modal,
  ModalBody,
  ModalContent,
  ModalFooter,
  ModalHeader,
} from "@nextui-org/react";
import { useSelector } from "react-redux";
import { RootState } from "../../redux/RootState";
import { useUIActions } from "../../redux/actions/useUIActions";
import { useState } from "react";

export function SettingsModal() {
  const uiActions = useUIActions();
  const uiState = useSelector((state: RootState) => state.uiState);

  const [rosUrl, setRosUrl] = useState<string>(uiState.rosUrl);

  const submit = () => {
    uiActions.updateROSurl(rosUrl);
    window.localStorage.setItem("baseIP", rosUrl);
    closeModal();
  };

  const onURLChange = (event: React.ChangeEvent<HTMLInputElement>) => {
    setRosUrl(event.target.value);
  };

  const closeModal = () => uiActions.setSettingsModal(false);
  return (
    <Modal
      className="dark text-foreground"
      isOpen={uiState.settingsModalOpen}
      onClose={closeModal}
    >
      <ModalContent>
        <ModalHeader className="flex flex-col gap-1">Settings</ModalHeader>
        <ModalBody>
          <Input
            fullWidth
            label="Base IP"
            value={rosUrl}
            type="url"
            onChange={onURLChange}
          />
          <Input
            fullWidth
            label="Base IP"
            value={rosUrl}
            type="url"
            onChange={onURLChange}
          />
        </ModalBody>
        <ModalFooter>
          <Button size="sm" color="success" variant="flat" onClick={submit}>
            Submit
          </Button>
          <Button size="sm" color="danger" variant="flat" onClick={closeModal}>
            Close
          </Button>
        </ModalFooter>
      </ModalContent>
    </Modal>
  );
}
