import { useEffect, useRef, useState } from "react";

export interface Detection {
  // Class index in the model's label set.
  classId: number;
  // Confidence score after combining objectness and class score.
  score: number;
  box: {
    // Top-left corner x in model input pixel space.
    x: number;
    // Top-left corner y in model input pixel space.
    y: number;
    // Box width in model input pixels.
    width: number;
    // Box height in model input pixels.
    height: number;
  };
}

interface Props {
  // React refs are nullable until mount.
  videoRefs: React.RefObject<HTMLVideoElement | null>[];
  modelPath: string;
  inputSize?: number;
  intervalMs?: number;
  scoreThreshold?: number;
}

type WorkerInitMessage = {
  type: "init";
  modelPath: string;
  inputSize: number;
  scoreThreshold: number;
  useWebGPU: boolean;
};

type WorkerFrameMessage = {
  type: "frame";
  batchId: number;
  frames: ImageBitmap[];
};

type WorkerResultMessage = {
  type: "result";
  batchId: number;
  detections: Detection[][];
  timings?: {
    mode: "batch" | "per-video";
    batch: number;
    preprocessMs: number;
    runMs: number;
    postMs: number;
    totalMs: number;
  };
};

type WorkerErrorMessage = { type: "error"; message: string };

type WorkerMessage = WorkerResultMessage | WorkerErrorMessage;

export function useYoloDetection({
  videoRefs,
  modelPath,
  inputSize = 640,
  intervalMs = 250,
  scoreThreshold = 0.4,
}: Props) {
  const [detections, setDetections] = useState<Detection[][]>([]);
  const workerRef = useRef<Worker | null>(null);
  const inFlightRef = useRef(false);
  const batchIdRef = useRef(0);

  useEffect(() => {
    let running = true;
    const minUpdateIntervalMs = 250;
    let lastUpdate = 0;
    // Track the last decoded frame per video to skip duplicates.
    const lastTimes = new WeakMap<HTMLVideoElement, number>();

    const worker = new Worker(new URL("./yoloWorker.ts", import.meta.url), {
      type: "module",
    });
    workerRef.current = worker;

    const initMessage: WorkerInitMessage = {
      type: "init",
      modelPath,
      inputSize,
      scoreThreshold,
      useWebGPU: import.meta.env.VITE_ENABLE_WEBGPU === "true",
    };
    worker.postMessage(initMessage);

    worker.onmessage = (event: MessageEvent<WorkerMessage>) => {
      if (!running) return;
      if (event.data.type === "result") {
        inFlightRef.current = false;
        const now = performance.now();
        if (now - lastUpdate >= minUpdateIntervalMs) {
          lastUpdate = now;
          setDetections(event.data.detections);
        }
      } else if (event.data.type === "error") {
        inFlightRef.current = false;
        console.error("YOLO worker error", event.data.message);
      }
    };

    async function loop() {
      while (running) {
        if (!workerRef.current || inFlightRef.current) {
          await new Promise((r) => setTimeout(r, intervalMs));
          continue;
        }

        const videos = videoRefs.map((r) => r.current).filter(Boolean) as HTMLVideoElement[];

        if (videos.length === videoRefs.length) {
          const videosToUse = videos
            .filter((video) => video.readyState >= 2)
            .filter((video) => {
              const last = lastTimes.get(video) ?? -1;
              if (video.currentTime === last) return false;
              lastTimes.set(video, video.currentTime);
              return true;
            });

          if (videosToUse.length === 0) {
            await new Promise((r) => setTimeout(r, intervalMs));
            continue;
          }

          try {
            inFlightRef.current = true;
            const batchId = batchIdRef.current++;
            const frames = await Promise.all(videosToUse.map((video) => createImageBitmap(video)));
            const frameMessage: WorkerFrameMessage = { type: "frame", batchId, frames };
            workerRef.current.postMessage(frameMessage, frames);
          } catch (error) {
            inFlightRef.current = false;
            console.error("YOLO frame capture error", error);
          }
        }

        await new Promise((r) => setTimeout(r, intervalMs));
      }
    }

    loop();

    return () => {
      running = false;
      workerRef.current?.terminate();
      workerRef.current = null;
    };
  }, [videoRefs, modelPath, inputSize, intervalMs, scoreThreshold]);

  return detections;
}
