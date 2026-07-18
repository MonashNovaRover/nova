import {BaseCameraComponentProps} from "../CameraComponent.tsx";
import {FC} from "react";
import DriveModeWidget from "../../../drive/DriveModeWidget/DriveModeWidget.tsx";
import DriveSpeedWidget from "../../../drive/DriveSpeedWidget/DriveSpeedWidget.tsx";

export const DriveCameraComponent: FC<BaseCameraComponentProps> = (_: BaseCameraComponentProps) => {
  return (
    <div>
      <DriveModeWidget className="w-full mb-3" />
      <DriveSpeedWidget className="w-full" />
    </div>
  )
}

export default DriveCameraComponent;
