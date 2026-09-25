import {
  Button,
  Input,
  Kbd,
  Modal,
  ModalBody,
  ModalContent,
  ModalFooter,
  ModalHeader, Select, SelectItem,
} from "@nextui-org/react";
import { useSelector } from "react-redux";
import { RootState } from "../../../../redux/RootState.ts";
import { MapPoint } from "../../../../redux/models/CartographerState.ts";
import { useEffect, useState } from "react";
import CopyableInput from "../../../shared/components/CopyableInput/CopyableInput.tsx";
import toast from "react-hot-toast";
import { calcLatFromDD, calcLatFromDDM, calcLatFromDMS, calcLongFromDD, calcLongFromDDM, calcLongFromDMS } from "../utils/convertCoords.ts";

interface NewMarkerModalProps {
  isOpen: boolean;
  closeModal: () => void;
  addPoint: (point: MapPoint) => void;
  latitude?: number;
  longitude?: number;
  labels?: { key: number; text: string }[];
}

export const NewMarkerModal = (props: NewMarkerModalProps) => {
  const [name, setName] = useState<string>();
  const [longitude, setLongitude] = useState("");
  const [latitude, setLatitude] = useState("");
  const [labelNumber, setLabelNumber] = useState<number | null>(null);
  const [labelName, setLabelName] = useState<string | null>(null);
  const [searchRadius, setSearchRadius] = useState<string>("");

  const points = useSelector(
    (state: RootState) => state.cartographerState.points
  );

  const convertLatLong = (val: string, isLat: boolean = false) => {
    const doFuncs = (functions: ((_: string) => number)[]) => {
      for (const f in functions) {
        const result = functions[f](val);
        console.log(result, functions[f])
        if (!isNaN(result)) return result;
      }
      return NaN
    }
    if (isLat) {
      const functions = [calcLatFromDD, calcLatFromDMS, calcLatFromDDM]
      return doFuncs(functions)
    } else {
      const functions = [calcLongFromDD, calcLongFromDMS, calcLongFromDDM]
      return doFuncs(functions)
    }
  }

  const isValidPoint = () => {
    return (
      latitude !== "" &&
      longitude !== "" &&
      !isNaN(convertLatLong(latitude, true)) &&
      !isNaN(convertLatLong(longitude)) && 
      points.reduce((acc, point) => acc && !(point.lat === convertLatLong(latitude, true) && point.long === convertLatLong(longitude)) && point.name !== name, true)
    );
  }

  const handleDropPin = () => {
    if (isValidPoint()) {
      const newPoint = {
        lat: convertLatLong(latitude, true),
        long: convertLatLong(longitude),
        labelNumber: labelNumber,
        labelName: labelName,
        name: !name || name === "" ? `Point ${points.length + 1}` : name,
        searchRadius: searchRadius !== "" ? Number(searchRadius) : null,
      } as MapPoint
      props.addPoint(newPoint);
      setName(undefined);
      props.closeModal();
    }
    else{
      console.log(convertLatLong(latitude, true), convertLatLong(longitude))
      toast.error("Point must have unique coordinates and name")
    }
};

  useEffect(() => {
      setLongitude(props.longitude?.toString() ?? "");
      setLatitude(props.latitude?.toString() ?? "");
      setLabelNumber(props.labels && props.labels.length > 0 ? 0 : null);
      setLabelName(props.labels && props.labels.length > 0 ? props.labels[0].text : null);
      setSearchRadius("");
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
          <div className="text-xs text-default-500">
            Lat/Long entry allows DD, DMS and DDM where {`°, ' and "`} are optional<br/> Direction indicator(N, E for Lat or S, W) is not optional for DMS & DDM
          </div>
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
          {props.labels && props.labels.length > 0 && (
            <Select
              label="Label"
              defaultSelectedKeys={["0"]}
              onChange={(e) => {
                setLabelNumber(Number(e.target.value))
                if (props.labels) {
                  setLabelName(
                    props.labels.find((label) => label.key === Number(e.target.value))?.text ?? null
                  );
                }
              }}
            >
              {props.labels.map((label) => (
                <SelectItem key={label.key}>{label.text}</SelectItem>
              ))}
            </Select>
          )}
          <Input
            type="number"
            value={searchRadius}
            onChange={(event) => setSearchRadius(event.target.value)}
            placeholder="Optional"
            label="Search Radius"
            min="0"
            max="50"
            step="1"
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
