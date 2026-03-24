import {BaseCameraComponentProps} from "../CameraComponent.tsx";
import {FC} from "react";
import SiteSelectWidget from "../../../science/SiteSelectWidget/SiteSelectWidget.tsx"
import PowerCycleCameraComponent from "./PowerCycleCameraComponent.tsx";

export const ScienceCombinedCameraComponent: FC<BaseCameraComponentProps> = (props: BaseCameraComponentProps) => {
  return (
    <div className="grid grid-cols-1 content-start gap-3">
      <SiteSelectWidget />
      <PowerCycleCameraComponent {...props}/>
    </div>
  )
}

export default ScienceCombinedCameraComponent;
