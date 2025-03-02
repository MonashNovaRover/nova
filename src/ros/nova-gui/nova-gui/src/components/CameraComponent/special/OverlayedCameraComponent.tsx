import {BaseCameraComponentProps, CameraComponent, CameraComponentProps} from "../CameraComponent.tsx";
import {FC, ReactNode, useState} from "react";
import {Switch} from "@nextui-org/react";
import Overlay from "../../shared/Overlay/Overlay.tsx";
import {ArmLineOverlay} from "../../shared/Overlay/ArmLineOverlay.tsx";
import CameraVideo, {CameraVideoProps} from "../components/CameraVideo.tsx";

export interface OverlayedCameraComponentProps extends CameraComponentProps {
  overlay: ReactNode
}

const OverlayedCameraComponent: FC<OverlayedCameraComponentProps> = (props) => {
  const [overlayToggle, setOverlayToggle] = useState(true)

  const settingsFormChildren = (
    <>
      <Switch
        size="sm"
        isSelected={overlayToggle}
        onChange={(_) => setOverlayToggle(!overlayToggle)}
      >
        Toggle Overlay
      </Switch>
      {props.settingsFormChildren}
    </>
  );

  const cameraVideo: FC<CameraVideoProps> = ({videoRef, filters}: CameraVideoProps) => (
    <Overlay
      overlay={overlayToggle && props.overlay}
    >
      <CameraVideo videoRef={videoRef} filters={filters}/>
    </Overlay>
  )

  return (
      <CameraComponent
        {...props}
        cameraVideoComponent={cameraVideo}
        settingsFormChildren={settingsFormChildren}
      />
  )
};

export const CrosshairOverlayedCameraComponent: FC<BaseCameraComponentProps> = (props) => (
  <OverlayedCameraComponent {...props} overlay={<ArmLineOverlay/>}/>
)

export default OverlayedCameraComponent;
