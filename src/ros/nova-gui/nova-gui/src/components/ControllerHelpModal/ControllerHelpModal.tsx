import {
    Button,
    Modal,
    ModalBody,
    ModalContent,
    ModalFooter,
    ModalHeader,
  } from "@nextui-org/react";
  import { useSelector } from "react-redux";
  import { RootState } from "../../redux/RootState";
  import { useUIActions } from "../../redux/actions/useUIActions";
  import controls from "../../assets/controls.svg";

  
const ControllerHelpModal : React.FC = () => {
    const uiActions = useUIActions();
    const uiState = useSelector((state: RootState) => state.uiState);

  
    const closeModal = () => uiActions.setControllerHelpModal(false);


    return (
      <Modal
        className="dark text-foreground"
        isOpen={uiState.controllerHelpModalOpen}
        onClose={closeModal}
      >
        <ModalContent className="w-1 p-30px">
          <ModalHeader className="flex flex-col gap-1">Settings</ModalHeader>
          <ModalBody>
            <img src={controls} alt="Image" />
          </ModalBody>
          <ModalFooter>
            <Button color="danger" variant="flat" onClick={closeModal}>
              Close
            </Button>
          </ModalFooter>
        </ModalContent>
      </Modal>
    );
  }
  export default ControllerHelpModal;
  