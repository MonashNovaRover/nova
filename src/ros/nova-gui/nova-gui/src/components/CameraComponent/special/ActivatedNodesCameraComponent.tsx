import {BaseCameraComponentProps} from "../CameraComponent.tsx";
import {FC} from "react";
import ActivatedNodeWidget from "../../ActivatedNodeWidget/ActivatedNodeWidget.tsx";
import {URCActivatedNodeConfig} from "../../ActivatedNodeWidget/ActivatedNodeWidgetConfig.tsx";

export const ActivatedNodesCameraComponent: FC<BaseCameraComponentProps> = (_: BaseCameraComponentProps) => {
  return (
    <div>
      <ActivatedNodeWidget config={URCActivatedNodeConfig}/>
    </div>
  )
}

export default ActivatedNodesCameraComponent;