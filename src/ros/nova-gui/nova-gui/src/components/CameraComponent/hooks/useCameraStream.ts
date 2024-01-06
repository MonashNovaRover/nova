import { useState } from "react";

export enum StreamingState {
  STOPPED,
  LOADING,
  STREAMING,
}

export const useCameraStream = () => {
  const [streamingState, setStreamingState] = useState<StreamingState>(
    StreamingState.STOPPED
  );

  const initiateStreaming = () => {
    setStreamingState(StreamingState.STREAMING);
  };

  return { streamingState, initiateStreaming };
};
