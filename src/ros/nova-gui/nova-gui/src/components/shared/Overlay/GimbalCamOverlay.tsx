/*

 */

import React, {useCallback, useState} from "react";
import {Simulate} from "react-dom/test-utils";
import mouseMove = Simulate.mouseMove;
import {useSelector} from "react-redux";
import {RootState} from "../../../redux/RootState.ts";
import {useBifrost} from "../../../redux/actions/bifrost/useBifrostAction.ts";
import {RosService} from "../../../ros/services/rosService.ts";

interface GimbalCamOverlayProps {}

export const GimbalCamOverlay : React.FC<GimbalCamOverlayProps> = (props) => {

  const [step, setStep] = useState(1);

  // const gimbalService = useSelector(
  //   (state: RootState) => state.MoveScimbalCam
  // );

  const serviceBifrost = useBifrost({ service: RosService.MoveScimbalCam});
  const incrementTilt = (step) => serviceBifrost.callServiceToRedux({angles[0]: MoveScimbalCam.angles[0]+step});
  const incrementPan = (step) => serviceBifrost.callServiceToRedux({angles[1]: MoveScimbalCam.angles[1]}+step);

  const [position, setPosition] = useState({ x: 100, y: 100 });

  //WASD Controls
  window.addEventListener('keydown', (e) =>{
    switch(e.key){
      case 'a':
        incrementPan(-step)
      case 'd':
        incrementPan(step)
      case 'w':
        incrementTilt(step)
      case 's':
        incrementTilt(-step)
    }
  });

  // CLICK & DRAG Controls
  // const handleMouseDown = (e) => {
  //   const startX = e.clientX;
  //   const startY = e.clientY;
  //   const startPosX = position.x;
  //   const startPosY = position.y;
  //
  //   const onMouseMove = (moveEvent) => {
  //     const newX = startPosX + (moveEvent.clientX - startX);
  //     const newY = startPosY + (moveEvent.clientY - startY);
  //     setPosition({
  //       x: newX,
  //       y: newY,
  //     });
  //     console.log(newX,newY)
  //   };
  //
  //   const onMouseUp = () => {
  //     document.removeEventListener('mousemove', onMouseMove);
  //     document.removeEventListener('mouseup', onMouseUp);
  //   };
  //
  //   document.addEventListener('mousemove', onMouseMove);
  //   document.addEventListener('mouseup', onMouseUp);
  // };

  return (
    <div>

        </div>
  );
};
