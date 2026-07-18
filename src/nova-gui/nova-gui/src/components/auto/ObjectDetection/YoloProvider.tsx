import React, { createContext, useContext, useRef, useEffect } from "react";
import { getYoloConfig } from "./YoloConfig";
import { Detection, useYoloDetection } from "./useYoloDetection";
import { useBifrost } from "../../../redux/actions/bifrost/useBifrostAction";
import { RosTopic } from "../../../ros/topics/rosTopic";
import { useGenericStore } from "../../../hooks/useGenericStore.ts";

interface YoloContextValue {
  registerVideoRef: (ref: React.RefObject<HTMLVideoElement | null>) => number;
  detections: Detection[][];
}

const YoloContext = createContext<YoloContextValue | null>(null);

export function YoloProvider({ children }: { children: React.ReactNode }) {
  const videoRefs = useRef<React.RefObject<HTMLVideoElement | null>[]>([]);
  const bifrostDetections = useBifrost({ topic: RosTopic.YOLO_DETECTIONS });
  const [activeModelId] = useGenericStore<string>("yoloActiveModel");
  const activeYoloConfig = getYoloConfig(activeModelId);

  const registerVideoRef = (ref: React.RefObject<HTMLVideoElement | null>) => {
    videoRefs.current.push(ref);
    return videoRefs.current.length - 1;
  };

  const detections = useYoloDetection({
    // eslint-disable-next-line react-hooks/refs
    videoRefs: videoRefs.current,
    modelPath: `/models/${activeYoloConfig.modelName}`,
    outputFormat: activeYoloConfig.outputFormat,
  });

  // Publish Logic
  useEffect(() => {
    if (!detections) return;

    const activeVideo = videoRefs.current[0]?.current;
    if (!activeVideo) return;

    const vidWidth = activeVideo.videoWidth || 640; // Fallback to 640p width
    const vidHeight = activeVideo.videoHeight || 480; // Fallback to 480p height

    const activeDetections = detections.flat();

    const nowMs = Date.now();
    const sec = Math.floor(nowMs / 1000);
    const nanosec = (nowMs % 1000) * 1_000_000;
    const msg = {
      header: {
        frame_id: "camera_link",
        stamp: { sec, nanosec },
      },
      detections: activeDetections.map((d) => ({
        class_name: activeYoloConfig.classNames[d.classId] ?? `class_${d.classId}`,
        score: d.score,
        pose: {
          position: {
            // Publish center in pixel coordinates as required by Detection2D.msg.
            x: d.box.x + (d.box.width / 2),
            y: d.box.y + (d.box.height / 2),
            z: 0.0,
          },
          orientation: { x: 0.0, y: 0.0, z: 0.0, w: 1.0 },
        },
        image_width: vidWidth,
        image_height: vidHeight,
      })),
    };

    bifrostDetections.publishToTopic(msg);
  }, [detections, bifrostDetections, activeYoloConfig]);

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
  const context = useContext(YoloContext);
  if (!context) {
    throw new Error("useYoloContext must be used within YoloProvider");
  }
  return context;
}