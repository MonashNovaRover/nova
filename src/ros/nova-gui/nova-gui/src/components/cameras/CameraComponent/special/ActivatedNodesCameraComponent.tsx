import {BaseCameraComponentProps} from "../CameraComponent.tsx";
import {FC} from "react";
import ActivatedNodeWidget from "../../../shared/widgets/ActivatedNodeWidget/ActivatedNodeWidget.tsx";
import {
  ActivatedNodeConfig,
} from "../../../shared/widgets/ActivatedNodeWidget/ActivatedNodeWidgetConfig.tsx";

export const ActivatedNodesCameraComponent: FC<BaseCameraComponentProps> = (config: ActivatedNodeConfig[]) => (_: BaseCameraComponentProps) => {
  return (
    <div>
      <ActivatedNodeWidget config={config}/>
    </div>
  )
}

export default ActivatedNodesCameraComponent;