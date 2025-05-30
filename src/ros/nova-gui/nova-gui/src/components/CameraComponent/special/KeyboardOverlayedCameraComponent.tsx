import {BaseCameraComponentProps} from "../CameraComponent.tsx";
import {FC, useState, useEffect} from "react";
import OverlayedCameraComponent from "./OverlayedCameraComponent.tsx";
import {StreamingState} from "../hooks/useCameraStream.ts";
import {RosTopic} from "../../../ros/topics/rosTopic.ts";
import {useBifrost} from "../../../redux/actions/bifrost/useBifrostAction.ts";

interface IKeyboardOverlayedCameraComponentProps extends BaseCameraComponentProps {
  cameraSerial: string;
  keyboardPoints: number[];
  overridePoints: number[];
  camera_width: number;
  camera_height: number;
}

export const KeyboardOverlayedCameraComponent: FC<IKeyboardOverlayedCameraComponentProps> = (props) => {
  const [showOverlay, setShowOverlay] = useState<boolean>(false)

  const bifrost = useBifrost({ topic: RosTopic.KEYBOARD_DATA });
  useEffect(() => {
    bifrost.syncWithTopic();
  }, [bifrost]);

  const polygonPoints = (points: number[]) => points.reduce<string[]>((acc:string[], val:number, idx:number, arr:number[]) => {
    if (idx % 2 === 0) acc.push(`${val},${arr[idx + 1]}`);
    return acc;
  }, []).join(" ");

  const overlayBody = (
    <div>
      <svg 
        className="absolute top-0 left-0 w-full h-full pointer-events-none"
        viewBox={`0 0 ${props.camera_width} ${props.camera_height}`}
        preserveAspectRatio="none"
      >
        <polygon
          points={polygonPoints(props.keyboardPoints)}
          fill="none"
          stroke="red"
          strokeWidth="2"
        />
        <polygon
          points={polygonPoints(props.overridePoints)}
          fill="none"
          stroke="green"
          strokeWidth="2"
        />
      </svg>
      <div className="self-center grow h-0.5 bg-black"/> 
    </div>
  )

  return (
    <OverlayedCameraComponent
      onStreamingStateChange={(s) => setShowOverlay(s === StreamingState.STREAMING)}
      {...props}
      overlay={showOverlay ? overlayBody : <div/>}
    />
  )
}

export default KeyboardOverlayedCameraComponent;
