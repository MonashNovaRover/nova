import { Modal, ModalBody, ModalContent, ModalHeader } from "@nextui-org/react";
import { PropRenderer } from "../../shared/PropRenderer";

export interface MarkerInfoModalProps {
  isOpen: boolean;
  setOpen: React.Dispatch<React.SetStateAction<boolean>>;
}

export const WaratahInfoModal = (props: MarkerInfoModalProps) => {
  return (
    <Modal
      isOpen={props.isOpen}
      className="dark text-foreground"
      onClose={() => props.setOpen(false)}
    >
      <ModalContent>
        <ModalHeader>Waratah</ModalHeader>
        <ModalBody>
          <PropRenderer
            props={{
              distance: "350m",
              latitude: -37.28819,
              longitude: 3,
            }}
            ignoreProps={[]}
          />
        </ModalBody>
      </ModalContent>
    </Modal>
  );
};
