import {BaseCameraComponentProps} from "../CameraComponent.tsx";
import {FC} from "react";
import URCActivatedNodeWidget from "../../ActivatedNodeWidget/URCActivatedNodeWidget.tsx";
import {URCActivatedNodeConfig} from "../../ActivatedNodeWidget/ActivatedNodeWidgetConfig.tsx";

export const ActivatedNodesCameraComponent: FC<BaseCameraComponentProps> = (_: BaseCameraComponentProps) => {
  return (
    <div>
      <URCActivatedNodeWidget config={URCActivatedNodeConfig}/>
    </div>
  )
}

export default ActivatedNodesCameraComponent;