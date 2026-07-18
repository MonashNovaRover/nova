import {BaseCameraComponentProps} from "../CameraComponent.tsx";
import {FC} from "react";
import ArmWidget from "../../../arm/ArmWidget/ArmWidget.tsx";

export const ArmCameraComponent: FC<BaseCameraComponentProps> = (_: BaseCameraComponentProps) => {
  return (
    <div>
      <ArmWidget/>
    </div>
  )
}

export default ArmCameraComponent;
