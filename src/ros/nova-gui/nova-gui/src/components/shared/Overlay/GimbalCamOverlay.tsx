/*

 */

import React, {useCallback, useEffect, useState} from "react";
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
  //   (state: RootState) => state.scimbalCamResponse
  // );

  const serviceBifrost = useBifrost({ service: RosService.SCIMBAL_COMMAND});
  const incrementTilt = (step: number) => serviceBifrost.callServiceToRedux({
    angles: [step, 0]
  });
  const incrementPan = (step: number) => serviceBifrost.callServiceToRedux({
    angles: [0, step]
  });

  //const [position, setPosition] = useState({ x: 100, y: 100 });

  //WASD Controls
  useEffect(() => {
  window.addEventListener('keydown', (e) =>{
    switch(e.key){
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
        continue
    }
  });
  }, []);

  // // CLICK & HOLD CONTROLS
  // const handleMouseDown = (e: React.MouseEvent<HTMLDivElement>) => {
  //   const getCenterCoordinates()=>{
  //     //const rect = element.getBoundingClientRect();
  //     const bounds = e.target.getBoundingClientRect() ?? {width: 1, height: 1};
  //     e.target.
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
  //
  //
  // };


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
    <div onMouseDown={handleMouseDown}>
      <p id="demoCoords"></p>

    </div>
  );
};
