import React, {useCallback, useEffect, useMemo, useState} from "react";
import {useBifrost} from "../../../redux/actions/bifrost/useBifrostAction.ts";
import {RosService} from "../../../ros/services/rosService.ts";
import OverlayedCameraComponent from "./OverlayedCameraComponent.tsx";
import {BaseCameraComponentProps} from "../CameraComponent.tsx";
import {StreamingState} from "../hooks/useCameraStream.ts";
import {Input, Tooltip} from "@nextui-org/react";
import {useGenericStore} from "../../../hooks/useGenericStore.ts";

export const GimbalOverlayedCameraComponent: React.FC<BaseCameraComponentProps> = (props) => {
  const [showOverlay, setShowOverlay] = useState<boolean>(false)

  // Default step size for incrementing angles
  const [step, setStep] = useGenericStore<string>("scimbalStepSize");
  const stepNumber = useMemo(() => {
    const val = parseInt(step)
    if (isNaN(val))
      return 1
    return val
  }, [step]);


  const serviceBifrost = useBifrost({service: RosService.SCIMBAL_COMMAND});
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

  //WASD Controls
  useEffect(() => {
    window.addEventListener('keyup', (e) => {
      switch (e.key) {
        case 'a':
          incrementPan(-stepNumber)
          break
        case 'd':
          incrementPan(stepNumber)
          break
        case 'w':
          incrementTilt(-stepNumber)
          break
        case 's':
          incrementTilt(stepNumber)
          break
        default:
          break
      }
    });
  }, [incrementPan, incrementTilt, step]);

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
              label= "Step size"
              labelPlacement= "inside"
              className=""
              type= "number"
              value={step}
              onValueChange={setStep}
          />
        </Tooltip>
      </>
  )

  return (
    <div>
      <OverlayedCameraComponent
        onStreamingStateChange={(s) => setShowOverlay(s === StreamingState.STREAMING)}
        {...props}
        overlay={<div/>}
        settingsFormChildren={stepSizeInput}
      />
    </div>
  );
}
