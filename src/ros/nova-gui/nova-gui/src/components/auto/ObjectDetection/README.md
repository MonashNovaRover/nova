# Object Detection (YOLO + ONNX Runtime Web)

This folder implements browser-side YOLO inference for live camera feeds using
ONNX Runtime Web (ORT). Cameras register their `<video>` refs through
`YoloProvider`, frames are captured in `useYoloDetection`, inference runs in
`yoloWorker`, and detections are drawn by `YoloOverlayCanvas`.

## Current flow

- `YoloProvider.tsx` collects video refs and starts detection with the active model.
- `useYoloDetection.ts` captures `ImageBitmap`s from ready videos, skips duplicate
  frames, avoids overlapping requests, and updates detection state on a throttled loop.
- `yoloWorker.ts` owns ORT session setup, preprocessing, inference, and YOLO output parsing.
- `YoloOverlayCanvas.tsx` maps model-space boxes back onto the rendered video and draws labels.

## Model and runtime

- Active model/labels are selected in `YoloConfig.ts` via `ActiveYoloConfig`.
- Models are loaded from `/models/${ActiveYoloConfig.modelName}`.
- Input size is currently fixed to `640`.
- **Letterboxing**: Frames are letterboxed (with black bars) to maintain aspect ratio
  when preprocessing. This prevents distortion and ensures accurate detections on
  non-square video feeds (e.g., 16:9 aspect ratio).
- WebGPU is tried first by default, with automatic fallback to WASM if WebGPU session creation
  fails. Set `VITE_ENABLE_WEBGPU=false` to skip WebGPU and use WASM directly.

## Batching

The worker reads the model input metadata to determine the expected batch size.
If the model is fixed to batch `1` and multiple videos are active, it falls back
to running inference once per video instead of sending a multi-frame batch.

## Model output format

The ONNX model output is expected to be shaped like `[batch, boxes, channels]`
where each detection row contains 6 values: `[coord1, coord2, coord3, coord4, score, classId]`.

Two coordinate formats are supported via the `outputFormat` field in `YoloConfig.ts`:

- **`"xyxy"`**: Corner coordinates `[x1, y1, x2, y2, confidence, classId]`
  - Used by older YOLO models and some post-NMS exports

- **`"xywh"`**: Center coordinates `[x_center, y_center, width, height, confidence, classId]`
  - Used by YOLOv8/v11 and most modern YOLO variants

When adding a new model, set the `outputFormat` in your `YOLOConfig`:

```typescript
export const MyModelConfig: YOLOConfig = {
  modelName: "my-model.onnx",
  classNames: ["class1", "class2"],
  outputFormat: "xywh",  // or "xyxy" depending on your model
}
```

If your model uses a different output layout (e.g., class probabilities instead
of a single classId, or transposed dimensions), you'll need to modify the
`postprocess(...)` function in `yoloWorker.ts`.

## Why the `ort/` folder lives here

ORT loads its `.mjs` and `.wasm` runtime files dynamically. In development, the
worker points `ort.env.wasm.wasmPaths` at:

- `/src/components/auto/ObjectDetection/ort/`

In production it points at:

- `/ort/`

This keeps the ORT loader resolvable in dev while still allowing static assets
to be served in the built app.

## Files

- `YoloCameraComponent.tsx`: camera wrapper that attaches the detection overlay
- `YoloConfig.ts`: available model/class-name configs and active model selection
- `YoloOverlayCanvas.tsx`: overlay drawing and coordinate mapping
- `YoloProvider.tsx`: context for video registration and detections
- `useYoloDetection.ts`: main-thread frame capture and worker messaging
- `yoloWorker.ts`: ORT setup, preprocessing, inference, and postprocessing
- `ort/`: ORT loader and WASM runtime files used by the worker

## References

- ORT env flags and `wasmPaths`: https://onnxruntime.ai/docs/tutorials/web/env-flags-and-session-options.html
- ORT WebGPU execution provider: https://onnxruntime.ai/docs/tutorials/web/ep-webgpu.html
- Vite public assets behavior: https://vite.dev/guide/assets.html
