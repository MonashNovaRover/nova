import * as ort from "onnxruntime-web/all";

// In dev, serve ORT loaders from /src/components/auto/ObjectDetection/ort so Vite can module-load them.
// In build, serve from /public/ort (copied to dist as-is).
// Configure ORT WASM loader paths so the worker can fetch runtime files in dev/prod.
ort.env.wasm.wasmPaths = import.meta.env.DEV
  ? "/src/components/auto/ObjectDetection/ort/"
  : "/ort/";

ort.env.wasm.proxy = true;

type YOLOOutputFormat = "xyxy" | "xywh";

// Minimal detection shape sent back to the main thread.
interface Detection {
  classId: number;
  score: number;
  box: {
    x: number;
    y: number;
    width: number;
    height: number;
  };
}

// Init message sets model path, input size, threshold, and provider preference.
type InitMessage = {
  type: "init";
  modelPath: string;
  inputSize: number;
  scoreThreshold: number;
  useWebGPU: boolean;
  outputFormat: YOLOOutputFormat;
};

// Frame message carries a batch of ImageBitmaps for inference.
type FrameMessage = {
  type: "frame";
  batchId: number;
  frames: ImageBitmap[];
};

type WorkerMessage = InitMessage | FrameMessage;

// Result message returns detections plus optional timing stats.
type ResultMessage = {
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

// Error message allows the main thread to surface worker failures.
type ErrorMessage = { type: "error"; message: string };

// Session and model metadata live in the worker for off-thread inference.
let session: ort.InferenceSession | null = null;
let inputName: string | null = null;
let expectedBatch: number | null = null;
let inputSize = 512;
let scoreThreshold = 0.7;
let outputFormat: YOLOOutputFormat = "xyxy";

// Reuse offscreen canvases to avoid allocations per frame.
const offscreenCanvases: OffscreenCanvas[] = [];
const contexts: OffscreenCanvasRenderingContext2D[] = [];

// Ensure we have enough offscreen canvases/contexts for the batch size.
function ensureOffscreen(batch: number) {
  while (offscreenCanvases.length < batch) {
    const canvas = new OffscreenCanvas(inputSize, inputSize);
    // Add the willReadFrequently attribute here
    const ctx = canvas.getContext("2d", { willReadFrequently: true })!;
    offscreenCanvases.push(canvas);
    contexts.push(ctx);
  }
}

// Letterbox parameters for aspect ratio preservation
interface LetterboxInfo {
  scale: number;
  offsetX: number;
  offsetY: number;
}

const letterboxInfos: LetterboxInfo[] = [];

// Convert ImageBitmaps to a NCHW float32 tensor in model input space with letterboxing.
function preprocessBatch(frames: ImageBitmap[]) {
  const batch = frames.length;

  ensureOffscreen(batch);
  letterboxInfos.length = 0;

  const tensorData = new Float32Array(batch * 3 * inputSize * inputSize);

  frames.forEach((frame, batchIndex) => {
    const ctx = contexts[batchIndex];

    // Clear canvas to black (letterbox background)
    ctx.fillStyle = "#000000";
    ctx.fillRect(0, 0, inputSize, inputSize);

    // Calculate letterbox scale and offsets to maintain aspect ratio
    const scale = Math.min(inputSize / frame.width, inputSize / frame.height);
    const scaledWidth = frame.width * scale;
    const scaledHeight = frame.height * scale;
    const offsetX = (inputSize - scaledWidth) / 2;
    const offsetY = (inputSize - scaledHeight) / 2;

    // Store letterbox info for this frame
    letterboxInfos[batchIndex] = { scale, offsetX, offsetY };

    // Draw the frame centered with black bars (letterboxing)
    ctx.drawImage(frame, offsetX, offsetY, scaledWidth, scaledHeight);

    const image = ctx.getImageData(0, 0, inputSize, inputSize);
    const offset = batchIndex * 3 * inputSize * inputSize;
    const planeSize = inputSize * inputSize;

    // Convert RGBA -> planar RGB in [0, 1].
    for (let i = 0; i < planeSize; i++) {
      const base = offset + i;
      const pixel = i * 4;
      tensorData[base] = image.data[pixel] / 255;
      tensorData[base + planeSize] = image.data[pixel + 1] / 255;
      tensorData[base + 2 * planeSize] = image.data[pixel + 2] / 255;
    }

    // Release the bitmap once pixels are copied.
    frame.close();
  });

  return new ort.Tensor("float32", tensorData, [batch, 3, inputSize, inputSize]);
}

// Parse model output into Detection[][] per camera index.
// Coordinates are converted from letterboxed model space to original frame pixel space.
function postprocess(output: ort.Tensor, batchSize: number): Detection[][] {
  const results: Detection[][] = [];
  const data = output.data as Float32Array;
  const [, boxes, channels] = output.dims;

  for (let b = 0; b < batchSize; b++) {
    const detectionsPerCamera: Detection[] = [];
    const offset = b * boxes * channels;
    const letterbox = letterboxInfos[b];

    if (!letterbox) {
      results.push([]);
      continue;
    }

    for (let i = 0; i < boxes; i++) {
      const base = offset + i * channels;

      let modelX: number, modelY: number, modelWidth: number, modelHeight: number;

      if (outputFormat === "xywh") {
        // YOLOv8/v11 format: [x_center, y_center, width, height, confidence, classId]
        const x_center = data[base + 0];
        const y_center = data[base + 1];
        const w = data[base + 2];
        const h = data[base + 3];

        // Convert from center-width-height to top-left corner format
        modelX = x_center - w / 2;
        modelY = y_center - h / 2;
        modelWidth = w;
        modelHeight = h;
      } else {
        // Original format: [x1, y1, x2, y2, confidence, classId]
        const x1 = data[base + 0];
        const y1 = data[base + 1];
        const x2 = data[base + 2];
        const y2 = data[base + 3];

        modelX = x1;
        modelY = y1;
        modelWidth = x2 - x1;
        modelHeight = y2 - y1;
      }

      const score = data[base + 4];
      const classId = data[base + 5];

      if (score < scoreThreshold) continue;

      // Remove letterbox padding and scale to get coordinates in original frame space
      const x = (modelX - letterbox.offsetX) / letterbox.scale;
      const y = (modelY - letterbox.offsetY) / letterbox.scale;
      const width = modelWidth / letterbox.scale;
      const height = modelHeight / letterbox.scale;

      detectionsPerCamera.push({
        classId: Math.round(classId),
        score,
        box: {
          x,
          y,
          width,
          height,
        },
      });
    }

    results.push(detectionsPerCamera);
  }

  return results;
}

// Initialize the ORT session and read model input metadata.
async function initSession({ modelPath, useWebGPU }: InitMessage) {
  const providers = useWebGPU ? ["webgpu", "wasm"] : ["wasm"];
  try {
    session = await ort.InferenceSession.create(modelPath, {
      executionProviders: providers,
    });
    console.log("YOLO execution provider", providers[0]);
  } catch (error) {
    if (providers[0] === "webgpu") {
      session = await ort.InferenceSession.create(modelPath, {
        executionProviders: ["wasm"],
      });
      console.log("YOLO execution provider", "wasm (webgpu fallback)");
    } else {
      throw error;
    }
  }

  // const inputMeta = session.inputMetadata;
  // inputName = session.inputNames[0] ?? null;
  // const firstMeta = inputMeta[0];
  // const shape = firstMeta && "shape" in firstMeta ? firstMeta.shape : undefined;
  // expectedBatch = typeof shape?.[0] === "number" ? shape[0] : null;
  const inputMeta = session.inputMetadata;
  inputName = session.inputNames[0] ?? null;
  
  // Extract dimensions from the model metadata
  const firstMeta = inputMeta[0];
  const shape = firstMeta && "shape" in firstMeta ? firstMeta.shape : undefined;
  
  if (shape) {
    // Usually YOLO shapes are [batch, channels, height, width]
    // Index 2 and 3 are height and width
    if (typeof shape[2] === 'number' && shape[2] > 0) {
      inputSize = shape[2];
    }
  }
  
  expectedBatch = typeof shape?.[0] === "number" ? shape[0] : null;
}

// Worker entrypoint: init session or run a batch of frames.
self.onmessage = async (event: MessageEvent<WorkerMessage>) => {
  try {
    if (event.data.type === "init") {
      const { modelPath, inputSize: size, scoreThreshold: threshold, useWebGPU, outputFormat: format } = event.data;
      // Persist configuration in the worker.
      inputSize = size;
      scoreThreshold = threshold;
      outputFormat = format;
      await initSession({ modelPath, inputSize: size, scoreThreshold: threshold, useWebGPU, outputFormat: format, type: "init" });
      return;
    }

    if (!session || !inputName) {
      const msg: ErrorMessage = { type: "error", message: "YOLO session not initialized" };
      self.postMessage(msg);
      return;
    }

    const { batchId, frames } = event.data;
    const batch = frames.length;
    if (batch === 0) return;

    const runPerVideo = expectedBatch === 1 && batch > 1;

    if (runPerVideo) {
      const parsedAll: Detection[][] = [];
      let preprocessMs = 0;
      let runMs = 0;
      let postMs = 0;
      const totalStart = performance.now();

      for (const frame of frames) {
        const t0 = performance.now();
        const tensor = preprocessBatch([frame]);
        const t1 = performance.now();
        const output = await session.run({ [inputName]: tensor });
        const t2 = performance.now();
        parsedAll.push(...postprocess(Object.values(output)[0], 1));
        const t3 = performance.now();
        preprocessMs += t1 - t0;
        runMs += t2 - t1;
        postMs += t3 - t2;
      }

      const totalMs = performance.now() - totalStart;

      const msg: ResultMessage = {
        type: "result",
        batchId,
        detections: parsedAll,
        timings: {
          mode: "per-video",
          batch,
          preprocessMs: Math.round(preprocessMs),
          runMs: Math.round(runMs),
          postMs: Math.round(postMs),
          totalMs: Math.round(totalMs),
        },
      };
      self.postMessage(msg);
      return;
    }

    const t0 = performance.now();
    const tensor = preprocessBatch(frames);
    const t1 = performance.now();
    const output = await session.run({ [inputName]: tensor });
    const t2 = performance.now();
    const detections = postprocess(Object.values(output)[0], batch);
    const t3 = performance.now();

    // console.log("YOLO timings (batch)", {
    //   batch,
    //   preprocessMs: Math.round(t1 - t0),
    //   runMs: Math.round(t2 - t1),
    //   postMs: Math.round(t3 - t2),
    //   totalMs: Math.round(t3 - t0),
    // });

    const msg: ResultMessage = {
      type: "result",
      batchId,
      detections,
      timings: {
        mode: "batch",
        batch,
        preprocessMs: Math.round(t1 - t0),
        runMs: Math.round(t2 - t1),
        postMs: Math.round(t3 - t2),
        totalMs: Math.round(t3 - t0),
      },
    };
    self.postMessage(msg);
  } catch (error) {
    const msg: ErrorMessage = { type: "error", message: String(error ?? "") };
    self.postMessage(msg);
  }
};
