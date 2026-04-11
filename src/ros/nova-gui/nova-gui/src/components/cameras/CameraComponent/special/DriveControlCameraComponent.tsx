import {BaseCameraComponentProps} from "../CameraComponent.tsx";
import {FC} from "react";
import DriveControlWidget from "../../../drive/DriveControlWidget/DriveControlWidget.tsx";

export const DriveControlCameraComponent: FC<BaseCameraComponentProps> = (_: BaseCameraComponentProps) => {
  return (
    <div>
      <DriveControlWidget/>
    </div>
)
}

export default DriveControlCameraComponent;
