import {StreamingState} from "../hooks/useCameraStream.ts";
import {Button, ButtonProps} from "@nextui-org/react";
import {Eye, EyeOff, Play, Square} from "react-feather";
import React from "react";

export interface CameraSessionStartStopButtonProps extends ButtonProps {
  streamingState: StreamingState,
  sendSessionStartMessage: () => void,
  closeSession: () => void,
}

/**
 * A reusable version of the start/stop button from CameraComponent.tsx
 * @constructor
 */
const CameraSessionStartStopButton: React.FC<CameraSessionStartStopButtonProps> = (props) => {
  const {
    streamingState,
    sendSessionStartMessage,
    closeSession,
    ...buttonProps
  } = props;

  return (
    streamingState === StreamingState.STOPPED ? (
      <Button {...buttonProps}
        size="sm"
        color="primary"
        className="w-min mx-auto"
        onPress={sendSessionStartMessage}
      >
        <Eye size="15px" fill="white" />
        Show
      </Button>
    ) : (
      <Button {...buttonProps}
        size="sm"
        color="danger"
        className="w-min mx-auto"
        onPress={closeSession}
      >
        <EyeOff size="15px" fill="white" /> Hide
      </Button>
    )
  );
}

export default CameraSessionStartStopButton;
