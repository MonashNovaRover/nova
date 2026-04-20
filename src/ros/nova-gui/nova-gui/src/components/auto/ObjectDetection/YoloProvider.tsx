import React, { createContext, useContext, useRef } from "react";
import { ActiveYoloConfig } from "./YoloConfig";
import { Detection, useYoloDetection } from "./useYoloDetection";

interface YoloContextValue {
  // Accept nullable refs since React sets .current after mount.
  registerVideoRef: (ref: React.RefObject<HTMLVideoElement | null>) => number;
  detections: Detection[][];
  inputSize: number;
}

const YoloContext = createContext<YoloContextValue | null>(null);

export function YoloProvider({ children }: { children: React.ReactNode }) {
  // Collect video refs from all mounted camera layers.
  const videoRefs = useRef<React.RefObject<HTMLVideoElement | null>[]>([]);

  const registerVideoRef = (ref: React.RefObject<HTMLVideoElement | null>) => {
    // Register in order and return the index for lookup.
    videoRefs.current.push(ref);

    return videoRefs.current.length - 1;
  };

  // Run detection whenever refs are available.
  const detections = useYoloDetection({
    // Preserve the existing mutable ref registration flow because the state-based rewrite broke overlays.
    // eslint-disable-next-line react-hooks/refs
    videoRefs: videoRefs.current,
    modelPath: `/models/${ActiveYoloConfig.modelName}`,
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
