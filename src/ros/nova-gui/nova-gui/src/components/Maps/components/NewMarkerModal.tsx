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
import { PropRenderer } from "../../shared/PropRenderer";
import { useSelector } from "react-redux";
import { RootState } from "../../../redux/RootState";
import { MapPoint } from "../../../redux/models/CartographerState";
import { useState } from "react";

interface NewMarkerModalProps {
  isOpen: boolean;
  setOpen: React.Dispatch<React.SetStateAction<boolean>>;
  addPoint: (point: MapPoint) => void;
  latitude?: number;
  longitude?: number;
}

export const NewMarkerModal = (props: NewMarkerModalProps) => {
  const [name, setName] = useState<string>();
  const points = useSelector(
    (state: RootState) => state.cartographerState.points
  );

  const handleDropPin = () => {
    if (!props.latitude || !props.longitude) return;
    props.addPoint({
      lat: props.latitude,
      long: props.longitude,
      name: !name || name === "" ? `Point ${points.length + 1}` : name,
    });
    setName(undefined);
    props.setOpen(false);
  };

  return (
    <Modal
      isOpen={props.isOpen}
      className="dark text-foreground"
      onClose={() => props.setOpen(false)}
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
              if (event.key === "Enter") {
                handleDropPin();
              }
            }}
          />
          <PropRenderer
            props={props}
            ignoreProps={["isOpen", "setOpen", "addPoint"]}
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
