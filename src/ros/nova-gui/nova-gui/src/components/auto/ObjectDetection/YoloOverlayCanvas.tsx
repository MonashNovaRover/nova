import { useEffect, useRef } from "react";
import { ActiveYoloConfig } from "./YoloConfig";
import { Detection } from "./useYoloDetection";
import AutosizedCanvas from "../../shared/components/AutosizedCanvas/AutosizedCanvas.tsx";

export default function YoloOverlayCanvas({
  detections,
  videoRef,
}: {
  detections: Detection[];
  // React refs are null until mounted; reflect that in the type.
  videoRef: React.RefObject<HTMLVideoElement | null>;
}) {
  // Canvas overlay that draws detections in video space.
  // AutosizedCanvas expects a non-null ref object; React assigns .current post-mount.
  const canvasRef = useRef<HTMLCanvasElement>(null!);

  useEffect(() => {
    const canvas = canvasRef.current;
    const video = videoRef.current;

    if (!canvas || !video) return;

    // console.log(detections, video.videoWidth, video.videoHeight);

    const ctx = canvas.getContext("2d");

    if (!ctx) return;

    // Detection coordinates are now in video pixel space (already adjusted for letterboxing).
    // We just need to scale from video space to canvas space.
    const containerWidth = canvas.width;
    const containerHeight = canvas.height;
    const videoWidth = video.videoWidth;
    const videoHeight = video.videoHeight;

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
      // Coordinates are already in video pixel space, just scale to canvas space
      const x = d.box.x * scale + offsetX;
      const y = d.box.y * scale + offsetY;
      const width = d.box.width * scale;
      const height = d.box.height * scale;

      ctx.strokeRect(x, y, width, height);

      // Create label
      const className = ActiveYoloConfig.classNames[d.classId] ?? `#${d.classId}`;
      const label = `${className} ${Math.round(d.score * 100)}%`;
      const textX = x;
      const textY = Math.max(y - 14, 0);

      // Draw label background for readability.
      ctx.fillStyle = "rgba(0, 0, 0, 0.6)";
      ctx.fillRect(textX, textY, ctx.measureText(label).width + 6, 14);
      ctx.fillStyle = "#00ff88";
      ctx.fillText(label, textX + 3, textY + 1);
    });
  }, [detections, videoRef]);

  return (
    <AutosizedCanvas
      canvasRef={canvasRef}
      // Match canvas size to the video element.
      // Preserve the existing overlay sizing behavior that matches the working YOLO path.
      // eslint-disable-next-line react-hooks/refs
      sizeTarget={videoRef.current}
      className="absolute inset-0 pointer-events-none"
    />
  );
}
