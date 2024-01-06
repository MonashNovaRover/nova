import { Slider, Switch } from "@nextui-org/react";
import {
  ArrowCounterclockwise,
  CircleFill,
  CircleHalf,
} from "react-bootstrap-icons";
import { Droplet } from "react-feather";

export const CameraSettingsForm = () => {
  return (
    <div className="mt-2 flex flex-col gap-3 w-full">
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
        showSteps
        step={90}
        label="Rotation"
        className="max-w-md"
        size="lg"
        startContent="0°"
        maxValue={360}
        minValue={0}
        endContent={<ArrowCounterclockwise />}
        marks={[
          { label: "90°", value: 90 },
          { label: "180°", value: 180 },
          { label: "270°", value: 270 },
        ]}
        getValue={(val) => `${val}°`}
      />
    </div>
  );
};
