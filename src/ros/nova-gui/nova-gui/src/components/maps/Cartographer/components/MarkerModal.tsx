import { Modal, ModalBody, ModalContent, ModalHeader } from "@nextui-org/react";
import { PropRenderer } from "../../../shared/components/PropRenderer.tsx";

export interface MarkerInfoModalProps {
  isOpen: boolean;
  setOpen: React.Dispatch<React.SetStateAction<boolean>>;
  title: string;
  data: object;
}

export const MarkerModal = (props: MarkerInfoModalProps) => {
  return (
    <Modal
      isOpen={props.isOpen}
      className="dark text-foreground"
      onClose={() => props.setOpen(false)}
    >
      <ModalContent>
        <ModalHeader>{props.title}</ModalHeader>
        <ModalBody>
          <PropRenderer props={props.data} ignoreProps={[]} />
        </ModalBody>
      </ModalContent>
    </Modal>
  );
};
