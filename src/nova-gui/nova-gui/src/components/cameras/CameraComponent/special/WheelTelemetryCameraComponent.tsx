import {BaseCameraComponentProps} from "../CameraComponent.tsx";
import {FC} from "react";
import WheelTelemetryWidget from "../../../drive/WheelTelemetryWidget/WheelTelemetryWidget.tsx";

export const WheelTelemetryCameraComponent: FC<BaseCameraComponentProps> = (_: BaseCameraComponentProps) => {
  return (
    <div>
      <WheelTelemetryWidget hideImage={true} className="row-span-2 w-full" />
    </div>
  )
}

export default WheelTelemetryCameraComponent;
