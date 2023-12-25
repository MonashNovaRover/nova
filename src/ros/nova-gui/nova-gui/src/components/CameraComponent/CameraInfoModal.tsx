import {
  Button,
  Modal,
  ModalBody,
  ModalContent,
  ModalFooter,
  ModalHeader,
} from "@nextui-org/react";
import { CameraComponentProps } from "./CameraComponent";
import { ExternalLink } from "react-feather";
import { PropRenderer } from "../shared/PropRenderer";

interface CameraModalProps extends CameraComponentProps {
  isModalOpen: boolean;
  setCameraModalOpen: React.Dispatch<React.SetStateAction<boolean>>;
}

export const CameraInfoModal = (props: CameraModalProps) => {
  const { cameraName, isModalOpen, setCameraModalOpen } = props;
  return (
    <Modal
      isOpen={isModalOpen}
      onClose={() => setCameraModalOpen(false)}
      className="dark text-foreground"
    >
      <ModalContent>
        <ModalHeader>{cameraName}</ModalHeader>
        <ModalBody>
          <PropRenderer<CameraModalProps>
            props={props}
            ignoreProps={["isModalOpen", "setCameraModalOpen", "src"]}
          />
        </ModalBody>
        <ModalFooter>
          <Button>
            Open Camera in New Tab <ExternalLink />
          </Button>
        </ModalFooter>
      </ModalContent>
    </Modal>
  );
};
