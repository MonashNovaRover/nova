import { useEffect, useRef, useState } from "react";
import type { YOLOOutputFormat } from "./YoloConfig";

export interface Detection {
  // Class index in the model's label set.
  classId: number;
  // Confidence score after combining objectness and class score.
  score: number;
  box: {
    // Top-left corner x in video pixel space (letterbox-adjusted).
    x: number;
    // Top-left corner y in video pixel space (letterbox-adjusted).
    y: number;
    // Box width in video pixels.
    width: number;
    // Box height in video pixels.
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
  outputFormat: YOLOOutputFormat;
}

// Worker init message configures model and runtime settings.
type WorkerInitMessage = {
  type: "init";
  modelPath: string;
  inputSize: number;
  scoreThreshold: number;
  useWebGPU: boolean;
  outputFormat: YOLOOutputFormat;
};

// Worker frame message delivers a batch of ImageBitmaps to process.
type WorkerFrameMessage = {
  type: "frame";
  batchId: number;
  frames: ImageBitmap[];
};

// Worker result message returns detections and optional timing stats.
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

// Worker error message surfaces failures back to the UI.
type WorkerErrorMessage = { type: "error"; message: string };

type WorkerMessage = WorkerResultMessage | WorkerErrorMessage;

export function useYoloDetection({
  videoRefs,
  modelPath,
  inputSize = 640,
  intervalMs = 250,
  scoreThreshold = 0.4,
  outputFormat,
}: Props) {
  const [detections, setDetections] = useState<Detection[][]>([]);
  // Worker runs inference off the main thread.
  const workerRef = useRef<Worker | null>(null);
  // Prevent concurrent batches while a worker request is in flight.
  const inFlightRef = useRef(false);
  // Incrementing id for batches (useful for debugging or ordering).
  const batchIdRef = useRef(0);
  // Track which camera indices were included in each worker batch.
  const pendingBatchIndicesRef = useRef(new Map<number, number[]>());

  useEffect(() => {
    let running = true;
    const minUpdateIntervalMs = 250;
    let lastUpdate = 0;
    pendingBatchIndicesRef.current = new Map<number, number[]>();
    const pendingBatchIndices = pendingBatchIndicesRef.current;
    // Track the last decoded frame per video to skip duplicates.
    const lastTimes = new WeakMap<HTMLVideoElement, number>();

    // Spin up the worker module for YOLO inference.
    const worker = new Worker(new URL("./yoloWorker.ts", import.meta.url), {
      type: "module",
    });
    workerRef.current = worker;

    // Initialize the worker with model/runtime settings.
    const initMessage: WorkerInitMessage = {
      type: "init",
      modelPath,
      inputSize,
      scoreThreshold,
      useWebGPU: import.meta.env.VITE_ENABLE_WEBGPU !== "false",
      outputFormat,
    };
    worker.postMessage(initMessage);

    // Handle worker responses and update UI state.
    worker.onmessage = (event: MessageEvent<WorkerMessage>) => {
      if (!running) return;
      if (event.data.type === "result") {
        const result = event.data;
        inFlightRef.current = false;
        const batchIndices = pendingBatchIndices.get(result.batchId) ?? [];
        pendingBatchIndices.delete(result.batchId);
        const now = performance.now();
        if (now - lastUpdate >= minUpdateIntervalMs) {
          lastUpdate = now;
          setDetections((previous) => {
            const next = Array.from(
              { length: Math.max(previous.length, videoRefs.length) },
              (_, index) => previous[index] ?? []
            );

            batchIndices.forEach((cameraIndex, resultIndex) => {
              next[cameraIndex] = result.detections[resultIndex] ?? [];
            });

            return next;
          });
        }
      } else if (event.data.type === "error") {
        inFlightRef.current = false;
        console.error("YOLO worker error", event.data.message);
      }
    };

    // Capture frames on an interval and send them to the worker.
    async function loop() {
      while (running) {
        if (!workerRef.current || inFlightRef.current) {
          await new Promise((r) => setTimeout(r, intervalMs));
          continue;
        }

        // Resolve refs to live video elements.
        const videos = videoRefs
          .map((ref, index) => ({ index, video: ref.current }))
          .filter((entry): entry is { index: number; video: HTMLVideoElement } => Boolean(entry.video));

        if (videos.length === videoRefs.length) {
          // Only process videos with a decoded frame that has advanced.
          const videosToUse = videos
            .filter(({ video }) => video.readyState >= 2)
            .filter(({ video }) => {
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
            pendingBatchIndices.set(
              batchId,
              videosToUse.map(({ index }) => index)
            );
            // Capture ImageBitmaps for transfer to the worker.
            const frames = await Promise.all(videosToUse.map(({ video }) => createImageBitmap(video)));
            const frameMessage: WorkerFrameMessage = { type: "frame", batchId, frames };
            // Transfer ownership of ImageBitmaps to avoid copies.
            workerRef.current.postMessage(frameMessage, frames);
          } catch (error) {
            pendingBatchIndices.delete(batchIdRef.current - 1);
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
      pendingBatchIndices.clear();
      workerRef.current?.terminate();
      workerRef.current = null;
    };
  }, [videoRefs, modelPath, inputSize, intervalMs, scoreThreshold, outputFormat]);

  return detections;
}
