import {BaseCameraComponentProps} from "../CameraComponent.tsx";
import {FC} from "react";
import DriveControlWidget from "../../../drive/DriveControlWidget/DriveControlWidget.tsx";

export const DriveControlCameraComponent: FC<BaseCameraComponentProps> = (_: BaseCameraComponentProps) => {
  return (
    <div>
      <DriveControlWidget /*hideImage={true} className="row-span-2 w-full"*/ />
    </div>
)
}

export default DriveControlCameraComponent;
