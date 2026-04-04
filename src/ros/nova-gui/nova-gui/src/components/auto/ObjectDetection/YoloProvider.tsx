import React, {createContext, useContext, useRef, useState,} from "react";
import {useYoloDetection, Detection,} from "./useYoloDetection";

interface YoloContextValue {
  registerVideoRef: (
    ref: React.RefObject<HTMLVideoElement>
  ) => number;
  detections: Detection[][];
  inputSize: number;
}

const YoloContext = createContext<YoloContextValue | null>(null);

export function YoloProvider({
                               children,
                             }: {
  children: React.ReactNode;
}) {
  const videoRefs =
    useRef<
      React.RefObject<HTMLVideoElement>[]
    >([]);

  const [refsReady, setRefsReady] = useState<boolean>(false);

  const registerVideoRef = (
    ref: React.RefObject<HTMLVideoElement>
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
        "/models/yolo26n.onnx",
      inputSize: 640,
    });

  return (
    <YoloContext.Provider
      value={{
        registerVideoRef,
        detections,
        inputSize: 640,
      }}
    >
      {children}
    </YoloContext.Provider>
  );
}

export function useYoloContext() {
  const context = useContext(YoloContext);
  if (!context) {
    throw new Error(
      "useYoloContext must be used within YoloProvider"
    );
  }
  return context;
}
