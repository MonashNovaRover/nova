import React, {
  useEffect,
  useRef,
} from "react";
import { Detection } from "./useYoloDetection";

export default function YoloOverlayCanvas({
                                            detections,
                                            videoRef,
                                          }: {
  detections: Detection[];
  videoRef: React.RefObject<HTMLVideoElement>;
}) {
  const canvasRef = useRef<HTMLCanvasElement>(null);

  useEffect(() => {
    const canvas = canvasRef.current;

    const video = videoRef.current;

    if (!canvas || !video) return;

    canvas.width = video.videoWidth;

    canvas.height = video.videoHeight;
    console.log(detections, video.videoWidth, video.videoHeight)

    const ctx = canvas.getContext("2d");

    if (!ctx) return;

    ctx.clearRect(
      0,
      0,
      canvas.width,
      canvas.height
    );

    ctx.lineWidth = 2;
    ctx.strokeStyle = "#00ff88";
    ctx.font = "12px sans-serif";
    ctx.textBaseline = "top";

    detections.forEach(
      (d) => {
        ctx.strokeRect(
          d.box.x,
          d.box.y,
          d.box.width,
          d.box.height
        );
        const label = `#${d.classId} ${Math.round(d.score * 100)}%`;
        const textX = d.box.x;
        const textY = Math.max(d.box.y - 14, 0);
        ctx.fillStyle = "rgba(0, 0, 0, 0.6)";
        ctx.fillRect(textX, textY, ctx.measureText(label).width + 6, 14);
        ctx.fillStyle = "#00ff88";
        ctx.fillText(label, textX + 3, textY + 1);
      }
    );
  }, [detections, videoRef]);

  return (
    <canvas
      ref={canvasRef}
      className="absolute inset-0 pointer-events-none"
    />
  );
}
