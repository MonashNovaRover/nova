import {
  Button,
  Input,
  Kbd,
  Modal,
  ModalBody,
  ModalContent,
  ModalFooter,
  ModalHeader,
} from "@nextui-org/react";
import { useSelector } from "react-redux";
import { RootState } from "../../../redux/RootState";
import { MapPoint } from "../../../redux/models/CartographerState";
import { useEffect, useState } from "react";
import CopyableInput from "../../CopyableInput/CopyableInput";

interface NewMarkerModalProps {
  isOpen: boolean;
  closeModal: () => void;
  addPoint: (point: MapPoint) => void;
  latitude?: number;
  longitude?: number;
}

export const NewMarkerModal = (props: NewMarkerModalProps) => {
  const [name, setName] = useState<string>();
  const [longitude, setLongitude] = useState("");
  const [latitude, setLatitude] = useState("");

  const points = useSelector(
    (state: RootState) => state.cartographerState.points
  );

  const isValidPoint = () => {
    return (
      latitude !== "" &&
      longitude !== "" &&
      !isNaN(Number(latitude)) &&
      !isNaN(Number(longitude))
    );
  }

  const handleDropPin = () => {
    if (isValidPoint()) {
      props.addPoint({
        lat: Number(latitude),
        long: Number(longitude),
        name: !name || name === "" ? `Point ${points.length + 1}` : name,
      });
      setName(undefined);
      props.closeModal();
    }
};

  useEffect(() => {
      setLongitude(props.longitude?.toString() ?? "");
      setLatitude(props.latitude?.toString() ?? "");
    },
    [props.isOpen]
  )

  return (
    <Modal
      isOpen={props.isOpen}
      className="dark text-foreground"
      onClose={() => props.closeModal()}
    >
      <ModalContent>
        <ModalHeader>{"Drop Pin"}</ModalHeader>
        <ModalBody>
          <Input
            autoFocus
            value={name}
            onChange={(event) => setName(event.target.value as string)}
            placeholder={`Point ${points.length + 1}`}
            label="Name"
            onKeyDown={(event) => {
              if (event.key === "Enter" && isValidPoint()) {
                handleDropPin();
              }
            }}
          />
          <CopyableInput
            value={latitude}
            onChange={(event) => setLatitude(event.target.value as string)}
            placeholder={`Latitude ${points.length + 1}`}
            label="Latitude"
            onKeyDown={(event) => {
              if (event.key === "Enter" && isValidPoint()) {
                handleDropPin();
              }
            }}
          />
          <CopyableInput
            value={longitude}
            onChange={(event) => setLongitude(event.target.value as string)}
            placeholder={`Longitude ${points.length + 1}`}
            label="Longitude"
            onKeyDown={(event) => {
              if (event.key === "Enter" && isValidPoint()) {
                handleDropPin();
              }
            }}
          />
        </ModalBody>
        <ModalFooter>
          <Button fullWidth onClick={handleDropPin}>
            Drop Pin <Kbd keys={["enter"]} />
          </Button>
        </ModalFooter>
      </ModalContent>
    </Modal>
  );
};
