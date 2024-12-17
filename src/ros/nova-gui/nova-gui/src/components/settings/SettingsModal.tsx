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
import { useEffect, useState } from "react";
import toast from "react-hot-toast";
import { isIPAddress } from "../../utils/regexUtils";

export function SettingsModal() {
  const uiActions = useUIActions();
  const uiState = useSelector((state: RootState) => state.uiState);

  const [baseStationIP, setBaseStationIP] = useState<string>(
    uiState.baseStationIP
  );

  const [roverIP, setRoverIP] = useState<string>(uiState.roverIP);

  useEffect(() => {
    if (uiState.baseStationIP != baseStationIP)
      setBaseStationIP(uiState.baseStationIP);

    if (uiState.roverIP != roverIP)
      setRoverIP(uiState.roverIP);

    // eslint-disable-next-line react-hooks/exhaustive-deps
  }, [uiState.baseStationIP, uiState.roverIP]);

  const submit = () => {
    if (!isIPAddress(baseStationIP) || !isIPAddress(roverIP)) {
      toast.error("Invalid IP address");
      return;
    }

    uiActions.updateIP(baseStationIP, roverIP);

    closeModal();
  };

  const onBaseIPChange = (event: React.ChangeEvent<HTMLInputElement>) => {
    setBaseStationIP(event.target.value);
  };

  const onRoverIPChange = (event: React.ChangeEvent<HTMLInputElement>) => {
    setRoverIP(event.target.value);
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
            value={baseStationIP}
            type="text"
            onChange={onBaseIPChange}
            isInvalid={!isIPAddress(baseStationIP)}
          />
          <Input
            fullWidth
            label="Rover IP"
            value={roverIP}
            type="text"
            onChange={onRoverIPChange}
            isInvalid={!isIPAddress(roverIP)}
          />
        </ModalBody>
        <ModalFooter>
          <Button size="sm" color="success" variant="flat" onPress={submit}>
            Submit
          </Button>
        </ModalFooter>
      </ModalContent>
    </Modal>
  );
}
