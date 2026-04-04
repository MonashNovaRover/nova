import { useEffect, useState } from "react";
// import * as ort from "onnxruntime-web/webgpu";
import * as ort from "onnxruntime-web";

// ort.env.wasm.proxy = true

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
  videoRefs: React.RefObject<HTMLVideoElement>[];
  modelPath: string;
  inputSize?: number;
  intervalMs?: number;
  scoreThreshold?: number;
}

let session: ort.InferenceSession | null = null;
let expectedBatch: number | null = null;

const offscreenCanvases: OffscreenCanvas[] = [];
const contexts: OffscreenCanvasRenderingContext2D[] = [];

export function useYoloDetection({
                                   videoRefs,
                                   modelPath,
                                   inputSize = 640,
                                   intervalMs = 250,
                                   scoreThreshold = 0.4,
                                 }: Props) {
  const [detections, setDetections] =
    useState<Detection[][]>([]);

  useEffect(() => {
    let running = true;
    let lastUpdate = 0;
    const minUpdateIntervalMs = 250;
    const lastTimes = new WeakMap<
      HTMLVideoElement,
      number
    >();

    async function loadSession() {
      if (!session) {
        const providers =
          import.meta.env.VITE_ENABLE_WEBGPU === "true"
            ? ["webgpu", "wasm"]
            : ["wasm"];

        try {
          session =
            await ort.InferenceSession.create(
              modelPath,
              {
                executionProviders: providers,
              }
            );
        } catch (error) {
          if (providers[0] === "webgpu") {
            session =
              await ort.InferenceSession.create(
                modelPath,
                {
                  executionProviders: ["wasm"],
                }
              );
          } else {
            throw error;
          }
        }

        const inputMeta =
          session.inputMetadata;
        const firstKey =
          inputMeta.images
            ? "images"
            : Object.keys(inputMeta)[0];
        const dims =
          inputMeta[firstKey]?.dimensions;
        expectedBatch =
          typeof dims?.[0] === "number"
            ? dims[0]
            : null;
      }

      return session;
    }

    function ensureOffscreen(batch: number) {
      while (
        offscreenCanvases.length < batch
        ) {
        const canvas =
          new OffscreenCanvas(
            inputSize,
            inputSize
          );

        const ctx =
          canvas.getContext("2d")!;

        offscreenCanvases.push(canvas);
        contexts.push(ctx);
      }
    }

    function preprocessBatch(
      videos: HTMLVideoElement[]
    ) {
      const batch = videos.length;

      ensureOffscreen(batch);

      const tensorData =
        new Float32Array(
          batch *
          3 *
          inputSize *
          inputSize
        );

      videos.forEach(
        (video, batchIndex) => {
          const ctx =
            contexts[batchIndex];

          ctx.drawImage(
            video,
            0,
            0,
            inputSize,
            inputSize
          );

          const image =
            ctx.getImageData(
              0,
              0,
              inputSize,
              inputSize
            );

          const offset =
            batchIndex *
            3 *
            inputSize *
            inputSize;

          for (
            let i = 0;
            i < inputSize * inputSize;
            i++
          ) {
            tensorData[offset + i] =
              image.data[i * 4] / 255;

            tensorData[
            offset +
            i +
            inputSize *
            inputSize
              ] =
              image.data[i * 4 + 1] /
              255;

            tensorData[
            offset +
            i +
            2 *
            inputSize *
            inputSize
              ] =
              image.data[i * 4 + 2] /
              255;
          }
        }
      );

      return new ort.Tensor(
        "float32",
        tensorData,
        [
          batch,
          3,
          inputSize,
          inputSize,
        ]
      );
    }

    function postprocess(
      output: ort.Tensor,
      batchSize: number
    ) {
      const results: Detection[][] =
        [];

      const data =
        output.data as Float32Array;

      const [
        ,
        channels,
        boxes,
      ] = output.dims;

      const stride =
        channels * boxes;

      for (
        let b = 0;
        b < batchSize;
        b++
      ) {
        const detectionsPerCamera: Detection[] =
          [];

        const offset = b * stride;

        for (
          let i = 0;
          i < boxes;
          i++
        ) {
          const x =
            data[
            offset +
            0 * boxes +
            i
              ];

          const y =
            data[
            offset +
            1 * boxes +
            i
              ];

          const w =
            data[
            offset +
            2 * boxes +
            i
              ];

          const h =
            data[
            offset +
            3 * boxes +
            i
              ];

          const obj =
            data[
            offset +
            4 * boxes +
            i
              ];

          if (
            obj <
            scoreThreshold
          )
            continue;

          let bestClass = 0;
          let bestScore = 0;

          for (
            let c = 5;
            c < channels;
            c++
          ) {
            const score =
              data[
              offset +
              c * boxes +
              i
                ];

            if (
              score >
              bestScore
            ) {
              bestScore = score;
              bestClass = c - 5;
            }
          }

          const confidence =
            obj * bestScore;

          if (
            confidence <
            scoreThreshold
          )
            continue;

          detectionsPerCamera.push(
            {
              classId:
              bestClass,
              score:
              confidence,
              box: {
                x:
                  (x - w / 2) *
                  inputSize,
                y:
                  (y - h / 2) *
                  inputSize,
                width:
                  w * inputSize,
                height:
                  h * inputSize,
              },
            }
          );
        }

        results.push(
          detectionsPerCamera
        );
      }

      return results;
    }

    async function loop() {
      const sess =
        await loadSession();

      while (running) {
        const videos =
          videoRefs
            .map(
              (r) =>
                r.current
            )
            .filter(
              Boolean
            ) as HTMLVideoElement[];

        if (
          videos.length ===
          videoRefs.length
        ) {
          const videosToUse =
            videos
              .filter(
                (video) =>
                  video.readyState >= 2
              )
              .filter((video) => {
                const last =
                  lastTimes.get(
                    video
                  ) ?? -1;
                if (
                  video.currentTime ===
                  last
                ) {
                  return false;
                }
                lastTimes.set(
                  video,
                  video.currentTime
                );
                return true;
              })
              .slice(0, 1);
          if (videosToUse.length === 0) {
            await new Promise((r) =>
              setTimeout(r, intervalMs)
            );
            continue;
          }
          const runPerVideo =
            expectedBatch === 1 &&
            videosToUse.length > 1;
          if (runPerVideo) {
            const parsedAll: Detection[][] =
              [];
            for (const video of videosToUse) {
              const tensor =
                preprocessBatch(
                  [video]
                );
              const output =
                await sess.run({
                  images:
                  tensor,
                });
              parsedAll.push(
                ...postprocess(
                  Object.values(
                    output
                  )[0],
                  1
                )
              );
            }
            setDetections(parsedAll);
          } else {
            const tensor =
              preprocessBatch(
                videosToUse
              );
            try {
              const output =
                await sess.run({
                  images:
                  tensor,
                });
              const parsed =
                postprocess(
                  Object.values(
                    output
                  )[0],
                  videosToUse.length
                );
              const now =
                performance.now();
              if (
                now - lastUpdate >=
                minUpdateIntervalMs
              ) {
                lastUpdate = now;
                setDetections(
                  parsed
                );
              }
            } catch (error) {
              const message =
                String(error ?? "");
              if (
                videosToUse.length > 1 &&
                message.includes(
                  "invalid dimensions"
                )
              ) {
                const parsedAll: Detection[][] =
                  [];
                for (const video of videosToUse) {
                  const singleTensor =
                    preprocessBatch(
                      [video]
                    );
                  const output =
                    await sess.run({
                      images:
                      singleTensor,
                    });
                  parsedAll.push(
                    ...postprocess(
                      Object.values(
                        output
                      )[0],
                      1
                    )
                  );
                }
                const now =
                  performance.now();
                if (
                  now - lastUpdate >=
                  minUpdateIntervalMs
                ) {
                  lastUpdate = now;
                  setDetections(
                    parsedAll
                  );
                }
              } else {
                throw error;
              }
            }
          }
        }

        await new Promise(
          (r) =>
            setTimeout(
              r,
              intervalMs
            )
        );
      }
    }

    loop();

    return () => {
      running = false;
    };
  }, [
    videoRefs,
    modelPath,
    inputSize,
    intervalMs,
    scoreThreshold,
  ]);

  return detections;
}
