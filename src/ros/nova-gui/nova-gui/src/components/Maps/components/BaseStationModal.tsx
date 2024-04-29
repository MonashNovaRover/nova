import {
  Button,
  Modal,
  ModalBody,
  ModalContent,
  ModalFooter,
  ModalHeader,
} from "@nextui-org/react";
import { MarkerInfoModalProps } from "./RoverInfoModal";
import CopyableInput from "../../CopyableInput/CopyableInput";
export const BaseStationModal = (props: MarkerInfoModalProps) => {
  const baseStationCoordinates = {
    latitude: -37.445,
    longitude: 29.2398,
  };

  return (
    <Modal
      isOpen={props.isOpen}
      className="dark text-foreground"
      size="lg"
      onClose={() => props.setOpen(false)}
    >
      <ModalContent>
        <ModalHeader>Base Station</ModalHeader>
        <ModalBody>
          <div className="flex flex-row gap-2">
            <CopyableInput
              variant="faded"
              label="Latitude"
              value={String(baseStationCoordinates.latitude)}
            />
            <CopyableInput
              variant="faded"
              label="Longitude"
              value={String(baseStationCoordinates.longitude)}
            />
          </div>
          <Button fullWidth color="primary">
            Reset Base Station to Rover's Location
          </Button>
        </ModalBody>
        <ModalFooter>
          <Button color="success" variant="flat">
            Save
          </Button>
        </ModalFooter>
      </ModalContent>
    </Modal>
  );
};
