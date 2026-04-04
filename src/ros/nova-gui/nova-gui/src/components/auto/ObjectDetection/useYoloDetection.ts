import { useEffect, useState } from "react";
// import * as ort from "onnxruntime-web/webgpu";
import * as ort from "onnxruntime-web";

// ort.env.wasm.proxy = true

ort.env.wasm.wasmPaths = "/ort/";

export interface Detection {
  classId: number;
  score: number;
  box: {
    x: number;
    y: number;
    width: number;
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

const offscreenCanvases: OffscreenCanvas[] = [];
const contexts: OffscreenCanvasRenderingContext2D[] = [];

export function useYoloDetection({
                                   videoRefs,
                                   modelPath,
                                   inputSize = 640,
                                   intervalMs = 100,
                                   scoreThreshold = 0.4,
                                 }: Props) {
  const [detections, setDetections] =
    useState<Detection[][]>([]);

  useEffect(() => {
    let running = true;

    async function loadSession() {
      if (!session) {
        session =
          await ort.InferenceSession.create(
            modelPath,
            {
              executionProviders: ["webgpu", "wasm"],
            }
          );
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
          const tensor =
            preprocessBatch(
              videos
            );

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
              videos.length
            );

          setDetections(
            parsed
          );
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