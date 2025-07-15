import {
  Accordion,
  AccordionItem,
  Button,
  Modal,
  ModalBody,
  ModalContent,
  ModalFooter,
  ModalHeader,
} from "@nextui-org/react";
import { CameraComponentProps } from "../CameraComponent.tsx";
import { ExternalLink } from "react-feather";
import { PropRenderer } from "../../../shared/components/PropRenderer.tsx";
// import { CameraSettingsForm } from "./CameraSettingsForm";

interface CameraModalProps extends CameraComponentProps {
  cameraName: string;
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
          <Accordion
            variant="bordered"
            selectionMode="multiple"
            defaultExpandedKeys={["settings"]}
          >
            <AccordionItem
              title="Camera Info"
              subtitle={`Camera Metadata and Information for ${cameraName}`}
            >
              <PropRenderer<CameraModalProps>
                props={props}
                ignoreProps={["isModalOpen", "setCameraModalOpen"]}
              />
            </AccordionItem>
            <AccordionItem
              key="settings"
              title="Camera Settings"
              subtitle={`Settings for ${cameraName}`}
            >
              {/* <CameraSettingsForm /> */}
            </AccordionItem>
          </Accordion>
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
