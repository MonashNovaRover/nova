import React, { useCallback, useMemo } from "react";
import { useBifrost } from "../../../../redux/actions/bifrost/useBifrostAction.ts";
import { RosService } from "../../../../ros/services/rosService.ts";
import OverlayedCameraComponent from "./OverlayedCameraComponent.tsx";
import { BaseCameraComponentProps } from "../CameraComponent.tsx";
import { Input, Tooltip } from "@nextui-org/react";
import { useGenericStore } from "../../../../hooks/useGenericStore.ts";

export const GimbalOverlayedCameraComponent: React.FC<BaseCameraComponentProps> = (props) => {
  // Default step size for incrementing angles
  const [step, setStep] = useGenericStore<string>("scimbalStepSize");
  const stepNumber = useMemo(() => {
    const val = parseInt(step)
    if (isNaN(val))
      return 1
    return val
  }, [step]);

  const serviceBifrost = useBifrost({ service: RosService.SCIMBAL_COMMAND });
  const incrementTilt = useCallback((step: number) => serviceBifrost.callServiceToRedux({
    angles: [step, 0]
  }, {
    responseToast: false
  }), [serviceBifrost]);
  const incrementPan = useCallback((step: number) => serviceBifrost.callServiceToRedux({
    angles: [0, step],
  }, {
    responseToast: false
  }), [serviceBifrost]);


  let clickAndHoldInterval: ReturnType<typeof setInterval>;

  // Handle click and hold movement
  const handleClickandHoldDown = useCallback((e: React.MouseEvent<HTMLDivElement>) => {
    const rect = e.currentTarget.getBoundingClientRect();
    const x = e.clientX - rect.left;
    const y = e.clientY - rect.top;

    const centerX = rect.width / 2
    const centerY = rect.height / 2;

    // 1 for right, and -1 for left
    const horizontalDirection = x >= centerX ? 1 : -1
    // 1 for down, and -1 for up
    const verticalDirection = y >= centerY ? 1 : -1


    clickAndHoldInterval = setInterval(() => {

      incrementPan(stepNumber * horizontalDirection);
      incrementTilt(stepNumber * verticalDirection);

    }, 50)
  }, [incrementPan, incrementTilt, stepNumber])

  const handleClickandHoldRelease = () => {
    clearInterval(clickAndHoldInterval)
  }

  // WASD Controls
  const handleInput = useCallback((e: React.KeyboardEvent<HTMLDivElement>) => {
    switch (e.key.toLowerCase()) {
      case 'a': incrementPan(-stepNumber); break;
      case 'd': incrementPan(stepNumber); break;
      case 'w': incrementTilt(-stepNumber); break;
      case 's': incrementTilt(stepNumber); break;
    }
  }, [incrementPan, incrementTilt, stepNumber])


  const stepSizeInput = (
    <>
      <Tooltip
        className="text-tiny text-default-500 rounded-md"
        content="Press Enter to confirm"
        placement="right"
        key={"Step Size"}
      >
        <Input
          aria-label="Scimbal cam step size"
          label="Step size"
          labelPlacement="inside"
          className=""
          type="number"
          value={step}
          onValueChange={setStep}
        />
      </Tooltip>
    </>
  )

  return (
    <div
      tabIndex={0}
      onKeyDown={handleInput}
      onMouseDown={handleClickandHoldDown}
          onMouseUp={handleClickandHoldRelease}
          onMouseLeave={handleClickandHoldRelease}
      
    >
      <OverlayedCameraComponent
        {...props}
        overlay={<div 
          
          onMouseDown={(e) => e.stopPropagation()}
    onMouseUp={(e) => e.stopPropagation()}
    onMouseLeave={(e) => e.stopPropagation()}
        />}
        settingsFormChildren={stepSizeInput}
        
      />
    </div>
  );
}
