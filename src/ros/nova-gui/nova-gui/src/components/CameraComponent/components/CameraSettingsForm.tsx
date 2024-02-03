import { Slider, Switch } from "@nextui-org/react";
import {
  ArrowCounterclockwise,
  CircleFill,
  CircleHalf,
} from "react-bootstrap-icons";
import { Droplet } from "react-feather";
import { CameraFilters } from "../CameraComponent";

export const CameraSettingsForm = ({
  cameraFilters,
  setCameraFilters,
}: {
  cameraFilters: CameraFilters;
  setCameraFilters: React.Dispatch<React.SetStateAction<CameraFilters>>;
}) => {
  return (
    <div className="mt-2 flex flex-col gap-3 w-full">
      <Switch
        size="sm"
        thumbIcon={<Droplet fill="white" />}
        isSelected={cameraFilters.flipCamera}
        onChange={(event) =>
          setCameraFilters((oldFilters) => ({
            ...oldFilters,
            flipCamera: event.target.checked,
          }))
        }
      >
        Flip Camera
      </Switch>
      <Switch size="sm" thumbIcon={<Droplet fill="white" />}>
        Invert Colors
      </Switch>
      <Slider
        className="max-w-md"
        size="lg"
        label="Contrast"
        startContent={<CircleFill />}
        endContent={<CircleHalf />}
      />
      <Slider
        step={10}
        label="Rotation"
        className="max-w-md"
        size="lg"
        startContent="0°"
        maxValue={180}
        minValue={-180}
        endContent={<ArrowCounterclockwise />}
        marks={[
          { label: "-180°", value: -180 },
          { label: "-90°", value: -90 },
          { label: "0°", value: 0 },
          { label: "90°", value: 90 },
          { label: "180°", value: 180 },
        ]}
        getValue={(val) => `${val}°`}
        value={cameraFilters.rotation}
        onChange={(value) =>
          setCameraFilters({ ...cameraFilters, rotation: value as number })
        }
      />
    </div>
  );
};
