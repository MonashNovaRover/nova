import {BaseCameraComponentProps} from "../CameraComponent.tsx";
import {FC, useState} from "react";
import OverlayedCameraComponent from "./OverlayedCameraComponent.tsx";
import {StreamingState} from "../hooks/useCameraStream.ts";
import {RosTopic} from "../../../ros/topics/rosTopic.ts";
import {useBifrost} from "../../../redux/actions/bifrost/useBifrostAction.ts";

const CAMERA_WIDTH = 640
const CAMERA_HEIGHT = 480

// dummy points
const overlayPoints = [
  { x: 100, y: 100 },
  { x: 600, y: 100 },
  { x: 600, y: 200 },
  { x: 100, y: 200 }
];

export const KeyboardOverlayedCameraComponent: FC<BaseCameraComponentProps> = (props) => {
  const [showOverlay, setShowOverlay] = useState<boolean>(false)
  const polygonPoints = overlayPoints.map(p => `${p.x},${p.y}`).join(" ");

  const bifrost = useBifrost({ topic: RosTopic.NIR_DATA });
  const takeReading = (led: number) => bifrost.callService({led: led});

  useEffect(() => {
    bifrost.syncWithTopic();
  }, [bifrost]);

  return (
    <OverlayedCameraComponent
      onStreamingStateChange={(s) => setShowOverlay(s === StreamingState.STOPPED)}
      {...props}
      overlay={showOverlay ? (
        <svg 
          className="absolute top-0 left-0 w-full h-full pointer-events-none"
          viewBox={`0 0 ${CAMERA_WIDTH} ${CAMERA_HEIGHT}`}
          preserveAspectRatio="none"
        >
          <polygon
            points={polygonPoints}
            fill="none"
            stroke="red"
            strokeWidth="2"
          />
        </svg>
      ) : <div/>}
    />
  )
}

export default KeyboardOverlayedCameraComponent;
