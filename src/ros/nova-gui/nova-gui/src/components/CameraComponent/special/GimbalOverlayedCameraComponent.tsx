
import React, {ReactNode, useEffect, useState} from "react";
import {useBifrost} from "../../../redux/actions/bifrost/useBifrostAction.ts";
import {RosService} from "../../../ros/services/rosService.ts";
import OverlayedCameraComponent from "./OverlayedCameraComponent.tsx";
import {BaseCameraComponentProps} from "../CameraComponent.tsx";
import {CameraSerials} from "../../../views/shared/CamerasPage/CameraPageConstants.tsx";
import {StreamingState} from "../hooks/useCameraStream.ts";
//import {Tooltip} from "@nextui-org/react";

// export interface GimbalCamOverlayProps {
//   cameraSerial: string;
//   overlayMap?: { [k: string]: ReactNode },
//   autostart?: boolean;
// }

export const GimbalOverlayedCameraComponent: React.FC<BaseCameraComponentProps> = (props) => {
  const [showOverlay, setShowOverlay] = useState<boolean>(false)

  // Default stepsize for incrementing angles
  const [step, setStep] = useState(1);
  //const [inputValue, setInputValue] = React.useState("1");

  const serviceBifrost = useBifrost({service: RosService.SCIMBAL_COMMAND});
  const incrementTilt = (step: number) => serviceBifrost.callServiceToRedux({
    angles: [step, 0]
  });
  const incrementPan = (step: number) => serviceBifrost.callServiceToRedux({
    angles: [0, step]
  });

  // // input for varying stepsize
  // <Tooltip
  //   className="text-tiny text-default-500 rounded-md"
  //   content="Press Enter to confirm"
  //   placement="left"
  // >
  //   <input
  //     aria-label="Temperature value"
  //     className="px-1 py-0.5 w-12 text-right text-small text-default-700 font-medium bg-default-100 outline-none transition-colors rounded-small border-medium border-transparent hover:border-primary focus:border-primary"
  //     type="text"
  //     value={inputValue}
  //     onChange={(e) => {
  //       const v = e.target.value;
  //
  //       setInputValue(v);
  //     }}
  //     onKeyDown={(e) => {
  //       if (e.key === "Enter" && !isNaN(Number(inputValue))) {
  //         setStep(Number(inputValue));
  //       }
  //     }}
  //   />
  // </Tooltip>

  //WASD Controls
  useEffect(() => {
    window.addEventListener('keydown', (e) => {
      switch (e.key) {
        case 'a':
          incrementPan(-step)
          break
        case 'd':
          incrementPan(step)
          break
        case 'w':
          incrementTilt(step)
          break
        case 's':
          incrementTilt(-step)
          break
        default:
          break
      }
    });
  }, []);

  // // CLICK & HOLD CONTROLS
  // const handleMouseDown = (e: React.MouseEvent<HTMLDivElement>) => {
  //   const getCenterCoordinates()=>{
  //     //const rect = element.getBoundingClientRect();
  //     const bounds = e.target.getBoundingClientRect() ?? {width: 1, height: 1};
  //     //e.target.
  //     const centerX = bounds.left + (bounds.width / 2);
  //     const centerY = bounds.top + (bounds.height / 2);
  //   }
  //   const getOriginX ={left: 50 %;}
  //   const getOriginY ={top: 50%;}
  //
  //   const posX = position.x;
  //   const posY = position.y;
  //
  //   // let coordsString = "Coordinates: (" + posX + "," + posY + ")";
  //   // document.getElementById("demoCoords").innerHTML = coordsString;
  //
  //   const dispX = posX-getOriginX;
  //   const dispY = posY-getOriginY;
  // };


  return (
    <OverlayedCameraComponent
      onStreamingStateChange={(s) => setShowOverlay(s === StreamingState.STREAMING)}
      {...props}
      overlay={showOverlay ? <div className="self-center grow h-0.5 bg-black"/> : <div/>}
      //<p id="demoCoords"></p>
    />

  );
}
