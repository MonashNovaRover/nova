import {BaseCameraComponentProps} from "../CameraComponent.tsx";
import {FC, useState, useEffect} from "react";
import OverlayedCameraComponent from "./OverlayedCameraComponent.tsx";
import {StreamingState} from "../hooks/useCameraStream.ts";
import {RosTopic} from "../../../ros/topics/rosTopic.ts";
import { RootState } from "../../../redux/RootState";
import { useSelector } from "react-redux";
import {useBifrost} from "../../../redux/actions/bifrost/useBifrostAction.ts";


export const KeyboardOverlayedCameraComponent: FC<BaseCameraComponentProps> = (props) => {
  const [showOverlay, setShowOverlay] = useState<boolean>(false)

  const bifrost = useBifrost({ topic: RosTopic.KEYBOARD_DATA });
  const keyboardPoints = useSelector((state: RootState) => state.keyboardDataStore.points);
  const camera_width = useSelector((state: RootState) => state.keyboardDataStore.width);
  const camera_height = useSelector((state: RootState) => state.keyboardDataStore.height);
  const polygonPoints = keyboardPoints.reduce<string[]>((acc, val, idx, arr) => {
    if (idx % 2 === 0) acc.push(`${val},${arr[idx + 1]}`);
    return acc;
  }, []).join(" ");

  useEffect(() => {
    bifrost.syncWithTopic();
  }, [bifrost]);

  const overlayBody = (
    <svg 
      className="absolute top-0 left-0 w-full h-full pointer-events-none"
      viewBox={`0 0 ${camera_width} ${camera_height}`}
      preserveAspectRatio="none"
    >
      <polygon
        points={polygonPoints}
        fill="none"
        stroke="red"
        strokeWidth="2"
      />
    </svg>
  )

  return (
    <OverlayedCameraComponent
      onStreamingStateChange={(s) => setShowOverlay(s === StreamingState.STOPPED)}
      {...props}
      overlay={showOverlay ? overlayBody : <div/>}
    />
  )
}

export default KeyboardOverlayedCameraComponent;
