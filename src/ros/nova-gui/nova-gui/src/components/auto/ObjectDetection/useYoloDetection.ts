import { useEffect, useState } from "react";
import * as ort from "onnxruntime-web";

// In dev, serve ORT loaders from /src/components/auto/ObjectDetection/ort so Vite can module-load them.
// In build, serve from /public/ort (copied to dist as-is).
ort.env.wasm.wasmPaths = import.meta.env.DEV
  ? "/src/components/auto/ObjectDetection/ort/"
  : "/ort/";

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

let session: ort.InferenceSession | null = null;
let expectedBatch: number | null = null;
let inputName: string | null = null;
let hasLoggedOutputInfo = false;

// Reuse offscreen canvases to avoid per-frame allocations.
const offscreenCanvases: OffscreenCanvas[] = [];
const contexts: OffscreenCanvasRenderingContext2D[] = [];

export function useYoloDetection({
  videoRefs,
  modelPath,
  inputSize = 640,
  intervalMs = 250,
  scoreThreshold = 0.4,
}: Props) {
  const [detections, setDetections] = useState<Detection[][]>([]);

  useEffect(() => {
    let running = true;
    let lastUpdate = 0;
    const minUpdateIntervalMs = 250;
    // Track the last decoded frame per video to skip duplicates.
    const lastTimes = new WeakMap<HTMLVideoElement, number>();

    async function loadSession() {
      if (!session) {
        // Prefer WebGPU when enabled; fall back to WASM otherwise.
        const providers =
          import.meta.env.VITE_ENABLE_WEBGPU === "true" ? ["webgpu", "wasm"] : ["wasm"];

        try {
          session = await ort.InferenceSession.create(modelPath, {
            executionProviders: providers,
          });
        } catch (error) {
          // WebGPU can fail on some devices; fall back to WASM.
          if (providers[0] === "webgpu") {
            session = await ort.InferenceSession.create(modelPath, {
              executionProviders: ["wasm"],
            });
          } else {
            throw error;
          }
        }

        // onnxruntime-web exposes input metadata as an array; use inputNames for the feed key.
        const inputMeta = session.inputMetadata;
        inputName = session.inputNames[0] ?? null;
        // inputMetadata is a union; only tensor metadata has a shape we can read.
        const firstMeta = inputMeta[0];
        const shape =
          firstMeta && "shape" in firstMeta ? firstMeta.shape : undefined;
        expectedBatch = typeof shape?.[0] === "number" ? shape[0] : null;
      }

      return session;
    }

    function ensureOffscreen(batch: number) {
      while (offscreenCanvases.length < batch) {
        // Keep a canvas/context per batch index to resize and draw video frames.
        const canvas = new OffscreenCanvas(inputSize, inputSize);
        const ctx = canvas.getContext("2d")!;

        offscreenCanvases.push(canvas);
        contexts.push(ctx);
      }
    }

    function preprocessBatch(videos: HTMLVideoElement[]) {
      const batch = videos.length;

      ensureOffscreen(batch);

      // NCHW float32 tensor normalized to [0, 1].
      const tensorData = new Float32Array(batch * 3 * inputSize * inputSize);

      videos.forEach((video, batchIndex) => {
        const ctx = contexts[batchIndex];

        // Draw the current frame into a square input buffer.
        ctx.drawImage(video, 0, 0, inputSize, inputSize);

        const image = ctx.getImageData(0, 0, inputSize, inputSize);
        const offset = batchIndex * 3 * inputSize * inputSize;
        const planeSize = inputSize * inputSize;

        // Convert RGBA -> planar RGB.
        for (let i = 0; i < planeSize; i++) {
          const base = offset + i;
          const pixel = i * 4;
          tensorData[base] = image.data[pixel] / 255;
          tensorData[base + planeSize] = image.data[pixel + 1] / 255;
          tensorData[base + 2 * planeSize] = image.data[pixel + 2] / 255;
        }
      });

      return new ort.Tensor("float32", tensorData, [
        batch,
        3,
        inputSize,
        inputSize,
      ]);
    }

    function postprocess(output: ort.Tensor, batchSize: number) {
      const results: Detection[][] = [];
      const data = output.data as Float32Array;

      // Expected layout: [batch, boxes, channels].
      const [, boxes, channels] = output.dims;

      if (!hasLoggedOutputInfo) {
        hasLoggedOutputInfo = true;
        // Log once for debugging output shape and sample values.
        console.log("YOLO output.dims", output.dims);
        console.log(
          "YOLO sample raw values (first 8)",
          Array.from(data.slice(0, 8))
        );
      }

      for (let b = 0; b < batchSize; b++) {
        const detectionsPerCamera: Detection[] = [];
        const offset = b * boxes * channels;

        for (let i = 0; i < boxes; i++) {
          const base = offset + i * channels;
          // Model outputs x1, y1, x2, y2, objectness, classId.
          const x1 = data[base + 0];
          const y1 = data[base + 1];
          const x2 = data[base + 2];
          const y2 = data[base + 3];
          const score = data[base + 4];
          const classId = data[base + 5];

          if (score < scoreThreshold) continue;

          detectionsPerCamera.push({
            classId: Math.round(classId),
            score,
            box: {
              x: x1,
              y: y1,
              width: x2 - x1,
              height: y2 - y1,
            },
          });
        }

        results.push(detectionsPerCamera);
      }

      return results;
    }

    async function loop() {
      const sess = await loadSession();

      while (running) {
        // Resolve refs to live video elements.
        const videos = videoRefs.map((r) => r.current).filter(Boolean) as HTMLVideoElement[];

        if (videos.length === videoRefs.length) {
          // Only process videos with a decoded frame that has advanced.
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

          // Some models accept only batch=1 even if multiple videos are present.
          const runPerVideo = expectedBatch === 1 && videosToUse.length > 1;
          if (!inputName) {
            throw new Error("YOLO model input name unavailable.");
          }
          if (runPerVideo) {
            const parsedAll: Detection[][] = [];
            let preprocessMs = 0;
            let runMs = 0;
            let postMs = 0;
            const totalStart = performance.now();
            for (const video of videosToUse) {
              const t0 = performance.now();
              const tensor = preprocessBatch([video]);
              const t1 = performance.now();
              // Feed key must match model input name; use the resolved inputName.
              const output = await sess.run({ [inputName]: tensor });
              const t2 = performance.now();
              parsedAll.push(...postprocess(Object.values(output)[0], 1));
              const t3 = performance.now();
              preprocessMs += t1 - t0;
              runMs += t2 - t1;
              postMs += t3 - t2;
            }
            const totalMs = performance.now() - totalStart;
            console.log("YOLO timings (per-video)", {
              batch: videosToUse.length,
              preprocessMs: Math.round(preprocessMs),
              runMs: Math.round(runMs),
              postMs: Math.round(postMs),
              totalMs: Math.round(totalMs),
            });
            setDetections(parsedAll);
          } else {
            const t0 = performance.now();
            const tensor = preprocessBatch(videosToUse);
            const t1 = performance.now();
            try {
              // Feed key must match model input name; use the resolved inputName.
              const output = await sess.run({ [inputName]: tensor });
              const t2 = performance.now();
              const parsed = postprocess(Object.values(output)[0], videosToUse.length);
              const t3 = performance.now();
              console.log("YOLO timings (batch)", {
                batch: videosToUse.length,
                preprocessMs: Math.round(t1 - t0),
                runMs: Math.round(t2 - t1),
                postMs: Math.round(t3 - t2),
                totalMs: Math.round(t3 - t0),
              });
              const now = performance.now();
              // Throttle state updates to reduce UI churn.
              if (now - lastUpdate >= minUpdateIntervalMs) {
                lastUpdate = now;
                setDetections(parsed);
              }
            } catch (error) {
              const message = String(error ?? "");
              if (videosToUse.length > 1 && message.includes("invalid dimensions")) {
                // Fallback: run each video independently when batching fails.
                const parsedAll: Detection[][] = [];
                for (const video of videosToUse) {
                  const singleTensor = preprocessBatch([video]);
                  // Feed key must match model input name; use the resolved inputName.
                  const output = await sess.run({ [inputName]: singleTensor });
                  parsedAll.push(...postprocess(Object.values(output)[0], 1));
                }
                const now = performance.now();
                if (now - lastUpdate >= minUpdateIntervalMs) {
                  lastUpdate = now;
                  setDetections(parsedAll);
                }
              } else {
                throw error;
              }
            }
          }
        }

        await new Promise((r) => setTimeout(r, intervalMs));
      }
    }

    // Start the detection loop; cleanup flips the running flag.
    loop();

    return () => {
      running = false;
    };
  }, [videoRefs, modelPath, inputSize, intervalMs, scoreThreshold]);

  return detections;
}
