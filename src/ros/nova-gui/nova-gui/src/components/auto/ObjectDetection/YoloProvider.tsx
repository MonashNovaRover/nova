import React, {
  createContext,
  useContext,
  useRef,
  useState,
} from "react";

import {
  useYoloDetection,
} from "./useYoloDetection";

const YoloContext =
  createContext<any>(null);

export function YoloProvider({
                               children,
                             }) {
  const videoRefs =
    useRef<
      React.RefObject<HTMLVideoElement>[]
    >([]);

  const [refsReady, setRefsReady] =
    useState(false);

  const registerVideoRef = (
    ref
  ) => {
    videoRefs.current.push(ref);
    setRefsReady(true);

    return videoRefs.current.length - 1;
  };

  const detections =
    useYoloDetection({
      videoRefs:
      videoRefs.current,
      modelPath:
        "/models/urc-object-detection.onnx",
    });

  return (
    <YoloContext.Provider
      value={{
        registerVideoRef,
        detections,
      }}
    >
      {children}
    </YoloContext.Provider>
  );
}

export function useYoloContext() {
  return useContext(YoloContext);
}
