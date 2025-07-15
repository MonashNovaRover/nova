import { Slider, Switch } from "@nextui-org/react";
import {
  ArrowCounterclockwise,
  CircleFill,
  CircleHalf,
} from "react-bootstrap-icons";
import { Droplet, RotateCcw } from "react-feather";
import { CameraFilters } from "../CameraComponent.tsx";
import {ReactNode} from "react";

const snapTo90 = (value: number): number => {
  const remainder = value % 90;
  const snap = value - remainder;
  return Math.abs(remainder) > 45 ? (value >= 0 ? snap + 90 : snap - 90) : snap;
};

export const CameraSettingsForm = ({
  cameraFilters,
  setCameraFilters,
  children
}: {
  cameraFilters: CameraFilters;
  setCameraFilters: React.Dispatch<React.SetStateAction<CameraFilters>>;
  children?: ReactNode
}) => {
  return (
    <div className="mt-2 flex flex-col gap-3 w-full">
      <Switch
        size="sm"
        thumbIcon={<RotateCcw fill="white" />}
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
      <Switch
        size="sm"
        thumbIcon={<Droplet fill="white" />}
        isSelected={cameraFilters.invertCamera}
        onChange={(event) =>
          setCameraFilters((oldFilters) => ({
            ...oldFilters,
            invertCamera: event.target.checked,
          }))
        }
      >
        Invert Colors
      </Switch>
      <Slider
        className="max-w-md"
        size="lg"
        label="Brightness"
        startContent={<CircleFill />}
        endContent={<CircleHalf />}
        minValue={0}
        maxValue={200}
        value={cameraFilters.brightness}
        onChange={(value) =>
          setCameraFilters({ ...cameraFilters, brightness: value as number })
        }
      />
      <Slider
        className="max-w-md"
        size="lg"
        label="Contrast"
        startContent={<CircleFill />}
        endContent={<CircleHalf />}
        minValue={0}
        maxValue={200}
        value={cameraFilters.contrast}
        onChange={(value) =>
          setCameraFilters({ ...cameraFilters, contrast: value as number })
        }
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
          setCameraFilters({
            ...cameraFilters,
            rotation: snapTo90(value as number),
          })
        }
      />
      {children}
    </div>
  );
};
