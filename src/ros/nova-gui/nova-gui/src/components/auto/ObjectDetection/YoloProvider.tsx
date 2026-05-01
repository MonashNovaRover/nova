import React, { createContext, useContext, useRef, useEffect, useState } from "react";
import { useSelector } from "react-redux";
import * as ROSLIB from "roslib"; // Ensure you run: npm install roslib
import { ActiveYoloConfig } from "./YoloConfig";
import { Detection, useYoloDetection } from "./useYoloDetection";
import { RootState } from "../../../redux/RootState";

interface YoloContextValue {
  registerVideoRef: (ref: React.RefObject<HTMLVideoElement | null>) => number;
  detections: Detection[][];
}

const YoloContext = createContext<YoloContextValue | null>(null);

export function YoloProvider({ children }: { children: React.ReactNode }) {
  const videoRefs = useRef<React.RefObject<HTMLVideoElement | null>[]>([]);
  const baseStationIP = useSelector((state: RootState) => state.uiState.baseStationIP);
  
  // State to track if ROS is actually connected
  const [isRosConnected, setIsRosConnected] = useState(false);

  // Refs to hold the ROS connection and Topic so they persist across renders
  const rosRef = useRef<ROSLIB.Ros | null>(null);
  const topicRef = useRef<ROSLIB.Topic<any> | null>(null);

  // Initialize ROS Connection on Mount
  useEffect(() => {
    const ros = new ROSLIB.Ros({
      url: "ws://" + baseStationIP + ":9090", 
    });

    ros.on("connection", () => {
      console.log("ROS: Connected to websocket server.");
      setIsRosConnected(true);
    });
    
    ros.on("error", (error) => {
      console.error("ROS: Error connecting:", error);
      setIsRosConnected(false);
    });
    
    ros.on("close", () => {
      console.log("ROS: Connection closed.");
      setIsRosConnected(false);
    });

    // Detection2DArray topic definition
    const detectionTopic = new ROSLIB.Topic({
      ros: ros,
      name: "/yolo/object_detections",
      messageType: "nova_interfaces/Detection2DArray",
    });

    rosRef.current = ros;
    topicRef.current = detectionTopic;

    // Cleanup connection when the provider unmounts
    return () => {
      ros.close();
    };
  }, [baseStationIP]);

  const registerVideoRef = (ref: React.RefObject<HTMLVideoElement | null>) => {
    videoRefs.current.push(ref);
    return videoRefs.current.length - 1;
  };

  const detections = useYoloDetection({
    // eslint-disable-next-line react-hooks/refs
    videoRefs: videoRefs.current,
    modelPath: `/models/${ActiveYoloConfig.modelName}`,
    outputFormat: ActiveYoloConfig.outputFormat,
  });

  // Publish Logic
  useEffect(() => {
    if (!topicRef.current || !detections) return;
    
    // Don't try to publish if the websocket is offline
    if (!isRosConnected) return;

    const activeVideo = videoRefs.current[0]?.current;
    if (!activeVideo) return;

    const vidWidth = activeVideo.videoWidth || 640; // Fallback to 640p width
    const vidHeight = activeVideo.videoHeight || 480; // Fallback to 480p height

    const activeDetections = detections.flat();

    const msg = {
      header: {
        frame_id: "camera_link",
        stamp: { secs: 0, nsecs: 0 },
      },
      detections: activeDetections.map((d) => ({
        class: ActiveYoloConfig.classNames[d.classId] ?? `class_${d.classId}`,
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

    topicRef.current.publish(msg);
  }, [detections, isRosConnected]);

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