import {
  Button,
  Input,
  Kbd,
  Modal,
  ModalBody,
  ModalContent,
  ModalFooter,
  ModalHeader,
  Select,
  SelectItem
} from "@nextui-org/react";
import { useSelector } from "react-redux";
import { RootState } from "../../../redux/RootState";
import { MapPoint } from "../../../redux/models/CartographerState";
import { useEffect, useState } from "react";
import CopyableInput from "../../CopyableInput/CopyableInput";
import toast from "react-hot-toast";

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
  const [goalType, setGoalType] = useState<number>(0);

  const goalTypes = [
    { key: 0, label: "Nothing" },
    { key: 1, label: "AR Tag" },
    { key: 2, label: "Object" },
  ]

  const points = useSelector(
    (state: RootState) => state.cartographerState.points
  );

  const isValidPoint = () => {
    return (
      latitude !== "" &&
      longitude !== "" &&
      !isNaN(Number(latitude)) &&
      !isNaN(Number(longitude)) && 
      points.reduce((acc, point) => acc && !(point.lat === Number(latitude) && point.long === Number(longitude)) && point.name !== name, true)
    );
  }

  const handleDropPin = () => {
    if (isValidPoint()) {
      const newPoint = {
        lat: Number(latitude),
        long: Number(longitude),
        goalType: Number(goalType),
        name: !name || name === "" ? `Point ${points.length + 1}` : name,
      } as MapPoint
      props.addPoint(newPoint);
      setName(undefined);
      props.closeModal();
    }
    else{
      toast.error("Point must have unique coordinates and name")
    }
};

  useEffect(() => {
      setLongitude(props.longitude?.toString() ?? "");
      setLatitude(props.latitude?.toString() ?? "");
      setGoalType(0);
    },
    // eslint-disable-next-line react-hooks/exhaustive-deps
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
          <Select
            label="Goal Type"
            defaultSelectedKeys={["0"]}
            onChange={(e) => setGoalType(Number(e.target.value))}
          >
            {goalTypes.map((goalType) => (
              <SelectItem key={goalType.key}>
                {goalType.label}
              </SelectItem>
            ))}
          </Select>
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
