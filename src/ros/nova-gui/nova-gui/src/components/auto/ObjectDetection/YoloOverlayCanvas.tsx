import React, {
  useEffect,
  useRef,
} from "react";

export default function YoloOverlayCanvas({
                                            detections,
                                            videoRef,
                                          }) {
  const canvasRef =
    useRef<HTMLCanvasElement>(null);

  useEffect(() => {
    const canvas =
      canvasRef.current;

    const video =
      videoRef.current;

    if (!canvas || !video)
      return;

    canvas.width =
      video.videoWidth;

    canvas.height =
      video.videoHeight;

    const ctx =
      canvas.getContext("2d");

    if (!ctx) return;

    ctx.clearRect(
      0,
      0,
      canvas.width,
      canvas.height
    );

    ctx.lineWidth = 2;
    ctx.strokeStyle =
      "#00ff88";

    detections.forEach(
      (d) => {
        ctx.strokeRect(
          d.box.x,
          d.box.y,
          d.box.width,
          d.box.height
        );
      }
    );
  }, [detections]);

  return (
    <canvas
      ref={canvasRef}
      className="absolute inset-0 pointer-events-none"
    />
  );
}