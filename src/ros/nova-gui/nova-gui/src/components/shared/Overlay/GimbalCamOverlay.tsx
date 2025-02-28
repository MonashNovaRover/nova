/*

 */

import React, {useCallback, useState} from "react";
import {Simulate} from "react-dom/test-utils";
import mouseMove = Simulate.mouseMove;

interface GimbalCamOverlayProps {}

export const GimbalCamOverlay : React.FC<GimbalCamOverlayProps> = (props) => {


  const [position, setPosition] = useState({ x: 100, y: 100 });

  const handleMouseDown = (e) => {
    const startX = e.clientX;
    const startY = e.clientY;
    const startPosX = position.x;
    const startPosY = position.y;

    const onMouseMove = (moveEvent) => {
      const newX = startPosX + (moveEvent.clientX - startX);
      const newY = startPosY + (moveEvent.clientY - startY);
      setPosition({
        x: newX,
        y: newY,
      });
      console.log(newX,newY)
    };

    const onMouseUp = () => {
      document.removeEventListener('mousemove', onMouseMove);
      document.removeEventListener('mouseup', onMouseUp);
    };

    document.addEventListener('mousemove', onMouseMove);
    document.addEventListener('mouseup', onMouseUp);
  };

  return (
    <div>

        </div>
  );
};
