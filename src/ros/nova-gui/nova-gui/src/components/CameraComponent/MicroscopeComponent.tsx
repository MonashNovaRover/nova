import { Card, Slider } from "@nextui-org/react";
import { CameraComponent } from "./CameraComponent";
import { useState } from "react";
import { useCameraStreamer } from "./hooks/useCameraStreamer";

export const MicroscopeComponent = () => {
  const [zoomFocus, setZoomFocus] = useState<number>(0);

  useCameraStreamer();

  return (
    <Card className={`m-4`}>
      <CameraComponent cameraSerial={"microscope_camera"} />
      <Slider
        className="max-w-full pt-2 pb-6 pl-8 pr-8"
        size="lg"
        label="Zoom/Focus"
        startContent="1x"
        endContent="100x"
        value={zoomFocus}
        onChange={(value) => setZoomFocus(value as number)}
      />
    </Card>
  );
};
