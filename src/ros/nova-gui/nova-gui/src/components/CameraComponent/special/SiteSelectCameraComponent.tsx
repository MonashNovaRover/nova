import {BaseCameraComponentProps} from "../CameraComponent.tsx";
import {FC} from "react";
import SiteSelectWidget from "../../SiteSelectWidget/SiteSelectWidget.tsx"

export const SiteSelectCameraComponent: FC<BaseCameraComponentProps> = (_: BaseCameraComponentProps) => {
    return (
        <div>
            <SiteSelectWidget />
        </div>
    )
}

export default SiteSelectCameraComponent;
