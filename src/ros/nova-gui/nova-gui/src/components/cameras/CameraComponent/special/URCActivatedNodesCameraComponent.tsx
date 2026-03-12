import {BaseCameraComponentProps} from "../CameraComponent.tsx";
import {FC} from "react";
import ActivatedNodeWidgetOld from "../../../shared/widgets/ActivatedNodeWidget/ActivatedNodeWidgetOld.tsx";
import {URCActivatedNodeConfig} from "../../../shared/widgets/ActivatedNodeWidget/ActivatedNodeWidgetConfig.tsx";

export const URCActivatedNodesCameraComponent: FC<BaseCameraComponentProps> = (_: BaseCameraComponentProps) => {
  return (
    <div>
      <ActivatedNodeWidgetOld config={URCActivatedNodeConfig}/>
    </div>
  )
}

export default URCActivatedNodesCameraComponent;