import { Modal, ModalBody, ModalDialog, ModalHeader} from "@heroui/react";
import guide from '../../../assets/auto-typing-keyboard-guide.png';

interface IAutoTypingModalProps {
  showModal: boolean;
  closeModal: () => void;
}

const AutoTypingModal: React.FC<IAutoTypingModalProps>= (props) => {
  return (
    <Modal
      isOpen={props.showModal}
      className="dark text-foreground"
      onClose={props.closeModal}
      size="5xl"
    >
      <ModalDialog>
        <ModalHeader className="flex flex-col gap-1">Auto Typing Keyboard Guide</ModalHeader>
        <ModalBody>
          <img src={guide} alt="Auto Typing Key Guide"/>
        </ModalBody>
      </ModalDialog>
    </Modal>
  );
};

export default AutoTypingModal;
