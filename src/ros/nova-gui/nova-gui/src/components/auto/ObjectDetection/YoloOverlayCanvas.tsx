import React, { useEffect, useRef } from "react";
import { Detection } from "./useYoloDetection";
import cocoClasses from "./cocoClasses";
import AutosizedCanvas from "../../shared/components/AutosizedCanvas/AutosizedCanvas.tsx";

export default function YoloOverlayCanvas({
  detections,
  videoRef,
  modelInputSize,
}: {
  detections: Detection[];
  videoRef: React.RefObject<HTMLVideoElement>;
  modelInputSize: number;
}) {
  // Canvas overlay that draws detections in video space.
  const canvasRef = useRef<HTMLCanvasElement>(null);

  useEffect(() => {
    const canvas = canvasRef.current;
    const video = videoRef.current;

    if (!canvas || !video) return;

    console.log(detections, video.videoWidth, video.videoHeight);

    const ctx = canvas.getContext("2d");

    if (!ctx) return;

    // console.log("YOLO overlay sizes", {
    //   canvasWidth: canvas.width,
    //   canvasHeight: canvas.height,
    //   videoWidth: video.videoWidth,
    //   videoHeight: video.videoHeight,
    //   modelInputSize,
    // });

    // Map model input coordinates -> video pixels.
    const containerWidth = canvas.width;
    const containerHeight = canvas.height;
    const videoWidth = video.videoWidth;
    const videoHeight = video.videoHeight;
    const modelToVideoX = modelInputSize > 0 ? videoWidth / modelInputSize : 1;
    const modelToVideoY = modelInputSize > 0 ? videoHeight / modelInputSize : 1;

    // Scale/center the video to match how it's rendered in the container.
    const scale = Math.max(
      containerWidth / videoWidth,
      containerHeight / videoHeight
    );
    const renderWidth = videoWidth * scale;
    const renderHeight = videoHeight * scale;
    const offsetX = (containerWidth - renderWidth) / 2;
    const offsetY = (containerHeight - renderHeight) / 2;

    // Clear previous frame and set up drawing styles.
    ctx.clearRect(0, 0, canvas.width, canvas.height);

    ctx.lineWidth = 2;
    ctx.strokeStyle = "#00ff88";
    ctx.font = "12px sans-serif";
    ctx.textBaseline = "top";

    detections.forEach((d) => {
      // Convert model-space box -> scaled canvas-space box.
      const videoX = d.box.x * modelToVideoX;
      const videoY = d.box.y * modelToVideoY;
      const videoWidthBox = d.box.width * modelToVideoX;
      const videoHeightBox = d.box.height * modelToVideoY;

      const x = videoX * scale + offsetX;
      const y = videoY * scale + offsetY;
      const width = videoWidthBox * scale;
      const height = videoHeightBox * scale;

      ctx.strokeRect(x, y, width, height);

      // Create label
      const className = cocoClasses[d.classId] ?? `#${d.classId}`;
      const label = `${className} ${Math.round(d.score * 100)}%`;
      const textX = x;
      const textY = Math.max(y - 14, 0);

      // Draw label background for readability.
      ctx.fillStyle = "rgba(0, 0, 0, 0.6)";
      ctx.fillRect(textX, textY, ctx.measureText(label).width + 6, 14);
      ctx.fillStyle = "#00ff88";
      ctx.fillText(label, textX + 3, textY + 1);
    });
  }, [detections, modelInputSize, videoRef]);

  return (
    <AutosizedCanvas
      canvasRef={canvasRef}
      // Match canvas size to the video element.
      sizeTarget={videoRef.current}
      className="absolute inset-0 pointer-events-none"
    />
  );
}
