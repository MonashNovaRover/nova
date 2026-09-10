import { Modal, ModalBody, ModalDialog, ModalHeader } from "@heroui/react";
import { useSelector } from "react-redux";
import { RootState } from "../../../redux/RootState.ts";
import { useUIActions } from "../../../redux/actions/useUIActions.ts";
import controls from "../../../assets/controls.svg";

const ControllerHelpModal: React.FC = () => {
  const uiActions = useUIActions();
  const uiState = useSelector((state: RootState) => state.uiState);

  const closeModal = () => uiActions.setControllerHelpModal(false);

  return (
    <Modal
      size="5xl"
      className="dark text-foreground p-2"
      isOpen={uiState.controllerHelpModalOpen}
      onClose={closeModal}
    >
      <ModalDialog>
        <ModalHeader>Controller Help</ModalHeader>
        <ModalBody>
          <img className="w-full my-4" src={controls} alt="Image" />
        </ModalBody>
      </ModalDialog>
    </Modal>
  );
};
export default ControllerHelpModal;
