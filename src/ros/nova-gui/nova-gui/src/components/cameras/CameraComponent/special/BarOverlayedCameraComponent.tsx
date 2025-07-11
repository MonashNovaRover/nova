import {BaseCameraComponentProps} from "../CameraComponent.tsx";
import {FC, useState} from "react";
import OverlayedCameraComponent from "./OverlayedCameraComponent.tsx";
import {StreamingState} from "../hooks/useCameraStream.ts";

export const BarOverlayedCameraComponent: FC<BaseCameraComponentProps> = (props) => {
  const [showOverlay, setShowOverlay] = useState<boolean>(false)

  return (
    <OverlayedCameraComponent
      onStreamingStateChange={(s) => setShowOverlay(s === StreamingState.STREAMING)}
      {...props}
      overlay={showOverlay ? <div className="self-center grow h-0.5 bg-black"/> : <div/>}
    />
  )
}

export default BarOverlayedCameraComponent;
