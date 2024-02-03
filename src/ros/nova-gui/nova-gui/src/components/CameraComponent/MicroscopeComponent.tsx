import {
  Card,
  Slider,
} from "@nextui-org/react";
import { CameraComponent, CameraComponentProps } from "./CameraComponent";
import { useState } from "react";

interface MicroscopeComponentProps extends CameraComponentProps {

}



export const MicroscopeComponent = (props: MicroscopeComponentProps) => {
  const { cameraName, cameraSerial } = props;
  const [zoomFocus, setZoomFocus] = useState<number>(0)

  return (
    <Card className={`m-4`}>
      <CameraComponent
        cameraName={cameraName}
        cameraSerial={cameraSerial} />
      <Slider 
        className="max-w-full pt-2 pb-6 pl-8 pr-8"
        size="lg" label="Zoom/Focus" 
        startContent="1x" 
        endContent="100x"
        value={zoomFocus}
        onChange={(value) => setZoomFocus(value as number)}/>

    </Card>
  );
};

