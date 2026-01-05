import React, { useCallback, useMemo, useEffect } from "react";
import { useBifrost } from "../../../../redux/actions/bifrost/useBifrostAction.ts";
import { RosService } from "../../../../ros/services/rosService.ts";
import OverlayedCameraComponent from "./OverlayedCameraComponent.tsx";
import { BaseCameraComponentProps } from "../CameraComponent.tsx";
import { Input, Tooltip, Switch } from "@nextui-org/react";
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
  const [clickAndHoldEnabled, setClickAndHoldEnabled] = useGenericStore<boolean>("clickAndHold");
  const [windowWideWASD, setWindowWideWASD] = useGenericStore<boolean>("windowWideWASD");
  const [hasFocus, setHasFocus] = React.useState(false);

  // Handle click and hold movement
  const handleClickandHoldDown = useCallback((e: React.MouseEvent<HTMLDivElement>) => {
    if (!clickAndHoldEnabled) return;
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

  // WASD Controls Component Wide
  const handleInput = useCallback((e: React.KeyboardEvent<HTMLDivElement>) => {
    switch (e.key.toLowerCase()) {
      case 'a': incrementPan(-stepNumber); break;
      case 'd': incrementPan(stepNumber); break;
      case 'w': incrementTilt(-stepNumber); break;
      case 's': incrementTilt(stepNumber); break;
    }
  }, [incrementPan, incrementTilt, stepNumber])

  useEffect(() => {
    if (!windowWideWASD) return;

    const handleKey = (e: KeyboardEvent) => {
      switch (e.key.toLowerCase()) {
        case 'a':
          incrementPan(-stepNumber); //negative pan is left
          break;
        case 'd':
          incrementPan(stepNumber);//positive pan is right
          break;
        case 'w':
          incrementTilt(-stepNumber);//negative tilt is up
          break;
        case 's':
          incrementTilt(stepNumber);//positive tilt is down
          break;
        default:
          break;
      }
    };


    window.addEventListener('keyup', handleKey);
    return () => window.removeEventListener('keyup', handleKey);
  }, [incrementPan, incrementTilt, stepNumber, windowWideWASD]);


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
      <Switch
        size="sm"
        isSelected={clickAndHoldEnabled}
        onValueChange={setClickAndHoldEnabled}
      >
        Enable Click & Hold
      </Switch>
      <Switch
        size="sm"
        isSelected={windowWideWASD}
        onValueChange={setWindowWideWASD}
      >
        Enable Window Wide WASD
      </Switch>
    </>
  )

  return (
    <div
      tabIndex={0}
      onKeyDown={windowWideWASD ? undefined : handleInput}
      onMouseDown={handleClickandHoldDown}
      onMouseUp={handleClickandHoldRelease}
      onMouseLeave={handleClickandHoldRelease}
      onFocus={() => setHasFocus(true)}
      onBlur={() => setHasFocus(false)}
      style={
        windowWideWASD || hasFocus
          ? {
            outline: '2px solid #4da3ff',
            outlineOffset: '2px',
          }
          : undefined
      }

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
