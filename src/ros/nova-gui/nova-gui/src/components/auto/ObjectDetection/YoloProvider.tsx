import React, { createContext, useContext, useRef, useState } from "react";
import { Detection, useYoloDetection } from "./useYoloDetection";

interface YoloContextValue {
  registerVideoRef: (ref: React.RefObject<HTMLVideoElement>) => number;
  detections: Detection[][];
  inputSize: number;
}

const YoloContext = createContext<YoloContextValue | null>(null);

export function YoloProvider({ children }: { children: React.ReactNode }) {
  // Collect video refs from all mounted camera layers.
  const videoRefs = useRef<React.RefObject<HTMLVideoElement>[]>([]);

  const [refsReady, setRefsReady] = useState<boolean>(false);

  const registerVideoRef = (ref: React.RefObject<HTMLVideoElement>) => {
    // Register in order and return the index for lookup.
    videoRefs.current.push(ref);
    setRefsReady(true);

    return videoRefs.current.length - 1;
  };

  // Run detection whenever refs are available.
  const detections = useYoloDetection({
    videoRefs: videoRefs.current,
    modelPath: "/models/yolo26n.onnx",
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
    // Enforce provider usage for safe context access.
    throw new Error("useYoloContext must be used within YoloProvider");
  }
  return context;
}
