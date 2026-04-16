import {BaseCameraComponentProps} from "../CameraComponent.tsx";
import {FC} from "react";
import ActivatedNodeWidget from "../../../shared/widgets/ActivatedNodeWidget/ActivatedNodeWidget.tsx";
import {ARCActivatedNodeConfig} from "../../../shared/widgets/ActivatedNodeWidget/ActivatedNodeWidgetConfig.tsx";

export const ARCActivatedNodesCameraComponent: FC<BaseCameraComponentProps> = (_: BaseCameraComponentProps) => {
  return (
    <div>
      <ActivatedNodeWidget config={ARCActivatedNodeConfig}/>
    </div>
  )
}

export default ARCActivatedNodesCameraComponent;